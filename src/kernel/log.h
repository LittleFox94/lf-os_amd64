#ifndef _LOG_H_INCLUDED
#define _LOG_H_INCLUDED

void log_append(char level, char* component, char* message);
void log(char level, char* component, char* fmt, ...);

#define logd(...) log('D', __VA_ARGS__)
#define logi(...) log('I', __VA_ARGS__)
#define logw(...) log('W', __VA_ARGS__)
#define loge(...) log('E', __VA_ARGS__)
#define logf(...) log('F', __VA_ARGS__)

#endif
