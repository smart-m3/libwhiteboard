/*

  Copyright (c) 2009, Nokia Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.  
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.  
    * Neither the name of Nokia nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */
/*
 * Whiteboard Library
 *
 * whiteboard_log.h
 *
 * Copyright 2009 Nokia Corporation
 */

#ifndef WHITEBOARD_LOG_H
#define WHITEBOARD_LOG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <glib.h>
#include <stdarg.h>

#if 0 //REALLY CHANGE ALL THIS
#ifdef WHITEBOARD_TIMESTAMP_ENABLED
#include <sys/time.h>
#include <time.h>
#endif

#define DBUS_API_SUBJECT_TO_CHANGE

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#ifdef WHITEBOARD_LOG_C
#define WHITEBOARD_LOG_FILTER_ENV "WHITEBOARD_LOG_FILTER"
#define WHITEBOARD_SYSLOG_PREFIX "WHITEBOARD"
#define WHITEBOARD_MAX_DBUS_MSG_SIZE 256

/* 
 * These are the debug levels/contexts provided thru the Whiteboard logging
 * interface (whiteboard_log_debug_* functions).
 *
 * You can use whiteboard_log_debugc(LEVEL, "My debug message %d", 42) in code.
 * If you wish to just print out some general debug messages without a specific
 * context, you can use whiteboard_log_debug("Foo %d", 42).
 *
 * If you also export the shell environment variable WHITEBOARD_LOG_FILTER_ENV
 * to contain only the desired log level(s) then only those messages that you
 * are interested in will be printed out. Comma separated list of levels
 * is accepted as well, for example:
 * export WHITEBOARD_LOG_FILTER="WHITEBOARD_DEBUG_BEGIN_END,WHITEBOARD_DEBUG_USER_4"
 */
static gchar *whiteboard_log_debug_contexts[] = 
{
	"WHITEBOARD_DEBUG_BEGIN_END",   /* whiteboard_log_debug_fb() and _fe() */
	"WHITEBOARD_DEBUG_BASIC",       /* whiteboard_log_debug() */

	"WHITEBOARD_DEBUG_DISCOVER",    /* Reserved for Whiteboard */
	"WHITEBOARD_DEBUG_DBUS",        /* Reserved for Whiteboard */
	"WHITEBOARD_DEBUG_SIB_HANDLER",         /* Reserved for Whiteboard */
	"WHITEBOARD_DEBUG_NODE",        /* Reserved for Whiteboard */
	"WHITEBOARD_DEBUG_SIB",         /* Reserved for Whiteboard */

	/* User debug levels */
	"WHITEBOARD_DEBUG_USER_0",      /* User debug level */
	"WHITEBOARD_DEBUG_USER_1",      /* User debug level */
	"WHITEBOARD_DEBUG_USER_2",      /* User debug level */
	"WHITEBOARD_DEBUG_USER_3",      /* User debug level */
	"WHITEBOARD_DEBUG_USER_4",      /* User debug level */
	"WHITEBOARD_DEBUG_USER_5",      /* User debug level */
	"WHITEBOARD_DEBUG_USER_6",      /* User debug level */
	"WHITEBOARD_DEBUG_USER_7",      /* User debug level */
	"WHITEBOARD_DEBUG_USER_8",      /* User debug level */
	"WHITEBOARD_DEBUG_USER_9",      /* User debug level */

	NULL
};

static gchar *whiteboard_log_severities[] = 
{
	"WHITEBOARD_TIMESTAMP",
	"WHITEBOARD_ERROR",
	"WHITEBOARD_WARNING",
	"WHITEBOARD_INFO",
	"WHITEBOARD_TRACE",
	NULL
};
#endif /* WHITEBOARD_LOG_C */

