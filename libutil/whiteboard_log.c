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
 * whiteboard_log.c
 *
 * Copyright 2007 Nokia Corporation
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


//#define WHITEBOARD_LOG_C
#include "whiteboard_log.h"
//#undef WHITEBOARD_LOG_C
#if 0
#include <syslog.h>

#include "whiteboard_util.h"
#include "whiteboard_dbus_ifaces.h"

static GList *whiteboard_log_dbus_connections;
static GList *whiteboard_log_streams;
static gboolean whiteboard_log_initialized = FALSE;
static gboolean whiteboard_log_system_log_enabled = FALSE;
static gboolean whiteboard_log_stream_log_enabled = FALSE;
static gboolean whiteboard_log_dbus_log_enabled = FALSE;
static GStaticMutex whiteboard_log_mutex = G_STATIC_MUTEX_INIT;

/* Private function definitions */
inline void whiteboard_log_print_message(gint level, gint id,
				       gchar *whiteboard_log_levels[],
				       const gchar *format,
				       va_list list);

static void whiteboard_log_initialize();

/* Public functions */
void whiteboard_log_print(WhiteBoardSeverity level, WhiteBoardMessageID id,
			const gchar *format, ...)
{
  int stat;
	va_list var_args;

	va_start(var_args, format);
	stat=vfprintf (stderr, format, var_args);

	  //whiteboard_log_print_message(level, id, whiteboard_log_severities,
	//format, var_args);
	va_end(var_args);
}

void whiteboard_log_debug_fb2(const char *called_from) 
{
  whiteboard_log_print_debug(WHITEBOARD_DEBUG_BEGIN_END,
			     "%s() BEGIN\n",
			     called_from);
}

void whiteboard_log_print_debug(WhiteBoardDebugContext context,
			      const gchar *format, ...)
{
  
	va_list var_args;

	va_start(var_args, format);
	vfprintf (stderr, format, var_args);
		
	//whiteboard_log_print_message(context, -1, whiteboard_log_debug_contexts,
	//			     format, var_args);
	//no va_end() due to use of va_arg() by vprintf, which maks it undefined ???
	va_end(var_args);
}

void whiteboard_log_enable_system_log()
{
	g_static_mutex_lock(&whiteboard_log_mutex);
	whiteboard_log_initialize();

	whiteboard_log_system_log_enabled = TRUE;
	g_static_mutex_unlock(&whiteboard_log_mutex);
}

void whiteboard_log_disable_system_log()
{
	g_static_mutex_lock(&whiteboard_log_mutex);
	whiteboard_log_initialize();

	whiteboard_log_system_log_enabled = FALSE;
	g_static_mutex_unlock(&whiteboard_log_mutex);
}

void whiteboard_log_enable_dbus_log()
{
	g_static_mutex_lock(&whiteboard_log_mutex);
	whiteboard_log_initialize();

	whiteboard_log_dbus_log_enabled = TRUE;
	g_static_mutex_unlock(&whiteboard_log_mutex);
}

void whiteboard_log_disable_dbus_log()
{
	g_static_mutex_lock(&whiteboard_log_mutex);
	whiteboard_log_initialize();

	whiteboard_log_dbus_log_enabled = FALSE;
	g_static_mutex_unlock(&whiteboard_log_mutex);
}

void whiteboard_log_enable_stream_log()
{
	g_static_mutex_lock(&whiteboard_log_mutex);
	whiteboard_log_initialize();

	whiteboard_log_stream_log_enabled = TRUE;
	g_static_mutex_unlock(&whiteboard_log_mutex);
}

void whiteboard_log_disable_stream_log()
{
	g_static_mutex_lock(&whiteboard_log_mutex);
	whiteboard_log_initialize();

	whiteboard_log_stream_log_enabled = FALSE;
	g_static_mutex_unlock(&whiteboard_log_mutex);
}

void whiteboard_log_set_stream(FILE *stream)
{
	g_static_mutex_lock(&whiteboard_log_mutex);
	whiteboard_log_initialize();

	g_list_free(whiteboard_log_streams);
	whiteboard_log_streams = NULL;
	g_static_mutex_unlock(&whiteboard_log_mutex);
	whiteboard_log_add_stream(stream);
}

