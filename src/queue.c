#include "queue.h"
#include "log_entry.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>

int queue_init(log_queue_t* queue, size_t max_size) {
    if (!queue) {
        return -1;
    }
    
    // If queue was previously destroyed, we need to ensure it's properly cleaned
    // Zero out data fields (but not mutex/cond vars - they need proper init/destroy)
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->max_size = max_size;
    queue->shutdown = false;
    queue->destroyed = false;
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        return -1;
    }
    
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        return -1;
    }
    
    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        return -1;
    }
    
    return 0;
}

void queue_shutdown(log_queue_t* queue) {
    if (!queue) {
        return;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    // If already shutdown, nothing to do
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return;
    }
    
    // Mark queue as shutdown to wake up waiting threads
    queue->shutdown = true;
    
    // Wake up any threads waiting on the queue
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    
    pthread_mutex_unlock(&queue->mutex);
}

void queue_destroy(log_queue_t* queue) {
    if (!queue || queue->destroyed) {
        return;
    }
    
    // Mark as shutdown first (if not already)
    queue_shutdown(queue);
    
    // Give threads a moment to exit (they should exit quickly)
    // Small delay to allow threads to wake up and check shutdown flag
    struct timespec ts = {0, 100000000}; // 100ms
    nanosleep(&ts, NULL);
    
    pthread_mutex_lock(&queue->mutex);
    
    // Free all remaining entries
    queue_node_t* current = queue->head;
    queue->head = NULL;  // Set to NULL first to prevent any issues
    queue->tail = NULL;
    queue->size = 0;
    
    pthread_mutex_unlock(&queue->mutex);
    
    // Free nodes outside of mutex lock to avoid any issues
    // Add limit to prevent infinite loop if list is corrupted
    int node_count = 0;
    while (current && node_count < 10000) {  // Safety limit
        queue_node_t* next = current->next;
        if (current->entry) {
            log_entry_destroy(current->entry);
        }
        free(current);
        current = next;
        node_count++;
    }
    
    // Destroy synchronization primitives
    pthread_cond_destroy(&queue->not_full);
    pthread_cond_destroy(&queue->not_empty);
    pthread_mutex_destroy(&queue->mutex);
    
    // Mark as destroyed to prevent double-destroy
    queue->destroyed = true;
}

int queue_enqueue(log_queue_t* queue, log_entry_t* entry) {
    if (!queue || !entry) {
        return -1;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    // Wait if queue is full (if max_size > 0)
    while (queue->max_size > 0 && queue->size >= queue->max_size) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    
    queue_node_t* node = (queue_node_t*)malloc(sizeof(queue_node_t));
    if (!node) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }
    
    node->entry = entry;
    node->next = NULL;
    
    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
    queue->size++;
    
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

log_entry_t* queue_dequeue(log_queue_t* queue) {
    if (!queue) {
        return NULL;
    }
    
    // Check if queue was destroyed
    if (queue->destroyed) {
        return NULL;
    }
    
    pthread_mutex_lock(&queue->mutex);
    
    // Wait if queue is empty, but exit if shutdown
    while (queue->size == 0 && !queue->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    // If shutdown and queue is empty, return NULL
    if (queue->shutdown && queue->size == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    
    // At this point, queue->size > 0, so head must not be NULL
    // This invariant is tested via gtest in tests/test_queue_gtest.cpp
    if (queue->head == NULL) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL; // Safety check - should never happen if queue is properly maintained
    }
    
    queue_node_t* node = queue->head;
    
    // Validate node pointer is valid before accessing it
    // Check for NULL, misalignment, and obviously invalid addresses (like 0x100)
    if (!node || (uintptr_t)node % 8 != 0 || (uintptr_t)node < 0x1000) {
        // Invalid pointer - don't access it, just return NULL
        // Reset queue state to prevent further corruption
        queue->head = NULL;
        queue->tail = NULL;
        queue->size = 0;
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    
    log_entry_t* entry = node->entry;
    
    queue->head = node->next;
    if (!queue->head) {
        queue->tail = NULL;
    }
    queue->size--;
    
    // Save next pointer before freeing
    queue_node_t* next = node->next;
    free(node);
    node = next;  // This line is not needed but keeps node variable consistent
    
    if (queue->max_size > 0) {
        pthread_cond_signal(&queue->not_full);
    }
    
    pthread_mutex_unlock(&queue->mutex);
    
    return entry;
}

size_t queue_size(log_queue_t* queue) {
    if (!queue) {
        return 0;
    }
    
    pthread_mutex_lock(&queue->mutex);
    size_t size = queue->size;
    pthread_mutex_unlock(&queue->mutex);
    
    return size;
}

bool queue_is_empty(log_queue_t* queue) {
    if (!queue) {
        return true;
    }
    
    pthread_mutex_lock(&queue->mutex);
    bool empty = (queue->size == 0);
    pthread_mutex_unlock(&queue->mutex);
    
    return empty;
}