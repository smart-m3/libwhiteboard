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
 * Sib library
 *
 * sib_object.c
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

#include "sib_dbus_ifaces.h"

#include "sib_object.h"
#include <whiteboard_util.h>
#include <whiteboard_log.h>

#include "sib_marshal.h"

/*****************************************************************************
 * Private function prototypes
 *****************************************************************************/
static void sib_object_finalize(GObject* object);

static gboolean sib_object_register(SibObject *self, gchar *uuid);

static gboolean sib_object_register_control(SibObject *self,
					    gchar *uuid);

static void sib_object_unregister_handler(DBusConnection *connection,
					  gpointer user_data);

static DBusHandlerResult sib_object_dispatch_message(DBusConnection *conn,
						     DBusMessage *msg,
						     gpointer data);
static DBusHandlerResult sib_object_control_handler(DBusConnection* conn,
						    DBusMessage* message,
						    gpointer data);

static DBusHandlerResult sib_object_control_handler_register_sib(DBusConnection* conn,
								 DBusMessage* message,
								 gpointer data);
static DBusHandlerResult sib_object_control_handler_healthcheck(DBusConnection* conn,
								DBusMessage* message,
								gpointer data);
static DBusHandlerResult sib_object_control_handler_shutdown(DBusConnection* conn,
							     DBusMessage* message,
							     gpointer data);
static DBusHandlerResult sib_object_control_handler_refresh(DBusConnection* conn,
							    DBusMessage* message,
							    gpointer data);

//atic DBusHandlerResult sib_object_control_handler_starting(DBusConnection* conn,
//						    DBusMessage* message,
//						    gpointer data);

							

/******************************************************************************
 * SibObjectHandle definitions
 ******************************************************************************/

struct _SibObjectHandle
{
  SibObject *context;
  DBusMessage *message;
  gint refcount;
};

/******************************************************************************
 * SibObject definitions
 ******************************************************************************/

/**
 * The SibObject object structure
 */
struct _SibObject
{
  /* WhiteboardLibHeader BEGIN. DO NOT change the order of these vars! */
  GObject parent;

  gchar *uuid;
  gchar *control_uuid;
  gchar *friendly_name;
  gchar *mimetypes;
  gboolean local;
  DBusConnection *connection;
  GMainContext *main_context;
  /* WhiteBoardLibHeader END */

  DBusConnection* sessionbus_connection;
  gchar *description;
  
  GMutex *send_lock;
  GMutex *recv_lock;

};

/**
 * The SibObject class structure
 */
struct _SibObjectClass
{
  GObjectClass parent;

  SibObjectRefreshCB refresh_cb;
  SibObjectShutdownCB shutdown_cb;
  SibObjectHealthCheckCB healthcheck_cb;
  SibObjectRegisterSibCB register_sib_cb;


  SibObjectJoinCnfCB joincnf_cb;
  SibObjectLeaveCnfCB leavecnf_cb;
  SibObjectInsertCnfCB insertcnf_cb;
  SibObjectRemoveCnfCB removecnf_cb;
  SibObjectUpdateCnfCB updatecnf_cb;
  SibObjectQueryCnfCB querycnf_cb;
  SibObjectSubscribeCnfCB subscribecnf_cb;
  SibObjectSubscriptionIndCB subscriptionind_cb;

  SibObjectUnsubscribeCnfCB unsubscribecnf_cb;
  SibObjectUnsubscribeIndCB unsubscribeind_cb;
  SibObjectLeaveIndCB leaveind_cb;
  
};

enum
  {
    SIGNAL_REFRESH = 0,
    SIGNAL_SHUTDOWN,
    SIGNAL_HEALTHCHECK,
    SIGNAL_REGISTER_SIB,
    SIGNAL_JOIN_CNF,
    SIGNAL_LEAVE_CNF,
    SIGNAL_INSERT_CNF,
    SIGNAL_REMOVE_CNF,
    SIGNAL_UPDATE_CNF,
    SIGNAL_QUERY_CNF,
    SIGNAL_SUBSCRIBE_CNF,
    SIGNAL_SUBSCRIPTION_IND,
    SIGNAL_UNSUBSCRIBE_CNF,
    SIGNAL_UNSUBSCRIBE_IND,
    SIGNAL_LEAVE_IND,
    NUM_SIGNALS
  };

static guint sib_object_signals[NUM_SIGNALS];

static void sib_object_lock_send(SibObject *self)
{
  g_mutex_lock(self->send_lock);
}

static void sib_object_unlock_send(SibObject *self)
{
  g_mutex_unlock(self->send_lock);
}

static void sib_object_lock_recv(SibObject *self)
{
  g_mutex_lock(self->recv_lock);
}

static void sib_object_unlock_recv(SibObject *self)
{
  g_mutex_unlock(self->recv_lock);
}