void whiteboard_log_add_stream(FILE *stream)
{
	g_static_mutex_lock(&whiteboard_log_mutex);
	whiteboard_log_initialize();

	whiteboard_log_streams = g_list_prepend(whiteboard_log_streams, stream);
	g_static_mutex_unlock(&whiteboard_log_mutex);
}

void whiteboard_log_set_dbus_connection(DBusConnection *conn)
{
	g_static_mutex_lock(&whiteboard_log_mutex);
	whiteboard_log_initialize();

	/* TODO: connection unrefs */
	g_list_free(whiteboard_log_dbus_connections);
	whiteboard_log_dbus_connections = NULL;
	g_static_mutex_unlock(&whiteboard_log_mutex);
	whiteboard_log_add_dbus_connection(conn);
}

void whiteboard_log_add_dbus_connection(DBusConnection *conn)
{
	g_static_mutex_lock(&whiteboard_log_mutex);
	whiteboard_log_initialize();

	dbus_connection_ref(conn);
	whiteboard_log_dbus_connections = 
		g_list_prepend(whiteboard_log_dbus_connections, conn);
	whiteboard_log_dbus_log_enabled = TRUE;
	g_static_mutex_unlock(&whiteboard_log_mutex);
}

void whiteboard_log_remove_dbus_connection(DBusConnection *conn)
{
	g_static_mutex_lock(&whiteboard_log_mutex);
	whiteboard_log_initialize();

	whiteboard_log_dbus_connections = 
		g_list_remove(whiteboard_log_dbus_connections, conn);
	dbus_connection_unref(conn);
	/* TODO: if list is emtpy dbus log should be disabled */
	g_static_mutex_unlock(&whiteboard_log_mutex);
}

void whiteboard_log_init_system_log(const gchar *prefix, gint facility)
{
	g_static_mutex_lock(&whiteboard_log_mutex);
	if (whiteboard_log_initialized)
		closelog();

	openlog(prefix, 0, facility);
	g_static_mutex_unlock(&whiteboard_log_mutex);
}

/* Private functions */
static void whiteboard_log_initialize()
{
	DBusConnection *sessionb = NULL;
	DBusError err;

	/* TODO: error handlers */
	if (whiteboard_log_initialized)
		return;

	if (0 <= fprintf(stderr, "Whiteboard logging system initialized.\n"))
	{
		whiteboard_log_streams = g_list_prepend(whiteboard_log_streams,
						      stderr);
		whiteboard_log_stream_log_enabled = TRUE;
	}
	else
	{
		whiteboard_log_stream_log_enabled = FALSE;
	}

	dbus_error_init(&err);
	sessionb = dbus_bus_get(DBUS_BUS_SESSION, &err);

	if (dbus_error_is_set(&err) == TRUE)
	{
		fprintf(stderr, "DBus error: %s\n", err.message);
		whiteboard_log_dbus_log_enabled = FALSE;
		dbus_error_free(&err);
	}
	else
	{
		whiteboard_log_dbus_connections = 
			g_list_prepend(whiteboard_log_dbus_connections, sessionb);
		whiteboard_log_dbus_log_enabled = TRUE;
	}

	openlog(WHITEBOARD_SYSLOG_PREFIX, 0, LOG_USER);

#ifdef WHITEBOARD_DEBUG
	whiteboard_log_system_log_enabled = FALSE;
#else
	whiteboard_log_system_log_enabled = TRUE;
#endif

	whiteboard_log_initialized = TRUE;
}


