#include <gtest/gtest.h>
#include <pthread.h>
#include <unistd.h>
#include <cstring>

// Include C headers with extern "C" to make them compatible with C++
extern "C" {
#include "../include/queue.h"
#include "../include/log_entry.h"
}

// Test that queue_dequeue maintains the invariant: if size > 0, head != NULL
TEST(QueueTest, DequeueHeadNotNullInvariant) {
    log_queue_t queue;
    
    // Initialize queue
    ASSERT_EQ(queue_init(&queue, 100), 0);
    
    // Create and enqueue an entry
    log_entry_t* entry = log_entry_create("test.log", "Test message", LOG_LEVEL_INFO, "raw line");
    ASSERT_NE(entry, nullptr);
    ASSERT_EQ(queue_enqueue(&queue, entry), 0);
    
    // Verify queue is not empty
    ASSERT_FALSE(queue_is_empty(&queue));
    ASSERT_EQ(queue_size(&queue), 1);
    
    // Dequeue - at this point, queue->size > 0, so head must not be NULL
    // This tests the invariant that was previously checked with assert()
    log_entry_t* dequeued = queue_dequeue(&queue);
    
    // The dequeue should succeed and return a valid entry
    ASSERT_NE(dequeued, nullptr);
    ASSERT_STREQ(dequeued->source, "test.log");
    ASSERT_STREQ(dequeued->message, "Test message");
    
    // Queue should now be empty
    ASSERT_TRUE(queue_is_empty(&queue));
    ASSERT_EQ(queue_size(&queue), 0);
    
    // Cleanup
    log_entry_destroy(dequeued);
    queue_destroy(&queue);
}

// Test multiple dequeues to ensure head invariant is maintained
TEST(QueueTest, MultipleDequeuesHeadInvariant) {
    log_queue_t queue;
    
    ASSERT_EQ(queue_init(&queue, 100), 0);
    
    // Enqueue multiple entries
    for (int i = 0; i < 10; i++) {
        char source[64];
        char message[64];
        snprintf(source, sizeof(source), "test%d.log", i);
        snprintf(message, sizeof(message), "Message %d", i);
        
        log_entry_t* entry = log_entry_create(source, message, LOG_LEVEL_INFO, message);
        ASSERT_NE(entry, nullptr);
        ASSERT_EQ(queue_enqueue(&queue, entry), 0);
    }
    
    ASSERT_EQ(queue_size(&queue), 10);
    
    // Dequeue all entries - each dequeue should maintain the head != NULL invariant
    for (int i = 0; i < 10; i++) {
        ASSERT_FALSE(queue_is_empty(&queue));
        log_entry_t* dequeued = queue_dequeue(&queue);
        ASSERT_NE(dequeued, nullptr); // Head should not be NULL when size > 0
        log_entry_destroy(dequeued);
    }
    
    ASSERT_TRUE(queue_is_empty(&queue));
    ASSERT_EQ(queue_size(&queue), 0);
    
    queue_destroy(&queue);
}

// Helper function for thread test
struct thread_data {
    log_queue_t* queue;
    bool enqueued;
};

static void* enqueue_thread_func(void* arg) {
    thread_data* d = static_cast<thread_data*>(arg);
    sleep(1); // Wait 1 second
    log_entry_t* entry = log_entry_create("delayed.log", "Delayed message", 
                                         LOG_LEVEL_INFO, "raw");
    queue_enqueue(d->queue, entry);
    d->enqueued = true;
    return nullptr;
}

// Test that dequeue from empty queue blocks (thread safety test)
TEST(QueueTest, DequeueFromEmptyQueueBlocks) {
    log_queue_t queue;
    ASSERT_EQ(queue_init(&queue, 100), 0);
    
    // This test verifies that dequeue blocks when queue is empty
    // We'll use a separate thread to enqueue after a delay
    thread_data data = {&queue, false};
    
    pthread_t thread;
    pthread_create(&thread, nullptr, enqueue_thread_func, &data);
    
    // This should block until the other thread enqueues
    log_entry_t* dequeued = queue_dequeue(&queue);
    
    ASSERT_NE(dequeued, nullptr);
    ASSERT_TRUE(data.enqueued);
    ASSERT_STREQ(dequeued->source, "delayed.log");
    
    log_entry_destroy(dequeued);
    pthread_join(thread, nullptr);
    queue_destroy(&queue);
}

