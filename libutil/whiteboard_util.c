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
 * whiteboard_util.c
 *
 * Copyright 2007 Nokia Corporation
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>

#define DBUS_API_SUBJECT_TO_CHANGE

#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "whiteboard_util.h"
#include "whiteboard_dbus_ifaces.h"
#include "sib_dbus_ifaces.h"
#include "whiteboard_log.h"

struct WhiteBoardLibHeader
{
	GObject parent;

        gchar *uuid;
	gchar *control_uuid;
	gchar *friendly_name;
	gchar *mimetypes;
	gboolean local;
        DBusConnection *connection;
	GMainContext *main_context;
};


gboolean whiteboard_util_discover(gchar** address)
{
	DBusMessage *reply = NULL;
	DBusError err;
	DBusConnection *conn = NULL;
	gchar* temp = NULL;
	gboolean retval = FALSE;

	whiteboard_log_debug_fb();

	dbus_error_init(&err);
	conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
	dbus_connection_set_exit_on_disconnect(conn, FALSE);

	if (dbus_error_is_set(&err))
	{
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
				    "Could not get session connection:-- %s", err.message);
		dbus_error_free(&err);
		whiteboard_log_debug_fe();
		return FALSE;
	}

	g_return_val_if_fail(conn != NULL, FALSE);
	
	whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					     WHITEBOARD_DBUS_OBJECT,
					     WHITEBOARD_DBUS_INTERFACE,
					     WHITEBOARD_DBUS_METHOD_DISCOVERY,
					     conn, &reply,
					     WHITEBOARD_UTIL_LIST_END);
	
	if(reply==NULL)
	  {
	    whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, "Discovery failed, maybe whiteboardd is not running, trying to start...\n");
	    gchar *service = NULL;
	    DBusMessage *new_message = NULL;
	    DBusMessage *reply2 = NULL;
	    DBusError err2;
	    gint temp2=-1;
	    service = g_strdup("com.nokia.whiteboard");
	    gint flags = 0;
	    new_message = dbus_message_new_method_call( DBUS_SERVICE_DBUS,
							DBUS_PATH_DBUS,
							DBUS_INTERFACE_DBUS,
							"StartServiceByName");
	    
	    if(NULL == new_message)
	      {
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
				      "Message creation failed\n");
		whiteboard_log_debug_fe();
		return FALSE;
	      }
	    
	    dbus_message_append_args(new_message,
				     DBUS_TYPE_STRING, &service,
				     DBUS_TYPE_UINT32, &flags,
				     DBUS_TYPE_INVALID);
	    dbus_error_init( &err2);
	    reply2 =
	      dbus_connection_send_with_reply_and_block(conn,
							new_message,
							WHITEBOARD_SEND_TIMEOUT,
							&err2);

	    if(dbus_error_is_set(&err2))
	      {
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
				      "Could not send StartServiceByName method call: %s", err2.message);
		dbus_error_free(&err2);
		whiteboard_log_debug_fe();
		g_free(service);
		return FALSE;
	      }
	    dbus_message_get_args(reply2, NULL,
				  DBUS_TYPE_INT32, &temp2,
				  DBUS_TYPE_INVALID);
	    whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
				  "StartServiceByName reply: %d\n", temp2);
	    dbus_message_unref(reply2);
	    g_free(service);
	    return FALSE;
	  }

	dbus_message_get_args(reply, NULL,
			      DBUS_TYPE_STRING, &temp,
			      DBUS_TYPE_INVALID);

	whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
			    "Discover address: %s\n", temp);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR)
	{
		whiteboard_log_error("Discovery returned with an error: %s\n",
				   temp);
		temp = NULL;
	}

	if (temp != NULL)
	{
		*address = g_strdup(temp);
		retval = TRUE;
	}
	else
	{
	  *address = NULL;
	  retval = FALSE;
	}

	dbus_message_unref(reply);

	dbus_connection_close(conn);
	dbus_connection_unref(conn);

	whiteboard_log_debug_fe();
	return retval;
}

