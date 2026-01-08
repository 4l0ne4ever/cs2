// logger.c - Server-side logging implementation

#include "../../include/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <errno.h>

static FILE *g_log_file = NULL;
static LogLevel g_min_level = LOG_LEVEL_DEBUG;
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
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
    pthread_mutex_lock(&g_log_mutex);
    
    if (g_logger_initialized)
    {
        pthread_mutex_unlock(&g_log_mutex);
        return 0; // Already initialized
    }
    
    g_min_level = min_level;
    
    if (log_file_path)
    {
        g_log_file = fopen(log_file_path, "a");
        if (!g_log_file)
        {
            fprintf(stderr, "Failed to open log file %s: %s\n", log_file_path, strerror(errno));
            pthread_mutex_unlock(&g_log_mutex);
            return -1;
        }
        // Set line buffering for immediate writes
        setvbuf(g_log_file, NULL, _IOLBF, 0);
    }
    
    g_logger_initialized = 1;
    pthread_mutex_unlock(&g_log_mutex);
    
    LOG_INFO("Logger initialized (min_level=%s, file=%s)", 
             get_level_string(min_level), 
             log_file_path ? log_file_path : "disabled");
    
    return 0;
}

// Close logger
void logger_close(void)
{
    pthread_mutex_lock(&g_log_mutex);
    
    if (g_log_file)
    {
        fflush(g_log_file);
        fclose(g_log_file);
        g_log_file = NULL;
    }
    
    g_logger_initialized = 0;
    pthread_mutex_unlock(&g_log_mutex);
}

// Log a message
void logger_log(LogLevel level, const char *file, int line, const char *func, const char *format, ...)
{
    if (level < g_min_level)
        return;
    
    pthread_mutex_lock(&g_log_mutex);
    
    char timestamp[64];
    format_timestamp(timestamp, sizeof(timestamp));
    const char *filename = get_filename(file);
    const char *level_str = get_level_string(level);
    const char *level_color = get_level_color(level);
    const char *color_reset = "\033[0m";
    
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
    
    // Write to terminal (with colors)
    if (len > 0 && len < (int)sizeof(log_line))
    {
        fprintf(stdout, "%s%s%s", level_color, log_line, color_reset);
        fflush(stdout);
    }
    
    // Write to file (without colors)
    if (g_log_file)
    {
        fprintf(g_log_file, "%s", log_line);
        fflush(g_log_file);
    }
    
    pthread_mutex_unlock(&g_log_mutex);
}

// Log with context
void logger_log_with_context(LogLevel level, const char *file, int line, const char *func,
                              int user_id, int client_fd, const char *format, ...)
{
    if (level < g_min_level)
        return;
    
    pthread_mutex_lock(&g_log_mutex);
    
    char timestamp[64];
    format_timestamp(timestamp, sizeof(timestamp));
    const char *filename = get_filename(file);
    const char *level_str = get_level_string(level);
    const char *level_color = get_level_color(level);
    const char *color_reset = "\033[0m";
    
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
    
    // Write to terminal (with colors)
    if (len > 0 && len < (int)sizeof(log_line))
    {
        fprintf(stdout, "%s%s%s", level_color, log_line, color_reset);
        fflush(stdout);
    }
    
    // Write to file (without colors)
    if (g_log_file)
    {
        fprintf(g_log_file, "%s", log_line);
        fflush(g_log_file);
    }
    
    pthread_mutex_unlock(&g_log_mutex);
}

