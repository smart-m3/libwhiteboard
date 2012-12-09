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
 * Whiteboard library
 *
 * whiteboard_sib_access.c
 *
 * Copyright 2007 Nokia Corporation
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DBUS_API_SUBJECT_TO_CHANGE

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib.h>
#include <glib-object.h>

#include "whiteboard_dbus_ifaces.h"

#include "whiteboard_sib_access.h"
#include "whiteboard_util.h"
#include "whiteboard_command.h"
#include "whiteboard_log.h"

#include "whiteboard_marshal.h"

/*****************************************************************************
 * Private function prototypes
 *****************************************************************************/
static void whiteboard_sib_access_finalize(GObject* object);

static gboolean whiteboard_sib_access_register(WhiteBoardSIBAccess *self, guchar *uuid);

static gboolean whiteboard_sib_access_register_control(WhiteBoardSIBAccess *self,
						       guchar *uuid);

static void whiteboard_sib_access_unregister_handler(DBusConnection *connection,
						     gpointer user_data);

static DBusHandlerResult whiteboard_sib_access_dispatch_message(DBusConnection *conn,
								DBusMessage *msg,
								gpointer data);

static DBusHandlerResult whiteboard_sib_access_join(DBusConnection *conn,
						    DBusMessage *msg,
						    gpointer data);

static DBusHandlerResult whiteboard_sib_access_unsubscribe(DBusConnection *conn,
							   DBusMessage *msg,
							   gpointer data);

static DBusHandlerResult whiteboard_sib_access_insert(DBusConnection *conn,
						      DBusMessage *msg,
						      gpointer data);
static DBusHandlerResult whiteboard_sib_access_update(DBusConnection *conn,
						      DBusMessage *msg,
						      gpointer data);

static DBusHandlerResult whiteboard_sib_access_remove(DBusConnection *conn,
						      DBusMessage *msg,
						      gpointer data);
static DBusHandlerResult whiteboard_sib_access_subscribe_query(DBusConnection *conn,
							       DBusMessage *msg,
							       gpointer data,
							       gint method);
/******************************************************************************
 * WhiteBoardSIBAccessHandle definitions
 ******************************************************************************/

struct _WhiteBoardSIBAccessHandle
{
  WhiteBoardSIBAccess *context;
  DBusMessage *message;
  gint refcount;
};

/******************************************************************************
 * WhiteBoardSIBAccess definitions
 ******************************************************************************/

/**
 * The WhiteBoardSIBAccess object structure
 */
struct _WhiteBoardSIBAccess
{
  /* WhiteboardLibHeader BEGIN. DO NOT change the order of these vars! */
  GObject parent;

  guchar *uuid;
  guchar *control_uuid;
  guchar *friendly_name;
  gchar *mimetypes;
  gboolean local;
  DBusConnection *connection;
  GMainContext *main_context;
  /* WhiteboardLibHeader END */

  DBusConnection* sessionbus_connection;
  guchar *description;
};

/**
 * The WhiteBoardSIBAccess class structure
 */
struct _WhiteBoardSIBAccessClass
{
  GObjectClass parent;

  WhiteBoardSIBAccessRefreshCB refresh_cb;
  WhiteBoardSIBAccessShutdownCB shutdown_cb;
  WhiteBoardSIBAccessHealthCheckCB healthcheck_cb;

  WhiteBoardSIBAccessJoinCB join_cb;
  WhiteBoardSIBAccessLeaveCB leave_cb;
  WhiteBoardSIBAccessInsertCB insert_cb;
  WhiteBoardSIBAccessUpdateCB update_cb;
  WhiteBoardSIBAccessRemoveCB remove_cb;
  WhiteBoardSIBAccessQueryCB query_cb;
  WhiteBoardSIBAccessSubscribeCB subscribe_cb;
  WhiteBoardSIBAccessUnsubscribeCB unsubscribe_cb;
  
  WhiteBoardSIBAccessCustomCommandRequestCB custom_command_request_cb;
  WhiteBoardSIBAccessCustomCommandResponseCB custom_command_response_cb;
};

enum
  {
    SIGNAL_REFRESH,
    SIGNAL_SHUTDOWN,
    SIGNAL_HEALTHCHECK,

    SIGNAL_JOIN,
    SIGNAL_LEAVE, 
    SIGNAL_INSERT,
    SIGNAL_REMOVE,
    SIGNAL_UPDATE,
    SIGNAL_QUERY,
    SIGNAL_SUBSCRIBE,
    SIGNAL_UNSUBSCRIBE,

    SIGNAL_CUSTOM_COMMAND_REQUEST,
    SIGNAL_CUSTOM_COMMAND_RESPONSE,
	
    NUM_SIGNALS
  };

static guint whiteboard_sib_access_signals[NUM_SIGNALS];