static void sib_object_class_init(SibObjectClass *self)
{
  GObjectClass* object = G_OBJECT_CLASS(self);

  g_return_if_fail(self != NULL);

  // object->constructor = sib_object_constructor;
  // object->dispose = sib_object_dispose;
  object->finalize = sib_object_finalize;

  /*********************************************************************/

  sib_object_signals[SIGNAL_REFRESH] =
    g_signal_new(SIB_OBJECT_SIGNAL_REFRESH,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, refresh_cb),
		 NULL,
		 NULL,
		 marshal_VOID__VOID,
		 G_TYPE_NONE,
		 0,
		 G_TYPE_NONE);

  sib_object_signals[SIGNAL_SHUTDOWN] =
    g_signal_new(SIB_OBJECT_SIGNAL_SHUTDOWN,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, shutdown_cb),
		 NULL,
		 NULL,
		 marshal_VOID__VOID,
		 G_TYPE_NONE,
		 0,
		 G_TYPE_NONE);
  
  sib_object_signals[SIGNAL_HEALTHCHECK] =
    g_signal_new(SIB_OBJECT_SIGNAL_HEALTHCHECK,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, healthcheck_cb),
		 NULL,
		 NULL,
		 marshal_VOID__VOID,
		 G_TYPE_NONE,
		 0,
		 G_TYPE_NONE);


  sib_object_signals[SIGNAL_REGISTER_SIB] =
    g_signal_new(SIB_OBJECT_SIGNAL_REGISTER_SIB,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, register_sib_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER_STRING,
		 G_TYPE_NONE,
		 2,
		 G_TYPE_POINTER,
		 G_TYPE_STRING,
		 G_TYPE_NONE);


  sib_object_signals[SIGNAL_JOIN_CNF] =
    g_signal_new(SIB_OBJECT_SIGNAL_JOIN_CNF,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, joincnf_cb),
		 NULL,
		 NULL,
		 marshal_VOID__STRING_STRING_INT_INT_STRING,
		 G_TYPE_NONE,
		 5,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_NONE);

  sib_object_signals[SIGNAL_LEAVE_CNF] =
    g_signal_new(SIB_OBJECT_SIGNAL_LEAVE_CNF,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, leavecnf_cb),
		 NULL,
		 NULL,
		 marshal_VOID__STRING_STRING_INT_INT,
		 G_TYPE_NONE,
		 4,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_NONE);
    
  sib_object_signals[SIGNAL_INSERT_CNF] =
    g_signal_new(SIB_OBJECT_SIGNAL_INSERT_CNF,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, insertcnf_cb),
		 NULL,
		 NULL,
		 marshal_VOID__STRING_STRING_INT_INT_STRING,
		 G_TYPE_NONE,
		 5,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_NONE);

  sib_object_signals[SIGNAL_REMOVE_CNF] =
    g_signal_new(SIB_OBJECT_SIGNAL_REMOVE_CNF,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, removecnf_cb),
		 NULL,
		 NULL,
		 marshal_VOID__STRING_STRING_INT_INT,
		 G_TYPE_NONE,
		 4,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_NONE);

  sib_object_signals[SIGNAL_UPDATE_CNF] =
    g_signal_new(SIB_OBJECT_SIGNAL_UPDATE_CNF,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, updatecnf_cb),
		 NULL,
		 NULL,
		 marshal_VOID__STRING_STRING_INT_INT_STRING,
		 G_TYPE_NONE,
		 5,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_NONE);

  sib_object_signals[SIGNAL_QUERY_CNF] =
    g_signal_new(SIB_OBJECT_SIGNAL_QUERY_CNF,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, querycnf_cb),
		 NULL,
		 NULL,
		 marshal_VOID__STRING_STRING_INT_INT_STRING,
		 G_TYPE_NONE,
		 5,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_NONE);

  sib_object_signals[SIGNAL_SUBSCRIBE_CNF] =
    g_signal_new(SIB_OBJECT_SIGNAL_SUBSCRIBE_CNF,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, subscribecnf_cb),
		 NULL,
		 NULL,
		 marshal_VOID__STRING_STRING_INT_INT_STRING_STRING,
		 G_TYPE_NONE,
		 6,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_NONE);
    
  sib_object_signals[SIGNAL_SUBSCRIPTION_IND] =
    g_signal_new(SIB_OBJECT_SIGNAL_SUBSCRIPTION_IND,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, subscriptionind_cb),
		 NULL,
		 NULL,
		 marshal_VOID__STRING_STRING_INT_INT_STRING_STRING_STRING,
		 G_TYPE_NONE,
		 7,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_NONE);

  sib_object_signals[SIGNAL_UNSUBSCRIBE_IND] =
    g_signal_new(SIB_OBJECT_SIGNAL_UNSUBSCRIBE_IND,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, unsubscribeind_cb),
		 NULL,
		 NULL,
		 marshal_VOID__STRING_STRING_INT_INT_STRING,
		 G_TYPE_NONE,
		 5,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_NONE);

  sib_object_signals[SIGNAL_UNSUBSCRIBE_CNF] =
    g_signal_new(SIB_OBJECT_SIGNAL_UNSUBSCRIBE_CNF,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, unsubscribecnf_cb),
		 NULL,
		 NULL,
		 marshal_VOID__STRING_STRING_INT_INT_STRING,
		 G_TYPE_NONE,
		 5,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING,
		 G_TYPE_NONE);
  
  sib_object_signals[SIGNAL_LEAVE_IND] =
    g_signal_new(SIB_OBJECT_SIGNAL_LEAVE_IND,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(SibObjectClass, leaveind_cb),
		 NULL,
		 NULL,
		 marshal_VOID__STRING_STRING_INT_INT,
		 G_TYPE_NONE,
		 4,
		 G_TYPE_STRING,
		 G_TYPE_STRING,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_NONE);
    
}

static void sib_object_init(GTypeInstance* instance, gpointer g_class)
{
  SibObject* self = SIB_OBJECT(instance);

  g_return_if_fail(self != NULL);
  g_return_if_fail(IS_SIB_OBJECT(instance));
}

/*****************************************************************************
 * Public functions
 *****************************************************************************/

GType sib_object_get_type()
{
  static GType type = 0;

  whiteboard_log_debug_fb();

  if (!type)
    {
      static const GTypeInfo info =
	{
	  sizeof(SibObjectClass),
	  NULL,           /* base_init */
	  NULL,           /* base_finalize */
	  (GClassInitFunc) sib_object_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof(SibObject),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) sib_object_init
	};

      type = g_type_register_static(G_TYPE_OBJECT,
				    "SibObjectType", &info, 0);
    }

  whiteboard_log_debug_fe();

  return type;
}

