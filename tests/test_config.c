#include "../include/config.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void test_config(void) {
    config_t config;
    
    // Test default initialization
    config_init_defaults(&config);
    assert(config.poll_interval_seconds == 5);
    assert(config.network_port == 8080);
    assert(config.enable_network == true);
    assert(config.queue_max_size == 1000);
    assert(config.num_processing_threads == 2);
    assert(config.enable_alerts == true);
    assert(config.alert_threshold == LOG_LEVEL_WARNING);
    assert(strcmp(config.alert_file, "alerts.log") == 0);
    
    // Test loading from non-existent file (should use defaults)
    assert(config_load(&config, "nonexistent.txt") == 0);
    assert(config.poll_interval_seconds == 5); // Should still be default
    
    // Create a test config file
    FILE* test_file = fopen("test_config.txt", "w");
    assert(test_file != NULL);
    fprintf(test_file, "# Test configuration file\n");
    fprintf(test_file, "poll_interval=10\n");
    fprintf(test_file, "network_port=9090\n");
    fprintf(test_file, "enable_network=false\n");
    fprintf(test_file, "queue_max_size=2000\n");
    fprintf(test_file, "num_processing_threads=4\n");
    fprintf(test_file, "enable_alerts=true\n");
    fprintf(test_file, "alert_file=test_alerts.log\n");
    fprintf(test_file, "alert_threshold=ERROR\n");
    fprintf(test_file, "watch_directory0=/var/log\n");
    fprintf(test_file, "watch_directory1=/tmp/logs\n");
    fprintf(test_file, "alert_pattern0=ERROR\n");
    fprintf(test_file, "alert_pattern1=CRITICAL\n");
    fclose(test_file);
    
    // Test loading from file
    assert(config_load(&config, "test_config.txt") == 0);
    assert(config.poll_interval_seconds == 10);
    assert(config.network_port == 9090);
    assert(config.enable_network == false);
    assert(config.queue_max_size == 2000);
    assert(config.num_processing_threads == 4);
    assert(config.enable_alerts == true);
    assert(strcmp(config.alert_file, "test_alerts.log") == 0);
    assert(config.alert_threshold == LOG_LEVEL_ERROR);
    assert(config.num_directories == 2);
    assert(strcmp(config.watch_directories[0], "/var/log") == 0);
    assert(strcmp(config.watch_directories[1], "/tmp/logs") == 0);
    assert(config.num_patterns == 2);
    assert(strcmp(config.alert_patterns[0], "ERROR") == 0);
    assert(strcmp(config.alert_patterns[1], "CRITICAL") == 0);
    
    // Cleanup
    config_destroy(&config);
    remove("test_config.txt");
}