static void whiteboard_sib_access_class_init(WhiteBoardSIBAccessClass *self)
{
  GObjectClass* object = G_OBJECT_CLASS(self);

  g_return_if_fail(self != NULL);

  object->finalize = whiteboard_sib_access_finalize;

  /*********************************************************************/

  whiteboard_sib_access_signals[SIGNAL_REFRESH] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_REFRESH,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardSIBAccessClass, refresh_cb),
		 NULL,
		 NULL,
		 marshal_VOID__VOID,
		 G_TYPE_NONE,
		 0,
		 G_TYPE_NONE);

  whiteboard_sib_access_signals[SIGNAL_SHUTDOWN] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_SHUTDOWN,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 0,
		 NULL,
		 NULL,
		 marshal_VOID__VOID,
		 G_TYPE_NONE,
		 0,
		 G_TYPE_NONE);
  
  whiteboard_sib_access_signals[SIGNAL_HEALTHCHECK] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_HEALTHCHECK,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardSIBAccessClass, healthcheck_cb),
		 NULL,
		 NULL,
		 marshal_VOID__VOID,
		 G_TYPE_NONE,
		 0,
		 G_TYPE_NONE);

  /*********************************************************************/

  whiteboard_sib_access_signals[SIGNAL_JOIN] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_JOIN,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardSIBAccessClass, join_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER_INT_STRING_STRING_INT,
		 G_TYPE_NONE,
		 5,
		 G_TYPE_POINTER,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT);

  whiteboard_sib_access_signals[SIGNAL_LEAVE] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_LEAVE,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardSIBAccessClass, leave_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER_STRING_STRING_INT,
		 G_TYPE_NONE,
		 4,
		 G_TYPE_POINTER,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT);
	
  whiteboard_sib_access_signals[SIGNAL_INSERT] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_INSERT,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardSIBAccessClass, insert_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER_STRING_STRING_INT_INT_STRING,
		 G_TYPE_NONE,
		 6,
		 G_TYPE_POINTER,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING);
	
  whiteboard_sib_access_signals[SIGNAL_REMOVE] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_REMOVE,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardSIBAccessClass, remove_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER_STRING_STRING_INT_INT_STRING,
		 G_TYPE_NONE,
		 6,
		 G_TYPE_POINTER,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING);
  
  whiteboard_sib_access_signals[SIGNAL_UPDATE] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_UPDATE,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardSIBAccessClass, update_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER_STRING_STRING_INT_INT_STRING_STRING,
		 G_TYPE_NONE,
		 7,
		 G_TYPE_POINTER,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_STRING);  

  whiteboard_sib_access_signals[SIGNAL_QUERY] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_QUERY,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardSIBAccessClass, query_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER_INT_STRING_STRING_INT_INT_STRING,
		 G_TYPE_NONE,
		 7,
		 G_TYPE_POINTER,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING);
	
  whiteboard_sib_access_signals[SIGNAL_SUBSCRIBE] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_SUBSCRIBE,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardSIBAccessClass, subscribe_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER_INT_STRING_STRING_INT_INT_STRING,
		 G_TYPE_NONE,
		 7,
		 G_TYPE_POINTER,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING);
		
  whiteboard_sib_access_signals[SIGNAL_UNSUBSCRIBE] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_UNSUBSCRIBE,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardSIBAccessClass, unsubscribe_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER_INT_STRING_STRING_INT_STRING,
		 G_TYPE_NONE,
		 6,
		 G_TYPE_POINTER,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_STRING  );
	
  /**********************************************************************/
 

  whiteboard_sib_access_signals[SIGNAL_CUSTOM_COMMAND_REQUEST] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_CUSTOM_COMMAND_REQUEST,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardSIBAccessClass, custom_command_request_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER,
		 G_TYPE_NONE,
		 1,
		 G_TYPE_POINTER);

  whiteboard_sib_access_signals[SIGNAL_CUSTOM_COMMAND_RESPONSE] =
    g_signal_new(WHITEBOARD_SIB_ACCESS_SIGNAL_CUSTOM_COMMAND_RESPONSE,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardSIBAccessClass, custom_command_response_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER,
		 G_TYPE_NONE,
		 1,
		 G_TYPE_POINTER);
}

static void whiteboard_sib_access_init(GTypeInstance* instance, gpointer g_class)
{
  WhiteBoardSIBAccess* self = WHITEBOARD_SIB_ACCESS(instance);

  g_return_if_fail(self != NULL);
  g_return_if_fail(IS_WHITEBOARD_SIB_ACCESS(instance));
}

/*****************************************************************************
 * Public functions
 *****************************************************************************/

GType whiteboard_sib_access_get_type()
{
  static GType type = 0;

  whiteboard_log_debug_fb();

  if (!type)
    {
      static const GTypeInfo info =
	{
	  sizeof(WhiteBoardSIBAccessClass),
	  NULL,           /* base_init */
	  NULL,           /* base_finalize */
	  (GClassInitFunc) whiteboard_sib_access_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof(WhiteBoardSIBAccess),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) whiteboard_sib_access_init
	};

      type = g_type_register_static(G_TYPE_OBJECT,
				    "WhiteBoardSIBAccessType", &info, 0);
    }

  whiteboard_log_debug_fe();

  return type;
}

GObject* whiteboard_sib_access_new(guchar* serviceid, guchar* controlid,
				   GMainContext* main_context,
				   guchar* friendly_name, guchar* description)
{
  GObject* object = NULL;
  WhiteBoardSIBAccess* self = NULL;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(serviceid != NULL || controlid != NULL, NULL);

  object = g_object_new(WHITEBOARD_SIB_ACCESS_TYPE, NULL);
  if (object == NULL)
    {
      whiteboard_log_error("Out of memory!\n");
      return NULL;
    }

  /* TODO: Maybe the stuff below should be done in whiteboard_sib_access_init */

  self = WHITEBOARD_SIB_ACCESS(object);
  self->uuid = (guchar *)g_strdup((gchar *)serviceid);
  self->control_uuid = (guchar *)g_strdup((gchar *)controlid);
  self->friendly_name =(guchar *) g_strdup((gchar *)friendly_name);
  self->description = (guchar *)g_strdup((gchar *)description);
  self->mimetypes = g_strdup("N/A"); // To fulfill WhiteboardLibHeader
	
  /* TODO: Get local status as function argument and dispatch it
     all the way to UI */
  self->local = FALSE;
	
  /* Set main context */
  if (main_context != NULL)
    self->main_context = main_context;
  else
    self->main_context = g_main_context_default();

  /* Register normal source service with Whiteboard only if serviceid was
     given. This can be done any number of times for a process, but
     serviceid should be different each time. */
  if (serviceid != NULL)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
			    "Attempting to register the WhiteBoardSIBAccess with Whiteboard.\n");
      while (whiteboard_sib_access_register(self, serviceid) == FALSE)
	{
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
				"WhiteBoardSIBAccess registration failed. Sleeping.\n");
	  g_usleep(WHITEBOARD_REGISTRATION_TIMEOUT);
	}
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			    "WhiteBoardSIBAccess registered successfully with Whiteboard.\n");
    }

  /* Register control channel with Whiteboard if controlid was given. This
     should be done only once for each process. */
  if (controlid != NULL)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			    "Attempting to register control channel with Whiteboard.\n");
      while (whiteboard_sib_access_register_control(self, controlid) == FALSE)
	{
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
				"WhiteBoardSIBAccess control registration failed. Sleeping.\n");
	  g_usleep(WHITEBOARD_REGISTRATION_TIMEOUT);
	}
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			    "WhiteBoardSIBAccess registered control channel successfully with Whiteboard.\n");

      /* Only one pid file is wanted for the process, and only one 
       * control channel should be created for the process. */
      whiteboard_util_create_pid_file( (gchar *)controlid);
    }
	
  //whiteboard_log_add_dbus_connection(self->connection);
  whiteboard_log_debug("Message routing from source %s enabled\n", 
		       self->friendly_name);

  whiteboard_log_debug_fe();

  return object;
}

