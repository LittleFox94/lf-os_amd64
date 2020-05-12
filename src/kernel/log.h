#ifndef _LOG_H_INCLUDED
#define _LOG_H_INCLUDED

/**
 * Append a log message to the kernel log buffer
 * \param level Log level of the message (D, I, W, E, F -> debug, info, warning, error, fatal)
 * \param component Sub component (kernel, mm, vm, scheduler, ...)
 * \param message Message to log
 */
void log_append(char level, char* component, char* message);

/**
 * Append a log message to the kernel log buffer
 * \param level Log level of the message (D, I, W, E, F -> debug, info, warning, error, fatal)
 * \param component Sub component (kernel, mm, vm, scheduler, ...)
 * \param fmt sprintf format string
 * \param ... Variables to set in the resulting message
 */
void log(char level, char* component, char* fmt, ...);


/**
 * Append a debug log message to the kernel log buffer
 * \param component Sub component (kernel, mm, vm, scheduler, ...)
 * \param ... sprintf format string variables to set in the resulting message
 */
#define logd(...) log('D', __VA_ARGS__)

/**
 * Append a info log message to the kernel log buffer
 * \param component Sub component (kernel, mm, vm, scheduler, ...)
 * \param ... sprintf format string variables to set in the resulting message
 */
#define logi(...) log('I', __VA_ARGS__)

/**
 * Append a warning log message to the kernel log buffer
 * \param component Sub component (kernel, mm, vm, scheduler, ...)
 * \param ... sprintf format string variables to set in the resulting message
 */
#define logw(...) log('W', __VA_ARGS__)

/**
 * Append a error log message to the kernel log buffer
 * \param component Sub component (kernel, mm, vm, scheduler, ...)
 * \param ... sprintf format string variables to set in the resulting message
 */
#define loge(...) log('E', __VA_ARGS__)

/**
 * Append a fatal log message to the kernel log buffer
 * \param component Sub component (kernel, mm, vm, scheduler, ...)
 * \param ... sprintf format string variables to set in the resulting message
 */
#define logf(...) log('F', __VA_ARGS__)

#endif
