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
 * WhiteBoard Library
 *
 * whiteboard_discovery.c
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
#include <uuid/uuid.h>
#include "whiteboard_util.h"
#include "whiteboard_discovery.h"
#include "whiteboard_dbus_ifaces.h"
#include "whiteboard_log.h"
#include "whiteboard_marshal.h"


/*****************************************************************************
 * Structure definitions
 *****************************************************************************/
struct _WhiteBoardDiscovery
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
  /* WhiteboardLibHeader END */
  

};

struct _WhiteBoardDiscoveryClass
{
  GObjectClass parent;

  WhiteBoardDiscoverySIBCB sib_cb;
  WhiteBoardDiscoverySIBInsertedCB sib_inserted_cb;
  WhiteBoardDiscoverySIBRemovedCB sib_removed_cb;
  WhiteBoardDiscoveryAllFowNowCB all_for_now_cb;

};

enum
  {
    SIGNAL_SIB,
    SIGNAL_SIB_INSERTED,
    SIGNAL_SIB_REMOVED,
    SIGNAL_ALL_FOR_NOW,
    NUM_SIGNALS
  };

/*****************************************************************************
 * Private function prototypes
 *****************************************************************************/

static void whiteboard_discovery_unregister_handler(DBusConnection *connection,
						    gpointer user_data);

static DBusHandlerResult whiteboard_discovery_dispatch_message(DBusConnection *conn,
							       DBusMessage *msg,
							       gpointer data);
static void whiteboard_discovery_finalize(GObject* object);

static void whiteboard_discovery_class_init(WhiteBoardDiscoveryClass *self);

static gboolean whiteboard_discovery_register(WhiteBoardDiscovery *self, gchar *uuid);

static guint whiteboard_discovery_signals[NUM_SIGNALS];

static void whiteboard_discovery_class_init(WhiteBoardDiscoveryClass *self)
{
  GObjectClass* object = G_OBJECT_CLASS(self);

  g_return_if_fail(self != NULL);

  // object->constructor = whiteboard_discovery_constructor;
  // object->dispose = whiteboard_discovery_dispose;
  object->finalize = whiteboard_discovery_finalize;


  /* NODE_ACCESS */
  	/* Source */
	whiteboard_discovery_signals[SIGNAL_SIB] =
		g_signal_new(WHITEBOARD_DISCOVERY_SIGNAL_SIB,
			     G_OBJECT_CLASS_TYPE(object),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(WhiteBoardDiscoveryClass, sib_cb),
			     NULL,
			     NULL,
			     marshal_VOID__STRING_STRING,
			     G_TYPE_NONE,
			     2,
			     G_TYPE_STRING,
			     G_TYPE_STRING);

	whiteboard_discovery_signals[SIGNAL_SIB_INSERTED] =
		g_signal_new(WHITEBOARD_DISCOVERY_SIGNAL_SIB_INSERTED,
			     G_OBJECT_CLASS_TYPE(object),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(WhiteBoardDiscoveryClass, sib_inserted_cb),
			     NULL,
			     NULL,
			     marshal_VOID__STRING_STRING,
			     G_TYPE_NONE,
			     2,
			     G_TYPE_STRING,
			     G_TYPE_STRING);

	whiteboard_discovery_signals[SIGNAL_SIB_REMOVED] =
		g_signal_new(WHITEBOARD_DISCOVERY_SIGNAL_SIB_REMOVED,
			     G_OBJECT_CLASS_TYPE(object),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(WhiteBoardDiscoveryClass, sib_removed_cb),
			     NULL,
			     NULL,
			     marshal_VOID__STRING_STRING,
			     G_TYPE_NONE,
			     2,
			     G_TYPE_STRING,
			     G_TYPE_STRING);

	whiteboard_discovery_signals[SIGNAL_ALL_FOR_NOW] =
		g_signal_new(WHITEBOARD_DISCOVERY_SIGNAL_ALL_FOR_NOW,
			     G_OBJECT_CLASS_TYPE(object),
			     G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			     G_STRUCT_OFFSET(WhiteBoardDiscoveryClass, all_for_now_cb),
			     NULL,
			     NULL,
			     marshal_VOID__VOID,
			     G_TYPE_NONE,
			     0);

}

