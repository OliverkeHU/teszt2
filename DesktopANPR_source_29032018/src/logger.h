/*
 * logger.h
 *
 *  Created on: Mar 12, 2018
 *      Author: amis
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define LOGLEVEL_CRITICAL    0
#define LOGLEVEL_ERROR       1
#define LOGLEVEL_WARNING     2
#define LOGLEVEL_NOTICE      3
#define LOGLEVEL_INFO        4
#define LOGLEVEL_DEBUG       5

typedef enum
{
	LogLevel_Critical   = LOGLEVEL_CRITICAL,
	LogLevel_Error      = LOGLEVEL_ERROR,
	LogLevel_Warning    = LOGLEVEL_WARNING,
	LogLevel_Notice     = LOGLEVEL_NOTICE,
	LogLevel_Info       = LOGLEVEL_INFO,
	LogLevel_Debug      = LOGLEVEL_DEBUG,
	LogLevel_end
} LogLevel;

#ifndef LOGLEVEL_MAX
#  define LOGLEVEL_MAX LOGLEVEL_INFO
#endif

#define LOG_LINE_MAXLEN 1024
void stdout_log(LogLevel level, const char * file, int line, const char * format, ...);

#if LOGLEVEL_MAX < LOGLEVEL_CRITICAL
#  define LOG_CRITICAL(...)
#else
#  define LOG_CRITICAL(...) stdout_log(LogLevel_Critical, __FILE__, __LINE__, __VA_ARGS__)
#endif

#if LOGLEVEL_MAX < LOGLEVEL_ERROR
#  define LOG_ERROR(...)
#else
#  define LOG_ERROR(...) stdout_log(LogLevel_Error, __FILE__, __LINE__, __VA_ARGS__)
#endif

#if LOGLEVEL_MAX < LOGLEVEL_WARNING
#  define LOG_WARN(...)
#else
#  define LOG_WARN(...) stdout_log(LogLevel_Warning, __FILE__, __LINE__, __VA_ARGS__)
#endif

#if LOGLEVEL_MAX < LOGLEVEL_NOTICE
#  define LOG_NOTICE(...)
#else
#  define LOG_NOTICE(...) stdout_log(LogLevel_Notice, __FILE__, __LINE__, __VA_ARGS__)
#endif

#if LOGLEVEL_MAX < LOGLEVEL_INFO
#  define LOG_INFO(...)
#else
#  define LOG_INFO(...) stdout_log(LogLevel_Info, __FILE__, __LINE__, __VA_ARGS__)
#endif

#if LOGLEVEL_MAX < LOGLEVEL_DEBUG
#  define LOG_DEBUG(...)
#else
#  define LOG_DEBUG(...) stdout_log(LogLevel_Debug, __FILE__, __LINE__, __VA_ARGS__)
#endif

#endif