typedef enum
{
	WHITEBOARD_DEBUG_BEGIN_END,   /* whiteboard_log_debug_fb() and _fe() */
	WHITEBOARD_DEBUG_BASIC,       /* whiteboard_log_debug() */

	WHITEBOARD_DEBUG_DISCOVER,    /* Reserved for Whiteboard */
	WHITEBOARD_DEBUG_DBUS,        /* Reserved for Whiteboard */
	WHITEBOARD_DEBUG_SIB_HANDLER,        /* Reserved for Whiteboard */
	WHITEBOARD_DEBUG_NODE,        /* Reserved for Whiteboard */	
	WHITEBOARD_DEBUG_SIB,          /* Reserved for Whiteboard */
	

	/* User debug levels, use with whiteboard_log_debugc(LEVEL, ...) */
	WHITEBOARD_DEBUG_USER_0,      /* User debug level */
	WHITEBOARD_DEBUG_USER_1,      /* User debug level */
	WHITEBOARD_DEBUG_USER_2,      /* User debug level */
	WHITEBOARD_DEBUG_USER_3,      /* User debug level */
	WHITEBOARD_DEBUG_USER_4,      /* User debug level */
	WHITEBOARD_DEBUG_USER_5,      /* User debug level */
	WHITEBOARD_DEBUG_USER_6,      /* User debug level */
	WHITEBOARD_DEBUG_USER_7,      /* User debug level */
	WHITEBOARD_DEBUG_USER_8,      /* User debug level */
	WHITEBOARD_DEBUG_USER_9,      /* User debug level */

} WhiteBoardDebugContext;

typedef enum
{
	WHITEBOARD_TIMESTAMP,
	WHITEBOARD_ERROR,		/* Used in critical situations */
	WHITEBOARD_WARNING,	/* Used in situations that can be handled */
	WHITEBOARD_INFO,		/* Gives general "nice to know" information
				   on what is happening */
	WHITEBOARD_TRACE		/* Used in critical places to provide debugging 
				   information from released versions */
} WhiteBoardSeverity;

typedef enum
{
	WHITEBOARD_TIMESTAMP_ID,
	WHITEBOARD_ERROR_GET_MEDIA,
	WHITEBOARD_ERROR_SET_MEDIA,
	WHITEBOARD_ERROR_GET_METADATA,
	WHITEBOARD_ERROR_PLAYLIST_GET_AT,
	WHITEBOARD_ERROR_CLEAR_MEDIA,
	WHITEBOARD_ERROR_REFRESH_MEDIA,
	WHITEBOARD_ERROR_PLAY,
	WHITEBOARD_ERROR_STOP,
	WHITEBOARD_ERROR_PAUSE,
	WHITEBOARD_ERROR_GET_STATE,
	WHITEBOARD_ERROR_PREVIOUS,
	WHITEBOARD_ERROR_NEXT,
	WHITEBOARD_ERROR_REFRESH_STATE,
	WHITEBOARD_ERROR_SET_POSITION,
	WHITEBOARD_ERROR_GET_POSITION,
	WHITEBOARD_ERROR_REFRESH_POSITION,
	WHITEBOARD_ERROR_SET_VOLUME,
	WHITEBOARD_ERROR_GET_VOLUME,
	WHITEBOARD_ERROR_REFRESH_VOLUME,
	WHITEBOARD_ERROR_SET_MUTE,
	WHITEBOARD_ERROR_GET_MUTE,
	WHITEBOARD_ERROR_REFRESH_MUTE,
	WHITEBOARD_ERROR_METADATA_PARSE,
	WHITEBOARD_ERROR_BROWSE,
	WHITEBOARD_ERROR_INVALID_OBJECT_ID,
	WHITEBOARD_ERROR_AVT_LAST_CHANGED_EVENT,
	WHITEBOARD_ERROR_RCS_LAST_CHANGED_EVENT,
	WHITEBOARD_ERROR_SUBSCRIBE,
	WHITEBOARD_ERROR_IMAGE,
	WHITEBOARD_ERROR_INVALID_MEDIA_CLASS,
	WHITEBOARD_ERROR_NOT_IMPLEMENTED,
	WHITEBOARD_ERROR_UNKNOWN_METHOD,
	WHITEBOARD_ERROR_UNKNOWN_SIGNAL,
	WHITEBOARD_ERROR_UNKNOWN_INTERFACE,
	WHITEBOARD_ERROR_UNKNOWN_MESSAGE_TYPE,
	WHITEBOARD_ERROR_OUT_OF_MEMORY,
	WHITEBOARD_UNKNOWN
} WhiteBoardMessageID;

/**
 * Generic function to print out log messages into various
 * destinations. (Session dbus, syslog, stderr, ...).
 *
 * @param level Message serverity
 * @param id Unique id for this error or UNKNOWN if we don't care
 * @param format Format string (printf style)
 * @param ... Variable length list of attributer for format string
 */