static void whiteboard_sib_access_finalize(GObject* object)
{
  WhiteBoardSIBAccess *self = WHITEBOARD_SIB_ACCESS(object);

  whiteboard_log_debug_fb();

  g_return_if_fail(self != NULL);

  if ( NULL != self->uuid )
    {
      whiteboard_util_send_signal(WHITEBOARD_DBUS_OBJECT,
				  WHITEBOARD_DBUS_SIB_ACCESS_INTERFACE, 
				  WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_SIB_REMOVED,
				  self->connection,
				  DBUS_TYPE_STRING,
				  &self->uuid,
				  DBUS_TYPE_STRING,
				  &self->friendly_name,
				  WHITEBOARD_UTIL_LIST_END);
    }
  
  if ( NULL != self->control_uuid )
    {
      whiteboard_util_send_signal(WHITEBOARD_DBUS_OBJECT,
				  WHITEBOARD_DBUS_SIB_ACCESS_INTERFACE, 
				  WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_SIB_REMOVED,
				  self->connection,
				  DBUS_TYPE_STRING,
				  &self->control_uuid,
				  DBUS_TYPE_STRING,
				  &self->friendly_name,
				  WHITEBOARD_UTIL_LIST_END);
      
      whiteboard_util_remove_pid_file( (gchar *)self->control_uuid);
    }
  
  dbus_connection_flush(self->connection);
  dbus_connection_unref(self->connection);

  g_free(self->uuid);
  self->uuid = NULL;

  g_free(self->control_uuid);
  self->control_uuid = NULL;

  g_free(self->friendly_name);
  self->friendly_name = NULL;

  g_free(self->mimetypes);
  self->mimetypes = NULL;

  g_free(self->description);
  self->description = NULL;
  
  whiteboard_log_debug_fe();
}

GMainContext *whiteboard_sib_access_get_main_context(WhiteBoardSIBAccess *self)
{
  g_return_val_if_fail(self != NULL, NULL);
  return self->main_context;
}

const guchar *whiteboard_sib_access_get_uuid(WhiteBoardSIBAccess *self)
{
  g_return_val_if_fail(self != NULL, NULL);
  return self->uuid;
}

/******************************************************************************
 * WhiteBoardSIBAccessHandle functions
 ******************************************************************************/

static WhiteBoardSIBAccessHandle* whiteboard_sib_access_handle_new(WhiteBoardSIBAccess* context,
								   DBusMessage* message)
{
  WhiteBoardSIBAccessHandle* self = NULL;

  whiteboard_log_debug_fb();

  self = g_new0(WhiteBoardSIBAccessHandle, 1);
  g_return_val_if_fail(self != NULL, NULL);

  self->context = context;
  self->message = message;
  self->refcount = 1;

  if (self->context != NULL)
    g_object_ref(G_OBJECT(self->context));
	
  if (self->message != NULL)
    dbus_message_ref(message);

  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
			"Created WhiteBoardSIBAccessHandle with context %p and msg %p\n",
			context, message);

  whiteboard_log_debug_fe();

  return self;
}

void whiteboard_sib_access_handle_ref(WhiteBoardSIBAccessHandle* self)
{
  whiteboard_log_debug_fb();

  g_return_if_fail(self != NULL);

  if (g_atomic_int_get(&self->refcount) > 0)
    {
      /* Refcount is something sensible */
      g_atomic_int_inc(&self->refcount);

      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
			    "Handle refcount: %d\n",
			    self->refcount);
    }
  else
    {
      /* Refcount is already zero */
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
			    "Handle refcount already %d!\n",
			    self->refcount);
    }

  whiteboard_log_debug_fe();
}

static void whiteboard_sib_access_handle_destroy(WhiteBoardSIBAccessHandle* self)
{
  whiteboard_log_debug_fb();

  g_return_if_fail(self != NULL);

  if (self->message != NULL)
    dbus_message_unref(self->message);
	
  if (self->context != NULL)
    g_object_unref(G_OBJECT(self->context));

  self->message = NULL;
  self->context = NULL;

  g_free(self);

  whiteboard_log_debug_fe();
}

void whiteboard_sib_access_handle_unref(WhiteBoardSIBAccessHandle* self)
{
  whiteboard_log_debug_fb();

  g_return_if_fail(self != NULL);

  if (g_atomic_int_dec_and_test(&self->refcount) == FALSE)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
			    "Handle refcount: %d\n", self->refcount);
    }
  else
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
			    "Handle refcount zeroed\n");
      whiteboard_sib_access_handle_destroy(self);
    }

  whiteboard_log_debug_fe();
}

WhiteBoardSIBAccess *whiteboard_sib_access_handle_get_context(WhiteBoardSIBAccessHandle *self)
{
  g_return_val_if_fail(self != NULL, NULL);
  return self->context;
}

/*****************************************************************************
 * Custom commands
 *****************************************************************************/

guint whiteboard_sib_access_send_custom_command(WhiteBoardSIBAccess* self,
						WhiteBoardCmd* cmd)
{
  guint serial = 0;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(self != NULL, 0);
  g_return_val_if_fail(cmd != NULL, 0);

  serial = whiteboard_cmd_send(cmd, self->connection);

  whiteboard_log_debug_fe();

  return serial;
}

/*****************************************************************************
 * Private functions
 *****************************************************************************/

static DBusHandlerResult whiteboard_sib_access_refresh(DBusConnection *conn,
						       DBusMessage *msg,
						       gpointer data)
{
  WhiteBoardSIBAccess *context = (WhiteBoardSIBAccess *) data;

  whiteboard_log_debug_fb();

  g_signal_emit(context, whiteboard_sib_access_signals[SIGNAL_REFRESH], 0);

  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_HANDLED;

}

static DBusHandlerResult whiteboard_sib_access_shutdown(DBusConnection *conn,
							DBusMessage *msg,
							gpointer data)
{
  WhiteBoardSIBAccess *context = (WhiteBoardSIBAccess *) data;

  whiteboard_log_debug_fb();

  g_signal_emit(context, whiteboard_sib_access_signals[SIGNAL_SHUTDOWN], 0);

  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_HANDLED;
}