gboolean whiteboard_util_register_try(gpointer uself, const gchar* uuid,const gchar* method,
			    DBusObjectPathUnregisterFunction unregister_handler,
			    DBusObjectPathMessageFunction dispatch_message)
{
        GError *gerror = NULL;
        DBusGConnection *connection = NULL;
	DBusMessage *reply = NULL;
	dbus_int32_t register_success = -1;
        gchar *address = NULL;
        gboolean retval = FALSE;
	struct WhiteBoardLibHeader* self = (struct WhiteBoardLibHeader*)uself;
        DBusObjectPathVTable vtable = { unregister_handler,
                                        dispatch_message,
                                        NULL, NULL, NULL, NULL
        };

        whiteboard_log_debug_fb();

	if ( NULL == self->connection )
	{
		/* Discovering private address */
		if (whiteboard_util_discover(&address) == FALSE)
		{
			address = g_strdup("unix:path=/tmp/dbus-test");
			whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
					    "Discovery failed, trying " \
					    "default address: %s\n", address);
		}

		/* Open a connection to the address */
		connection = dbus_g_connection_open(address, &gerror);
		if (gerror != NULL)
		{
			whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
					    "Unable to open connection: %s\n",
					    gerror->message);
			g_error_free(gerror);

			self->connection = NULL;

			retval = FALSE;
		}
		else
		{
			/* Get the actual connection pointer */
			self->connection = (DBusConnection*)
				dbus_g_connection_get_connection(connection);
		}
	}
	else
	{
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
				    "Connection already established, " \
				    "using old connection\n");
	}

	if ( NULL != self->connection )
	{
                whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
				    "Sending registration request to daemon\n");

		whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					    WHITEBOARD_DBUS_OBJECT,
					    WHITEBOARD_DBUS_REGISTER_INTERFACE,
					    method,
					    self->connection,
					    &reply,
					    DBUS_TYPE_STRING, &uuid,
					    DBUS_TYPE_STRING, &self->friendly_name,
					    DBUS_TYPE_STRING, &self->mimetypes,
					    DBUS_TYPE_BOOLEAN, &self->local,
					    WHITEBOARD_UTIL_LIST_END);
		
		if(reply)
		  {
		    if(!whiteboard_util_parse_message(reply, 
						      DBUS_TYPE_INT32, &register_success,
						      WHITEBOARD_UTIL_LIST_END) )
		      {
			whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
					      "Could not register (reply message parse error)\n");
		      }
		    else 
		      {
			if(register_success < 0)
			  {
			    whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
						  "Could not register (error in reply message)\n");
			  }
			else
			  {
			    
			    whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
						  "Registered\n");
			    retval = TRUE;
			  }
			dbus_message_unref(reply);
		      }
		  }
		if(retval)
		  {
		    dbus_connection_register_fallback(self->connection,
		    				      WHITEBOARD_DBUS_OBJECT,
						      &vtable,
						      (gpointer)self);
		    //		    dbus_connection_add_filter(self->connection, 
		    //		     &dispatch_message, self, NULL);
		    
		    dbus_connection_setup_with_g_main(self->connection,
						      self->main_context);

		  }
	}
	else
	{
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
				    "Could not create private connection. " \
				    "Registration failed.\n");
	}

        g_free(address);

        whiteboard_log_debug_fe();

        return retval;
}

gboolean whiteboard_util_discover_sib(gchar** address)
{
	DBusMessage *reply = NULL;
	DBusError err;
	DBusConnection *conn = NULL;
	gchar* temp = NULL;
	gboolean retval = FALSE;

	whiteboard_log_debug_fb();

	dbus_error_init(&err);
	conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
	dbus_connection_set_exit_on_disconnect(conn, FALSE);

	if (dbus_error_is_set(&err))
	{
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
				    "-- %s", err.message);
		dbus_error_free(&err);
		whiteboard_log_debug_fe();
		return FALSE;
	}

	g_return_val_if_fail(conn != NULL, FALSE);
	
	whiteboard_util_send_method_with_reply(SIB_DBUS_SERVICE,
					       SIB_DBUS_OBJECT,
					       SIB_DBUS_INTERFACE,
					       SIB_DBUS_METHOD_DISCOVERY,
					       conn, &reply,
					       WHITEBOARD_UTIL_LIST_END);
	
	if(reply==NULL)
	  {
	    whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, "Discovery failed, maybe whiteboardd is not running, trying to start...\n");
	    gchar *service = NULL;
	    DBusMessage *new_message = NULL;
	    DBusMessage *reply2 = NULL;
	    DBusError err2;
	    gint temp2=-1;
	    service = g_strdup("com.nokia.sibd");
	    gint flags = 0;
	    new_message = dbus_message_new_method_call( DBUS_SERVICE_DBUS,
							DBUS_PATH_DBUS,
							DBUS_INTERFACE_DBUS,
							"StartServiceByName");
	    
	    if(NULL == new_message)
	      {
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
				      "Message creationn failed\n");
		whiteboard_log_debug_fe();
		return FALSE;
	      }
	    
	    dbus_message_append_args(new_message,
				     DBUS_TYPE_STRING, &service,
				     DBUS_TYPE_INT32, &flags,
				     DBUS_TYPE_INVALID);
	    dbus_error_init( &err2);
	    reply2 =
	      dbus_connection_send_with_reply_and_block(conn,
							new_message,
							WHITEBOARD_SEND_TIMEOUT,
							&err2);

	    if(dbus_error_is_set(&err2))
	      {
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
				      "-- %s", err2.message);
		dbus_error_free(&err2);
		whiteboard_log_debug_fe();
		g_free(service);
		return FALSE;
	      }
	    dbus_message_get_args(reply2, NULL,
				  DBUS_TYPE_INT32, &temp2,
				  DBUS_TYPE_INVALID);
	    whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
				  "StartServiceByName reply: %d\n", temp2);
	    dbus_message_unref(reply2);
	    g_free(service);
	    return FALSE;
	  }

	dbus_message_get_args(reply, NULL,
			      DBUS_TYPE_STRING, &temp,
			      DBUS_TYPE_INVALID);

	whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
			    "Discover address: %s\n", temp);

	if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR)
	{
		whiteboard_log_error("Discovery returned with an error: %s\n",
				   temp);
		temp = NULL;
	}

	if (temp != NULL)
	{
		*address = g_strdup(temp);
		retval = TRUE;
	}
	else
	{
		retval = FALSE;
	}

	dbus_message_unref(reply);

	dbus_connection_close(conn);
	dbus_connection_unref(conn);

	whiteboard_log_debug_fe();
	return retval;
}

