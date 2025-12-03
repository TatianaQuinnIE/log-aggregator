#ifndef ALERTER_H
#define ALERTER_H

#include "log_entry.h"
#include "queue.h"
#include "config.h"
#include <stdio.h>
#include <stdbool.h>

/**
 * @file alerter.h
 * @brief Alert generation and output
 */

// Alerter structure
typedef struct {
    log_queue_t* alert_queue;
    config_t* config;
    bool running;
    pthread_t thread;
    FILE* alert_file;
} alerter_t;

/**
 * @brief Initialize alerter
 * @param alerter Alerter to initialize
 * @param alert_queue Queue containing logs to alert on
 * @param config Configuration
 * @return 0 on success, -1 on failure
 */
int alerter_init(alerter_t* alerter, log_queue_t* alert_queue, config_t* config);

/**
 * @brief Start alerting thread
 * @param alerter Alerter to start
 * @return 0 on success, -1 on failure
 */
int alerter_start(alerter_t* alerter);

/**
 * @brief Stop alerting
 * @param alerter Alerter to stop
 */
void alerter_stop(alerter_t* alerter);

/**
 * @brief Destroy alerter
 * @param alerter Alerter to destroy
 */
void alerter_destroy(alerter_t* alerter);

/**
 * @brief Write alert to file
 * @param alerter Alerter instance
 * @param entry Log entry to alert on
 */
void alerter_write_alert(alerter_t* alerter, log_entry_t* entry);

#endif // ALERTER_H