static void whiteboard_discovery_init(GTypeInstance* instance, gpointer g_class)
{
  WhiteBoardDiscovery* self = WHITEBOARD_DISCOVERY(instance);

  g_return_if_fail(self != NULL);
  g_return_if_fail(IS_WHITEBOARD_DISCOVERY(instance));
}

/*****************************************************************************
 * Private static functions
 *****************************************************************************/
static DBusHandlerResult whiteboard_discovery_dispatch_message(DBusConnection *conn,
							       DBusMessage *msg,
							       gpointer data)
{
  
  WhiteBoardDiscovery *self = (WhiteBoardDiscovery *) data;
  const gchar* member = NULL;
  gint type = 0;

  whiteboard_log_debug_fb();
  
  member = dbus_message_get_member(msg);
  type = dbus_message_get_type(msg);
  switch (type)
    {
    case DBUS_MESSAGE_TYPE_SIGNAL:
      
      if (!strcmp(member, WHITEBOARD_DBUS_DISCOVERY_SIGNAL_SIB))
	{
	  gchar *uuid = NULL;
	  gchar *name = NULL;
			
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				"Got SIB signal.\n");
			
	  whiteboard_util_parse_message(msg,
					DBUS_TYPE_STRING, &uuid,
					DBUS_TYPE_STRING, &name,
					WHITEBOARD_UTIL_LIST_END);

	  g_signal_emit(self,
			whiteboard_discovery_signals[SIGNAL_SIB],
			0,
			uuid,
			name);
	}
      else if (!strcmp(member, WHITEBOARD_DBUS_DISCOVERY_SIGNAL_SIB_INSERTED))
	{
	  gchar *uuid = NULL;
	  gchar *name = NULL;
			
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				"Got sib inserted signal.\n");

	  whiteboard_util_parse_message(msg,
					DBUS_TYPE_STRING, &uuid,
					DBUS_TYPE_STRING, &name,
					WHITEBOARD_UTIL_LIST_END);
			
	  g_signal_emit(self,
			whiteboard_discovery_signals[SIGNAL_SIB_INSERTED],
			0,
			uuid,
			name);
	}
      else if (!strcmp(member, WHITEBOARD_DBUS_DISCOVERY_SIGNAL_SIB_REMOVED))
	{
	  gchar *uuid = NULL;
	  gchar *name = NULL;
			
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				"Got node removed signal.\n");
			
	  whiteboard_util_parse_message(msg,
					DBUS_TYPE_STRING, &uuid,
					DBUS_TYPE_STRING, &name,
					WHITEBOARD_UTIL_LIST_END);
			
	  g_signal_emit(self,
			whiteboard_discovery_signals[SIGNAL_SIB_REMOVED],
			0,
			uuid,
			name);
	}
      else if (!strcmp(member, WHITEBOARD_DBUS_DISCOVERY_SIGNAL_ALL_FOR_NOW))
	{
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				"Got All for now signal.\n");
			
	  g_signal_emit(self,
			whiteboard_discovery_signals[SIGNAL_ALL_FOR_NOW],
			0);
	}
      else
	{
	  whiteboard_log_debug("Unsupported signal: %s\n", member);
	}
      break;
       
    default:
      
      whiteboard_log_warning("Unknown message type: %d, member: %s\n", type, member);
      
      break;
    }
  whiteboard_log_debug_fe();
  return DBUS_HANDLER_RESULT_HANDLED;
}



/*****************************************************************************
 * Initialization
 *****************************************************************************/

GType whiteboard_discovery_get_type()
{
  static GType type = 0;

  whiteboard_log_debug_fb();

  if (!type)
    {
      static const GTypeInfo info =
	{
	  sizeof(WhiteBoardDiscoveryClass),
	  NULL,           /* base_init */
	  NULL,           /* base_finalize */
	  (GClassInitFunc) whiteboard_discovery_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof(WhiteBoardDiscovery),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) whiteboard_discovery_init
	};

      type = g_type_register_static(G_TYPE_OBJECT,
				    "WhiteBoardDiscoveryType", &info, 0);
    }

  whiteboard_log_debug_fe();

  return type;
}

