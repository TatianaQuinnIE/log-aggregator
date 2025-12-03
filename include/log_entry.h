#ifndef LOG_ENTRY_H
#define LOG_ENTRY_H

#include <time.h>
#include <stdbool.h>

/**
 * @file log_entry.h
 * @brief Log entry data structure and operations
 */

// Log severity levels
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_CRITICAL = 4
} log_level_t;

// Log entry structure
typedef struct {
    char* source;           // Source identifier (file path or network client)
    char* message;          // Log message content
    log_level_t level;      // Severity level
    time_t timestamp;       // Timestamp when log was received
    char* raw_line;         // Original raw log line
} log_entry_t;

/**
 * @brief Create a new log entry
 * @param source Source identifier
 * @param message Log message
 * @param level Severity level
 * @param raw_line Original raw log line
 * @return Pointer to new log entry, or NULL on failure
 */
log_entry_t* log_entry_create(const char* source, const char* message, 
                               log_level_t level, const char* raw_line);

/**
 * @brief Free a log entry and its resources
 * @param entry Log entry to free
 */
void log_entry_destroy(log_entry_t* entry);

/**
 * @brief Parse log level from string
 * @param level_str String representation of level
 * @return Log level enum value
 */
log_level_t log_entry_parse_level(const char* level_str);

/**
 * @brief Get string representation of log level
 * @param level Log level enum
 * @return String representation
 */
const char* log_entry_level_to_string(log_level_t level);

#endif // LOG_ENTRY_H