gboolean whiteboard_util_register_sib_try(gpointer uself, const gchar* uuid,const gchar* method,
			    DBusObjectPathUnregisterFunction unregister_handler,
			    DBusObjectPathMessageFunction dispatch_message)
{
        GError *gerror = NULL;
        DBusGConnection *connection = NULL;
	DBusMessage *reply = NULL;
	dbus_int32_t register_success = -1;
        gchar *address = NULL;
        gboolean retval = FALSE;
	struct WhiteBoardLibHeader* self = (struct WhiteBoardLibHeader*)uself;
        DBusObjectPathVTable vtable = { unregister_handler,
                                        dispatch_message,
                                        NULL, NULL, NULL, NULL
        };

        whiteboard_log_debug_fb();

	if ( NULL == self->connection )
	{
		/* Discovering private address */
		if (whiteboard_util_discover_sib(&address) == FALSE)
		{
			address = g_strdup("unix:path=/tmp/sib-dbus");
			whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
					    "Discovery failed, trying " \
					    "default address: %s\n", address);
		}

		/* Open a connection to the address */
		connection = dbus_g_connection_open(address, &gerror);
		if (gerror != NULL)
		{
			whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
					    "Unable to open connection: %s\n",
					    gerror->message);
			g_error_free(gerror);

			self->connection = NULL;

			retval = FALSE;
		}
		else
		{
			/* Get the actual connection pointer */
			self->connection = (DBusConnection*)
				dbus_g_connection_get_connection(connection);
		}
	}
	else
	{
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
				    "Connection already established, " \
				    "using old connection\n");
	}

	if ( NULL != self->connection )
	{
                whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
				      "Sending registration request to daemon\n");
		
		whiteboard_util_send_method_with_reply(SIB_DBUS_SERVICE,
						       SIB_DBUS_OBJECT,
						       SIB_DBUS_REGISTER_INTERFACE,
						       method,
						       self->connection,
						       &reply,
						       DBUS_TYPE_STRING, &uuid,
						       DBUS_TYPE_STRING, &self->friendly_name,
						       DBUS_TYPE_STRING, &self->mimetypes,
						       DBUS_TYPE_BOOLEAN, &self->local,
					    WHITEBOARD_UTIL_LIST_END);
		
		if(reply)
		  {
		    if(!whiteboard_util_parse_message(reply, 
						      DBUS_TYPE_INT32, &register_success,
						      WHITEBOARD_UTIL_LIST_END) )
		      {
			whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
					      "Could not register (reply message parse error)\n");
		      }
		    else 
		      {
			if(register_success < 0)
			  {
			    whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
						  "Could not register (error in reply message)\n");
			  }
			else
			  {
			    
			    whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
						  "Registered\n");
			    retval = TRUE;
			  }
			dbus_message_unref(reply);
		      }
		  }
		if(retval)
		  {
		    dbus_connection_register_fallback(self->connection,
		    				      SIB_DBUS_OBJECT,
						      &vtable,
						      (gpointer)self);
		    //		    dbus_connection_add_filter(self->connection, 
		    //		     &dispatch_message, self, NULL);
		    
		    dbus_connection_setup_with_g_main(self->connection,
						      self->main_context);

		  }
	}
	else
	{
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
				    "Could not create private connection. " \
				    "Registration failed.\n");
	}

        g_free(address);

        whiteboard_log_debug_fe();

        return retval;
}

