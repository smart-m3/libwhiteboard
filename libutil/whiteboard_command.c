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
 * whiteboard_command.c
 *
 * Copyright 2007 Nokia Corporation
 */
#include <glib.h>
#include <string.h>
#include <stdlib.h>

#include "whiteboard_command.h"
#include "whiteboard_dbus_ifaces.h"
#include "whiteboard_log.h"

/**
 * Custom command structure:
 *
 * DBUS_TYPE_STRING: target UUID
 * DBUS_TYPE_STRING: method name
 *
 * DBUS_TYPE_STRING: WHITEBOARD_CMD_DATA_BEGIN
 * DBUS_TYPE_STRING: param name
 * DBUS_TYPE_STRING: param data
 * DBUS_TYPE_STRING: param name
 * DBUS_TYPE_STRING: param data
 * ....
 * DBUS_TYPE_STRING: WHITEBOARD_CMD_DATA_END
 *
 */

/**
 * Warning! If you modify this struct, you have to make the same
 * changes to whiteboard_metadata.c as well.
 */
struct _WhiteBoardCmd
{
	DBusMessage* msg;
	gint refcount;
};

/*****************************************************************************
 * Static prototypes
 *****************************************************************************/

/**
 * Append a method name into a WhiteBoardCmd message
 *
 * @param self A command to append the method to
 * @param method The method name to append
 */
static void whiteboard_cmd_append_method_name(WhiteBoardCmd* self,
					    const gchar* method);

/**
 * Append a target UUID into a WhiteBoardCmd message
 *
 * @param self A command to append the UUID to
 * @param uuid The UUID to append
 */
static void whiteboard_cmd_append_target_uuid(WhiteBoardCmd* self,
					    const gchar* uuid);

/*****************************************************************************
 * Constructors etc.
 *****************************************************************************/

WhiteBoardCmdRequest* whiteboard_cmd_new_request(const gchar* method,
					     const gchar* target_uuid)
{
	WhiteBoardCmd* self = NULL;
	const gchar* begin = WHITEBOARD_CMD_DATA_BEGIN;

	whiteboard_log_debug_fb();

	g_return_val_if_fail(method != NULL, NULL);
	g_return_val_if_fail(target_uuid != NULL, NULL);

	self = g_new0(WhiteBoardCmd, 1);
	g_return_val_if_fail(self != NULL, NULL);

	self->refcount = 1;
	self->msg = dbus_message_new_method_call(WHITEBOARD_DBUS_SERVICE,
						 WHITEBOARD_DBUS_OBJECT,
						 WHITEBOARD_DBUS_INTERFACE,
						 WHITEBOARD_DBUS_METHOD_CUSTOM_COMMAND);
	g_return_val_if_fail(self->msg != NULL, NULL);

	whiteboard_cmd_append_method_name(self, method);
	whiteboard_cmd_append_target_uuid(self, target_uuid);

	dbus_message_append_args(self->msg,
				 DBUS_TYPE_STRING, &begin,
				 DBUS_TYPE_INVALID);

	whiteboard_log_debug_fe();

	return (WhiteBoardCmdRequest*) self;
}

WhiteBoardCmd* whiteboard_cmd_new_response(WhiteBoardCmd* request)
{
	WhiteBoardCmd* self = NULL;
	const gchar *member = NULL;
	const gchar* begin = WHITEBOARD_CMD_DATA_BEGIN;

	whiteboard_log_debug_fb();

	g_return_val_if_fail(request != NULL, NULL);
	g_return_val_if_fail(request->msg != NULL, NULL);

	self = g_new0(WhiteBoardCmd, 1);
	g_return_val_if_fail(self != NULL, NULL);

	self->refcount = 1;
	self->msg = dbus_message_new_method_return(request->msg);
	g_return_val_if_fail(self->msg != NULL, NULL);

	/* The response message is directed back to the whiteboard generic I/F */
	dbus_message_set_interface(self->msg, WHITEBOARD_DBUS_INTERFACE);

	/* Copy member name from the request to this response */
	member = dbus_message_get_member(request->msg);
	dbus_message_set_member(self->msg, member);

	/* Copy method name from the request to the response */
	whiteboard_cmd_append_method_name(self,
					whiteboard_cmd_get_method_name(request));

	/* Copy target UUID from the request to the response */
	whiteboard_cmd_append_target_uuid(self,
					whiteboard_cmd_get_target_uuid(request));

	dbus_message_append_args(self->msg,
				 DBUS_TYPE_STRING, &begin,
				 DBUS_TYPE_INVALID);

	whiteboard_log_debug_fe();

	return (WhiteBoardCmdResponse*) self;
}