static DBusHandlerResult whiteboard_sib_access_healthcheck(DBusConnection *conn,
							   DBusMessage *msg,
							   gpointer data)
{
  WhiteBoardSIBAccess *context = (WhiteBoardSIBAccess *) data;
  gboolean alive = TRUE;

  whiteboard_log_debug_fb();

  /* TODO: Get return value from signal emission */
  g_signal_emit(context, whiteboard_sib_access_signals[SIGNAL_HEALTHCHECK], 0);
	
  whiteboard_util_send_method_return(conn, msg,
				     DBUS_TYPE_BOOLEAN, &alive,
				     WHITEBOARD_UTIL_LIST_END);
	
  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult whiteboard_sib_access_join(DBusConnection *conn,
						    DBusMessage *msg,
						    gpointer data)
{
  WhiteBoardSIBAccess *context = (WhiteBoardSIBAccess *) data;
  WhiteBoardSIBAccessHandle* handle = NULL;
  
  guchar *uuid = NULL;
  gint join_id = 0;
  gint msgnum = 0;
  guchar *nodeid = NULL;
  whiteboard_log_debug_fb();
  g_return_val_if_fail(context != NULL,DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
  
  if (whiteboard_util_parse_message(msg,
				DBUS_TYPE_INT32, &join_id,
				DBUS_TYPE_STRING, &nodeid,
				DBUS_TYPE_STRING, &uuid,
				DBUS_TYPE_INT32, &msgnum,
				    WHITEBOARD_UTIL_LIST_END))
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "Join NodeID: %s\tUUID: %s\n", nodeid, uuid);

      /* TODO: Is this necessary??? */
      //whiteboard_util_send_method_return(conn, msg, WHITEBOARD_UTIL_LIST_END);

      handle = whiteboard_sib_access_handle_new(context, msg);

      whiteboard_log_debug("handle:\t%p\n", handle);
      whiteboard_log_debug("context:\t%p\n", context);
      whiteboard_log_debug("handle->context:\t%p\n", handle->context);
      whiteboard_log_debug("conn:\t%p\n", conn);
      whiteboard_log_debug("handle->contex->connection:\t%p\n", handle->context->connection);
  
      g_signal_emit(context,
		    whiteboard_sib_access_signals[SIGNAL_JOIN],
		    0,
		    handle,
		    join_id,
		    nodeid,
		    uuid,
		    msgnum);

    }
  else if(join_id != 0){
    whiteboard_sib_access_send_join_complete(handle, join_id, -1);
  }
  whiteboard_sib_access_handle_unref(handle);
  whiteboard_log_debug_fe();
  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult whiteboard_sib_access_leave(DBusConnection *conn,
						     DBusMessage *msg,
						     gpointer data)
{
  WhiteBoardSIBAccess *context = (WhiteBoardSIBAccess *) data;
  WhiteBoardSIBAccessHandle* handle = NULL;
  gint msgnum = 0;
  guchar *uuid = NULL;
  guchar *nodeid = NULL;
  whiteboard_log_debug_fb();
  g_return_val_if_fail(context != NULL,DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
  
  whiteboard_util_parse_message(msg,
				DBUS_TYPE_STRING, &nodeid,
				DBUS_TYPE_STRING, &uuid,
				DBUS_TYPE_INT32, &msgnum, 
				WHITEBOARD_UTIL_LIST_END);
  
  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "Leave, NodeID: %s\tUUID: %s\n", nodeid, uuid);
  
  /* TODO: Is this necessary??? */
  //whiteboard_util_send_method_return(conn, msg, WHITEBOARD_UTIL_LIST_END);

  handle = whiteboard_sib_access_handle_new(context, msg);

  whiteboard_log_debug("handle:\t%p\n", handle);
  whiteboard_log_debug("context:\t%p\n", context);
  whiteboard_log_debug("handle->context:\t%p\n", handle->context);
  whiteboard_log_debug("conn:\t%p\n", conn);
  whiteboard_log_debug("handle->contex->connection:\t%p\n", handle->context->connection);
  
  g_signal_emit(context,
		whiteboard_sib_access_signals[SIGNAL_LEAVE],
		0,
		handle,
		nodeid,
		uuid,
		msgnum);
  whiteboard_sib_access_handle_unref(handle);

  whiteboard_log_debug_fe();
  return DBUS_HANDLER_RESULT_HANDLED;
}


static DBusHandlerResult whiteboard_sib_access_unsubscribe(DBusConnection *conn,
							   DBusMessage *msg,
							   gpointer data)
{
  WhiteBoardSIBAccess *context = (WhiteBoardSIBAccess *) data;
  WhiteBoardSIBAccessHandle* handle = NULL;
  gint msgnum = 0;
  guchar *uuid = NULL;
  guchar *nodeid = NULL;
  guchar *request = NULL;
  gint accessid = -1;
  whiteboard_log_debug_fb();
  g_return_val_if_fail(context != NULL,DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
  
  whiteboard_util_parse_message(msg,
				DBUS_TYPE_INT32, &accessid,
				DBUS_TYPE_STRING, &nodeid,
				DBUS_TYPE_STRING, &uuid,
				DBUS_TYPE_INT32, &msgnum,
				DBUS_TYPE_STRING, &request,
				WHITEBOARD_UTIL_LIST_END);
  
  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "Unsubscribe, NodeID: %s\tUUID: %s\tsubscription_ids: %s\n", nodeid, uuid, request);
  
  /* TODO: Is this necessary??? */
  //whiteboard_util_send_method_return(conn, msg, WHITEBOARD_UTIL_LIST_END);

  handle = whiteboard_sib_access_handle_new(context, msg);

  whiteboard_log_debug("handle:\t%p\n", handle);
  whiteboard_log_debug("context:\t%p\n", context);
  whiteboard_log_debug("handle->context:\t%p\n", handle->context);
  whiteboard_log_debug("conn:\t%p\n", conn);
  whiteboard_log_debug("handle->contex->connection:\t%p\n", handle->context->connection);
  
  g_signal_emit(context,
		whiteboard_sib_access_signals[SIGNAL_UNSUBSCRIBE],
		0,
		handle,
		accessid,
		nodeid,
		uuid,
		msgnum,
		request);
  whiteboard_sib_access_handle_unref(handle);

  whiteboard_log_debug_fe();
  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult whiteboard_sib_access_remove(DBusConnection *conn,
						      DBusMessage *msg,
						      gpointer data)
{
  WhiteBoardSIBAccess *context = (WhiteBoardSIBAccess *) data;
  WhiteBoardSIBAccessHandle *handle = NULL;
  guchar *nodeid = NULL;
  guchar *sibid = NULL;
  guchar *request = NULL;
  gint msgnum = 0;
  EncodingType encoding;
  whiteboard_log_debug_fb();
  
  if(whiteboard_util_parse_message(msg,
				DBUS_TYPE_STRING, &nodeid,
				DBUS_TYPE_STRING, &sibid,
				DBUS_TYPE_INT32, &msgnum,
				DBUS_TYPE_INT32, &encoding,
				DBUS_TYPE_STRING, &request,
				   WHITEBOARD_UTIL_LIST_END) )
    {

      whiteboard_log_debug("Remove: nodeid:%s, sibid :%s, msgnum: %d, encoding: %d, request :%s\n", nodeid, sibid, msgnum, encoding, request);
      
      handle = whiteboard_sib_access_handle_new(context, msg);
      
      g_signal_emit(context,
		    whiteboard_sib_access_signals[SIGNAL_REMOVE],
		    0,
		    handle,
		    nodeid,
		    sibid,
		    msgnum,
		    encoding,
		    request);
    }
  else
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
			    " Could not parse remove DBUS message\n");
      whiteboard_sib_access_send_remove_response(handle, -1, (guchar *)"sib:invalidTripleId");
    }
  whiteboard_sib_access_handle_unref(handle);
  whiteboard_log_debug_fe();
  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult whiteboard_sib_access_insert(DBusConnection *conn,
						      DBusMessage *msg,
						      gpointer data )
{
  WhiteBoardSIBAccess *context = (WhiteBoardSIBAccess *) data;
  WhiteBoardSIBAccessHandle *handle = NULL;
  guchar *nodeid = NULL;
  guchar *sibid = NULL;
  guchar *request = NULL;
  gint msgnum = 0;
  EncodingType encoding;
  whiteboard_log_debug_fb();
  
  if(whiteboard_util_parse_message(msg,
				   DBUS_TYPE_STRING, &nodeid,
				   DBUS_TYPE_STRING, &sibid,
				   DBUS_TYPE_INT32, &msgnum,
				   DBUS_TYPE_INT32, &encoding,
				   DBUS_TYPE_STRING, &request,
				   WHITEBOARD_UTIL_LIST_END) )
    {
      
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
			    "Insert Message, node: %s\tSib: %s\t, msgnum: %d\t, request: %s\n", nodeid, sibid, msgnum, request);
      
      handle = whiteboard_sib_access_handle_new(context, msg);
      
      g_signal_emit(context,
		    whiteboard_sib_access_signals[SIGNAL_INSERT],
		    0,
		    handle,
		    nodeid,
		    sibid,
		    msgnum,
		    encoding,
		    request);
    }
  else
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
			    " Could not parse Insert DBUS message\n");
      whiteboard_sib_access_send_insert_response(handle, -1, (guchar*)"sib:invalidTripleId");
    }

  whiteboard_sib_access_handle_unref(handle);
    
  whiteboard_log_debug_fe();
  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult whiteboard_sib_access_update(DBusConnection *conn,
						      DBusMessage *msg,
						      gpointer data )
{
  WhiteBoardSIBAccess *context = (WhiteBoardSIBAccess *) data;
  WhiteBoardSIBAccessHandle *handle = NULL;
  guchar *nodeid = NULL;
  guchar *sibid = NULL;
  guchar *insert_request = NULL;
  guchar *remove_request = NULL;
  EncodingType encoding;
  gint msgnum = 0;
  whiteboard_log_debug_fb();

  if( whiteboard_util_parse_message(msg,
				DBUS_TYPE_STRING, &nodeid,
				DBUS_TYPE_STRING, &sibid,
				DBUS_TYPE_INT32, &msgnum,
				DBUS_TYPE_INT32, &encoding,
				DBUS_TYPE_STRING, &insert_request,
				DBUS_TYPE_STRING, &remove_request,
				    WHITEBOARD_UTIL_LIST_END) )
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
			    "Update Message, node: %s\tSib: %s\t, msgnum: %d\t, ins_request: %s\t, rem_request: %s\n", nodeid, sibid, msgnum, insert_request, remove_request);
      
      handle = whiteboard_sib_access_handle_new(context, msg);
      
      g_signal_emit(context,
		    whiteboard_sib_access_signals[SIGNAL_UPDATE],
		    0,
		    handle,
		    nodeid,
		    sibid,
		    msgnum,
		    encoding,
		    insert_request,
		    remove_request);
    }
  else
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
			    " Could not parse Update DBUS message\n");
      whiteboard_sib_access_send_update_response(handle, -1, (guchar *)"sib:invalidTripleId");
    }      
  whiteboard_sib_access_handle_unref(handle);
  whiteboard_log_debug_fe();
  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult whiteboard_sib_access_subscribe_query(DBusConnection *conn,
							       DBusMessage *msg,
							       gpointer data,
							       gint method)
{
  WhiteBoardSIBAccess *context = (WhiteBoardSIBAccess *) data;
  WhiteBoardSIBAccessHandle *handle = NULL;
  gint access_id = -1;
  guchar *nodeid = NULL;
  guchar *sibid = NULL;
  guchar *request = NULL;
  gint type = -1;
  gint msgnum = 0;
  whiteboard_log_debug_fb();
  
  whiteboard_util_parse_message(msg,
				DBUS_TYPE_INT32, &access_id,
				DBUS_TYPE_STRING, &nodeid,
				DBUS_TYPE_STRING, &sibid,
				DBUS_TYPE_INT32, &msgnum,
				DBUS_TYPE_INT32, &type,
				DBUS_TYPE_STRING, &request,
				WHITEBOARD_UTIL_LIST_END);

  handle = whiteboard_sib_access_handle_new(context, msg);
  {
    gchar *debugtxt=NULL;
    switch(method){case SIGNAL_QUERY: debugtxt=g_strdup("Query");break;
    case SIGNAL_INSERT:  debugtxt=g_strdup("Insert");break;
    case SIGNAL_SUBSCRIBE:  debugtxt=g_strdup("Subscribe");break;
    case SIGNAL_REMOVE:  debugtxt=g_strdup("Remove");break;
    default: debugtxt=g_strdup("Unknown");break;
    }
    whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "Emitting %s signal\n",debugtxt);
    g_free(debugtxt);
  }
   
  g_signal_emit(context,
		whiteboard_sib_access_signals[method],
		0,
		handle,
		access_id,
		nodeid,
		sibid,
		msgnum,
		type,
		request);

  whiteboard_sib_access_handle_unref(handle);
  whiteboard_log_debug_fe();
  return DBUS_HANDLER_RESULT_HANDLED;
}

