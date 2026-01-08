// logger.c - Client-side logging implementation

#include "../../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>

static FILE *g_log_file = NULL;
static LogLevel g_min_level = LOG_LEVEL_DEBUG;
static int g_logger_initialized = 0;

// Get log level string
static const char *get_level_string(LogLevel level)
{
    switch (level)
    {
    case LOG_LEVEL_DEBUG:
        return "DEBUG";
    case LOG_LEVEL_INFO:
        return "INFO";
    case LOG_LEVEL_WARNING:
        return "WARN";
    case LOG_LEVEL_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

// Get log level color for terminal (ANSI codes)
static const char *get_level_color(LogLevel level)
{
    switch (level)
    {
    case LOG_LEVEL_DEBUG:
        return "\033[0;36m"; // Cyan
    case LOG_LEVEL_INFO:
        return "\033[0;32m"; // Green
    case LOG_LEVEL_WARNING:
        return "\033[0;33m"; // Yellow
    case LOG_LEVEL_ERROR:
        return "\033[0;31m"; // Red
    default:
        return "\033[0m"; // Reset
    }
}

// Format timestamp
static void format_timestamp(char *buffer, size_t buffer_size)
{
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// Get filename from full path
static const char *get_filename(const char *filepath)
{
    const char *filename = strrchr(filepath, '/');
    return filename ? filename + 1 : filepath;
}

// Initialize logger
int logger_init(const char *log_file_path, LogLevel min_level)
{
    if (g_logger_initialized)
        return 0; // Already initialized
    
    g_min_level = min_level;
    
    if (log_file_path)
    {
        g_log_file = fopen(log_file_path, "a");
        if (!g_log_file)
        {
            fprintf(stderr, "Failed to open log file %s: %s\n", log_file_path, strerror(errno));
            return -1;
        }
        // Set line buffering for immediate writes
        setvbuf(g_log_file, NULL, _IOLBF, 0);
    }
    
    g_logger_initialized = 1;
    
    // Don't log initialization message for client (would clutter UI)
    // Just write to file if available
    if (g_log_file)
    {
        char timestamp[64];
        format_timestamp(timestamp, sizeof(timestamp));
        fprintf(g_log_file, "[%s] [INFO] [logger.c:%d:logger_init] Logger initialized (min_level=%s, file=%s)\n",
                timestamp, __LINE__, get_level_string(min_level), 
                log_file_path ? log_file_path : "disabled");
        fflush(g_log_file);
    }
    
    return 0;
}

// Close logger
void logger_close(void)
{
    if (g_log_file)
    {
        fflush(g_log_file);
        fclose(g_log_file);
        g_log_file = NULL;
    }
    
    g_logger_initialized = 0;
}

// Log a message
void logger_log(LogLevel level, const char *file, int line, const char *func, const char *format, ...)
{
    if (level < g_min_level)
        return;
    
    char timestamp[64];
    format_timestamp(timestamp, sizeof(timestamp));
    const char *filename = get_filename(file);
    const char *level_str = get_level_string(level);
    
    // Format message
    char message[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // Format log line
    char log_line[4096];
    int len = snprintf(log_line, sizeof(log_line),
                       "[%s] [%s] [%s:%d:%s] %s\n",
                       timestamp, level_str, filename, line, func, message);
    
    // Client logger: Only write to file (not terminal to avoid cluttering UI)
    if (g_log_file && len > 0 && len < (int)sizeof(log_line))
    {
        fprintf(g_log_file, "%s", log_line);
        fflush(g_log_file);
    }
}

// Log with context
void logger_log_with_context(LogLevel level, const char *file, int line, const char *func,
                              int user_id, int client_fd, const char *format, ...)
{
    if (level < g_min_level)
        return;
    
    char timestamp[64];
    format_timestamp(timestamp, sizeof(timestamp));
    const char *filename = get_filename(file);
    const char *level_str = get_level_string(level);
    
    // Format message
    char message[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // Format log line with context
    char log_line[4096];
    int len;
    if (user_id > 0 && client_fd > 0)
    {
        len = snprintf(log_line, sizeof(log_line),
                       "[%s] [%s] [%s:%d:%s] [user_id=%d, fd=%d] %s\n",
                       timestamp, level_str, filename, line, func, user_id, client_fd, message);
    }
    else if (user_id > 0)
    {
        len = snprintf(log_line, sizeof(log_line),
                       "[%s] [%s] [%s:%d:%s] [user_id=%d] %s\n",
                       timestamp, level_str, filename, line, func, user_id, message);
    }
    else if (client_fd > 0)
    {
        len = snprintf(log_line, sizeof(log_line),
                       "[%s] [%s] [%s:%d:%s] [fd=%d] %s\n",
                       timestamp, level_str, filename, line, func, client_fd, message);
    }
    else
    {
        len = snprintf(log_line, sizeof(log_line),
                       "[%s] [%s] [%s:%d:%s] %s\n",
                       timestamp, level_str, filename, line, func, message);
    }
    
    // Client logger: Only write to file (not terminal to avoid cluttering UI)
    if (g_log_file && len > 0 && len < (int)sizeof(log_line))
    {
        fprintf(g_log_file, "%s", log_line);
        fflush(g_log_file);
    }
}

