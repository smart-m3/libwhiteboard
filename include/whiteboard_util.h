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
 * whiteboard_util.h
 *
 * Copyright 2007 Nokia Corporation
 */

#ifndef WHITEBOARD_UTIL_H
#define WHITEBOARD_UTIL_H

#define DBUS_API_SUBJECT_TO_CHANGE

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <glib.h>

/*****************************************************************************
 * Type definitions
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

#define WHITEBOARD_UTIL_LIST_END DBUS_TYPE_INVALID
#define WHITEBOARD_SEND_TIMEOUT 500000
#define WHITEBOARD_REGISTRATION_TIMEOUT 5000000

#define WHITEBOARD_UTIL_PID_DIR "/tmp/"

/*****************************************************************************
 * Miscellaneous utility functions
 *****************************************************************************/

/**
 * Generic registration routine for node / sib-access libraries
 *
 * @param self WhiteBoardNode/WhiteBoardSibAccess instance pointer
 * @param uuid The UUID to register
 * @param method Kind of library we are registering
 * @param unregister_handler Handler for unregistering DBus connection
 * @param dispatch_message Handler for DBus messages
 * @return TRUE when successful, FALSE if fail
 */
gboolean whiteboard_util_register_try(gpointer self, const gchar* uuid, const gchar* method,
				    DBusObjectPathUnregisterFunction unregister_handler,
				    DBusObjectPathMessageFunction dispatch_message);

/**
 * Generic discovery routine for all library objects
 *
 * @param address A newly-created string containing the address
 * @return TRUE if address was set, otherwise FALSE
 */
gboolean whiteboard_util_discover(gchar** address);

/**
 * Generic registration routine for sibs libraries
 *
 * @param self SibObject instance pointer
 * @param uuid The UUID to register
 * @param method Kind of library we are registering
 * @param unregister_handler Handler for unregistering DBus connection
 * @param dispatch_message Handler for DBus messages
 * @return TRUE when successful, FALSE if fail
 */
gboolean whiteboard_util_register_sib_try(gpointer self, const gchar* uuid, const gchar* method,
				    DBusObjectPathUnregisterFunction unregister_handler,
				    DBusObjectPathMessageFunction dispatch_message);

/**
 * Generic discovery routine for sib library objects
 *
 * @param address A newly-created string containing the address
 * @return TRUE if address was set, otherwise FALSE
 */
gboolean whiteboard_util_discover_sib(gchar** address);

/**
 * Utility function to send dbus messages with arbitrary argument list.
 * Use / create convenience macro instead of using this one directly.
 *
 * In variable argument list last "type" argument must be WHITEBOARD_UTIL_LIST_END.
 *
 * @param destination DBus service name
 * @param path DBus object path
 * @param interface DBus interface name
 * @param method DBus method name
 * @param message_type DBus message type
 * @param conn DBus connection pointer
 * @param msg DBus message pointer (NULL allowed)
 * @param reply Points to possible reply message, if not NULL call will block until reply to method call message is received.
 * @param serial Location of returned serial
 * @param first_argument_type Defines the first message argument type (e.g.
 * DBUS_TYPE_INT32, DBUS_TYPE_UINT32, DBUS_TYPE_STRING,
 * WHITEBOARD_UTIL_LIST_END )
 * @param ... Variable length list of dbus message parameters (type, value pairs)
 *
 * @return TRUE when successful, FALSE if fail
 */
gboolean whiteboard_util_send_message(const gchar *destination,const gchar *path,
				    const gchar *interface, const gchar *method,
				    gint message_type, DBusConnection *conn,
				    DBusMessage *msg, DBusMessage **reply,
				    dbus_uint32_t *serial,
				    gint first_argument_type, ...);

/**
 * Utility function to parse dbus message with arbitrary argument list.
 *
 * @param msg DBus message pointer
 * @param first_argument_type Defines the first message argument type
 * @param ... Variable length list of dbus message parameters (type, value pairs)
 * @return TRUE when successful, FALSE if fail
 */
gboolean whiteboard_util_parse_message(DBusMessage *msg,
				     gint first_argument_type, 
				     ...);

/**
 * Convenience macro to send dbus signals with arbitrary argument list
 *
 * @param path DBus object path
 * @param interface DBus interface name
 * @param name DBus signal name
 * @param conn DBus connection pointer
 * @param first_type Defines the first message argument type
 * @param ... Variable length list of dbus message parameters (type, value pairs)
 *
 * @return 1 when successfull 0 when failed
 */