gint whiteboard_sib_access_send_join_complete(WhiteBoardSIBAccessHandle *handle,
					      gint browse_id,
					      ssStatus_t status)
{
  DBusConnection *conn = handle->context->connection;
  
  whiteboard_log_debug_fb();
  g_return_val_if_fail(conn != NULL,-1);
  
  whiteboard_util_send_signal(WHITEBOARD_DBUS_OBJECT,
			      WHITEBOARD_DBUS_SIB_ACCESS_INTERFACE,
			      WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_JOIN_COMPLETE,
			      conn,
			      DBUS_TYPE_INT32, &browse_id,
			      DBUS_TYPE_INT32, &status,
			      WHITEBOARD_UTIL_LIST_END);
  
  whiteboard_log_debug_fe();
  
  return 0;	
}

void whiteboard_sib_access_send_insert_response(WhiteBoardSIBAccessHandle *handle,
						gint status,
						const guchar *insert_response)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(insert_response != NULL);
  DBusConnection* conn = handle->context->connection;
  DBusMessage *packet= handle->message;
  whiteboard_log_debug_fb();

  g_return_if_fail(conn != NULL );

  whiteboard_util_send_method_return(conn, packet,
				     DBUS_TYPE_INT32, &status,
				     DBUS_TYPE_STRING, &insert_response,
				     WHITEBOARD_UTIL_LIST_END);
  whiteboard_log_debug_fe();
}


