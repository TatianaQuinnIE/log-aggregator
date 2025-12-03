#ifndef LOG_SOURCE_H
#define LOG_SOURCE_H

#include "queue.h"
#include "config.h"
#include <stdbool.h>

/**
 * @file log_source.h
 * @brief Log source monitoring (files and network)
 */

// File monitor structure
typedef struct {
    char* directory;
    int poll_interval;
    log_queue_t* queue;
    bool running;
    pthread_t thread;
} file_monitor_t;

// Network server structure
typedef struct {
    int port;
    log_queue_t* queue;
    bool running;
    pthread_t thread;
    int server_fd;
} network_server_t;

/**
 * @brief Initialize file monitor
 * @param monitor Monitor to initialize
 * @param directory Directory to monitor
 * @param poll_interval Poll interval in seconds
 * @param queue Queue to add logs to
 * @return 0 on success, -1 on failure
 */
int file_monitor_init(file_monitor_t* monitor, const char* directory, 
                      int poll_interval, log_queue_t* queue);

/**
 * @brief Start file monitoring thread
 * @param monitor Monitor to start
 * @return 0 on success, -1 on failure
 */
int file_monitor_start(file_monitor_t* monitor);

/**
 * @brief Stop file monitoring
 * @param monitor Monitor to stop
 */
void file_monitor_stop(file_monitor_t* monitor);

/**
 * @brief Destroy file monitor
 * @param monitor Monitor to destroy
 */
void file_monitor_destroy(file_monitor_t* monitor);

/**
 * @brief Initialize network server
 * @param server Server to initialize
 * @param port Port to listen on
 * @param queue Queue to add logs to
 * @return 0 on success, -1 on failure
 */
int network_server_init(network_server_t* server, int port, log_queue_t* queue);

/**
 * @brief Start network server thread
 * @param server Server to start
 * @return 0 on success, -1 on failure
 */
int network_server_start(network_server_t* server);

/**
 * @brief Stop network server
 * @param server Server to stop
 */
void network_server_stop(network_server_t* server);

/**
 * @brief Destroy network server
 * @param server Server to destroy
 */
void network_server_destroy(network_server_t* server);

#endif // LOG_SOURCE_H

