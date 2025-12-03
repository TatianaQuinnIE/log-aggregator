#ifndef CONFIG_H
#define CONFIG_H

#include "log_entry.h"
#include <stdbool.h>

/**
 * @file config.h
 * @brief Configuration management
 */

// Configuration structure
typedef struct {
    // File monitoring
    char** watch_directories;      // Directories to watch
    size_t num_directories;        // Number of directories
    int poll_interval_seconds;     // How often to poll directories
    
    // Network
    int network_port;              // Port for network log reception
    bool enable_network;           // Enable network log source
    
    // Processing
    size_t queue_max_size;         // Maximum queue size
    int num_processing_threads;    // Number of processing threads
    
    // Alerting
    bool enable_alerts;            // Enable alerting
    char* alert_file;              // File to write alerts to
    log_level_t alert_threshold;   // Minimum level to alert on
    
    // Pattern detection
    char** alert_patterns;         // Patterns to alert on
    size_t num_patterns;           // Number of patterns
} config_t;

/**
 * @brief Load configuration from file
 * @param config Config structure to populate
 * @param filename Configuration file path
 * @return 0 on success, -1 on failure
 */
int config_load(config_t* config, const char* filename);

/**
 * @brief Free configuration resources
 * @param config Config to free
 */
void config_destroy(config_t* config);

/**
 * @brief Initialize config with default values
 * @param config Config to initialize
 */
void config_init_defaults(config_t* config);

#endif // CONFIG_H