void whiteboard_sib_access_send_update_response(WhiteBoardSIBAccessHandle *handle,
						gint status,
						const guchar *response)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(response != NULL);
  DBusConnection* conn = handle->context->connection;
  DBusMessage *packet= handle->message;
  whiteboard_log_debug_fb();

  g_return_if_fail(conn != NULL );

  whiteboard_util_send_method_return(conn, packet,
				     DBUS_TYPE_INT32, &status,
				     DBUS_TYPE_STRING, &response,
				     WHITEBOARD_UTIL_LIST_END);
  whiteboard_log_debug_fe();
}

void whiteboard_sib_access_send_remove_response(WhiteBoardSIBAccessHandle *handle,
						gint success,
						const guchar *response)
{
  g_return_if_fail(handle != NULL);
  DBusConnection* conn = handle->context->connection;
  DBusMessage *packet= handle->message;
  whiteboard_log_debug_fb();

  g_return_if_fail(conn != NULL );
  g_return_if_fail(response != NULL );

  whiteboard_util_send_method_return(conn, packet,
				     DBUS_TYPE_INT32, &success,
				     DBUS_TYPE_STRING, &response,
				     WHITEBOARD_UTIL_LIST_END);
  whiteboard_log_debug_fe();
}

void whiteboard_sib_access_send_query_response(WhiteBoardSIBAccessHandle *handle,
					       gint access_id,
					       gint status,
					       const guchar *results)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(results != NULL);
  DBusConnection* conn = handle->context->connection;
  DBusMessage *packet= handle->message;
  whiteboard_log_debug_fb();

  g_return_if_fail(conn != NULL );

  whiteboard_util_send_method_return(conn, packet,
				     DBUS_TYPE_INT32, &access_id,
				     DBUS_TYPE_INT32, &status,
				     DBUS_TYPE_STRING, &results,
				     WHITEBOARD_UTIL_LIST_END);
  whiteboard_log_debug_fe();
}


void whiteboard_sib_access_send_subscribe_response(WhiteBoardSIBAccessHandle *handle,
						   gint access_id,
						   gint status,
						   const guchar *subscription_id,
						   const guchar *results)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(subscription_id != NULL);
  g_return_if_fail(results != NULL);
  DBusConnection* conn = handle->context->connection;
  DBusMessage *packet= handle->message;
  whiteboard_log_debug_fb();
  
  g_return_if_fail(conn != NULL );
  
  whiteboard_util_send_method_return(conn, packet,
				     DBUS_TYPE_INT32, &access_id,
				     DBUS_TYPE_INT32, &status,
				     DBUS_TYPE_STRING, &subscription_id,
				     DBUS_TYPE_STRING, &results,
				     WHITEBOARD_UTIL_LIST_END);
  whiteboard_log_debug_fe(); 
}

void whiteboard_sib_access_send_subscription_indication(WhiteBoardSIBAccessHandle *handle,
							gint access_id,
							gint update_sequence,
							const guchar* subscriptionid,
							const guchar *results_added,
							const guchar *results_removed)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(subscriptionid != NULL);
  g_return_if_fail(results_added != NULL);
  g_return_if_fail(results_removed != NULL);
  DBusConnection* conn = handle->context->connection;
  whiteboard_log_debug_fb();
  
  g_return_if_fail(conn != NULL );
  
  whiteboard_util_send_signal(WHITEBOARD_DBUS_OBJECT,
			      WHITEBOARD_DBUS_SIB_ACCESS_INTERFACE,
			      WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_SUBSCRIPTION_IND,
			      conn,
			      DBUS_TYPE_INT32, &access_id,
			      DBUS_TYPE_INT32, &update_sequence,
			      DBUS_TYPE_STRING, &subscriptionid,
			      DBUS_TYPE_STRING, &results_added,
			      DBUS_TYPE_STRING, &results_removed,
			      WHITEBOARD_UTIL_LIST_END);
  whiteboard_log_debug_fe(); 
}