gboolean whiteboard_util_send_message(const gchar *destination, const gchar *path,
				    const gchar *interface, const gchar *method, gint type,
				    DBusConnection *conn, DBusMessage *msg,
				    DBusMessage **reply,
				    dbus_uint32_t *serial,
				    gint first_argument_type, ...)
{
	DBusMessage *new_message = NULL;
	DBusError err;
	va_list argp;
	whiteboard_log_debug_fb();
	g_return_val_if_fail(NULL != conn, FALSE);

	/* TODO: sanity checks for not used values */

	/* Select message to create */
	switch (type)
	{
	case DBUS_MESSAGE_TYPE_METHOD_CALL:
		new_message = dbus_message_new_method_call(destination,
							   path,
							   interface,
							   method);
		break;

	case DBUS_MESSAGE_TYPE_METHOD_RETURN:
		new_message = dbus_message_new_method_return(msg);

		/* By copying these we can utilize our static routing
		 * mechanism found in whiteboard daemon */
		dbus_message_set_path(new_message,
				dbus_message_get_path(msg));
		dbus_message_set_interface(new_message,
				dbus_message_get_interface(msg));
		dbus_message_set_member(new_message,
				dbus_message_get_member(msg));
		break;

	case DBUS_MESSAGE_TYPE_ERROR:
		break;

	case DBUS_MESSAGE_TYPE_SIGNAL:
		new_message = dbus_message_new_signal(path, interface, method);
		break;

	default:
		break;
	}

	if (NULL == new_message)
	{
	  fprintf(stderr, "Message creation failed!\n");
#ifdef WHITEBOARD_DEBUG
		fprintf(stderr, "Message creation failed!\n");
#endif
		return FALSE;
	}


	whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,"Sending message type: %d\n",type);
	whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,"Path: %s\n", dbus_message_get_path(new_message));
	whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,"Destination: %s\n",
			      dbus_message_get_destination(new_message));
	whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,"Interface: %s\n",
			      dbus_message_get_interface(new_message));
	whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,"Member: %s\n",
			      dbus_message_get_member(new_message));
	/*	printf("whiteboard_util_send_message: %s %s %d\n",
	       dbus_message_get_interface(new_message),
	       dbus_message_get_member(new_message),
	       type);*/

	va_start(argp, first_argument_type);
	dbus_message_append_args_valist(new_message, first_argument_type, argp);
	va_end(argp);

	dbus_error_init(&err);

	if (NULL != reply)
	{
		*reply = dbus_connection_send_with_reply_and_block(
			conn, new_message, WHITEBOARD_SEND_TIMEOUT, &err);
		/*printf("whiteboard_util_send_message: got reply %s %s %d\n",
		       dbus_message_get_interface(*reply),
	       	       dbus_message_get_member(*reply),
	               dbus_message_get_type(*reply)); */
	}
	else
	{
		dbus_uint32_t my_serial;

		if (DBUS_MESSAGE_TYPE_METHOD_RETURN == type)
		{
			dbus_connection_send(conn, new_message, &my_serial);
			dbus_connection_flush(conn);
		}
		else
		  {
		    dbus_connection_send(conn, new_message, serial);
			dbus_connection_flush(conn);
		  }
	}

	if (dbus_error_is_set(&err))
	{
		fprintf(stderr, "Message send failed: %s\n", err.message);

#ifdef WHITEBOARD_DEBUG
		fprintf(stderr, "Message send failed: %s\n", err.message);
#endif
		dbus_error_free(&err);
		return FALSE;
	}

	dbus_message_unref(new_message);
	whiteboard_log_debug_fe();
	return TRUE;
}