WhiteBoardCmd* whiteboard_cmd_new_with_msg(DBusMessage* msg)
{
	WhiteBoardCmd* self = NULL;

	whiteboard_log_debug_fb();

	g_return_val_if_fail(msg != NULL, NULL);

	self = g_new0(WhiteBoardCmd, 1);
	g_return_val_if_fail(self != NULL, NULL);

	self->refcount = 1;
	self->msg = msg;
	dbus_message_ref(msg);

	whiteboard_log_debug_fe();

	return self;
}

void whiteboard_cmd_ref(WhiteBoardCmd *self)
{
	whiteboard_log_debug_fb();

	g_return_if_fail(self != NULL);
	
	if (g_atomic_int_get(&self->refcount) > 0)
	{
		/* Refcount is something sensible */
		g_atomic_int_inc(&self->refcount);

		whiteboard_log_debug("WhiteBoardCmd refcount inc: %d.\n",
				   self->refcount);
	}
	else
	{
		/* Refcount is already zero */
		whiteboard_log_debug("WhiteBoardCmd refcount already 0!");
	}

	whiteboard_log_debug_fe();
}

static void whiteboard_cmd_destroy(WhiteBoardCmd* self)
{
	whiteboard_log_debug_fb();

	g_return_if_fail(self != NULL);

	dbus_message_unref(self->msg);
	self->msg = NULL;

	g_free(self);

	whiteboard_log_debug_fe();
}

void whiteboard_cmd_unref(WhiteBoardCmd *self)
{
	whiteboard_log_debug_fb();

	g_return_if_fail(self != NULL);

	if (g_atomic_int_dec_and_test(&self->refcount) == FALSE)
	{
		whiteboard_log_debug("WhiteBoardCmd refcount dec: %d.\n",
				   self->refcount);
	}
	else
	{
		whiteboard_log_debug("WhiteBoardCmd refcount zeroed.\n");
		whiteboard_cmd_destroy(self);
	}

	whiteboard_log_debug_fe();
}

guint whiteboard_cmd_response_get_serial(WhiteBoardCmdResponse* self)
{
	g_return_val_if_fail(self != NULL, 0);
	g_return_val_if_fail(self->msg != NULL, 0);

	return dbus_message_get_serial(self->msg);
}

/*****************************************************************************
 * Method name
 *****************************************************************************/

static void whiteboard_cmd_append_method_name(WhiteBoardCmd* self,
					    const gchar* method)
{
	whiteboard_log_debug_fb();

	dbus_message_append_args(self->msg,
				 DBUS_TYPE_STRING, &method,
				 DBUS_TYPE_INVALID);

	whiteboard_log_debug_fe();
}

const gchar* whiteboard_cmd_get_method_name(WhiteBoardCmd* self)
{
	DBusMessageIter iter;
	const gchar* method = NULL;

	if (dbus_message_iter_init(self->msg, &iter) == FALSE)
		return NULL;

	/* The method name should always be the first argument */
	dbus_message_iter_get_basic(&iter, &method);

	return method;
}

/*****************************************************************************
 * Target UUID
 *****************************************************************************/

static void whiteboard_cmd_append_target_uuid(WhiteBoardCmd* self,
					    const gchar* uuid)
{
	whiteboard_log_debug_fb();

	dbus_message_append_args(self->msg,
				 DBUS_TYPE_STRING, &uuid,
				 DBUS_TYPE_INVALID);

	whiteboard_log_debug_fe();
}

const gchar* whiteboard_cmd_get_target_uuid(WhiteBoardCmd* self)
{
	DBusMessageIter iter;
	const gchar* uuid = NULL;

	if (dbus_message_iter_init(self->msg, &iter) == FALSE)
		return NULL;

	if (dbus_message_iter_next(&iter) == FALSE)
		return NULL;

	/* The target UUID should always be the second argument */
	dbus_message_iter_get_basic(&iter, &uuid);

	return uuid;
}

/*****************************************************************************
 * Data insertion
 *****************************************************************************/

gboolean whiteboard_cmd_append(WhiteBoardCmd* self, const gchar** key,
			     const gchar** value)
{
	gboolean retval = FALSE;

	whiteboard_log_debug_fb();

	g_return_val_if_fail(self != NULL, FALSE);
	g_return_val_if_fail(key != NULL, FALSE);
	g_return_val_if_fail(value != NULL, FALSE);

	whiteboard_log_debug("Appending key (%s) and value (%s)\n", *key, *value);
	retval = dbus_message_append_args(self->msg,
					  DBUS_TYPE_STRING, key,
					  DBUS_TYPE_STRING, value,
					  DBUS_TYPE_INVALID);

	whiteboard_log_debug_fe();

	return retval;
}

/*****************************************************************************
 * Data extraction
 *****************************************************************************/

