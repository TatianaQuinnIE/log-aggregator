#include "../include/log_entry.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

void test_log_entry(void) {
    // Test log entry creation
    log_entry_t* entry = log_entry_create("test.log", "Test message", 
                                          LOG_LEVEL_INFO, "raw line");
    assert(entry != NULL);
    assert(strcmp(entry->source, "test.log") == 0);
    assert(strcmp(entry->message, "Test message") == 0);
    assert(strcmp(entry->raw_line, "raw line") == 0);
    assert(entry->level == LOG_LEVEL_INFO);
    assert(entry->timestamp > 0);
    
    // Test level parsing
    assert(log_entry_parse_level("DEBUG") == LOG_LEVEL_DEBUG);
    assert(log_entry_parse_level("INFO") == LOG_LEVEL_INFO);
    assert(log_entry_parse_level("WARNING") == LOG_LEVEL_WARNING);
    assert(log_entry_parse_level("ERROR") == LOG_LEVEL_ERROR);
    assert(log_entry_parse_level("CRITICAL") == LOG_LEVEL_CRITICAL);
    assert(log_entry_parse_level("debug") == LOG_LEVEL_DEBUG); // Case insensitive
    assert(log_entry_parse_level("unknown") == LOG_LEVEL_INFO); // Default
    
    // Test level to string
    assert(strcmp(log_entry_level_to_string(LOG_LEVEL_DEBUG), "DEBUG") == 0);
    assert(strcmp(log_entry_level_to_string(LOG_LEVEL_INFO), "INFO") == 0);
    assert(strcmp(log_entry_level_to_string(LOG_LEVEL_WARNING), "WARNING") == 0);
    assert(strcmp(log_entry_level_to_string(LOG_LEVEL_ERROR), "ERROR") == 0);
    assert(strcmp(log_entry_level_to_string(LOG_LEVEL_CRITICAL), "CRITICAL") == 0);
    
    // Test NULL handling
    log_entry_t* null_entry = log_entry_create(NULL, "msg", LOG_LEVEL_INFO, "raw");
    assert(null_entry == NULL);
    
    null_entry = log_entry_create("src", NULL, LOG_LEVEL_INFO, "raw");
    assert(null_entry == NULL);
    
    // Test destruction
    log_entry_destroy(entry);
    log_entry_destroy(NULL); // Should handle NULL gracefully
}