void whiteboard_log_print(WhiteBoardSeverity level,
			  WhiteBoardMessageID id,
			  const gchar *format, ...);

/**
 * Generic function to print out debug messages into various
 * destinations. (Session dbus, syslog, stderr, ...).
 *
 * @param level Debug message "context"
 * @param format Format string (printf style)
 * @param ... Variable length list of attributer for format string
 */
void whiteboard_log_print_debug(WhiteBoardDebugContext level,
				const gchar *format, ...);

/**
 * Enables messages to syslog.
 *
 */
void whiteboard_log_enable_system_log();

/**
 * Disables messages to syslog.
 *
 */
void whiteboard_log_disable_system_log();

/**
 * Enables messages to dbus destination(s).
 *
 */
void whiteboard_log_enable_dbus_log();

/**
 * Disables messages to dbus destination(s).
 *
 */
void whiteboard_log_disable_dbus_log();

/**
 * Enables messages to stream destination(s).
 *
 */
void whiteboard_log_enable_stream_log();

/**
 * Disables messages to stream destination(s).
 *
 */
void whiteboard_log_disable_stream_log();

/**
 * Sets new stream for log output, zeroes existing list
 * and enables log output to this stream destination.
 *
 * @param stream Pointer to the file descriptor destination
 */

void whiteboard_log_set_stream(FILE *stream);

/**
 * Adds new stream for log output and enables log output
 * to stream destination(s).
 *
 * @param stream Pointer to the file descriptor destination
 */
void whiteboard_log_add_stream(FILE *stream);

/**
 * Sets new dbus connection for log output, zeroes existing list
 * and enables log output to this dbus connection.
 *
 */
void whiteboard_log_set_dbus_connection(DBusConnection *conn);

/**
 * Adds new dbus connection for log output and enables log output
 * to dbus destination(s).
 *
 * @param conn DBus connection to add
 */
void whiteboard_log_add_dbus_connection(DBusConnection *conn);

/**
 * Set syslog prefix and facility
 *
 * @param prefix String to be appended into every syslog message
 * @param facility Source type for messages
 */
void whiteboard_log_init_syslog(const gchar *prefix, gint facility);

#ifdef WHITEBOARD_DEBUG
#	define WHITEBOARD_D_PREFIX "%s: %s() (line %u): "
#	define WHITEBOARD_D_POSTFIX , __FILE__, __FUNCTION__, __LINE__
#else
#	define WHITEBOARD_D_PREFIX ""
#	define WHITEBOARD_D_POSTFIX ""
#endif 

/**
 * Logs error messages.
 *
 * @param fmt Format string
 * @param ... Format string modifiers
 */
