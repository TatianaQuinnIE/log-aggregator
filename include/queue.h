#ifndef QUEUE_H
#define QUEUE_H

#include "log_entry.h"
#include <pthread.h>
#include <stdbool.h>

/**
 * @file queue.h
 * @brief Thread-safe queue for log entries
 */

// Queue node structure
typedef struct queue_node {
    log_entry_t* entry;
    struct queue_node* next;
} queue_node_t;

// Thread-safe queue structure
typedef struct {
    queue_node_t* head;
    queue_node_t* tail;
    size_t size;
    size_t max_size;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} log_queue_t;

/**
 * @brief Initialize a log queue
 * @param queue Queue to initialize
 * @param max_size Maximum queue size (0 for unlimited)
 * @return 0 on success, -1 on failure
 */
int queue_init(log_queue_t* queue, size_t max_size);

/**
 * @brief Destroy a log queue and free resources
 * @param queue Queue to destroy
 */
void queue_destroy(log_queue_t* queue);

/**
 * @brief Enqueue a log entry (thread-safe)
 * @param queue Queue to add to
 * @param entry Log entry to enqueue
 * @return 0 on success, -1 on failure
 */
int queue_enqueue(log_queue_t* queue, log_entry_t* entry);

/**
 * @brief Dequeue a log entry (thread-safe, blocks if empty)
 * @param queue Queue to remove from
 * @return Log entry pointer, or NULL on error
 */
log_entry_t* queue_dequeue(log_queue_t* queue);

/**
 * @brief Get current queue size
 * @param queue Queue to check
 * @return Current size
 */
size_t queue_size(log_queue_t* queue);

/**
 * @brief Check if queue is empty
 * @param queue Queue to check
 * @return true if empty, false otherwise
 */
bool queue_is_empty(log_queue_t* queue);

#endif // QUEUE_H

