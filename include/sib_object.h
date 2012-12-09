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
 * SIB Library
 *
 * sib_object.h
 *
 * Copyright 2007 Nokia Corporation
 */

#ifndef SIB_OBJECT_H
#define SIB_OBJECT_H

#include <glib.h>
#include <glib-object.h>
//#include <whiteboard_util.h>
#include <sibdefs.h>

G_BEGIN_DECLS

/******************************************************************************
 * Type checks and casts
 ******************************************************************************/

#define SIB_OBJECT_TYPE sib_object_get_type()

#define SIB_OBJECT(object) G_TYPE_CHECK_INSTANCE_CAST(object, \
								 SIB_OBJECT_TYPE, \
								 SibObject)

#define SIB_OBJECT_CLASS(object) G_TYPE_CHECK_CLASS_CAST(object, \
								    SIB_OBJECT_TYPE, \
								    SibObjectClass)

#define IS_SIB_OBJECT(object) G_TYPE_CHECK_INSTANCE_TYPE(object, \
								    SIB_OBJECT_TYPE)

#define IS_SIB_OBJECT_CLASS(object) G_TYPE_CHECK_CLASS_TYPE(object, \
								       SIB_OBJECT_TYPE)

/******************************************************************************
 * Type definitions
 ******************************************************************************/

struct _SibObject;
typedef struct _SibObject SibObject;

struct _SibObjectClass;
typedef struct _SibObjectClass SibObjectClass;

struct _SibObjectHandle;
typedef struct _SibObjectHandle SibObjectHandle;


/**
 * Defines Callback function for refreshing SIB
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectRefreshCB) (SibObject* source,
					      gpointer user_data);
/**
 * Use this identifier with g_signal_connect() to receive refresh signals
 */
#define SIB_OBJECT_SIGNAL_REFRESH "refresh"

/**
 * Defines Callback function for shutting down SIB using the provided library
 * context
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectShutdownCB) (SibObject* source,
					       gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive shutdown signals
 */
#define SIB_OBJECT_SIGNAL_SHUTDOWN "shutdown"

/**
 * Defines Callback function for healtcheck procedure
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectHealthCheckCB) (SibObject* source,
					gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive healthcheck signals
 */
#define SIB_OBJECT_SIGNAL_HEALTHCHECK "healthcheck"

/**
 * Defines Callback function for register SIB
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectRegisterSibCB) (SibObject* source,
					SibObjectHandle* handle,
					gchar *uri,
					gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive "register_sib" signals
 */
#define SIB_OBJECT_SIGNAL_REGISTER_SIB "register_sib"

/**
 * Defines Callback function 
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectJoinCnfCB) (SibObject* source,
				    guchar *spaceid,
				    guchar *nodeid,
				    gint msgnum,
				    gint success,
				    guchar *credentials,
				    gpointer user_data);

#define SIB_OBJECT_SIGNAL_JOIN_CNF "join_cnf"

/**
 * Defines Callback function 
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectLeaveCnfCB) (SibObject* source,
				     guchar *spaceid,
				     guchar *nodeid,
				     gint msgnum,
				     gint success,
				     gpointer user_data);

#define SIB_OBJECT_SIGNAL_LEAVE_CNF "leave_cnf"


/**
 * Defines Callback function 
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectInsertCnfCB) (SibObject* source,
				      guchar *spaceid,
				      guchar *nodeid,
				      gint msgnum,
				      gint success,
				      guchar *bNodes,
				      gpointer user_data);

#define SIB_OBJECT_SIGNAL_INSERT_CNF "insert_cnf"

/**
 * Defines Callback function 
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectRemoveCnfCB) (SibObject* source,
				      guchar *spaceid,
				      guchar *nodeid,
				      gint msgnum,
				      gint success,
				      gpointer user_data);

#define SIB_OBJECT_SIGNAL_REMOVE_CNF "removet_cnf"

/**
 * Defines Callback function 
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectUpdateCnfCB) (SibObject* source,
				      guchar *spaceid,
				      guchar *nodeid,
				      gint msgnum,
				      gint success,
				      guchar *bNodes,
				      gpointer user_data);

#define SIB_OBJECT_SIGNAL_UPDATE_CNF "update_cnf"

/**
 * Defines Callback function 
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectQueryCnfCB) (SibObject* source,
					 guchar *spaceid,
					 guchar *nodeid,
					 gint msgnum,
					 gint success,
					 guchar *results,
					 gpointer user_data);

#define SIB_OBJECT_SIGNAL_QUERY_CNF "query_cnf"

/**
 * Defines Callback function 
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectSubscribeCnfCB) (SibObject* source,
					 guchar *spaceid,
					 guchar *nodeid,
					 gint msgnum,
					 gint success,
					 guchar *subscription_id,
					 guchar *results,
					 gpointer user_data);

#define SIB_OBJECT_SIGNAL_SUBSCRIBE_CNF "subscribe_cnf"

/**
 * Defines Callback function 
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectSubscriptionIndCB) (SibObject* source,
					    guchar *spaceid,
					    guchar *nodeid,
					    gint msgnum,
					    gint seqnum,
					    guchar *subscription_id,
					    guchar *results_new,
					    guchar *results_obsolete,
					    gpointer user_data);

#define SIB_OBJECT_SIGNAL_SUBSCRIPTION_IND "subscription_ind"

/**
 * Defines Callback function 
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectUnsubscribeCnfCB) (SibObject* source,
					   guchar *spaceid,
					   guchar *nodeid,
					   gint msgnum,
					   gint success,
					   guchar *subid,
					   gpointer user_data);

#define SIB_OBJECT_SIGNAL_UNSUBSCRIBE_CNF "unsubscribe_cnf"

/**
 * Defines Callback function 
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectUnsubscribeIndCB) (SibObject* source,
					   guchar *spaceid,
					   guchar *nodeid,
					   gint msgnum,
					   gint status,
					   guchar *subscription_id,
					   gpointer user_data);

#define SIB_OBJECT_SIGNAL_UNSUBSCRIBE_IND "unsubscribe_ind"

/**
 * Defines Callback function 
 *
 * @param source The signaling SibObject instance
 * @param user_data Optional userdata pointer
 */