#if 0 //0 until we fix this
static void whiteboard_log_print_message(gint level, gint id,
				       gchar *whiteboard_log_levels[],
				       const gchar *format,
				       va_list list)
{
	static gint once = 0;
	gint filter_pass = 0;
	gchar *message_filter = getenv(WHITEBOARD_LOG_FILTER_ENV);
	gchar *token = NULL;

	g_static_mutex_lock(&whiteboard_log_mutex);
	whiteboard_log_initialize();

	if (NULL != message_filter)
	{
		message_filter = strdup(message_filter);

		token = strtok(message_filter, ",");

		if ( NULL != token )
		{
			do
			{
				if (!strcmp(whiteboard_log_levels[level], token))
					filter_pass = 1;
			}
			while (!filter_pass && (token = strtok(NULL, ",")));

			free(message_filter);
		}
	}
	else
	{
		if (!once && whiteboard_log_stream_log_enabled)
		{
			gint i = 0;

			printf("\n%s not defined, passing all messages through.\n\n" "If you want to see only spesific messages, set\n" "the above mentioned env variable and fill out a comma\n" "separated list from the following levels: ", WHITEBOARD_LOG_FILTER_ENV);

			for (i = 0; whiteboard_log_levels[i + 1]; i++)
				printf("%s,", whiteboard_log_levels[i]);
			printf("%s\n", whiteboard_log_levels[i]);
			once = 1;
		}

		filter_pass = 1;
	}

	if (!filter_pass)
	{
		g_static_mutex_unlock(&whiteboard_log_mutex);
		return;
	}

	if (whiteboard_log_stream_log_enabled)
	{
		GList *temp = whiteboard_log_streams;

		for (temp = whiteboard_log_streams; NULL != temp;
		     temp = g_list_next(temp))
			vfprintf((FILE *) temp->data, format, list);

		fflush(NULL);
	}

	if (whiteboard_log_system_log_enabled)
	{
		gint sysloglevel = LOG_INFO;

		/* TODO: Maybe this hack should be removed at some point... */
		if (-1 == id)
		{
			sysloglevel = LOG_DEBUG;
		}
		else
		{
			switch (level)
			{
			case WHITEBOARD_ERROR:
				sysloglevel = LOG_ERR;
				break;
			case WHITEBOARD_WARNING:
				sysloglevel = LOG_WARNING;
				break;
			case WHITEBOARD_INFO:
				sysloglevel = LOG_INFO;
				break;
			case WHITEBOARD_TRACE:
				sysloglevel = LOG_DEBUG;
				break;
			default:
				/* TODO: fill up with something meaningful */
				break;
			}
		}
		// vsyslog(sysloglevel, format, list);
	}

	if (whiteboard_log_dbus_log_enabled &&
	    (level == WHITEBOARD_ERROR || level == WHITEBOARD_TIMESTAMP) &&
	    -1 != id)
	{
		GList *temp = whiteboard_log_streams;
		gchar msg[WHITEBOARD_MAX_DBUS_MSG_SIZE];
		gchar *msg_s = NULL;

		vsnprintf(msg, WHITEBOARD_MAX_DBUS_MSG_SIZE,
			  format, list);

		msg_s = g_strdup(msg);

		for (temp = whiteboard_log_dbus_connections; NULL != temp;
		     temp = g_list_next(temp))
		{
			whiteboard_util_send_signal(WHITEBOARD_DBUS_OBJECT,
						  WHITEBOARD_DBUS_LOG_INTERFACE,
						  WHITEBOARD_DBUS_LOG_SIGNAL_MESSAGE,
						  (DBusConnection*) temp->data,
						  DBUS_TYPE_INT32, &level,
						  DBUS_TYPE_INT32, &id, 
						  DBUS_TYPE_STRING, &msg_s,
						  WHITEBOARD_UTIL_LIST_END);
		}

		g_free(msg_s);
	}

	g_static_mutex_unlock(&whiteboard_log_mutex);
}
#else
void whiteboard_log_print_message(gint level, gint id,
				       gchar *whiteboard_log_levels[],
				       const gchar *format,
				       va_list list)
{
  g_static_mutex_lock(&whiteboard_log_mutex);
  //no can do! whiteboard_log_initialize();
  
  vfprintf (stderr, format, list);

  g_static_mutex_unlock(&whiteboard_log_mutex);
}
#endif


#else
   
void whiteboard_log_debug2(const gchar *fmt, ...)
{
  va_list ap;					    
  va_start(ap, fmt);				    
  g_logv(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, fmt, ap); 
  va_end(ap);
}

void whiteboard_log_warning2(const gchar *fmt, ...)
{
  va_list ap;					    
  va_start(ap, fmt);				    
  g_logv(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, fmt, ap); 
  va_end(ap);
}

#endif