GObject* sib_object_new(gchar* serviceid, gchar* controlid,
			GMainContext* main_context,
			gchar* friendly_name, gchar* description)
{
  GObject* object = NULL;
  SibObject* self = NULL;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(serviceid != NULL || controlid != NULL, NULL);

  object = g_object_new(SIB_OBJECT_TYPE, NULL);
  if (object == NULL)
    {
      whiteboard_log_error("Out of memory!\n");
      return NULL;
    }

  /* TODO: Maybe the stuff below should be done in sib_object_init */

  self = SIB_OBJECT(object);
  
  self->uuid = g_strdup(serviceid);
  self->control_uuid = g_strdup(controlid);
  self->friendly_name = g_strdup(friendly_name);
  self->description = g_strdup(description);
  self->mimetypes = g_strdup("N/A"); // To fulfill WhiteboardLibHeader
  self->local = FALSE;
	
  self->send_lock = g_mutex_new();
  self->recv_lock = g_mutex_new();
  /* Set main context */
  if (main_context != NULL)
    self->main_context = main_context;
  else
    self->main_context = g_main_context_default();

  /* Register normal source service with Sib only if serviceid was
     given. This can be done any number of times for a process, but
     serviceid should be different each time. */
  if (serviceid != NULL)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
		     "Attempting to register the SibObject with Sib.\n");
      while (sib_object_register(self, serviceid) == FALSE)
	{
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
			 "SibObject registration failed. Sleeping.\n");
	  g_usleep(WHITEBOARD_REGISTRATION_TIMEOUT);
	}
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
		     "SibObject registered successfully with Sib.\n");
    }

  /* Register control channel with Sib if controlid was given. This
     should be done only once for each process. */
  if (controlid != NULL)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
		     "Attempting to register control channel with Sib.\n");
      while (sib_object_register_control(self, controlid) == FALSE)
	{
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			 "SibObject control registration failed. Sleeping.\n");
	  g_usleep(WHITEBOARD_REGISTRATION_TIMEOUT);
	}
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
		     "SibObject registered control channel successfully with Sib.\n");

      /* Only one pid file is wanted for the process, and only one 
       * control channel should be created for the process. */
      whiteboard_util_create_pid_file(controlid);
    }
	
  //whiteboard_log_add_dbus_connection(self->connection);
  whiteboard_log_debug("Message routing from source %s enabled\n", 
		self->friendly_name);

  whiteboard_log_debug_fe();

  return object;
}

static void sib_object_finalize(GObject* object)
{
  SibObject *self = SIB_OBJECT(object);

  whiteboard_log_debug_fb();

  g_return_if_fail(self != NULL);

  if ( NULL != self->uuid )
    {
      whiteboard_util_send_signal(SIB_DBUS_OBJECT,
			   SIB_DBUS_REGISTER_INTERFACE, 
			   SIB_DBUS_REGISTER_SIGNAL_UNREGISTER_KP,
			   self->connection,
			   DBUS_TYPE_STRING,
			   &self->uuid,
			   DBUS_TYPE_STRING,
			   &self->friendly_name,
			   WHITEBOARD_UTIL_LIST_END);
    }
  
  if ( NULL != self->control_uuid )
    {
      whiteboard_util_send_signal(SIB_DBUS_OBJECT,
			   SIB_DBUS_REGISTER_INTERFACE, 
			   SIB_DBUS_REGISTER_SIGNAL_UNREGISTER_KP,
			   self->connection,
			   DBUS_TYPE_STRING,
			   &self->control_uuid,
			   DBUS_TYPE_STRING,
			   &self->friendly_name,
			   WHITEBOARD_UTIL_LIST_END);
      
      whiteboard_util_remove_pid_file(self->control_uuid);
    }
  
  dbus_connection_flush(self->connection);
  dbus_connection_unref(self->connection);

  g_free(self->uuid);
  self->uuid = NULL;

  g_free(self->control_uuid);
  self->control_uuid = NULL;

  g_free(self->friendly_name);
  self->friendly_name = NULL;

  g_free(self->description);
  self->description = NULL;

  g_free(self->mimetypes);
  self->mimetypes=NULL;
  
  g_mutex_free(self->send_lock);
  g_mutex_free(self->recv_lock);
  whiteboard_log_debug_fe();
}

GMainContext *sib_object_get_main_context(SibObject *self)
{
  g_return_val_if_fail(self != NULL, NULL);
  return self->main_context;
}

const gchar *sib_object_get_uuid(SibObject *self)
{
  g_return_val_if_fail(self != NULL, NULL);
  return self->uuid;
}

/******************************************************************************
 * SibObjectHandle functions

 ******************************************************************************/

static SibObjectHandle* sib_object_handle_new(SibObject* context,
					      DBusMessage* message)
{
  SibObjectHandle* self = NULL;

  whiteboard_log_debug_fb();

  self = g_new0(SibObjectHandle, 1);
  g_return_val_if_fail(self != NULL, NULL);
  self->context = context;
  self->message = message;
  self->refcount = 1;

  if (self->context != NULL)
    g_object_ref(G_OBJECT(self->context));
	
  if (self->message != NULL)
    dbus_message_ref(message);

  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
		 "Created SibObjectHandle with context %p and msg %p\n",
		 context, message);

  whiteboard_log_debug_fe();

  return self;
}

void sib_object_handle_ref(SibObjectHandle* self)
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

static void sib_object_handle_destroy(SibObjectHandle* self)
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

void sib_object_handle_unref(SibObjectHandle* self)
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
      sib_object_handle_destroy(self);
    }

  whiteboard_log_debug_fe();
}

SibObject *sib_object_handle_get_context(SibObjectHandle *self)
{
  g_return_val_if_fail(self != NULL, NULL);
  return self->context;
}

static DBusHandlerResult sib_object_control_handler_healthcheck(DBusConnection *conn,
								DBusMessage *msg,
								gpointer data)
{
  SibObject *context = (SibObject *) data;
  gboolean alive = TRUE;
  whiteboard_log_debug_fb();

  /* TODO: Get return value from signal emission */
  g_signal_emit(context, sib_object_signals[SIGNAL_HEALTHCHECK], 0);
        
  whiteboard_util_send_method_return(conn, msg,
			      DBUS_TYPE_BOOLEAN, &alive,
			      WHITEBOARD_UTIL_LIST_END);
        


  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_HANDLED;

}