gboolean whiteboard_util_parse_message(DBusMessage *msg,
				     gint first_argument_type, ...)
{
	DBusError err;
	va_list var_args;
	dbus_bool_t retval = FALSE;

#ifdef WHITEBOARD_DEBUG
	DBusMessageIter args;
	gint arg_count = 0;
	gint type = dbus_message_get_type(msg);

	switch (type)
	{
	case DBUS_MESSAGE_TYPE_METHOD_CALL:
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,
				    "Parsing method call message.\n");
		break;
	case DBUS_MESSAGE_TYPE_METHOD_RETURN:
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,
				    "Parsing method return message.\n");
		break;
	case DBUS_MESSAGE_TYPE_ERROR:
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,
				    "Parsing error message.\n");
		break;
	case DBUS_MESSAGE_TYPE_SIGNAL:
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS,
				    "Parsing signal message.\n");
		break;
	default:
		whiteboard_log_warning("Unknown message type!\n");
		return FALSE;
		break;
	}

	whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS, 
			    "Object path: %s\n", dbus_message_get_path(msg));
	whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS, 
			    "Interface: %s\n", dbus_message_get_interface(msg));
	whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS, 
			    "Member: %s\n", dbus_message_get_member(msg));

	whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS, 
			    "Reading arguments from dbus message.\n");

	if (!dbus_message_iter_init(msg, &args))
	{
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS, 
				    "Message has no arguments!\n");
		return FALSE;
	}

	whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS, 
			    "(string: %d, int: %d, boolean: %d invalid: %d\n", 
			    DBUS_TYPE_STRING, 
			    DBUS_TYPE_INT32, 
			    DBUS_TYPE_BOOLEAN,
			    DBUS_TYPE_INVALID);
	
	whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS, 
			    "Argument with type %d found.\n",
			    dbus_message_iter_get_arg_type(&args));
	arg_count++;

	while (dbus_message_iter_has_next(&args))
	{
		dbus_message_iter_next(&args);
		whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS, 
				    "Argument with type %d found.\n",
				    dbus_message_iter_get_arg_type(&args));
		arg_count++;
	}

	whiteboard_log_debugc(WHITEBOARD_DEBUG_DBUS, 
			    "Total %d arguments found.\n", arg_count);
#endif
	
	dbus_error_init(&err);
	va_start(var_args, first_argument_type);
	retval = dbus_message_get_args_valist(msg, &err,
					      first_argument_type, var_args);
	va_end(var_args);

	dbus_error_free(&err);

	if (FALSE == retval)
	{
		whiteboard_log_error("%s: %s\n", err.name, err.message);
	}

	return TRUE;
}

gboolean whiteboard_util_split_objectid(gchar* objectid,
				      gchar** serviceid, gchar** itemid)
{
	gchar **stringv = NULL;
	gboolean retval = FALSE;
	
	g_return_val_if_fail(objectid != NULL, FALSE);

	stringv = g_strsplit(objectid, "::", 2);

	g_return_val_if_fail(stringv != NULL, FALSE);

	if ( (stringv[0] != NULL) && (stringv[1] != NULL) )
	{
		if ( NULL != serviceid )
		{
			*serviceid = g_strdup(stringv[0]);
			retval = TRUE;
		}
		
		if ( NULL != itemid )
		{
			*itemid = g_strdup(stringv[1]);
			retval = TRUE;
		}
	}

	g_strfreev(stringv);

        return retval;
}

gboolean whiteboard_util_remove_pid_file(gchar *component_name)
{
	gchar *pid_file_name = NULL;

	pid_file_name = g_strconcat(WHITEBOARD_UTIL_PID_DIR, component_name, 
				    ".pid", NULL);

	if ( 0 != g_remove(pid_file_name) )
	{
		whiteboard_log_warning("Could not remove pid file: %s\n",
				     pid_file_name);
		g_free(pid_file_name);
		
		return FALSE;
	}

	g_free(pid_file_name);

	return TRUE;
}

gboolean whiteboard_util_create_pid_file(gchar *component_name)
{
	GIOChannel* pid_file = NULL;
	gchar *pid_file_name = NULL;
	GError* error = NULL;
	gchar pid[6];
	gsize written = 0;

	pid_file_name = g_strconcat(WHITEBOARD_UTIL_PID_DIR, component_name,
				    ".pid", NULL);

	pid_file = g_io_channel_new_file(pid_file_name, "w+", &error);

	whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			    "Creating pid file: %s\n", pid_file_name);

	if ( NULL != error )
	{
		whiteboard_log_error("Could not create pid file %s: %s\n",
				   pid_file_name, error->message);

		g_error_free(error);
		g_free(pid_file_name);
		return FALSE;
	}

	sprintf(pid, "%d", getpid());

	while (written != strlen(pid))
	{
		g_io_channel_write_chars(pid_file, pid, -1, &written, &error);

		if ( NULL != error )
		{
			whiteboard_log_error("Could not write pid (%s) into file %s: %s\n", 
					   pid, pid_file_name, error->message);

			g_error_free(error);
			g_free(pid_file_name);
			return FALSE;
		}
	}

	g_io_channel_shutdown(pid_file, TRUE, &error);
	
	if ( NULL != error )
	{
		whiteboard_log_warning("Could not close pid file %s: %s\n",
				     pid_file_name, error->message);

		g_error_free(error);
		g_free(pid_file_name);
		return FALSE;
	}

	g_io_channel_unref(pid_file);
	g_free(pid_file_name);

	return TRUE;
}