#define whiteboard_log_error(fmt, ...) \
        whiteboard_log_error_id(WHITEBOARD_UNKNOWN, fmt, ## __VA_ARGS__)

/**
 * Logs error messages with unique id
 *
 * @param id Unique id
 * @param fmt Format string
 * @param ... Format string modifiers
 */
#define whiteboard_log_error_id(id, fmt, ...) \
        whiteboard_log_print(WHITEBOARD_ERROR, id, \
			   WHITEBOARD_D_PREFIX "Error: " fmt WHITEBOARD_D_POSTFIX, \
			   ## __VA_ARGS__)

/**
 * Logs warning messages.
 *
 * @param fmt Format string
 * @param ... Format string modifiers
 */
#define whiteboard_log_warning(fmt, ...) \
        whiteboard_log_warning_id(WHITEBOARD_UNKNOWN, fmt, \
				## __VA_ARGS__)

/**
 * Logs warning messages with id
 *
 * @param id Unique id
 * @param fmt Format string
 * @param ... Format string modifiers
 */
#define whiteboard_log_warning_id(id, fmt, ...) \
        whiteboard_log_print(WHITEBOARD_WARNING, id, \
			   WHITEBOARD_D_PREFIX "Warning: " fmt \
			   WHITEBOARD_D_POSTFIX, \
			   ## __VA_ARGS__)

/**
 * Logs informational messages.
 *
 * @param fmt Format string
 * @param ... Format string modifiers
 */
#define whiteboard_log_info(fmt, ...) \
        whiteboard_log_info_id(WHITEBOARD_UNKNOWN, \
			     WHITEBOARD_D_PREFIX "Info: " fmt \
			     WHITEBOARD_D_POSTFIX, \
			     ## __VA_ARGS__)

/**
 * Logs informational messages with unique id.
 *
 * @param id Unique id
 * @param fmt Format string
 * @param ... Format string modifiers
 */
#define whiteboard_log_info_id(id, fmt, ...) \
        whiteboard_log_print(WHITEBOARD_INFO, id, fmt, \
			   ## __VA_ARGS__)

/**
 * Logs trace messages.
 *
 * @param fmt Format string
 * @param ... Format string modifiers
 */
#define whiteboard_log_trace(fmt, ...) \
        whiteboard_log_print(WHITEBOARD_TRACE, WHITEBOARD_UNKNOWN, \
			   WHITEBOARD_D_PREFIX "Trace: " fmt WHITEBOARD_D_POSTFIX, \
			   ## __VA_ARGS__)

#ifdef WHITEBOARD_DEBUG
/**
 * Basic debug prints.
 *
 * @param fmt Format string
 * @param ... Format string modifiers
 */


#define whiteboard_log_debug(fmt, ...) \
        whiteboard_log_print_debug(WHITEBOARD_DEBUG_BASIC, \
				 WHITEBOARD_D_PREFIX fmt WHITEBOARD_D_POSTFIX, \
				 ## __VA_ARGS__)

/**
 * Debug print with context.
 *
 * @param context Enum specifying debug level context
 * @param fmt Format string
 * @param ... Format string modifiers
 */
#define whiteboard_log_debugc(context, fmt, ...) \
        whiteboard_log_print_debug(context, \
				 WHITEBOARD_D_PREFIX fmt WHITEBOARD_D_POSTFIX, \
				 ## __VA_ARGS__)

/**
 * Could be used to mark function entry points.
 */
#if 0
#define whiteboard_log_debug_fb() \
        whiteboard_log_print_debug(WHITEBOARD_DEBUG_BEGIN_END, \
				 "%s() BEGIN\n", \
				 __FUNCTION__)
#else
void whiteboard_log_debug_fb2(const char *called_from);
#define whiteboard_log_debug_fb() whiteboard_log_debug_fb2(__FUNCTION__);
#endif

/**
 * Could be used to mark function leaving points.
 */
#define whiteboard_log_debug_fe() \
        whiteboard_log_print_debug(WHITEBOARD_DEBUG_BEGIN_END, \
				 "%s() END\n", \
				 __FUNCTION__)


#else /* WHITEBOARD_DEBUG */

#define whiteboard_log_debug(fmt, ...)
#define whiteboard_log_debugc(context, fmt, ...)
#define whiteboard_log_debug_fb()
#define whiteboard_log_debug_fe()

#endif /* WHITEBOARD_DEBUG */

/*****************************************************************************
 * Benchmarking & timestamp stuff
 *****************************************************************************/
#ifdef WHITEBOARD_TIMESTAMP_ENABLED
#define WHITEBOARD_TIMESTAMP_PREFIX "[%.2d:%.2d:%.2d.%.2d] %s: %s() (line %u): "
#define WHITEBOARD_TIMESTAMP_POSTFIX ts.tm_hour, ts.tm_min, ts.tm_sec, tv.tv_usec / 1000, __FILE__, __FUNCTION__, __LINE__

/**
 * Timestamp printing for poor man's performance measurement
 *
 * @param fmt Format string
 * @param ... Format string modifiers
 */
#define whiteboard_log_timestamp(fmt, ...) \
{ \
	struct timeval tv; \
	struct tm ts; \
	gettimeofday(&tv, NULL); \
	ts = *localtime(&(tv.tv_sec)); \
	whiteboard_log_print(WHITEBOARD_TIMESTAMP, \
			   WHITEBOARD_TIMESTAMP_ID, \
			   WHITEBOARD_TIMESTAMP_PREFIX fmt, \
			   WHITEBOARD_TIMESTAMP_POSTFIX, \
			   ## __VA_ARGS__); \
}
#else
#	define WHITEBOARD_TIMESTAMP_PREFIX ""
#	define WHITEBOARD_TIMESTAMP_POSTFIX ""
#define whiteboard_log_timestamp(fmt, ...)
#endif // WHITEBOARD_TIMESTAMP_ENABLED

#else //REALLY CHANGE ALL THIS

void whiteboard_log_debug2(const gchar *fmt, ...);
void whiteboard_log_warning2(const gchar *fmt, ...);
#if WHITEBOARD_DEBUG==1
#define whiteboard_log_debug( fmt, ...) whiteboard_log_debug2("%s: %s() (line %u): " fmt ,__FILE__  ,__FUNCTION__ ,__LINE__, ##__VA_ARGS__  )
#define whiteboard_log_warning( fmt, ...) whiteboard_log_warning2("%s: %s() (line %u): " fmt ,__FILE__  ,__FUNCTION__ ,__LINE__, ##__VA_ARGS__  )
  
#define whiteboard_log_debugc(context, fmt, ...) g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define whiteboard_log_debug_fb() g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s() BEGIN\n", __FUNCTION__)
#define whiteboard_log_debug_fe()g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s() END\n", __FUNCTION__)
//#define whiteboard_log_warning(fmt, ...) g_log(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define whiteboard_log_error(fmt, ...) g_log(G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define whiteboard_log_error_id(id, fmt, ...) g_log(G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, "%s: %s() (line %u): " fmt , __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__  ) 
#define whiteboard_log_trace(fmt, ...) g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "%s: %s() (line %u): Trace: " fmt , __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__  ) 

#else

#define whiteboard_log_debug(fmt, ...)
#define whiteboard_log_debugc(context, fmt, ...)
#define whiteboard_log_debug_fb()
#define whiteboard_log_debug_fe()
#define whiteboard_log_warning(fmt, ...)


#define whiteboard_log_error(fmt, ...)  g_log(G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define whiteboard_log_error_id(id, fmt, ...) g_log(G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, "%s: %s() (line %u): " fmt , __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__  ) 
#define whiteboard_log_trace(fmt, ...)
#endif

typedef enum
{
	WHITEBOARD_TIMESTAMP_ID,
	WHITEBOARD_ERROR_GET_MEDIA,
	WHITEBOARD_ERROR_SET_MEDIA,
	WHITEBOARD_ERROR_GET_METADATA,
	WHITEBOARD_ERROR_PLAYLIST_GET_AT,
	WHITEBOARD_ERROR_CLEAR_MEDIA,
	WHITEBOARD_ERROR_REFRESH_MEDIA,
	WHITEBOARD_ERROR_PLAY,
	WHITEBOARD_ERROR_STOP,
	WHITEBOARD_ERROR_PAUSE,
	WHITEBOARD_ERROR_GET_STATE,
	WHITEBOARD_ERROR_PREVIOUS,
	WHITEBOARD_ERROR_NEXT,
	WHITEBOARD_ERROR_REFRESH_STATE,
	WHITEBOARD_ERROR_SET_POSITION,
	WHITEBOARD_ERROR_GET_POSITION,
	WHITEBOARD_ERROR_REFRESH_POSITION,
	WHITEBOARD_ERROR_SET_VOLUME,
	WHITEBOARD_ERROR_GET_VOLUME,
	WHITEBOARD_ERROR_REFRESH_VOLUME,
	WHITEBOARD_ERROR_SET_MUTE,
	WHITEBOARD_ERROR_GET_MUTE,
	WHITEBOARD_ERROR_REFRESH_MUTE,
	WHITEBOARD_ERROR_METADATA_PARSE,
	WHITEBOARD_ERROR_BROWSE,
	WHITEBOARD_ERROR_INVALID_OBJECT_ID,
	WHITEBOARD_ERROR_AVT_LAST_CHANGED_EVENT,
	WHITEBOARD_ERROR_RCS_LAST_CHANGED_EVENT,
	WHITEBOARD_ERROR_SUBSCRIBE,
	WHITEBOARD_ERROR_IMAGE,
	WHITEBOARD_ERROR_INVALID_MEDIA_CLASS,
	WHITEBOARD_ERROR_NOT_IMPLEMENTED,
	WHITEBOARD_ERROR_UNKNOWN_METHOD,
	WHITEBOARD_ERROR_UNKNOWN_SIGNAL,
	WHITEBOARD_ERROR_UNKNOWN_INTERFACE,
	WHITEBOARD_ERROR_UNKNOWN_MESSAGE_TYPE,
	WHITEBOARD_ERROR_OUT_OF_MEMORY,
	WHITEBOARD_UNKNOWN
} WhiteBoardMessageID;

#endif //REALLY CHANGE ALL THIS

#endif