static DBusHandlerResult sib_object_control_handler_refresh(DBusConnection *conn,
							    DBusMessage *msg,
							    gpointer data)
{
  SibObject *context = (SibObject *) data;

  whiteboard_log_debug_fb();

  g_signal_emit(context, sib_object_signals[SIGNAL_REFRESH], 0);

  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_HANDLED;

}

static DBusHandlerResult sib_object_control_handler_shutdown(DBusConnection *conn,
							     DBusMessage *msg,
							     gpointer data)
{
  SibObject *context = (SibObject *) data;

  whiteboard_log_debug_fb();

  g_signal_emit(context, sib_object_signals[SIGNAL_SHUTDOWN], 0);

  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_HANDLED;
}


static DBusHandlerResult sib_object_control_handler_register_sib(DBusConnection *conn,
								 DBusMessage *msg,
								 gpointer data)
{
  SibObject *context = (SibObject *) data;
  SibObjectHandle *handle = NULL;
  gchar *uri = NULL;
  whiteboard_log_debug_fb();

  handle = sib_object_handle_new(context, msg);
  if(whiteboard_util_parse_message(msg,
			    DBUS_TYPE_STRING, &uri,
			    WHITEBOARD_UTIL_LIST_END))
    {
      
      g_signal_emit(context,
		    sib_object_signals[SIGNAL_REGISTER_SIB],
		    0,
		    handle,
		    uri);
    }
  else
    {
      sib_object_send_register_sib_return(handle, -1);
    }
  sib_object_handle_unref(handle);
  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult sib_object_handler(DBusConnection* conn,
					    DBusMessage* message,
					    gpointer data)
{
  
  SibObject *context = (SibObject *) data;
  DBusHandlerResult retval = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const gchar* member = NULL;
  gint type = 0;

  whiteboard_log_debug_fb();

  member = dbus_message_get_member(message);
  type = dbus_message_get_type(message);

  g_return_val_if_fail(member != NULL,
		       DBUS_HANDLER_RESULT_NOT_YET_HANDLED);

  whiteboard_log_debug("%s %d\n", member, type);
  switch (type)
    {
    case DBUS_MESSAGE_TYPE_SIGNAL:
      {

	if( !strcmp(member, SIB_DBUS_KP_SIGNAL_SUBSCRIPTION_IND) )
	  {
	    gchar *spaceid = NULL;
	    gchar *nodeid = NULL;
	    gchar *results_new = NULL;
	    gchar *results_obsolete = NULL;
	    gchar *subscription_id = NULL;
	    gint msgnum = -1;
	    gint seqnum = -10;

	    whiteboard_log_debug("Subsciprion ind signal\n");
	    if (whiteboard_util_parse_message(message,
					      DBUS_TYPE_STRING, &spaceid,
					      DBUS_TYPE_STRING, &nodeid,
					      DBUS_TYPE_INT32, &msgnum,
					      DBUS_TYPE_INT32, &seqnum,
					      DBUS_TYPE_STRING, &subscription_id,
					      DBUS_TYPE_STRING, &results_new,
					      DBUS_TYPE_STRING, &results_obsolete,
					      WHITEBOARD_UTIL_LIST_END))
	      {
		g_signal_emit( context,
			       sib_object_signals[SIGNAL_SUBSCRIPTION_IND],
			       0,
			       spaceid,
			       nodeid,
			       msgnum,
			       seqnum,
			       subscription_id,
			       results_new,
			       results_obsolete);
	      }
	    else
	      {
		whiteboard_log_warning("Could not parse SUBSCRIPTION IND signal parameters\n");
	      }
	    retval = DBUS_HANDLER_RESULT_HANDLED;
	  }
	else if( !strcmp(member, SIB_DBUS_KP_SIGNAL_UNSUBSCRIBE_IND) )
	  {
	    gchar *spaceid = NULL;
	    gchar *nodeid = NULL;
	    gchar *subscription_id = NULL;
	    gint status;
	    gint msgnum = -1;
	    
	    whiteboard_log_debug("Unsubscibe ind signal\n");
	    if (whiteboard_util_parse_message(message,
				       DBUS_TYPE_STRING, &spaceid,
				       DBUS_TYPE_STRING, &nodeid,
				       DBUS_TYPE_INT32, &msgnum,
				       DBUS_TYPE_INT32, &status,
				       DBUS_TYPE_STRING, &subscription_id,
				       WHITEBOARD_UTIL_LIST_END))
	      {
		g_signal_emit( context,
			       sib_object_signals[SIGNAL_UNSUBSCRIBE_IND],
			       0,
			       spaceid,
			       nodeid,
			       msgnum,
			       status,
			       subscription_id);
	      }
	    else
	      {
		whiteboard_log_warning("Could not parse UNSUBSCRIBE IND signal parameters\n");
	      }
	    retval = DBUS_HANDLER_RESULT_HANDLED;
	  }
	else if( !strcmp(member, SIB_DBUS_KP_SIGNAL_LEAVE_IND) )
	  {
	    gchar *spaceid = NULL;
	    gchar *nodeid = NULL;
	    gint status;
	    gint msgnum = -1;

	    whiteboard_log_debug("Leave ind signal\n");
	    if (whiteboard_util_parse_message(message,
				       DBUS_TYPE_STRING, &spaceid,
				       DBUS_TYPE_STRING, &nodeid,
				       DBUS_TYPE_INT32, &msgnum,
				       DBUS_TYPE_INT32, &status,
				       WHITEBOARD_UTIL_LIST_END))
	      {
		g_signal_emit( context,
			       sib_object_signals[SIGNAL_LEAVE_IND],
			       0,
			       spaceid,
			       nodeid,
			       msgnum,
			       status);
	      }
	    else
	      {
		whiteboard_log_warning("Could not parse UNSUBSCRIBE IND signal parameters\n");
	      }
	    retval = DBUS_HANDLER_RESULT_HANDLED;
	  }
	else
	  {
	    whiteboard_log_error("Unknown signal: %s\n",
			    member);
	  }
      }
      break;

    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
      if( !strcmp(member, SIB_DBUS_KP_METHOD_JOIN ) )
	{
	  gchar *spaceid = NULL;
	  gchar *nodeid = NULL;
	  gchar *credentials = NULL;
	  gint msgnum = -1;
	  gint success = -1;
	  whiteboard_log_debug("Join method return\n");
	  if (whiteboard_util_parse_message(message,
				     DBUS_TYPE_STRING, &spaceid,
				     DBUS_TYPE_STRING, &nodeid,
				     DBUS_TYPE_INT32, &msgnum,
				     DBUS_TYPE_INT32, &success,
				     DBUS_TYPE_STRING, &credentials,
				     WHITEBOARD_UTIL_LIST_END))
	    {
	      g_signal_emit( context,
			     sib_object_signals[SIGNAL_JOIN_CNF],
			     0,
			     spaceid,
			     nodeid,
			     msgnum,
			     success,
			     credentials);
	    }
	  else
	    {
	      whiteboard_log_warning("Could not parse JOIN method return parameters\n");
	    }
	  retval = DBUS_HANDLER_RESULT_HANDLED;
	}
      else if( !strcmp(member, SIB_DBUS_KP_METHOD_LEAVE ) )
	{
	  gchar *spaceid = NULL;
	  gchar *nodeid = NULL;
	  gint msgnum = -1;
	  gint success = -1;
	  whiteboard_log_debug("Leave method return\n");
	  if (whiteboard_util_parse_message(message,
				     DBUS_TYPE_STRING, &spaceid,
				     DBUS_TYPE_STRING, &nodeid,
				     DBUS_TYPE_INT32, &msgnum,
				     DBUS_TYPE_INT32, &success,
				     WHITEBOARD_UTIL_LIST_END))
	    {
	      g_signal_emit( context,
			     sib_object_signals[SIGNAL_LEAVE_CNF],
			     0,
			     spaceid,
			     nodeid,
			     msgnum,
			     success);
	    }
	  else
	    {
	      whiteboard_log_warning("Could not parse LEAVE method return parameters\n");
	    }
	  retval = DBUS_HANDLER_RESULT_HANDLED;
	}
      else if( !strcmp(member, SIB_DBUS_KP_METHOD_INSERT ) )
	{
	  gchar *spaceid = NULL;
	  gchar *nodeid = NULL;
	  gchar *bNodes = NULL;
	  gint msgnum = -1;
	  gint success = -1;
	  whiteboard_log_debug("Insert method return\n");
	  if (whiteboard_util_parse_message(message,
				     DBUS_TYPE_STRING, &spaceid,
				     DBUS_TYPE_STRING, &nodeid,
				     DBUS_TYPE_INT32, &msgnum,
				     DBUS_TYPE_INT32, &success,
				     DBUS_TYPE_STRING, &bNodes,
				     WHITEBOARD_UTIL_LIST_END))
	    {
	      g_signal_emit( context,
			     sib_object_signals[SIGNAL_INSERT_CNF],
			     0,
			     spaceid,
			     nodeid,
			     msgnum,
			     success,
			     bNodes);
	    }
	  else
	    {
	      whiteboard_log_warning("Could not parse INSERT method return parameters\n");
	    }
	  retval = DBUS_HANDLER_RESULT_HANDLED;
	}
      else if( !strcmp(member, SIB_DBUS_KP_METHOD_REMOVE ) )
	{
	  gchar *spaceid = NULL;
	  gchar *nodeid = NULL;
	  gint msgnum = -1;
	  gint success = -1;
	  whiteboard_log_debug("Remove method return\n");
	  if (whiteboard_util_parse_message(message,
				     DBUS_TYPE_STRING, &spaceid,
				     DBUS_TYPE_STRING, &nodeid,
				     DBUS_TYPE_INT32, &msgnum,
				     DBUS_TYPE_INT32, &success,
				     WHITEBOARD_UTIL_LIST_END))
	    {
	      g_signal_emit( context,
			     sib_object_signals[SIGNAL_REMOVE_CNF],
			     0,
			     spaceid,
			     nodeid,
			     msgnum,
			     success);
	    }
	  else
	    {
	      whiteboard_log_error("Could not parse REMOVE method return parameters\n");
	    }
	  retval = DBUS_HANDLER_RESULT_HANDLED;
	}
      else if( !strcmp(member, SIB_DBUS_KP_METHOD_UPDATE ) )
	{
	  gchar *spaceid = NULL;
	  gchar *nodeid = NULL;
	  gchar *bNodes = NULL;
	  gint msgnum = -1;
	  gint success = -1;
	  whiteboard_log_debug("Update method return\n");
	  if (whiteboard_util_parse_message(message,
				     DBUS_TYPE_STRING, &spaceid,
				     DBUS_TYPE_STRING, &nodeid,
				     DBUS_TYPE_INT32, &msgnum,
				     DBUS_TYPE_INT32, &success,
				     DBUS_TYPE_STRING, &bNodes,
				     WHITEBOARD_UTIL_LIST_END))
	    {
	      g_signal_emit( context,
			     sib_object_signals[SIGNAL_UPDATE_CNF],
			     0,
			     spaceid,
			     nodeid,
			     msgnum,
			     success,
			     bNodes);
	    }
	  else
	    {
	      whiteboard_log_warning("Could not parse UPDATE method return parameters\n");
	    }
	  retval = DBUS_HANDLER_RESULT_HANDLED;
	}
      else if( !strcmp(member, SIB_DBUS_KP_METHOD_QUERY ) )
	{
	  gchar *spaceid = NULL;
	  gchar *nodeid = NULL;
	  gchar *results = NULL;
	  gint msgnum = -1;
	  gint success = -1;
	  whiteboard_log_debug("Query method return\n");
	  if (whiteboard_util_parse_message(message,
				     DBUS_TYPE_STRING, &spaceid,
				     DBUS_TYPE_STRING, &nodeid,
				     DBUS_TYPE_INT32, &msgnum,
				     DBUS_TYPE_INT32, &success,
				     DBUS_TYPE_STRING, &results,
				     WHITEBOARD_UTIL_LIST_END))
	    {
	      g_signal_emit( context,
			     sib_object_signals[SIGNAL_QUERY_CNF],
			     0,
			     spaceid,
			     nodeid,
			     msgnum,
			     success,
			     results);
	    }
	  else
	    {
	      whiteboard_log_warning("Could not parse QUERY method return parameters\n");
	    }
	  retval = DBUS_HANDLER_RESULT_HANDLED;
	}
      else if( !strcmp(member, SIB_DBUS_KP_METHOD_SUBSCRIBE ) )
	{
	  gchar *spaceid = NULL;
	  gchar *nodeid = NULL;
	  gchar *results = NULL;
	  gchar *subscription_id = NULL;
	  gint msgnum = -1;
	  gint success = -1;
	  whiteboard_log_debug("Subscribe method return\n");
	  if (whiteboard_util_parse_message(message,
				     DBUS_TYPE_STRING, &spaceid,
				     DBUS_TYPE_STRING, &nodeid,
				     DBUS_TYPE_INT32, &msgnum,
				     DBUS_TYPE_INT32, &success,
				     DBUS_TYPE_STRING, &subscription_id,
				     DBUS_TYPE_STRING, &results,
				     WHITEBOARD_UTIL_LIST_END))
	    {
	      g_signal_emit( context,
			     sib_object_signals[SIGNAL_SUBSCRIBE_CNF],
			     0,
			     spaceid,
			     nodeid,
			     msgnum,
			     success,
			     subscription_id,
			     results);
	    }
	  else
	    {
	      whiteboard_log_warning("Could not parse SUBSCRIBE method return parameters\n");
	    }
	  retval = DBUS_HANDLER_RESULT_HANDLED;
	}
      else if( !strcmp(member, SIB_DBUS_KP_METHOD_UNSUBSCRIBE ) )
	{
	  gchar *spaceid = NULL;
	  gchar *nodeid = NULL;
	  gchar *subid = NULL;
	  gint msgnum = -1;
	  gint success = -1;
	  whiteboard_log_debug("Unsubscribe method return\n");
	  if (whiteboard_util_parse_message(message,
					    DBUS_TYPE_STRING, &spaceid,
					    DBUS_TYPE_STRING, &nodeid,
					    DBUS_TYPE_INT32, &msgnum,
					    DBUS_TYPE_INT32, &success,
					    DBUS_TYPE_STRING, &subid,
					    WHITEBOARD_UTIL_LIST_END))
	    {
	      g_signal_emit( context,
			     sib_object_signals[SIGNAL_UNSUBSCRIBE_CNF],
			     0,
			     spaceid,
			     nodeid,
			     msgnum,
			     success,
			     subid);
	    }
	  else
	    {
	      whiteboard_log_warning("Could not parse SUBSCRIBE method return parameters\n");
	    }
	  retval = DBUS_HANDLER_RESULT_HANDLED;
	}
      else
	{
	  whiteboard_log_error("Unknown method return: %s\n", member);
	}
      break;

    case DBUS_MESSAGE_TYPE_METHOD_CALL:


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

static DBusHandlerResult sib_object_control_handler(DBusConnection* conn,
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

      if (!strcmp(member, SIB_DBUS_CONTROL_SIGNAL_STARTING))
	{
	  /* If we get this signal it means that Sib has 
	     been shut down (abnormally) and is restarting. To
	     avoid duplicate processes we'll shut down and let
	     Sib restart us right away. */
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB,
			 "Sib restart detected, " \
			 "shutting down.\n");
	  retval = sib_object_control_handler_shutdown(conn,
						       message,
						       data);
	}
      else
	{
	  whiteboard_log_error("Unknown signal: %s\n", member);
	}
		
      break;

    case DBUS_MESSAGE_TYPE_METHOD_RETURN:

      whiteboard_log_error("Unknown method return: %s\n",
		      member);
      
      retval = DBUS_HANDLER_RESULT_HANDLED;
      
      break;

    case DBUS_MESSAGE_TYPE_METHOD_CALL:

      if (!strcmp(member, SIB_DBUS_CONTROL_METHOD_REFRESH))
	{
	  retval = sib_object_control_handler_refresh(conn,
						      message,
						      data);
	}
      else if (!strcmp(member, SIB_DBUS_CONTROL_METHOD_SHUTDOWN))
	{
	  retval = sib_object_control_handler_shutdown(conn,
						       message,
						       data);
	}
      else if (!strcmp(member, SIB_DBUS_CONTROL_METHOD_HEALTHCHECK))
	{
	  retval = sib_object_control_handler_healthcheck(conn,
							  message,
							  data);
	}
      else if (!strcmp(member, SIB_DBUS_CONTROL_METHOD_REGISTER_SIB))
	{
	  retval = sib_object_control_handler_register_sib(conn,
							   message,
							   data);
	}
      else
	{
	  whiteboard_log_error("Unknown method call: %s\n",
			  member);
	}

      break;
    default:
      whiteboard_log_error("Unknown message type: %d\n", type);
      break;
    }

  whiteboard_log_debug_fe();

  return retval;
}

static DBusHandlerResult sib_object_dispatch_message(DBusConnection *conn,
						     DBusMessage *message,
						     gpointer data)
{
  DBusHandlerResult retval = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const gchar* interface = NULL;
  SibObject *self = (SibObject *)data;
  whiteboard_log_debug_fb();

  interface = dbus_message_get_interface(message);
  g_return_val_if_fail(self != NULL,
		       DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
  g_return_val_if_fail(interface != NULL,
		       DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
  sib_object_lock_recv(self);
  if (!strcmp(interface, SIB_DBUS_KP_INTERFACE))
    {
      retval = sib_object_handler(conn, message, data);
    }
  else if (!strcmp(interface, SIB_DBUS_CONTROL_INTERFACE))
    {
      retval = sib_object_control_handler(conn, message, data);
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
  sib_object_unlock_recv(self);
  return retval;
}

/*****************************************************************************
 * Connection registration
 *****************************************************************************/

/**
 * Handle the unregistration of a DBus connection
 *
 */
static void sib_object_unregister_handler(DBusConnection *connection,
					  gpointer data)
{
  whiteboard_log_debug_fb();
  whiteboard_log_debugc(WHITEBOARD_DEBUG_SIB, "Unregistered\n");
  whiteboard_log_warning("sib_object_unregister_handler\n");
  whiteboard_log_debug_fe();
}

static gboolean sib_object_register(SibObject *self, gchar *uuid)
{
  whiteboard_log_debug_fb();

  /* Register with Sib */
  /* TODO: should not try forever... */

  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
		 "Attempting to register SibObject with Sib Daemon.\n");
  sib_object_lock_send(self);
  while (whiteboard_util_register_sib_try(self, uuid,
			       SIB_DBUS_REGISTER_METHOD_KP,
			       sib_object_unregister_handler,
			       sib_object_dispatch_message) 
	 == FALSE)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
		     "Registration failed. Sleeping\n");
      g_usleep(WHITEBOARD_REGISTRATION_TIMEOUT);
    }
  sib_object_unlock_send(self);
  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
		 "SibObject registered successfully with Sib Daemon.\n");

  whiteboard_log_debug_fe();

  return TRUE;
}