typedef void (*SibObjectLeaveIndCB) (SibObject* source,
				     guchar *spaceid,
				     guchar *nodeid,
				     gint msgnum,
				     gint status,
				     gpointer user_data);

#define SIB_OBJECT_SIGNAL_LEAVE_IND "leave_ind"

/**
 * Get the GLib type of SibObject
 */
GType sib_object_get_type();

/**
 * Create a new SibObject instance.
 * Unique ids for service and control channel can be same, but if multiple virtual
 * sibs are created from on process, only one control channel is required.
 *
 * @param serviceid Unique id for this sib-access component
 * @param controlid Unique id for this control channel component
 * @param main_context A pre-allocated GMainContext (usually obtained with
 *                    g_main_context_default(), unless you know better)
 * @param friendly_name Friendly
 * @param description Description of the SIB instance.  NOT IMPLEMENTED
 * @return Pointer to a new SibObject instance
 */
GObject *sib_object_new(gchar* serviceid,
			gchar* controlid,
			GMainContext* main_context,
			gchar* friendly_name,
			gchar* description);


/**
 * Get the SibObject's mainloop, you can let sib_object to create the
 * mainloop, then fetch it and run it externally without explicitly calling
 * sib_object_start.
 *
 * @param self The SibObject, whose mainloop to get
 * @return Pointer to the main loop
 */
GMainContext *sib_object_get_main_context(SibObject *self);

/**
 * Get the UUID of an SibObject instance
 *
 * @param self A SibObject instance
 * @return A string containing the instance UUID (must NOT be freed)
 */
const gchar *sib_object_get_uuid(SibObject *self);


void sib_object_send_register_sib_return(SibObjectHandle *handle, gint ret);

gint sib_object_send_join(SibObject *self, guchar *spaceid, guchar *nodeid, gint msgnum, guchar *credentials);
gint sib_object_send_leave(SibObject *self, guchar *spaceid, guchar *nodeid, gint msgnum);
gint sib_object_send_insert(SibObject *self, guchar *spaceid, guchar *nodeid, gint msg, EncodingType encoding, guchar *insert_graph);

gint sib_object_send_remove(SibObject *self, guchar *spaceid, guchar *nodeid, gint msg, EncodingType encoding, guchar *remove_graph);
gint sib_object_send_update(SibObject *self, guchar *spaceid, guchar *nodeid, gint msg, EncodingType encoding, guchar *insert_graph, guchar *remove_graph);
gint sib_object_send_query(SibObject *self, guchar *spaceid, guchar *nodeid, gint msg, QueryType type, guchar *query);
gint sib_object_send_subscribe(SibObject *self, guchar *spaceid, guchar *nodeid, gint msg, QueryType type, guchar *query);
gint sib_object_send_unsubscribe(SibObject *self, guchar *spaceid, guchar *nodeid, gint msg, guchar *subscription_id);

G_END_DECLS

#endif
