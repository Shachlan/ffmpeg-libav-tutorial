#pragma once

#include <string>

/// This file presents flexible, non-intrusive logging macros that can be efficiently enabled or
/// disabled via compile switches.
//
// There are four levels of logging: Info, Warning, Error and Debug, and each can be enabled
// independently via the LOGGING_LEVEL_INFO, LOGGING_LEVEL_WARNING, LOGGING_LEVEL_ERROR and
// LOGGING_LEVEL_DEBUG preprocessor variables, respectively.
// Logging functions are implemented here via macros. Disabling logging, either entirely, or
// at a specific level, completely removes the corresponding log invocations from the compiled
// code, thus eliminating both the memory and CPU overhead that the logging calls would add.
//
// To perform logging, use any of the following function calls in your code:
//
// LogDebug(fmt, ...) - recommended for temporary use during debugging.
//
// LogInfo(fmt, ...) - recommended for general, infrequent, information messages.
//
// LogWarning(fmt, ...) - recommended for use only when there is an warning to be logged.
//
// LogError(fmt, ...) - recommended for use only when there is an error to be logged.
//
// In each case, the functions follow the general NSLog/printf template, where the first argument
// "fmt" is a string that optionally includes embedded Format Specifiers, and subsequent optional
// arguments indicate data to be formatted and inserted into the string. The number of optional
// arguments must match the number of embedded Format Specifiers.

/// Log levels are enabled by default.
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

/// Default logging call. This method shouldn't be called directly, but rather called through the
/// macros given above.
void log(const std::string fmt, const std::string log_level, ...);
