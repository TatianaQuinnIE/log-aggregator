#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>

#include "config.h"
#include "queue.h"
#include "log_source.h"
#include "processor.h"
#include "alerter.h"

// Global flag for graceful shutdown
static volatile bool g_running = true;

// Signal handler for graceful shutdown
static void signal_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    g_running = false;
}

int main(int argc, char* argv[]) {
    const char* config_file = "config.txt";
    
    // Parse command line arguments
    if (argc > 1) {
        config_file = argv[1];
    }
    
    printf("Log Aggregator Starting...\n");
    printf("Loading configuration from: %s\n", config_file);
    
    // Load configuration
    config_t config;
    if (config_load(&config, config_file) != 0) {
        fprintf(stderr, "Failed to load configuration\n");
        return 1;
    }
    
    // Initialize queues
    log_queue_t input_queue;
    log_queue_t alert_queue;
    
    if (queue_init(&input_queue, config.queue_max_size) != 0) {
        fprintf(stderr, "Failed to initialize input queue\n");
        config_destroy(&config);
        return 1;
    }
    
    if (queue_init(&alert_queue, config.queue_max_size) != 0) {
        fprintf(stderr, "Failed to initialize alert queue\n");
        queue_destroy(&input_queue);
        config_destroy(&config);
        return 1;
    }
    
    // Initialize file monitors
    file_monitor_t* monitors = NULL;
    if (config.num_directories > 0) {
        monitors = (file_monitor_t*)calloc(config.num_directories, sizeof(file_monitor_t));
        if (!monitors) {
            fprintf(stderr, "Failed to allocate memory for monitors\n");
            queue_destroy(&alert_queue);
            queue_destroy(&input_queue);
            config_destroy(&config);
            return 1;
        }
        
        for (size_t i = 0; i < config.num_directories; i++) {
            if (file_monitor_init(&monitors[i], config.watch_directories[i],
                                 config.poll_interval_seconds, &input_queue) != 0) {
                fprintf(stderr, "Failed to initialize monitor for %s\n", 
                       config.watch_directories[i]);
                // Cleanup already initialized monitors
                for (size_t j = 0; j < i; j++) {
                    file_monitor_destroy(&monitors[j]);
                }
                free(monitors);
                queue_destroy(&alert_queue);
                queue_destroy(&input_queue);
                config_destroy(&config);
                return 1;
            }
            
            if (file_monitor_start(&monitors[i]) != 0) {
                fprintf(stderr, "Failed to start monitor for %s\n",
                       config.watch_directories[i]);
                file_monitor_destroy(&monitors[i]);
                for (size_t j = 0; j < i; j++) {
                    file_monitor_stop(&monitors[j]);
                    file_monitor_destroy(&monitors[j]);
                }
                free(monitors);
                queue_destroy(&alert_queue);
                queue_destroy(&input_queue);
                config_destroy(&config);
                return 1;
            }
            
            printf("Monitoring directory: %s\n", config.watch_directories[i]);
        }
    }
    
    // Initialize network server
    network_server_t network_server;
    if (config.enable_network) {
        if (network_server_init(&network_server, config.network_port, &input_queue) != 0) {
            fprintf(stderr, "Failed to initialize network server\n");
            // Cleanup
            if (monitors) {
                for (size_t i = 0; i < config.num_directories; i++) {
                    file_monitor_stop(&monitors[i]);
                    file_monitor_destroy(&monitors[i]);
                }
                free(monitors);
            }
            queue_destroy(&alert_queue);
            queue_destroy(&input_queue);
            config_destroy(&config);
            return 1;
        }
        
        if (network_server_start(&network_server) != 0) {
            fprintf(stderr, "Failed to start network server\n");
            network_server_destroy(&network_server);
            if (monitors) {
                for (size_t i = 0; i < config.num_directories; i++) {
                    file_monitor_stop(&monitors[i]);
                    file_monitor_destroy(&monitors[i]);
                }
                free(monitors);
            }
            queue_destroy(&alert_queue);
            queue_destroy(&input_queue);
            config_destroy(&config);
            return 1;
        }
    }
    
    // Initialize processor
    processor_t processor;
    if (processor_init(&processor, &input_queue, &alert_queue, &config) != 0) {
        fprintf(stderr, "Failed to initialize processor\n");
        // Cleanup
        if (config.enable_network) {
            network_server_stop(&network_server);
            network_server_destroy(&network_server);
        }
        if (monitors) {
            for (size_t i = 0; i < config.num_directories; i++) {
                file_monitor_stop(&monitors[i]);
                file_monitor_destroy(&monitors[i]);
            }
            free(monitors);
        }
        queue_destroy(&alert_queue);
        queue_destroy(&input_queue);
        config_destroy(&config);
        return 1;
    }
    
    if (processor_start(&processor) != 0) {
        fprintf(stderr, "Failed to start processor\n");
        processor_destroy(&processor);
        if (config.enable_network) {
            network_server_stop(&network_server);
            network_server_destroy(&network_server);
        }
        if (monitors) {
            for (size_t i = 0; i < config.num_directories; i++) {
                file_monitor_stop(&monitors[i]);
                file_monitor_destroy(&monitors[i]);
            }
            free(monitors);
        }
        queue_destroy(&alert_queue);
        queue_destroy(&input_queue);
        config_destroy(&config);
        return 1;
    }
    
    // Initialize alerter
    alerter_t alerter;
    if (alerter_init(&alerter, &alert_queue, &config) != 0) {
        fprintf(stderr, "Failed to initialize alerter\n");
        processor_stop(&processor);
        processor_destroy(&processor);
        if (config.enable_network) {
            network_server_stop(&network_server);
            network_server_destroy(&network_server);
        }
        if (monitors) {
            for (size_t i = 0; i < config.num_directories; i++) {
                file_monitor_stop(&monitors[i]);
                file_monitor_destroy(&monitors[i]);
            }
            free(monitors);
        }
        queue_destroy(&alert_queue);
        queue_destroy(&input_queue);
        config_destroy(&config);
        return 1;
    }
    
    if (alerter_start(&alerter) != 0) {
        fprintf(stderr, "Failed to start alerter\n");
        alerter_destroy(&alerter);
        processor_stop(&processor);
        processor_destroy(&processor);
        if (config.enable_network) {
            network_server_stop(&network_server);
            network_server_destroy(&network_server);
        }
        if (monitors) {
            for (size_t i = 0; i < config.num_directories; i++) {
                file_monitor_stop(&monitors[i]);
                file_monitor_destroy(&monitors[i]);
            }
            free(monitors);
        }
        queue_destroy(&alert_queue);
        queue_destroy(&input_queue);
        config_destroy(&config);
        return 1;
    }
    
    printf("Log Aggregator running. Press Ctrl+C to stop.\n");
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Main loop - wait for shutdown signal
    while (g_running) {
        sleep(1);
    }
    
    printf("\nShutting down...\n");
    
    // Mark queues as shutdown first (this wakes up waiting threads)
    queue_shutdown(&input_queue);
    queue_shutdown(&alert_queue);
    
    // Stop components (this sets running flags to false and waits for threads)
    alerter_stop(&alerter);
    processor_stop(&processor);
    
    if (config.enable_network) {
        network_server_stop(&network_server);
    }
    
    if (monitors) {
        for (size_t i = 0; i < config.num_directories; i++) {
            file_monitor_stop(&monitors[i]);
        }
    }
    
    // Now destroy queues (cleanup remaining entries and resources)
    queue_destroy(&alert_queue);
    queue_destroy(&input_queue);
    
    // Now destroy components
    alerter_destroy(&alerter);
    processor_destroy(&processor);
    
    if (config.enable_network) {
        network_server_destroy(&network_server);
    }
    
    if (monitors) {
        for (size_t i = 0; i < config.num_directories; i++) {
            file_monitor_destroy(&monitors[i]);
        }
        free(monitors);
    }
    
    config_destroy(&config);
    
    printf("Shutdown complete.\n");
    return 0;
}