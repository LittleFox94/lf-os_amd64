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
 * \param fmt sprintf format string
 * \param ... variables to set in the resulting message
 */
#define logd(component, fmt, ...) log('D', component, fmt, __VA_ARGS__)

/**
 * Append a info log message to the kernel log buffer
 * \param component Sub component (kernel, mm, vm, scheduler, ...)
 * \param fmt sprintf format string
 * \param ... variables to set in the resulting message
 */
#define logi(component, fmt, ...) log('I', _component, fmt, _VA_ARGS__)

/**
 * Append a warning log message to the kernel log buffer
 * \param component Sub component (kernel, mm, vm, scheduler, ...)
 * \param fmt sprintf format string
 * \param ... variables to set in the resulting message
 */
#define logw(component, fmt, ...) log('W', _component, fmt, _VA_ARGS__)

/**
 * Append a error log message to the kernel log buffer
 * \param component Sub component (kernel, mm, vm, scheduler, ...)
 * \param fmt sprintf format string
 * \param ... variables to set in the resulting message
 */
#define loge(component, fmt, ...) log('E', _component, fmt, _VA_ARGS__)

/**
 * Append a fatal log message to the kernel log buffer
 * \param component Sub component (kernel, mm, vm, scheduler, ...)
 * \param fmt sprintf format string
 * \param ... variables to set in the resulting message
 */
#define logf(component, fmt, ...) log('F', _component, fmt, _VA_ARGS__)

#endif