void whiteboard_sib_access_send_unsubscribe_complete(WhiteBoardSIBAccessHandle* handle,
						     gint access_id,
						     gint status,
						     const guchar *subscription_id)
{
  g_return_if_fail(handle != NULL);
  g_return_if_fail(subscription_id != NULL);
  DBusConnection* conn = handle->context->connection;
  whiteboard_log_debug_fb();
  
  g_return_if_fail(conn != NULL );
  
  whiteboard_util_send_signal(WHITEBOARD_DBUS_OBJECT,
			      WHITEBOARD_DBUS_SIB_ACCESS_INTERFACE,
			      WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_UNSUBSCRIBE_COMPLETE,
			      conn,
			      DBUS_TYPE_INT32, &access_id,
			      DBUS_TYPE_INT32, &status,
			      DBUS_TYPE_STRING, &subscription_id,			      
			      WHITEBOARD_UTIL_LIST_END);
  whiteboard_log_debug_fe(); 
} 

static DBusHandlerResult whiteboard_sib_access_custom_command_request(
								      DBusConnection *conn, DBusMessage *msg, gpointer data)
{
  WhiteBoardSIBAccess *context = (WhiteBoardSIBAccess *) data;
  WhiteBoardCmdRequest* request = NULL;
	
  whiteboard_log_debug_fb();
	
  request = (WhiteBoardCmdRequest*) whiteboard_cmd_new_with_msg(msg);
	
  g_signal_emit(context,
		whiteboard_sib_access_signals[SIGNAL_CUSTOM_COMMAND_REQUEST],
		0,
		request);
	
  whiteboard_cmd_unref(request);

  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult whiteboard_sib_access_custom_command_response(
								       DBusConnection *conn, DBusMessage *msg, gpointer data)
{
  WhiteBoardSIBAccess *context = (WhiteBoardSIBAccess *) data;
  WhiteBoardCmdResponse* response = NULL;
	
  whiteboard_log_debug_fb();
	
  response = (WhiteBoardCmdResponse*) whiteboard_cmd_new_with_msg(msg);
	
  g_signal_emit(context,
		whiteboard_sib_access_signals[SIGNAL_CUSTOM_COMMAND_RESPONSE],
		0,
		response);
	
  whiteboard_cmd_unref(response);

  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult whiteboard_sib_access_handler(DBusConnection* conn,
						       DBusMessage* message,
						       gpointer data)
{
  DBusHandlerResult retval = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const gchar* member = NULL;
  gint type = 0;

  whiteboard_log_debug_fb();

  member = dbus_message_get_member(message);
  type = dbus_message_get_type(message);

  g_return_val_if_fail(member != NULL,
		       DBUS_HANDLER_RESULT_NOT_YET_HANDLED);

  switch (type)
    {
    case DBUS_MESSAGE_TYPE_SIGNAL:
      if(!strcmp(member, WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_JOIN)) 
	{
	  whiteboard_log_debug("Got JOIN signal\n");
	  retval = whiteboard_sib_access_join(conn,message,data);
	}
      else if(!strcmp(member, WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_LEAVE)) 
	{
	  whiteboard_log_debug("Got LEAVE signal\n");
	  retval = whiteboard_sib_access_leave(conn,message,data);
	}
      else if(!strcmp(member, WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_UNSUBSCRIBE)) 
	{
	  whiteboard_log_debug("Got UNSUBSCRIBE signal\n");
	  retval = whiteboard_sib_access_unsubscribe(conn,message,data);
	}
      break;

    case DBUS_MESSAGE_TYPE_METHOD_RETURN:

      whiteboard_log_warning("Unknown method return: %s\n", member);
		
      retval = DBUS_HANDLER_RESULT_HANDLED;
		
      break;

    case DBUS_MESSAGE_TYPE_METHOD_CALL:

      if(!strcmp(member, WHITEBOARD_DBUS_SIB_ACCESS_METHOD_INSERT)) 
	{
	  whiteboard_log_debug("Got INSERT method\n");
	  retval = whiteboard_sib_access_insert(conn,message,data);
	}
      else if(!strcmp(member, WHITEBOARD_DBUS_SIB_ACCESS_METHOD_UPDATE)) 
	{
	  whiteboard_log_debug("Got UPDATE method\n");
	  retval = whiteboard_sib_access_update(conn,message,data);
	}
      else if(!strcmp(member, WHITEBOARD_DBUS_SIB_ACCESS_METHOD_QUERY)) 
	{
	  whiteboard_log_debug("Got QUERY method\n");
	  retval = whiteboard_sib_access_subscribe_query(conn,message,data, SIGNAL_QUERY);
	}
      else if(!strcmp(member, WHITEBOARD_DBUS_SIB_ACCESS_METHOD_SUBSCRIBE))
	{
	  whiteboard_log_debug("Got SUBSCRIBE/QUERY method\n");
	  retval = whiteboard_sib_access_subscribe_query(conn,message,data,SIGNAL_SUBSCRIBE);
	}
      else if(!strcmp(member, WHITEBOARD_DBUS_SIB_ACCESS_METHOD_REMOVE)) 
	{
	  whiteboard_log_debug("Got REMOVE method\n");
	  retval = whiteboard_sib_access_remove(conn,message,data);
	}
      else
	{
	  whiteboard_log_warning("Unknown method call: %s\n",
				 member);
	}

      break;
    default:
      whiteboard_log_warning("Unknown message type: %d\n", type);
      break;
    }

  whiteboard_log_debug_fe();

  return retval;
}

static DBusHandlerResult whiteboard_sib_access_control_handler(DBusConnection* conn,
							       DBusMessage* message,
							       gpointer data)
{
  DBusHandlerResult retval = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const gchar* member = NULL;
  gint type = 0;

  whiteboard_log_debug_fb();

  member = dbus_message_get_member(message);
  type = dbus_message_get_type(message);

  g_return_val_if_fail(member != NULL,
		       DBUS_HANDLER_RESULT_NOT_YET_HANDLED);

  switch (type)
    {
    case DBUS_MESSAGE_TYPE_SIGNAL:

      if (!strcmp(member, WHITEBOARD_DBUS_CONTROL_SIGNAL_STARTING))
	{
	  /* If we get this signal it means that Whiteboard has 
	     been shut down (abnormally) and is restarting. To
	     avoid duplicate processes we'll shut down and let
	     Whiteboard restart us right away. */
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
				"Whiteboard restart detected, " \
				"shutting down.\n");
	  retval = whiteboard_sib_access_shutdown(conn,
						  message,
						  data);
	}
      else
	{
	  whiteboard_log_warning("Unknown signal: %s\n", member);
	}
		
      break;

    case DBUS_MESSAGE_TYPE_METHOD_RETURN:

      if (!strcmp(member, WHITEBOARD_DBUS_METHOD_CUSTOM_COMMAND))
	{
	  retval = whiteboard_sib_access_custom_command_response(conn,
								 message,
								 data);
	}
      else
	{
	  whiteboard_log_warning("Unknown method return: %s\n",
				 member);
	}
		
      retval = DBUS_HANDLER_RESULT_HANDLED;
		
      break;

    case DBUS_MESSAGE_TYPE_METHOD_CALL:

      if (!strcmp(member, WHITEBOARD_DBUS_CONTROL_METHOD_REFRESH))
	{
	  retval = whiteboard_sib_access_refresh(conn,
						 message,
						 data);
	}
      else if (!strcmp(member, WHITEBOARD_DBUS_CONTROL_METHOD_SHUTDOWN))
	{
	  retval = whiteboard_sib_access_shutdown(conn,
						  message,
						  data);
	}
      else if (!strcmp(member, WHITEBOARD_DBUS_CONTROL_METHOD_HEALTHCHECK))
	{
	  retval = whiteboard_sib_access_healthcheck(conn,
						     message,
						     data);
	}
      else if (!strcmp(member, WHITEBOARD_DBUS_CONTROL_METHOD_CUSTOM_COMMAND))
	{
	  retval = whiteboard_sib_access_custom_command_request(conn,
								message,
								data);
	}
      else
	{
	  whiteboard_log_warning("Unknown method call: %s\n",
				 member);
	}

      break;
    default:
      whiteboard_log_warning("Unknown message type: %d\n", type);
      break;
    }

  whiteboard_log_debug_fe();

  return retval;
}

static DBusHandlerResult whiteboard_sib_access_dispatch_message(DBusConnection *conn,
								DBusMessage *message,
								gpointer data)
{
  DBusHandlerResult retval = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const gchar* interface = NULL;

  whiteboard_log_debug_fb();

  interface = dbus_message_get_interface(message);

  g_return_val_if_fail(interface != NULL,
		       DBUS_HANDLER_RESULT_NOT_YET_HANDLED);

  if (!strcmp(interface, WHITEBOARD_DBUS_SIB_ACCESS_INTERFACE))
    {
      retval = whiteboard_sib_access_handler(conn, message, data);
    }
  else if (!strcmp(interface, WHITEBOARD_DBUS_CONTROL_INTERFACE))
    {
      retval = whiteboard_sib_access_control_handler(conn, message, data);
    }
  else
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,
			    "Unknown message: type:%d, if:%s, mem:%s\n",
			    dbus_message_get_type(message),
			    dbus_message_get_interface(message),
			    dbus_message_get_member(message));
		
      retval = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
	
  whiteboard_log_debug_fe();
	
  return retval;
}

