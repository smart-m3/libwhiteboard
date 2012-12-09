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
 * whiteboard_sib_access.h
 *
 * Copyright 2007 Nokia Corporation
 */

#ifndef WHITEBOARD_SIB_ACCESS_H
#define WHITEBOARD_SIB_ACCESS_H

#include <glib.h>
#include <glib-object.h>
#include <whiteboard_util.h>
#include <whiteboard_command.h>
#include <sibdefs.h>
G_BEGIN_DECLS

/******************************************************************************
 * Type checks and casts
 ******************************************************************************/

#define WHITEBOARD_SIB_ACCESS_TYPE whiteboard_sib_access_get_type()

#define WHITEBOARD_SIB_ACCESS(object) G_TYPE_CHECK_INSTANCE_CAST(object, \
								 WHITEBOARD_SIB_ACCESS_TYPE, \
								 WhiteBoardSIBAccess)

#define WHITEBOARD_SIB_ACCESS_CLASS(object) G_TYPE_CHECK_CLASS_CAST(object, \
								    WHITEBOARD_SIB_ACCESS_TYPE, \
								    WhiteBoardSIBAccessClass)

#define IS_WHITEBOARD_SIB_ACCESS(object) G_TYPE_CHECK_INSTANCE_TYPE(object, \
								    WHITEBOARD_SIB_ACCESS_TYPE)

#define IS_WHITEBOARD_SIB_ACCESS_CLASS(object) G_TYPE_CHECK_CLASS_TYPE(object, \
								       WHITEBOARD_SIB_ACCESS_TYPE)

/******************************************************************************
 * Type definitions
 ******************************************************************************/

struct _WhiteBoardSIBAccess;
typedef struct _WhiteBoardSIBAccess WhiteBoardSIBAccess;

struct _WhiteBoardSIBAccessClass;
typedef struct _WhiteBoardSIBAccessClass WhiteBoardSIBAccessClass;

struct _WhiteBoardSIBAccessHandle;
typedef struct _WhiteBoardSIBAccessHandle WhiteBoardSIBAccessHandle;