GObject *whiteboard_discovery_new(GMainContext *main_context)
{
  GObject *object = NULL;
  WhiteBoardDiscovery *self = NULL;
  gchar tmp[37];
  uuid_t u1;
  whiteboard_log_debug_fb();
  

  //g_return_val_if_fail(username != NULL, NULL);

  object = g_object_new(WHITEBOARD_DISCOVERY_TYPE, NULL);
  if (object == NULL)
    {
      whiteboard_log_error("Out of memory!\n");
      return NULL;
    }

  uuid_generate(u1);
  uuid_unparse(u1, tmp);
  self = WHITEBOARD_DISCOVERY(object);
  self->uuid = g_strdup(tmp);
  if (main_context != NULL)
    self->main_context = main_context;
  else
    self->main_context = g_main_context_default();

  // to fill WhiteBoardLibHeader
  self->friendly_name = g_strdup("N/A");
  self->mimetypes = g_strdup("N/A");
  self->local = FALSE;
  
  while( whiteboard_discovery_register(self, tmp) == FALSE)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			    "Registration failed. Sleeping.\n");
      g_usleep(WHITEBOARD_REGISTRATION_TIMEOUT);
    }
  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
			"WhiteBoardNode registered successfully with WhiteBoard Daemon.\n");
  whiteboard_log_debug_fe();

  return object;
}

static void whiteboard_discovery_finalize(GObject* object)
{
  WhiteBoardDiscovery* self = WHITEBOARD_DISCOVERY(object);

  whiteboard_log_debug_fb();

  g_return_if_fail(self != NULL);

  self->main_context = NULL;

  if(self->uuid)
    {
      g_free(self->uuid);
      self->uuid=NULL;
    }
  if(self->mimetypes)
    {
      g_free(self->mimetypes);
      self->mimetypes=NULL;
    }
  if(self->friendly_name)
    {
      g_free(self->friendly_name);
      self->friendly_name=NULL;
    }
  
  
  dbus_connection_unref(self->connection);
  self->connection = NULL;


  whiteboard_log_debug_fe();
}

static gboolean whiteboard_discovery_register(WhiteBoardDiscovery *self, gchar *uuid )
{

  whiteboard_log_debug_fb();

  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			"Attempting to register WhiteBoardDiscovery with Whiteboard Daemon.\n");
  while (whiteboard_util_register_try(self, uuid,
				      WHITEBOARD_DBUS_REGISTER_METHOD_DISCOVERY,
				      &whiteboard_discovery_unregister_handler,
				      &whiteboard_discovery_dispatch_message) 
	 == FALSE)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			    "Registration failed. Sleeping\n");
      g_usleep(WHITEBOARD_REGISTRATION_TIMEOUT);
    }
  
  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			"WhiteBoardDiscovery registered successfully with Whiteboard Daemon.\n");
  
  whiteboard_log_debug_fe();
  
  return TRUE;
}

GMainContext *whiteboard_discovery_get_main_context(WhiteBoardDiscovery *self)
{
  g_return_val_if_fail(self != NULL, NULL);
  return self->main_context;
}

gint whiteboard_discovery_get_sibs(WhiteBoardDiscovery *self)
{
  whiteboard_log_debug_fb();
  gint retval = -1;
  if(self->connection && dbus_connection_get_is_connected(self->connection))
    {
        whiteboard_util_send_method(WHITEBOARD_DBUS_SERVICE,
				    WHITEBOARD_DBUS_OBJECT,
				    WHITEBOARD_DBUS_DISCOVERY_INTERFACE,
				    WHITEBOARD_DBUS_DISCOVERY_METHOD_GET_SIBS,
				    self->connection,
				    WHITEBOARD_UTIL_LIST_END);
	retval = 0;

    }
  else
    {
      whiteboard_log_debug("Not connected, connection:%p\n", self->connection);
    }
  whiteboard_log_debug_fe();
  return retval;
}

static void whiteboard_discovery_unregister_handler(DBusConnection *connection,
						    gpointer user_data)
{
  whiteboard_log_debug_fb();
  whiteboard_log_debug_fe();
}
