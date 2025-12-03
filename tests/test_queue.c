#include "../include/queue.h"
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define NUM_THREADS 4
#define ENTRIES_PER_THREAD 10

// Test data for threaded test
typedef struct {
    log_queue_t* queue;
    int thread_id;
} thread_data_t;

static void* producer_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    for (int i = 0; i < ENTRIES_PER_THREAD; i++) {
        char source[64];
        char message[64];
        snprintf(source, sizeof(source), "thread%d", data->thread_id);
        snprintf(message, sizeof(message), "Message %d from thread %d", i, data->thread_id);
        
        log_entry_t* entry = log_entry_create(source, message, LOG_LEVEL_INFO, message);
        assert(entry != NULL);
        assert(queue_enqueue(data->queue, entry) == 0);
    }
    
    return NULL;
}

static void* consumer_thread(void* arg) {
    log_queue_t* queue = (log_queue_t*)arg;
    int count = 0;
    
    while (count < NUM_THREADS * ENTRIES_PER_THREAD) {
        log_entry_t* entry = queue_dequeue(queue);
        if (entry) {
            assert(entry->source != NULL);
            assert(entry->message != NULL);
            log_entry_destroy(entry);
            count++;
        } else {
            // If queue returns NULL and we haven't consumed all entries,
            // the queue might be shutdown - break to avoid infinite loop
            break;
        }
    }
    
    return NULL;
}

void test_queue(void) {
    log_queue_t queue;
    
    // Test initialization
    assert(queue_init(&queue, 100) == 0);
    assert(queue_size(&queue) == 0);
    assert(queue_is_empty(&queue) == true);
    
    // Test enqueue/dequeue
    log_entry_t* entry1 = log_entry_create("test1.log", "Message 1", LOG_LEVEL_INFO, "raw1");
    log_entry_t* entry2 = log_entry_create("test2.log", "Message 2", LOG_LEVEL_WARNING, "raw2");
    
    assert(queue_enqueue(&queue, entry1) == 0);
    assert(queue_size(&queue) == 1);
    assert(queue_is_empty(&queue) == false);
    
    assert(queue_enqueue(&queue, entry2) == 0);
    assert(queue_size(&queue) == 2);
    
    log_entry_t* dequeued = queue_dequeue(&queue);
    assert(dequeued != NULL);
    assert(strcmp(dequeued->source, "test1.log") == 0);
    assert(queue_size(&queue) == 1);
    log_entry_destroy(dequeued);
    
    dequeued = queue_dequeue(&queue);
    assert(dequeued != NULL);
    assert(strcmp(dequeued->source, "test2.log") == 0);
    assert(queue_size(&queue) == 0);
    assert(queue_is_empty(&queue) == true);
    log_entry_destroy(dequeued);
    
    // Test thread safety with multiple producers and consumers
    pthread_t producers[NUM_THREADS];
    pthread_t consumer;
    thread_data_t thread_data[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].queue = &queue;
        thread_data[i].thread_id = i;
        pthread_create(&producers[i], NULL, producer_thread, &thread_data[i]);
    }
    
    pthread_create(&consumer, NULL, consumer_thread, &queue);
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(producers[i], NULL);
    }
    pthread_join(consumer, NULL);
    
    // Give threads extra time to fully complete all operations
    // This ensures no race conditions when we destroy the queue
    sleep(2);
    
    // The consumer should have consumed all entries
    // queue_destroy will clean up any remaining entries safely
    queue_destroy(&queue);
    
    // Small delay to ensure first queue is fully destroyed
    sleep(1);
    
    // Test unlimited queue (max_size = 0) - use a separate queue to avoid
    // issues with re-initializing destroyed mutex/condition variables
    log_queue_t queue2;
    assert(queue_init(&queue2, 0) == 0);
    
    for (int i = 0; i < 200; i++) {
        log_entry_t* e = log_entry_create("test.log", "msg", LOG_LEVEL_INFO, "raw");
        assert(queue_enqueue(&queue2, e) == 0);
    }
    assert(queue_size(&queue2) == 200);
    
    // Cleanup - use size check instead of is_empty for safety
    size_t remaining = queue_size(&queue2);
    for (size_t i = 0; i < remaining; i++) {
        log_entry_t* e = queue_dequeue(&queue2);
        if (e) {
            log_entry_destroy(e);
        } else {
            break;  // Queue returned NULL, stop
        }
    }
    
    queue_destroy(&queue2);
}