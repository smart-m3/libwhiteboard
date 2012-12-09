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
 * whiteboard_command.h
 *
 * Copyright 2007 Nokia Corporation
 */

#ifndef WHITEBOARD_COMMAND_H
#define WHITEBOARD_COMMAND_H

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <glib.h>

#define WHITEBOARD_CMD_DATA_BEGIN "<whiteboard_cmd_data>"
#define WHITEBOARD_CMD_DATA_END "</whiteboard_cmd_data>"

#define WHITEBOARD_METHOD_CUSTOM_COMMAND "custom_command"

typedef struct _WhiteBoardCmd WhiteBoardCmd;
typedef struct _WhiteBoardCmd WhiteBoardCmdRequest;
typedef struct _WhiteBoardCmd WhiteBoardCmdResponse;

typedef struct _WhiteBoardCmdIter
{
	WhiteBoardCmd* data;    /* Don't use this! */
	DBusMessageIter iter; /* Don't use this! */
} WhiteBoardCmdIter;

/*****************************************************************************
 * Constructors etc.
 *****************************************************************************/

/**
 * Create a new request-type command with the given method name and target
 * UUID. The UUID can be either a component UUID or an object ID.
 * @param method Method name to be included in the request
 * @param uuid A UUID that identifies either a component or an object
 *                    that this command is targeted to.
 * @return A newly-created WhiteBoardCmd structure pointer or NULL
 */
WhiteBoardCmdRequest* whiteboard_cmd_new_request(const gchar* method,
						 const gchar* uuid);

/**
 * Create an empty response for a received request.
 *
 * @param request A request-type command to send an answer to
 * @return A newly-created WhiteBoardCmd structure pointer
 */
WhiteBoardCmdResponse* whiteboard_cmd_new_response(WhiteBoardCmd* request);

/**
 * Create a new command structure based on a received DBusMessage. This is
 * used internally by the WhiteBoard Library, so component developers should
 * not use this.
 *
 * @param msg A DBusMessage conforming to the WhiteBoardCmd specifications
 * @return A newly-created WhiteBoardCmd structure pointer
 */
WhiteBoardCmd* whiteboard_cmd_new_with_msg(DBusMessage* msg);

/**
 * Increase the reference count of a WhiteBoardCmd instance.
 *
 * @param self A WhiteBoardCmd instance
 */
void whiteboard_cmd_ref(WhiteBoardCmd *self);

/**
 * Decrease the reference count of a WhiteBoardCmd instance.
 *
 * @param self A WhiteBoardCmd instance
 */
void whiteboard_cmd_unref(WhiteBoardCmd *self);

/**
 * Get the serial ID of a response message. This serial can be used to
 * associate a response with a request.
 *
 * @param self A WhiteBoardCmd instance
 * @return A serial ID (zero if the message has no serial)
 */
guint whiteboard_cmd_response_get_serial(WhiteBoardCmdResponse* self);

/*****************************************************************************
 * Method name & target UUID
 *****************************************************************************/

/**
 * Get the method name from a WhiteBoardCmd structure
 *
 * @param self A WhiteBoardCmd structure
 * @return The method name or NULL (don't free the pointer!)
 */
const gchar* whiteboard_cmd_get_method_name(WhiteBoardCmd* self);

/**
 * Get the target UUID from a WhiteBoardCmd structure
 *
 * @param self A WhiteBoardCmd structure
 * @return The target UUID or NULL (don't free the pointer!)
 */
const gchar* whiteboard_cmd_get_target_uuid(WhiteBoardCmd* self);

/*****************************************************************************
 * Data insertion
 *****************************************************************************/

/**
 * Append a key and its value to a WhiteBoardCmd message. This function will not
 * accept NULL parameters, because they always produce problems with the
 * underlying IPC system (DBus).
 *
 * @param self A WhiteBoardCmd structure
 * @param key The name of a key
 * @param value The value for the key
 */
gboolean whiteboard_cmd_append(WhiteBoardCmd* self, const gchar** key,
			     const gchar** value);

/*****************************************************************************
 * Data extraction
 *****************************************************************************/

/**
 * Initialize an iterator that can be used to get the keys and values from the
 * command structure one by one.
 *
 * @param self A WhiteBoardCmd structure
 * @param iter A reference to a WhiteBoardCmdIter to initialize
 */
gboolean whiteboard_cmd_iter_init(WhiteBoardCmd* self, WhiteBoardCmdIter* iter);

/**
 * Get the current key-value pair from a WhiteBoardCmd pointed to by
 * a WhiteBoardCmdIter iterator. Don't free or store the pointers.
 *
 * @param iter The iterator object pointing to the current key-value-pair
 * @param key The current key
 * @param value The value associated with key
 * @return TRUE if key and value was found, otherwise FALSE.
 */
gboolean whiteboard_cmd_iter_get(WhiteBoardCmdIter* iter, const gchar** key,
			       const gchar** value);

/**
 * Move an iterator to point to the next key-value-pair.
 * Use whiteboard_cmd_iter_get() to get the actual key and its value.
 *
 * @param iter An iterator reference
 * @return TRUE if the iterator is valid (i.e. items left), otherwise FALSE
 */
gboolean whiteboard_cmd_iter_next(WhiteBoardCmdIter* iter);

/*****************************************************************************
 * Sending
 *****************************************************************************/

/**
 * Send a custom command using the given DBus connection. This function
 * is used only by the WhiteBoard Library, so you shouldn't use this.
 *
 * @param self The custom command to send
 * @param connection A DBus connection to WhiteBoard Daemon
 */
guint whiteboard_cmd_send(WhiteBoardCmd* self, DBusConnection* connection);

#endif