#define whiteboard_util_send_signal(path, interface, name, conn, first_type, ...) \
        whiteboard_util_send_message(NULL, path, interface, name, \
        DBUS_MESSAGE_TYPE_SIGNAL, conn, NULL, NULL, NULL, first_type, ##__VA_ARGS__)

/**
 * Convenience macro to send dbus signals with arbitrary argument list
 * to a list of connections
 *
 * @param path DBus object path
 * @param interface DBus interface name
 * @param name DBus signal name
 * @param clist A list of DBus connection pointers
 * @param first_type Defines the first message argument type
 * @param ... Variable length list of dbus message parameters (type, value pairs)
 */
#define whiteboard_util_send_signal_to_list(path, interface, name, clist, first_type, ...) \
	do { \
		GList *temp; \
		for (temp = clist; NULL != temp; temp = g_list_next(temp)) { \
			whiteboard_util_send_signal(path, interface, name, \
					(DBusConnection *)temp->data, \
					first_type, ##__VA_ARGS__); \
		} \
	} while(0)

/**
 * Convenience macro to send dbus signals with arbitrary argument list
 * to a list of connections
 *
 * @param clist A list of DBus connection pointers
 * @param message Pointer to DBUS Message to be sent
 * @param ... Variable length list of dbus message parameters (type, value pairs)
 */
#define whiteboard_util_send_message_to_list(clist, message) \
        do { \
                GList *temp; \
                for (temp = clist; NULL!=temp; temp = g_list_next(temp)) { \
                        dbus_connection_send((DBusConnection *)temp->data, \
                                        message, NULL); \
                } \
        } while(0)

/**
 * Convenience macro to send dbus method calls with arbitrary argument list
 *
 * @param destination DBus service name
 * @param path DBus object path
 * @param interface DBus interface name
 * @param method DBus method name
 * @param conn DBus connection pointer
 * @param first_type Defines the first message argument type
 * @param ... Variable length list of dbus message parameters (type, value pairs)
 *
 * @return 1 when successfull 0 when failed
 */
#define whiteboard_util_send_method(destination, path, interface, method, conn, first_type, ...) \
        whiteboard_util_send_message(destination, path, interface, method, \
			     DBUS_MESSAGE_TYPE_METHOD_CALL, conn, NULL, NULL, \
			     NULL, first_type, ##__VA_ARGS__)

/**
 * Convenience macro to send dbus method calls with arbitrary argument list
 * (with serial)
 *
 * @param destination DBus service name
 * @param path DBus object path
 * @param interface DBus interface name
 * @param method DBus method name
 * @param conn DBus connection pointer
 * @param serial Location of returned serial
 * @param first_type Defines the first message argument type
 * @param ... Variable length list of dbus message parameters (type, value pairs)
 *
 * @return 1 when successfull 0 when failed
 */
#define whiteboard_util_send_method_serial(destination, path, interface, method, conn, serial, first_type, ...) \
        whiteboard_util_send_message(destination, path, interface, method, \
			     DBUS_MESSAGE_TYPE_METHOD_CALL, conn, NULL, NULL, \
			     serial, first_type, ##__VA_ARGS__)

/**
 * Convenience macro to forward dbus messages and change message properties
 *
 * @param conn DBus connection pointer
 * @param msg DBus message to forward
 * @param destination DBus service name
 * @param path DBus object path
 * @param interface DBus interface name
 * @param method DBus method name
 *
 * @return 1 when successfull 0 when failed
 */
#define whiteboard_util_forward_packet(conn, msg, destination, path, interface, method) \
        do { \
                if ( NULL != destination ) \
			dbus_message_set_destination(msg, destination); \
                if ( NULL != path ) \
			dbus_message_set_path(msg, path); \
                if ( NULL != interface ) \
			dbus_message_set_interface(msg, interface); \
                if ( NULL != method ) \
			dbus_message_set_member(msg, method); \
                dbus_connection_send(conn, msg, NULL); \
        } while(0)

/**
 * Convenience macro to send dbus method calls with arbitrary argument list.
 * Waits for reply message to arrive before returning.
 *
 * @param destination DBus service name
 * @param path DBus object path
 * @param interface DBus interface name
 * @param method DBus method name
 * @param conn DBus connection pointer
 * @param reply Storage area for reply message
 * @param first_type Defines the first message argument type
 * @param ... Variable length list of dbus message parameters (type, value pairs)
 *
 * @return 1 when successfull 0 when failed
 */
#define whiteboard_util_send_method_with_reply(destination, path, interface, method, conn, reply, first_type, ...) \
        whiteboard_util_send_message(destination, path, interface, method, \
			     DBUS_MESSAGE_TYPE_METHOD_CALL, conn, NULL, reply, \
			     NULL, first_type, ##__VA_ARGS__)

/**
 * Convenience macro to send dbus method returns with arbitrary argument list
 * @param conn DBus connection pointer
 * @param msg  DBus message pointer to the send message
 * @param first_type Defines the first message argument type
 * @param ... Variable length list of dbus message parameters (type, value pairs)
 *
 * @return 1 when successfull 0 when failed
 */
#define whiteboard_util_send_method_return(conn, msg, first_type, ...) \
        whiteboard_util_send_message(NULL, NULL, NULL, NULL, \
			     DBUS_MESSAGE_TYPE_METHOD_RETURN, conn, msg, NULL, \
			     NULL, first_type, ##__VA_ARGS__)

/**
 * Split a Whiteboard object ID to service & item parts
 *
 * @param objectid The item id to extract the service id from
 * @param serviceid A newly-created string containing the service part
 * @param itemid A newly-created string containing the item part
 */
gboolean whiteboard_util_split_objectid(gchar* objectid, gchar** serviceid,
				      gchar** itemid);

/**
 * Remove pid file
 *
 * @param component_name Component name
 */
gboolean whiteboard_util_remove_pid_file(gchar *component_name);

/**
 * Create pid file
 *
 * @param component_name Component name
 */
gboolean whiteboard_util_create_pid_file(gchar *component_name);

#endif /* WHITEBOARD_UTIL_H */