/*****************************************************************************
 * Connection registration
 *****************************************************************************/

/**
 * Handle the unregistration of a DBus connection
 *
 */
static void whiteboard_sib_access_unregister_handler(DBusConnection *connection,
						     gpointer data)
{
  whiteboard_log_debug_fb();
  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "Unregistered\n");
  whiteboard_log_debug_fe();
}

static gboolean whiteboard_sib_access_register(WhiteBoardSIBAccess *self, guchar *uuid)
{
  whiteboard_log_debug_fb();

  /* Register with Whiteboard */
  /* TODO: should not try forever... */

  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			"Attempting to register WhiteBoardSIBAccess with Whiteboard Daemon.\n");
  while (whiteboard_util_register_try(self,(gchar *) uuid,
				      WHITEBOARD_DBUS_REGISTER_METHOD_SIB,
				      whiteboard_sib_access_unregister_handler,
				      whiteboard_sib_access_dispatch_message) 
	 == FALSE)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			    "Registration failed. Sleeping\n");
      g_usleep(WHITEBOARD_REGISTRATION_TIMEOUT);
    }
  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			"WhiteBoardSIBAccess registered successfully with Whiteboard Daemon.\n");

  whiteboard_log_debug_fe();

  return TRUE;
}

static gboolean whiteboard_sib_access_register_control(WhiteBoardSIBAccess *self,
						       guchar *uuid)
{
  DBusError err;
  DBusObjectPathVTable vtable = { whiteboard_sib_access_unregister_handler,
				  whiteboard_sib_access_dispatch_message,
				  NULL, NULL, NULL, NULL
  };

  whiteboard_log_debug_fb();

  /* Register control channel with Whiteboard */
  /* TODO: should not try forever... */

  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			"Attempting to register control channel with Whiteboard Daemon.\n");
  while (whiteboard_util_register_try(self, (gchar *)uuid,
				      WHITEBOARD_DBUS_REGISTER_METHOD_CONTROL,
				      whiteboard_sib_access_unregister_handler,
				      whiteboard_sib_access_dispatch_message) 
	 == FALSE)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			    "Registration failed. Sleeping.\n");
      g_usleep(WHITEBOARD_REGISTRATION_TIMEOUT);
    }

  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			"WhiteBoardSIBAccess registered successfully with Whiteboard Daemon.\n");

  /* We'll listen Whiteboard common alive broadcasts from session bus
   * and call local shutdown callback if we detect that common 
   * has been restarted.
   */
  dbus_error_init(&err);
  self->sessionbus_connection = dbus_bus_get_private(DBUS_BUS_SESSION, &err);

  if (dbus_error_is_set(&err))
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			    "Could not get session bus connection: %s\n",
			    err.message);

      return FALSE;
    }
  else
    {
      dbus_connection_register_fallback(self->sessionbus_connection, 
					WHITEBOARD_DBUS_OBJECT, &vtable,
					self);

      dbus_connection_setup_with_g_main(self->sessionbus_connection,
					self->main_context);
    }

  dbus_error_free(&err);

  dbus_bus_add_match(self->sessionbus_connection, 
		     "type='signal', interface='" \
		     WHITEBOARD_DBUS_CONTROL_INTERFACE "'", &err);
	
  if (dbus_error_is_set(&err))
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			    "Could not set match for control interface: %s\n",
			    err.message);
    }

  dbus_connection_flush(self->sessionbus_connection);

  whiteboard_log_debug_fe();

  return TRUE;
}