static gboolean sib_object_register_control(SibObject *self,
					    gchar *uuid)
{
  DBusError err;
  DBusObjectPathVTable vtable = { sib_object_unregister_handler,
				  sib_object_dispatch_message,
				  NULL, NULL, NULL, NULL
  };

  whiteboard_log_debug_fb();

  /* Register control channel with Sib */
  /* TODO: should not try forever... */

  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
		 "Attempting to register control channel with Sib Daemon.\n");
  sib_object_lock_send(self);
  while (whiteboard_util_register_sib_try(self, uuid,
			       SIB_DBUS_REGISTER_METHOD_CONTROL,
			       sib_object_unregister_handler,
			       sib_object_dispatch_message) 
	 == FALSE)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
		     "Registration failed. Sleeping.\n");
      g_usleep(WHITEBOARD_REGISTRATION_TIMEOUT);
    }
  sib_object_unlock_send(self);
  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
		 "SibObject registered successfully with Sib Daemon.\n");

  /* We'll listen Sib common alive broadcasts from session bus
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
					SIB_DBUS_OBJECT, &vtable,
					self);

      dbus_connection_setup_with_g_main(self->sessionbus_connection,
					self->main_context);
    }

  dbus_error_free(&err);

  dbus_bus_add_match(self->sessionbus_connection, 
		     "type='signal', interface='" \
		     SIB_DBUS_CONTROL_INTERFACE "'", &err);
	
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

void sib_object_send_register_sib_return( SibObjectHandle *handle,
					  gint ret)
{
  whiteboard_log_debug_fb();
  DBusConnection * conn = handle->context->connection;
  DBusMessage *msg = handle->message;
  sib_object_lock_send(handle->context);
  whiteboard_util_send_method_return(conn, msg,
			      DBUS_TYPE_INT32, &ret,
			      WHITEBOARD_UTIL_LIST_END);
  sib_object_unlock_send(handle->context);
  whiteboard_log_debug_fe();
}

gint sib_object_send_join( SibObject *self,
			   guchar *spaceid,
			   guchar *nodeid,
			   gint msgnum,
			   guchar *credentials)
{
  gint err = -1;
  whiteboard_log_debug_fb();
  sib_object_lock_send(self);
  err = whiteboard_util_send_method(SIB_DBUS_SERVICE,
			     SIB_DBUS_OBJECT,
			     SIB_DBUS_KP_INTERFACE,
			     SIB_DBUS_KP_METHOD_JOIN,
			     self->connection,
			     DBUS_TYPE_STRING, &spaceid,
			     DBUS_TYPE_STRING, &nodeid,
			     DBUS_TYPE_INT32, &msgnum,
			     DBUS_TYPE_STRING, &credentials,
			     WHITEBOARD_UTIL_LIST_END);
  sib_object_unlock_send(self);
  whiteboard_log_debug_fe();
  return err;
}

gint sib_object_send_leave( SibObject *self,
			    guchar *spaceid,
			    guchar *nodeid,
			    gint msgnum)
  
{
  gint err = -1;
  whiteboard_log_debug_fb();
  sib_object_lock_send(self);
  err = whiteboard_util_send_method(SIB_DBUS_SERVICE,
			     SIB_DBUS_OBJECT,
			     SIB_DBUS_KP_INTERFACE,
			     SIB_DBUS_KP_METHOD_LEAVE,
			     self->connection,
			     DBUS_TYPE_STRING, &spaceid,
			     DBUS_TYPE_STRING, &nodeid,
			     DBUS_TYPE_INT32, &msgnum,
			     WHITEBOARD_UTIL_LIST_END);
   sib_object_unlock_send(self);
  whiteboard_log_debug_fe();
  return err;
}

gint sib_object_send_insert( SibObject *self,
			     guchar *spaceid,
			     guchar *nodeid,
			     gint msgnum,
			     EncodingType encoding,
			     guchar *insert_graph)
{
  gint err = -1;
  whiteboard_log_debug_fb();
  sib_object_lock_send(self);
  err = whiteboard_util_send_method(SIB_DBUS_SERVICE,
				    SIB_DBUS_OBJECT,
				    SIB_DBUS_KP_INTERFACE,
				    SIB_DBUS_KP_METHOD_INSERT,
				    self->connection,
				    DBUS_TYPE_STRING, &spaceid,
				    DBUS_TYPE_STRING, &nodeid,
				    DBUS_TYPE_INT32, &msgnum,
				    DBUS_TYPE_INT32, &encoding,
				    DBUS_TYPE_STRING, &insert_graph,
				    WHITEBOARD_UTIL_LIST_END);
  sib_object_unlock_send(self);
  whiteboard_log_debug_fe();
  return err;
}

gint sib_object_send_remove( SibObject *self,
			     guchar *spaceid,
			     guchar *nodeid,
			     gint msgnum,
			     EncodingType encoding,
			     guchar *remove_graph)
{
  gint err = -1;
  whiteboard_log_debug_fb();
  sib_object_lock_send(self);
  err = whiteboard_util_send_method(SIB_DBUS_SERVICE,
				    SIB_DBUS_OBJECT,
				    SIB_DBUS_KP_INTERFACE,
				    SIB_DBUS_KP_METHOD_REMOVE,
				    self->connection,
				    DBUS_TYPE_STRING, &spaceid,
				    DBUS_TYPE_STRING, &nodeid,
				    DBUS_TYPE_INT32, &msgnum,
				    DBUS_TYPE_INT32, &encoding,
				    DBUS_TYPE_STRING, &remove_graph,
				    WHITEBOARD_UTIL_LIST_END);
  sib_object_unlock_send(self);
  whiteboard_log_debug_fe();
  return err;
}

gint sib_object_send_update( SibObject *self,
			     guchar *spaceid,
			     guchar *nodeid,
			     gint msgnum,
			     EncodingType encoding,
			     guchar *insert_graph,
			     guchar *remove_graph)
{
  gint err = -1;
  whiteboard_log_debug_fb();
  sib_object_lock_send(self);
  err = whiteboard_util_send_method(SIB_DBUS_SERVICE,
				    SIB_DBUS_OBJECT,
				    SIB_DBUS_KP_INTERFACE,
				    SIB_DBUS_KP_METHOD_UPDATE,
				    self->connection,
				    DBUS_TYPE_STRING, &spaceid,
				    DBUS_TYPE_STRING, &nodeid,
				    DBUS_TYPE_INT32, &msgnum,
				    DBUS_TYPE_INT32, &encoding,
				    DBUS_TYPE_STRING, &insert_graph,
				    DBUS_TYPE_STRING, &remove_graph,
				    WHITEBOARD_UTIL_LIST_END);
  sib_object_unlock_send(self);
  whiteboard_log_debug_fe();
  return err;
}

gint sib_object_send_query( SibObject *self,
			    guchar *spaceid,
			    guchar *nodeid,
			    gint msgnum,
			    QueryType type,
			    guchar *query)
{
  gint err = -1;
  whiteboard_log_debug_fb();
  sib_object_lock_send(self);
  err = whiteboard_util_send_method(SIB_DBUS_SERVICE,
			     SIB_DBUS_OBJECT,
			     SIB_DBUS_KP_INTERFACE,
			     SIB_DBUS_KP_METHOD_QUERY,
			     self->connection,
			     DBUS_TYPE_STRING, &spaceid,
			     DBUS_TYPE_STRING, &nodeid,
			     DBUS_TYPE_INT32, &msgnum,
			     DBUS_TYPE_INT32, &type,
			     DBUS_TYPE_STRING, &query,
			     WHITEBOARD_UTIL_LIST_END);
  sib_object_unlock_send(self);
  whiteboard_log_debug_fe();
  return err;
}

gint sib_object_send_subscribe( SibObject *self,
				guchar *spaceid,
				guchar *nodeid,
				gint msgnum,
				QueryType type,
				guchar *query)
{
  gint err = -1;
  whiteboard_log_debug_fb();
  sib_object_lock_send(self);
  err = whiteboard_util_send_method(SIB_DBUS_SERVICE,
			     SIB_DBUS_OBJECT,
			     SIB_DBUS_KP_INTERFACE,
			     SIB_DBUS_KP_METHOD_SUBSCRIBE,
			     self->connection,
			     DBUS_TYPE_STRING, &spaceid,
			     DBUS_TYPE_STRING, &nodeid,
			     DBUS_TYPE_INT32, &msgnum,
			     DBUS_TYPE_INT32, &type,
			     DBUS_TYPE_STRING, &query,
			     WHITEBOARD_UTIL_LIST_END);
  sib_object_unlock_send(self);
  whiteboard_log_debug_fe();
  return err;
}

gint sib_object_send_unsubscribe( SibObject *self,
				  guchar *spaceid,
				  guchar *nodeid,
				  gint msgnum,
				  guchar *subscription_id)
{
  gint err = -1;
  whiteboard_log_debug_fb();
  sib_object_lock_send(self);
  err = whiteboard_util_send_method(SIB_DBUS_SERVICE,
			     SIB_DBUS_OBJECT,
			     SIB_DBUS_KP_INTERFACE,
			     SIB_DBUS_KP_METHOD_UNSUBSCRIBE,
			     self->connection,
			     DBUS_TYPE_STRING, &spaceid,
			     DBUS_TYPE_STRING, &nodeid,
			     DBUS_TYPE_INT32, &msgnum,
			     DBUS_TYPE_STRING, &subscription_id,
			     WHITEBOARD_UTIL_LIST_END);
  sib_object_unlock_send(self);
  whiteboard_log_debug_fe();
  return err;
}

