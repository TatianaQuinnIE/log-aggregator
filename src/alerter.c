#include "alerter.h"
#include "log_entry.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

// Forward declaration for thread function
static void* alerter_thread_func(void* arg);

int alerter_init(alerter_t* alerter, log_queue_t* alert_queue, config_t* config) {
    if (!alerter || !alert_queue || !config) {
        return -1;
    }
    
    alerter->alert_queue = alert_queue;
    alerter->config = config;
    alerter->running = false;
    alerter->alert_file = NULL;
    
    // Open alert file
    if (config->alert_file) {
        alerter->alert_file = fopen(config->alert_file, "a");
        if (!alerter->alert_file) {
            perror("Failed to open alert file");
            return -1;
        }
    }
    
    return 0;
}

static void* alerter_thread_func(void* arg) {
    alerter_t* alerter = (alerter_t*)arg;
    
    while (alerter->running) {
        log_entry_t* entry = queue_dequeue(alerter->alert_queue);
        if (!entry) {
            // If queue returned NULL, it might be shutdown
            // Check running flag and exit if needed
            if (!alerter->running) {
                break;
            }
            continue;
        }
        
        alerter_write_alert(alerter, entry);
        log_entry_destroy(entry);
    }
    
    return NULL;
}

int alerter_start(alerter_t* alerter) {
    if (!alerter || alerter->running) {
        return -1;
    }
    
    if (!alerter->config->enable_alerts) {
        return 0; // Alerts disabled
    }
    
    alerter->running = true;
    if (pthread_create(&alerter->thread, NULL, alerter_thread_func, alerter) != 0) {
        alerter->running = false;
        return -1;
    }
    
    return 0;
}

void alerter_stop(alerter_t* alerter) {
    if (!alerter) {
        return;
    }
    
    alerter->running = false;
    if (alerter->thread) {
        pthread_join(alerter->thread, NULL);
    }
}

void alerter_destroy(alerter_t* alerter) {
    if (!alerter) {
        return;
    }
    
    alerter_stop(alerter);
    
    if (alerter->alert_file) {
        fclose(alerter->alert_file);
        alerter->alert_file = NULL;
    }
}

void alerter_write_alert(alerter_t* alerter, log_entry_t* entry) {
    if (!alerter || !entry) {
        return;
    }
    
    char timestamp_str[64];
    struct tm* timeinfo = localtime(&entry->timestamp);
    strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    const char* level_str = log_entry_level_to_string(entry->level);
    
    // Write to file
    if (alerter->alert_file) {
        fprintf(alerter->alert_file, "[%s] [%s] [%s] %s\n",
                timestamp_str, level_str, entry->source, entry->message);
        fflush(alerter->alert_file);
    }
    
    // Also print to stdout
    printf("[ALERT] [%s] [%s] [%s] %s\n",
           timestamp_str, level_str, entry->source, entry->message);
}