gboolean whiteboard_cmd_iter_init(WhiteBoardCmd* self, WhiteBoardCmdIter* iter)
{
	gint arg_type = 0;
	const gchar* str = NULL;

	whiteboard_log_debug_fb();

	g_return_val_if_fail(self != NULL, FALSE);
	g_return_val_if_fail(iter != NULL, FALSE);

	iter->data = self;
	dbus_message_iter_init(self->msg, &iter->iter);

	/* Skip all arguments before the begin marker */
	while ( (arg_type = dbus_message_iter_get_arg_type(&iter->iter))
		!= DBUS_TYPE_INVALID)
	{
		if (arg_type == DBUS_TYPE_STRING)
		{
			dbus_message_iter_get_basic(&iter->iter, &str);
			if (str != NULL &&
			    strcmp(str, WHITEBOARD_CMD_DATA_BEGIN) == 0)
			{
				dbus_message_iter_next(&iter->iter);
				return TRUE;
			}
		}

		dbus_message_iter_next(&iter->iter);
	}

	whiteboard_log_debug_fe();

	return FALSE;
}

gboolean whiteboard_cmd_iter_get(WhiteBoardCmdIter* iter, const gchar** key,
			       const gchar** value)
{
	gchar* key_arg = NULL;
	gchar* value_arg = NULL;
	gint arg_type = 0;
	DBusMessageIter temp_iter;

	whiteboard_log_debug_fb();

	g_return_val_if_fail(iter != NULL, FALSE);

	/* Make a copy of the DBusMessageIter so that we don't move the
	   original iter until whiteboard_cmd_iter_next is called. */
	temp_iter = iter->iter;

	/* There should be a key string next in line */
	arg_type = dbus_message_iter_get_arg_type(&temp_iter);
	if (arg_type == DBUS_TYPE_STRING)
		dbus_message_iter_get_basic(&temp_iter, &key_arg);

	dbus_message_iter_next(&temp_iter);

	/* And now there should be the value string for the key */
	arg_type = dbus_message_iter_get_arg_type(&temp_iter);
	if (arg_type == DBUS_TYPE_STRING)
		dbus_message_iter_get_basic(&temp_iter, &value_arg);
	
	if (key != NULL)
		*key = key_arg;
	
	if (value != NULL)
		*value = value_arg;
	
	whiteboard_log_debug_fe();
	
	return ((key == NULL || *key != NULL) && 
		(value == NULL || *value != NULL)) ? TRUE : FALSE;
}

/**
 * Attempt to iterate to the next DBusMessage argument, while checking that
 * the new argument isn't WHITEBOARD_CMD_DATA_END
 *
 * @param iter An iterator
 */
static gboolean whiteboard_cmd_iter_next_internal(WhiteBoardCmdIter* iter)
{
	const gchar* str = NULL;

	whiteboard_log_debug_fb();

	/* Jump to the next item, fail if there are no more items */
	if (dbus_message_iter_next(&iter->iter) == FALSE)
	{
		whiteboard_log_debug_fe();
		return FALSE;
	}

	/* Check that the current item is a string, fail if not */
	if (dbus_message_iter_get_arg_type(&iter->iter) != DBUS_TYPE_STRING)
	{
		whiteboard_log_debug_fe();
		return FALSE;
	}

	/* Check that the current item is not an end marker */
	dbus_message_iter_get_basic(&iter->iter, &str);
	if (str == NULL || strcmp(str, WHITEBOARD_CMD_DATA_END) == 0)
	{
		whiteboard_log_debug_fe();
		return FALSE;
	}
	
	whiteboard_log_debug_fe();

	return TRUE;
}

gboolean whiteboard_cmd_iter_next(WhiteBoardCmdIter* iter)
{
	g_return_val_if_fail(iter != NULL, FALSE);
	
	/* Attempt to skip twice: We are first at key n. Then we try to
	   jump to value n. After that, we try to jump to key n+1 */
	if (whiteboard_cmd_iter_next_internal(iter) == TRUE)
		return whiteboard_cmd_iter_next_internal(iter);
	else
		return FALSE;
}

/*****************************************************************************
 * Sending
 *****************************************************************************/

guint whiteboard_cmd_send(WhiteBoardCmd* self, DBusConnection* connection)
{
	guint serial = 0;
	const gchar* end = WHITEBOARD_CMD_DATA_END;

	whiteboard_log_debug_fb();

	g_return_val_if_fail(self != NULL, 0);
	g_return_val_if_fail(connection != NULL, 0);

	dbus_message_append_args(self->msg,
				 DBUS_TYPE_STRING, &end,
				 DBUS_TYPE_INVALID);

	dbus_connection_send(connection, self->msg, &serial);
	
	whiteboard_log_debug_fe();

	return serial;
}
