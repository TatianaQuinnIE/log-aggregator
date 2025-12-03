#include "processor.h"
#include "log_entry.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

// Forward declaration for thread function
static void* processor_thread_func(void* arg);

int processor_init(processor_t* processor, log_queue_t* input_queue,
                   log_queue_t* output_queue, config_t* config) {
    if (!processor || !input_queue || !output_queue || !config) {
        return -1;
    }
    
    processor->input_queue = input_queue;
    processor->output_queue = output_queue;
    processor->config = config;
    processor->running = false;
    processor->num_threads = config->num_processing_threads;
    
    processor->threads = (pthread_t*)calloc(processor->num_threads, sizeof(pthread_t));
    if (!processor->threads) {
        return -1;
    }
    
    return 0;
}

static void* processor_thread_func(void* arg) {
    processor_t* processor = (processor_t*)arg;
    
    while (processor->running) {
        log_entry_t* entry = queue_dequeue(processor->input_queue);
        if (!entry) {
            // If queue returned NULL, it might be shutdown
            // Check running flag and exit if needed
            if (!processor->running) {
                break;
            }
            continue;
        }
        
        // Process the entry
        bool should_alert = processor_process_entry(entry, processor->config);
        
        // If it should be alerted, add to alert queue
        if (should_alert && processor->output_queue) {
            queue_enqueue(processor->output_queue, entry);
        } else {
            // Entry doesn't meet alert criteria, destroy it
            log_entry_destroy(entry);
        }
    }
    
    return NULL;
}

int processor_start(processor_t* processor) {
    if (!processor || processor->running) {
        return -1;
    }
    
    processor->running = true;
    
    for (int i = 0; i < processor->num_threads; i++) {
        if (pthread_create(&processor->threads[i], NULL, processor_thread_func, processor) != 0) {
            processor->running = false;
            // Wait for already created threads
            for (int j = 0; j < i; j++) {
                pthread_join(processor->threads[j], NULL);
            }
            return -1;
        }
    }
    
    return 0;
}

void processor_stop(processor_t* processor) {
    if (!processor) {
        return;
    }
    
    processor->running = false;
    
    // Wake up threads waiting on queues by broadcasting
    // The queue_destroy will handle waking them up, but we need to ensure
    // threads exit their loops. They'll exit when running becomes false
    // and queue_dequeue returns NULL (after queue is destroyed)
    
    // Wait for threads with a timeout approach - they should exit when
    // queue operations fail or running is false
    for (int i = 0; i < processor->num_threads; i++) {
        if (processor->threads[i]) {
            pthread_join(processor->threads[i], NULL);
        }
    }
}

void processor_destroy(processor_t* processor) {
    if (!processor) {
        return;
    }
    
    processor_stop(processor);
    free(processor->threads);
}

bool processor_process_entry(log_entry_t* entry, config_t* config) {
    if (!entry || !config) {
        return false;
    }
    
    // Check if level meets threshold
    if (entry->level < config->alert_threshold) {
        return false;
    }
    
    // If level meets threshold, alert
    // Also check for pattern matches (patterns can trigger alerts even below threshold)
    if (processor_check_patterns(entry, config)) {
        return true;
    }
    
    // Alert on entries that meet or exceed threshold
    return true;
}

bool processor_check_patterns(log_entry_t* entry, config_t* config) {
    if (!entry || !config || !config->alert_patterns) {
        return false;
    }
    
    for (size_t i = 0; i < config->num_patterns; i++) {
        if (config->alert_patterns[i] && 
            (strstr(entry->message, config->alert_patterns[i]) != NULL ||
             strstr(entry->raw_line, config->alert_patterns[i]) != NULL)) {
            return true;
        }
    }
    
    return false;
}