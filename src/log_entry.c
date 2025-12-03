#include "log_entry.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>

#define MAX_LEVEL_STRING 16

log_entry_t* log_entry_create(const char* source, const char* message, 
                               log_level_t level, const char* raw_line) {
    if (!source || !message || !raw_line) {
        return NULL;
    }
    
    log_entry_t* entry = (log_entry_t*)malloc(sizeof(log_entry_t));
    if (!entry) {
        return NULL;
    }
    
    entry->source = strdup(source);
    entry->message = strdup(message);
    entry->raw_line = strdup(raw_line);
    entry->level = level;
    entry->timestamp = time(NULL);
    
    if (!entry->source || !entry->message || !entry->raw_line) {
        log_entry_destroy(entry);
        return NULL;
    }
    
    return entry;
}

void log_entry_destroy(log_entry_t* entry) {
    if (!entry) {
        return;
    }
    
    free(entry->source);
    free(entry->message);
    free(entry->raw_line);
    free(entry);
}

log_level_t log_entry_parse_level(const char* level_str) {
    if (!level_str) {
        return LOG_LEVEL_INFO;
    }
    
    // Convert to lowercase for case-insensitive matching
    char lower[MAX_LEVEL_STRING];
    size_t len = strlen(level_str);
    if (len >= MAX_LEVEL_STRING) {
        len = MAX_LEVEL_STRING - 1;
    }
    
    for (size_t i = 0; i < len; i++) {
        lower[i] = tolower(level_str[i]);
    }
    lower[len] = '\0';
    
    if (strstr(lower, "debug") || strstr(lower, "dbg")) {
        return LOG_LEVEL_DEBUG;
    } else if (strstr(lower, "info")) {
        return LOG_LEVEL_INFO;
    } else if (strstr(lower, "warn")) {
        return LOG_LEVEL_WARNING;
    } else if (strstr(lower, "error") || strstr(lower, "err")) {
        return LOG_LEVEL_ERROR;
    } else if (strstr(lower, "critical") || strstr(lower, "crit") || strstr(lower, "fatal")) {
        return LOG_LEVEL_CRITICAL;
    }
    
    return LOG_LEVEL_INFO; // Default
}

const char* log_entry_level_to_string(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG:
            return "DEBUG";
        case LOG_LEVEL_INFO:
            return "INFO";
        case LOG_LEVEL_WARNING:
            return "WARNING";
        case LOG_LEVEL_ERROR:
            return "ERROR";
        case LOG_LEVEL_CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}