/**
 * Defines Callback function for refreshing SIB
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessRefreshCB) (WhiteBoardSIBAccess* source,
					      gpointer user_data);
/**
 * Use this identifier with g_signal_connect() to receive refresh signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_REFRESH "refresh"

/**
 * Defines Callback function for shutting down SIB using the provided library
 * context
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessShutdownCB) (WhiteBoardSIBAccess* source,
					       gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive shutdown signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_SHUTDOWN "shutdown"

/**
 * Defines Callback function for healtcheck procedure
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessHealthCheckCB) (WhiteBoardSIBAccess* source,
						      gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive healthcheck signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_HEALTHCHECK "healthcheck"

/**
 * Defines callback function for insert action
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param handle A WhiteBoardSIBAccessHandle instance necessary for reply messages
 * @param nodeid Identifier of the node that sent the request to be handled
 * @param sibid  Identifier of the SIB the request is sent for
 * @param msgnumber Message sequence number
 * @param encoding Encoding type of the serialized triples.
 * @param insert_request Inserted triples in serialiazed format
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessInsertCB) (WhiteBoardSIBAccess* source,
					     WhiteBoardSIBAccessHandle *handle,
					     gchar *nodeid,
					     gchar *sibid,
					     gint msgnumber,
					     EncodingType encoding,
					     gchar *insert_request,
					     gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive browse children signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_INSERT "insert"


/**
 * Defines callback function for remove action
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param handle A WhiteBoardSIBAccessHandle instance necessary for reply messages
 * @param nodeid Identifier of the node that sent the request to be handled
 * @param sibid  Identifier of the SIB the request is sent for
 * @param msgnumber Message sequence number
 * @param encoding Encoding type of the serialized triples.
 * @param remove_request Triple ids of removed triples in serialiazed format
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessRemoveCB) (WhiteBoardSIBAccess* source,
					     WhiteBoardSIBAccessHandle *handle,
					     gchar *nodeid,
					     gchar *sibid,
					     gint msgnumber,
					     EncodingType encoding,
					     gchar *remove_request,
					     gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive remove signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_REMOVE "remove"


/**
 * Defines callback function for update action
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param handle A WhiteBoardSIBAccessHandle instance necessary for reply messages
 * @param nodeid Identifier of the node that sent the request to be handled
 * @param sibid  Identifier of the SIB the request is sent for
 * @param msgnumber Message sequence number
 * @param encoding Encoding type of the serialized triples (applies both for inserted and removed triples).
 * @param insert_request Inserted triples in serialiazed format
 * @param remove_request Removedtriples in serialiazed format
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessUpdateCB) (WhiteBoardSIBAccess* source,
					     WhiteBoardSIBAccessHandle *handle,
					     gchar *nodeid,
					     gchar *sibid,
					     gint msgnumber,
					     EncodingType encoding,
					     gchar *insert_request,
					     gchar *remove_request,
					     gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive browse children signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_UPDATE "update"

/**
 * Defines callback function for query action
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param handle A WhiteBoardSIBAccessHandle instance necessary for reply messages
 * @param access_id Related access id
 * @param nodeid Identifier of the node
 * @param sibid Identifier of the SIB
 * @param msgnumber Message sequence number
 * @param type Query type (QueryType enum)
 * @param query_request Query in serialised format (RDF-M3, WQL, SPARQL)
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessQueryCB) (WhiteBoardSIBAccess* source,
					    WhiteBoardSIBAccessHandle *handle,
					    gint access_id,
					    gchar *nodeid,
					    gchar *sibid,
					    gint msgnumber,
					    gint type,
					    gchar *query_request,
					    gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive query signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_QUERY "query"

/**
 * Defines callback function for Join action
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param handle A WhiteBoardSIBAccessHandle instance necessary for reply messages
 * @param join_id Access id
 * @param username Pointer to the string containing username of the user
 * @param nodeid Identifier of the Node
 * @param udn Identifier of the SIB
 * @param msgnumber Message sequence number
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessJoinCB) (WhiteBoardSIBAccess* source,
					   WhiteBoardSIBAccessHandle *handle,
					   gint join_id,
					   gchar *nodeid,
					   gchar *udn,
					   gint msgnumber,
					   gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive join signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_JOIN "join"

/**
 * Defines callback function for leave action
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param handle A WhiteBoardSIBAccessHandle instance necessary for reply messages
 * @param nodeid Identifier of the Node
 * @param udn Identifier of the SIB
 * @param msgnumber Message sequence number
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessLeaveCB) (WhiteBoardSIBAccess* source,
					    WhiteBoardSIBAccessHandle *handle,
					    gchar *nodeid,
					    gchar *udn,
					    gint msgnumber,
					    gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive leave signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_LEAVE "leave"

/**
 * Defines callback function for unsubscribe action
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param handle A WhiteBoardSIBAccessHandle instance necessary for reply messages
 * @param access_id Related access id
 * @param nodeid Identifier of the Node
 * @param sibid Identifier of the SIB
 * @param msgnumber Message sequence number
 * @param request subscription_ids of the subscription to be unsubscribed in M3-RDF format
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessUnsubscribeCB) (WhiteBoardSIBAccess* source,
						  WhiteBoardSIBAccessHandle *handle,
						  gint access_id,
						  gchar *nodeid,
						  gchar *sibid,
						  gint msgnumber,
						  gchar *request,
						  gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive unsubscribe signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_UNSUBSCRIBE "unsubscribe"

/**
 * Defines callback function for subscribe action
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param handle A WhiteBoardSIBAccessHandle instance necessary for reply messages
 * @param access_id Related access id
 * @param nodeid Identifier of the Node
 * @param sibid Identifier of the SIB
 * @param msgnumber Message sequence number
 * @param type Query type (enum QueryType).
 * @param request Search query for the subscription.
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessSubscribeCB) (WhiteBoardSIBAccess* source,
						WhiteBoardSIBAccessHandle *handle,
						gint access_id,
						gchar *nodeid,
						gchar *sibid,
						gint msgnumber,
						gint type,
						gchar *request,
						gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive subscribe signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_SUBSCRIBE "subscribe"

/*****************************************************************************
 * Custom command
 *****************************************************************************/

