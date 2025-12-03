#include "config.h"
#include "log_entry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_LINE_LENGTH 1024
#define MAX_DIRECTORIES 32
#define MAX_PATTERNS 64

void config_init_defaults(config_t* config) {
    if (!config) {
        return;
    }
    
    memset(config, 0, sizeof(config_t));
    
    config->poll_interval_seconds = 5;
    config->network_port = 8080;
    config->enable_network = true;
    config->queue_max_size = 1000;
    config->num_processing_threads = 2;
    config->enable_alerts = true;
    config->alert_file = strdup("alerts.log");
    config->alert_threshold = LOG_LEVEL_WARNING;
}

int config_load(config_t* config, const char* filename) {
    if (!config || !filename) {
        return -1;
    }
    
    // Initialize with defaults first
    config_init_defaults(config);
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        // File doesn't exist, use defaults
        return 0;
    }
    
    char line[MAX_LINE_LENGTH];
    char key[256];
    char value[768];
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        // Parse key=value pairs
        if (sscanf(line, "%255[^=]=%767s", key, value) == 2) {
            // Remove trailing whitespace from value
            size_t len = strlen(value);
            while (len > 0 && (value[len-1] == '\n' || value[len-1] == '\r' || value[len-1] == ' ')) {
                value[--len] = '\0';
            }
            
            if (strcmp(key, "poll_interval") == 0) {
                config->poll_interval_seconds = atoi(value);
            } else if (strcmp(key, "network_port") == 0) {
                config->network_port = atoi(value);
            } else if (strcmp(key, "enable_network") == 0) {
                config->enable_network = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            } else if (strcmp(key, "queue_max_size") == 0) {
                config->queue_max_size = (size_t)atoi(value);
            } else if (strcmp(key, "num_processing_threads") == 0) {
                config->num_processing_threads = atoi(value);
            } else if (strcmp(key, "enable_alerts") == 0) {
                config->enable_alerts = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
            } else if (strcmp(key, "alert_file") == 0) {
                free(config->alert_file);
                config->alert_file = strdup(value);
            } else if (strcmp(key, "alert_threshold") == 0) {
                config->alert_threshold = log_entry_parse_level(value);
            } else if (strncmp(key, "watch_directory", 15) == 0) {
                // Support multiple watch_directory entries
                if (config->num_directories < MAX_DIRECTORIES) {
                    if (!config->watch_directories) {
                        config->watch_directories = (char**)calloc(MAX_DIRECTORIES, sizeof(char*));
                    }
                    config->watch_directories[config->num_directories++] = strdup(value);
                }
            } else if (strncmp(key, "alert_pattern", 13) == 0) {
                // Support multiple alert_pattern entries
                if (config->num_patterns < MAX_PATTERNS) {
                    if (!config->alert_patterns) {
                        config->alert_patterns = (char**)calloc(MAX_PATTERNS, sizeof(char*));
                    }
                    config->alert_patterns[config->num_patterns++] = strdup(value);
                }
            }
        }
    }
    
    fclose(file);
    return 0;
}

void config_destroy(config_t* config) {
    if (!config) {
        return;
    }
    
    if (config->watch_directories) {
        for (size_t i = 0; i < config->num_directories; i++) {
            free(config->watch_directories[i]);
        }
        free(config->watch_directories);
    }
    
    if (config->alert_patterns) {
        for (size_t i = 0; i < config->num_patterns; i++) {
            free(config->alert_patterns[i]);
        }
        free(config->alert_patterns);
    }
    
    free(config->alert_file);
    memset(config, 0, sizeof(config_t));
}