#pragma once

#include <string>
using std::string;

/// Default logging call.
void log(const string fmt, const string log_level, ...);

/// Log levels are enabled by default. To disable, add LOGGING_LEVEL_<level>=1 to the
/// GCC_PREPROCESSOR_DEFINITIONS build variable.
/// For these settings to be effective, LOGGING must also be defined and non-zero.
#ifndef LOGGING_LEVEL_DEBUG
#define LOGGING_LEVEL_DEBUG 1
#endif
#ifndef LOGGING_LEVEL_INFO
#define LOGGING_LEVEL_INFO 1
#endif
#ifndef LOGGING_LEVEL_WARNING
#define LOGGING_LEVEL_WARNING 1
#endif
#ifndef LOGGING_LEVEL_ERROR
#define LOGGING_LEVEL_ERROR 1
#endif

#if defined(LOGGING_LEVEL_DEBUG) && LOGGING_LEVEL_DEBUG
#define log_debug(fmt, ...) log(fmt, "DEBUG", ##__VA_ARGS__)
#else
#define log_debug(...)
#endif

#if defined(LOGGING_LEVEL_INFO) && LOGGING_LEVEL_INFO
#define log_info(fmt, ...) log(fmt, "INFO", ##__VA_ARGS__)
#else
#define log_info(...)
#endif

#if defined(LOGGING_LEVEL_WARNING) && LOGGING_LEVEL_WARNING
#define log_warning(fmt, ...) log(fmt, "WARNING", ##__VA_ARGS__)
#else
#define log_warning(...)
#endif

#if defined(LOGGING_LEVEL_ERROR) && LOGGING_LEVEL_ERROR
#define log_error(fmt, ...) log(fmt, "ERROR", ##__VA_ARGS__)
#else
#define log_error(...)
#endif
