#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

// Log levels
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR
} LogLevel;

// Initialize logger
// log_file_path: path to log file (NULL to disable file logging)
// min_level: minimum log level to output (DEBUG=0, INFO=1, WARNING=2, ERROR=3)
int logger_init(const char *log_file_path, LogLevel min_level);

// Close logger and flush all buffers
void logger_close(void);

// Log a message with level, format string and arguments
void logger_log(LogLevel level, const char *file, int line, const char *func, const char *format, ...);

// Convenience macros
#define LOG_DEBUG(...) logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(...) logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARNING(...) logger_log(LOG_LEVEL_WARNING, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...) logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)

// Log with context (user_id, client_fd, etc.)
void logger_log_with_context(LogLevel level, const char *file, int line, const char *func, 
                              int user_id, int client_fd, const char *format, ...);

// Convenience macros with context
#define LOG_DEBUG_CTX(user_id, client_fd, ...) logger_log_with_context(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, user_id, client_fd, __VA_ARGS__)
#define LOG_INFO_CTX(user_id, client_fd, ...) logger_log_with_context(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, user_id, client_fd, __VA_ARGS__)
#define LOG_WARNING_CTX(user_id, client_fd, ...) logger_log_with_context(LOG_LEVEL_WARNING, __FILE__, __LINE__, __func__, user_id, client_fd, __VA_ARGS__)
#define LOG_ERROR_CTX(user_id, client_fd, ...) logger_log_with_context(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, user_id, client_fd, __VA_ARGS__)

#endif // LOGGER_H

