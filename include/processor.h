#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "log_entry.h"
#include "queue.h"
#include "config.h"
#include <stdbool.h>

/**
 * @file processor.h
 * @brief Log processing and pattern detection
 */

// Processor structure
typedef struct {
    log_queue_t* input_queue;
    log_queue_t* output_queue;
    config_t* config;
    bool running;
    pthread_t* threads;
    int num_threads;
} processor_t;

/**
 * @brief Initialize processor
 * @param processor Processor to initialize
 * @param input_queue Input queue
 * @param output_queue Output queue
 * @param config Configuration
 * @return 0 on success, -1 on failure
 */
int processor_init(processor_t* processor, log_queue_t* input_queue,
                   log_queue_t* output_queue, config_t* config);

/**
 * @brief Start processing threads
 * @param processor Processor to start
 * @return 0 on success, -1 on failure
 */
int processor_start(processor_t* processor);

/**
 * @brief Stop processing
 * @param processor Processor to stop
 */
void processor_stop(processor_t* processor);

/**
 * @brief Destroy processor
 * @param processor Processor to destroy
 */
void processor_destroy(processor_t* processor);

/**
 * @brief Process a single log entry
 * @param entry Log entry to process
 * @param config Configuration
 * @return true if entry should be alerted, false otherwise
 */
bool processor_process_entry(log_entry_t* entry, config_t* config);

/**
 * @brief Check if log entry matches alert patterns
 * @param entry Log entry to check
 * @param config Configuration
 * @return true if matches, false otherwise
 */
bool processor_check_patterns(log_entry_t* entry, config_t* config);

#endif // PROCESSOR_H