/**
 * Defines callback function for a custom command request
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param request A WhiteBoardCmdRequest instance that holds the command data
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessCustomCommandRequestCB) (WhiteBoardSIBAccess* source,
							   WhiteBoardCmdRequest* request,
							   gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive custom command
 * request signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_CUSTOM_COMMAND_REQUEST "custom_command_request"

/**
 * Defines callback function for a custom command response
 *
 * @param source The signaling WhiteBoardSIBAccess instance
 * @param response A WhiteBoardCmdResponse instance that holds the command data
 * @param user_data Optional userdata pointer
 */
typedef void (*WhiteBoardSIBAccessCustomCommandResponseCB) (WhiteBoardSIBAccess* source,
							    WhiteBoardCmdResponse* response,
							    gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive custom command
 * response signals
 */
#define WHITEBOARD_SIB_ACCESS_SIGNAL_CUSTOM_COMMAND_RESPONSE "custom_command_response"

/**
 * Get the GLib type of WhiteBoardSIBAccess
 */
GType whiteboard_sib_access_get_type();

/**
 * Create a new WhiteBoardSIBAccess instance.
 * Unique ids for service and control channel can be same, but if multiple virtual
 * sibs are created from on process, only one control channel is required.
 *
 * @param serviceid Unique id for this sib-access component
 * @param controlid Unique id for this control channel component
 * @param main_context A pre-allocated GMainContext (usually obtained with
 *                    g_main_context_default(), unless you know better)
 * @param friendly_name Friendly
 * @param description Description of the SIB instance.  NOT IMPLEMENTED
 * @return Pointer to a new WhiteBoardSIBAccess instance
 */
GObject *whiteboard_sib_access_new(guchar* serviceid,
				   guchar* controlid,
				   GMainContext* main_context,
				   guchar* friendly_name,
				   guchar* description);

/**
 * Get the WhiteBoardSIBAccess's mainloop, you can let whiteboard_sib_access to create the
 * mainloop, then fetch it and run it externally without explicitly calling
 * whiteboard_sib_access_start.
 *
 * @param self The WhiteBoardSIBAccess, whose mainloop to get
 * @return Pointer to the main loop
 */
GMainContext *whiteboard_sib_access_get_main_context(WhiteBoardSIBAccess *self);

/**
 * Get the UUID of an WhiteBoardSIBAccess instance
 *
 * @param self A WhiteBoardSIBAccess instance
 * @return A string containing the instance UUID (must NOT be freed)
 */
const guchar *whiteboard_sib_access_get_uuid(WhiteBoardSIBAccess *self);

/**
 * Send join complete message
 *
 * @param handle pointer to corresponding handle instance of the join request.
 * @param join_id access_id from the corresponding join request
 * @param status join status
 * @return 0 if success, otherwise -1
 */
gint whiteboard_sib_access_send_join_complete(WhiteBoardSIBAccessHandle *handle,
					      gint join_id,
					      gint status);

/**
 * Send response to an insert request
 *
 * @param handle pointer to corresponding handle instance of the insert request.
 * @param success status of the insert transaction (ssStatus_t)
 * @param response Pointer to a string containing triple_id list reported by the SIB
 */
void whiteboard_sib_access_send_insert_response(WhiteBoardSIBAccessHandle *handle,
						gint success,
						const guchar *response);
/**
 * Send response to a remove request
 *
 * @param handle pointer to corresponding handle instance of the remove request.
 * @param success status of the remove transaction (ssStatus_t)
 * @param response Pointer to a string containing remove_status_list reported by the SIB
 */
void whiteboard_sib_access_send_remove_response(WhiteBoardSIBAccessHandle *handle,
						gint success,
						const guchar *response);

/**
 * Send response to a update request
 *
 * @param handle pointer to corresponding handle instance of the remove request.
 * @param success status of the remove transaction (ssStatus_t)
 * @param response Pointer to a string containing remove_status_list reported by the SIB
 */
void whiteboard_sib_access_send_update_response(WhiteBoardSIBAccessHandle *handle,
						gint success,
						const guchar *response);
/**
 * Send response to a query request
 *
 * @param handle pointer to corresponding handle instance of the query request.
 * @param access_id access_id from the corresponding query request
 * @param status status of the query transaction (ssStatus_t)
 * @param results Pointer to a string containing query_result reported by the SIB
 */
void whiteboard_sib_access_send_query_response(WhiteBoardSIBAccessHandle *handle,
					       gint access_id,
					       gint status,
					       const guchar *results);
/**
 * Send response to a subscribe request
 *
 * @param handle pointer to corresponding handle instance of the subscribe request.
 * @param access_id access_id from the corresponding subscribe request
 * @param status status of the subscribe transaction (ssStatus_t)
 * @param subscription_id Subscription_id of the created subscription.
 * @param response Pointer to a string containing initial query_result reported by the SIB
 */
void whiteboard_sib_access_send_subscribe_response(WhiteBoardSIBAccessHandle *handle,
						   gint access_id,
						   gint status,
						   const guchar *subscription_id,
						   const guchar *response);

/**
 * Send subscribtion indication
 *
 * @param handle pointer to corresponding handle instance of the subscribe request.
 * @param access_id access_id from the corresponding subscribe request
 * @param update_sequence nth indication modulo 10000, skiping zero (0:not supported by SIB, else: 1,2..9999,1,2..) 
 * @param subscriptionid Subscription_id of the subscription.
 * @param results_added Pointer to a string containing query results that were added since previous indication
 * @param results_removed Pointer to a string containing query result that were removed since previous indication
 */
void whiteboard_sib_access_send_subscription_indication(WhiteBoardSIBAccessHandle *handle,
							gint access_id,
							gint update_sequence,
							const guchar *subscriptionid,
							const guchar *results_added,
							const guchar *results_removed);
/**
 * Send unsubscribe complete signal (response to unsubscribe request or indication when subscription is expired)
 *
 * @param handle pointer to corresponding handle instance of the subscribe request.
 * @param access_id access_id from the corresponding subscribe request
 * @param status status of the unsubscribe transaction (ssStatus_t)
 * @param subscription_id Subscription_id of the subscription.
 */
void whiteboard_sib_access_send_unsubscribe_complete(WhiteBoardSIBAccessHandle* handle,
						     gint access_id,
						     gint status,
						     const guchar *subscription_id);
/*****************************************************************************
 * WhiteBoardSIBAccessHandle functions
 *****************************************************************************/

/**
 * Get the sib-access instance related to a request handle
 *
 * @param self A WhiteBoardSIBAccessHandle
 * @return Reference to an whiteboard_sib_access instance
 */
WhiteBoardSIBAccess *whiteboard_sib_access_handle_get_context(WhiteBoardSIBAccessHandle* self);

/**
 * Increase the reference count of an WhiteBoardSIBAccessHandle instance.
 *
 * You must take a reference to a WhiteBoardSIBAccessHandle if you wish to send
 * replies from another context (i.e. another thread than where your callback
 * gets called).
 *
 * @param self A WhiteBoardSIBAccessHandle
 */
void whiteboard_sib_access_handle_ref(WhiteBoardSIBAccessHandle* self);

/**
 * Decrease the reference count of an WhiteBoardSIBAccessHandle instance
 *
 * When you are certain that you no longer need your WhiteBoardSIBAccessHandle
 * reference, you should call this function.
 *
 * @param self A WhiteBoardSIBAccessHandle
 */
void whiteboard_sib_access_handle_unref(WhiteBoardSIBAccessHandle* self);


G_END_DECLS

#endif
