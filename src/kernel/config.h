#ifndef BUILD_ID
  #define BUILD_ID "dev-"__DATE__" "__TIME__
#endif

#define FONT_NAME   acorndata_8x8
#define FONT_WIDTH  8
#define FONT_HEIGHT 8
#define FONT_COL_SPACING 0
#define FONT_ROW_SPACING 2

// store 4MB of log messages max
#define LOG_MAX_BUFFER (4 * 1024 * 1024)
