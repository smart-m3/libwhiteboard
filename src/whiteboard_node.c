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
 * whiteboard_node.c
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
#include "whiteboard_node.h"
#include "whiteboard_dbus_ifaces.h"
#include "whiteboard_log.h"
#include "whiteboard_marshal.h"
#include "whiteboard_command.h"
#include "m3_sib_tokens.h"
#include "sibmsg.h"

#if 1
//rb <======================================================= check this..
char * sampleSparqlresults = 
"<sparql_results>"

"  <head>"
"    <variable name=\"contact\"/>"
"    <variable name=\"name\"/>"
"  </head>"

"  <results>"

"    <result>" 
"      <binding name=\"contact\">"
"	<uri>1828791f-4050-4502-99d4-98a5918fg6ac</uri>"
"      </binding>"
"      <binding name=\"name\">"
"	<literal><![CDATA[Charlie Brown]]></literal>"
"      </binding>"
"    </result>"

"    <result> "
"      <binding name=\"contact\">"
"	<uri>http://www.w3.org/1999/02/22-rdf-syntax-ns#Property</uri>"
"      </binding>"
"    </result>"

"    <result> "
"    </result>"

"    <result> "
"      <binding name=\"name\">"
"	<literal><![CDATA[George]]></literal>"
"      </binding>"
"    </result>"

"  </results>"
"</sparql_results>";
#endif

/*****************************************************************************
 * Structure definitions
 *****************************************************************************/

#define SUBSCRIBE_FLAGS_SUBSCRIBE   (1)
#define SUBSCRIBE_FLAGS_UNSUBSCRIBE (1 << 1)

typedef struct _SubscriptionData
{
  gchar *subscription_id;
  gint update_sequence;
  QueryType type;
  union {
    WhiteBoardNodeSubscriptionIndTemplateCB s_template;
    WhiteBoardNodeSubscriptionIndWQLvaluesCB s_wql_values;
    WhiteBoardNodeQSCommonWQLbooleanCB q_wql_boolean;
    WhiteBoardNodeQueryWQLnodelistCB q_wql_values, q_wql_types;
    WhiteBoardNodeQueryTemplateCB q_template;
    WhiteBoardNodeQuerySPARQLselectCB q_sparql_select;
    WhiteBoardNodeSubscriptionIndSPARQLselectCB s_sparql_select;
  } cb;
  gint flags;
  gpointer user_data;
  GHashTable *prefix_ns_map;
  GSList **selectedVariables;//for sparql select query
} SubscriptionData;

struct _WhiteBoardNode
{
  GObject parent;

  gchar *uuid;     // Object identifier 
  gchar *sib; /* URI of the SIB after join, NULL otherwise */
  gboolean joined;
  gint msgnumber;
  GHashTable *subscription_map; // subscription_id -> SubscriptionData
  DBusConnection *connection;

  GMutex *lock;
  
  GMainContext *main_context;
};

struct _WhiteBoardNodeClass
{
  GObjectClass parent;

  //  WhiteBoardNodeSIBInsertedCB sib_inserted_cb;
  //WhiteBoardNodeSIBRemovedCB sib_removed_cb;
  WhiteBoardNodeJoinCompleteCB join_complete_cb;
  WhiteBoardNodeUnsubscribeCompleteCB unsubscribe_complete_cb;
  
  /* Custom command */
  WhiteBoardNodeCustomCommandRequestCB custom_command_request_cb;
  WhiteBoardNodeCustomCommandResponseCB custom_command_response_cb;

  /* Log callback function pointers */
  WhiteBoardLogMessageCB log_message_cb;
};

enum
  {
    SIGNAL_SIB,
    SIGNAL_SIB_INSERTED,
    SIGNAL_SIB_REMOVED,
    SIGNAL_JOIN_COMPLETE,
    SIGNAL_UNSUBSCRIBE_COMPLETE,
    
    SIGNAL_CUSTOM_COMMAND_REQUEST,
    SIGNAL_CUSTOM_COMMAND_RESPONSE,
  
    SIGNAL_LOG_MESSAGE,
  
    NUM_SIGNALS
  };

/*****************************************************************************
 * Private function prototypes
 *****************************************************************************/
static gboolean invalidTriple (ssTriple_t *t, gboolean patternMatching);

static void whiteboard_node_finalize(GObject* object);

static gboolean whiteboard_node_register(WhiteBoardNode* self);

static void whiteboard_node_class_init(WhiteBoardNodeClass *self);
static gboolean whiteboard_node_joined(WhiteBoardNode *self);
static SubscriptionData *whiteboard_node_get_subscription_data(WhiteBoardNode *self, gint access_id);

static gboolean whiteboard_node_add_subscription_data(WhiteBoardNode *self, gint access_id, SubscriptionData *sd);

static gboolean whiteboard_node_remove_subscription_data(WhiteBoardNode *self, gint access_id);

static guint whiteboard_node_signals[NUM_SIGNALS];

static void whiteboard_node_class_init(WhiteBoardNodeClass *self)
{
  GObjectClass* object = G_OBJECT_CLASS(self);

  g_return_if_fail(self != NULL);

  // object->constructor = whiteboard_node_constructor;
  // object->dispose = whiteboard_node_dispose;
  object->finalize = whiteboard_node_finalize;


  /* NODE_ACCESS */
  /* Source */
  whiteboard_node_signals[SIGNAL_JOIN_COMPLETE] =
    g_signal_new(WHITEBOARD_NODE_SIGNAL_JOIN_COMPLETE,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardNodeClass, join_complete_cb),
		 NULL,
		 NULL,
		 marshal_VOID__INT,
		 G_TYPE_NONE,
		 1,
		 G_TYPE_INT);

  whiteboard_node_signals[SIGNAL_UNSUBSCRIBE_COMPLETE] =
    g_signal_new(WHITEBOARD_NODE_SIGNAL_UNSUBSCRIBE_COMPLETE,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardNodeClass, unsubscribe_complete_cb),
		 NULL,
		 NULL,
		 marshal_VOID__INT_INT,
		 G_TYPE_NONE,
		 2,
		 G_TYPE_INT,
		 G_TYPE_INT);
		
  /* Custom command */
  whiteboard_node_signals[SIGNAL_CUSTOM_COMMAND_REQUEST] =
    g_signal_new(WHITEBOARD_NODE_SIGNAL_CUSTOM_COMMAND_REQUEST,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardNodeClass, custom_command_request_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER,
		 G_TYPE_NONE,
		 1,
		 G_TYPE_POINTER);

  whiteboard_node_signals[SIGNAL_CUSTOM_COMMAND_RESPONSE] =
    g_signal_new(WHITEBOARD_NODE_SIGNAL_CUSTOM_COMMAND_RESPONSE,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardNodeClass, custom_command_response_cb),
		 NULL,
		 NULL,
		 marshal_VOID__POINTER,
		 G_TYPE_NONE,
		 1,
		 G_TYPE_POINTER);

  /* Logging */
  whiteboard_node_signals[SIGNAL_LOG_MESSAGE] =
    g_signal_new(WHITEBOARD_NODE_SIGNAL_LOG_MESSAGE,
		 G_OBJECT_CLASS_TYPE(object),
		 G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		 G_STRUCT_OFFSET(WhiteBoardNodeClass, log_message_cb),
		 NULL,
		 NULL,
		 marshal_VOID__INT_INT_STRING,
		 G_TYPE_NONE,
		 3,
		 G_TYPE_INT,
		 G_TYPE_INT,
		 G_TYPE_STRING);
  
}

static void whiteboard_node_init(GTypeInstance* instance, gpointer g_class)
{
  WhiteBoardNode* self = WHITEBOARD_NODE(instance);

  g_return_if_fail(self != NULL);
  g_return_if_fail(IS_WHITEBOARD_NODE(instance));
}

/*****************************************************************************
 * Private static functions
 *****************************************************************************/

static DBusConnection* whiteboard_node_validate_connection(WhiteBoardNode* self)
{
  whiteboard_log_debug_fb();

  if (!dbus_connection_get_is_connected (self->connection))
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
			    "Disconnected from WhiteBoard Daemon! " \
			    "Re-connecting.\n");
       whiteboard_log_debug_fe();
       return NULL;
#if 0
      while (whiteboard_node_register(self) == FALSE)
	{
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
				"WhiteBoardNode registration " \
				"failed. Sleeping.\n");
	  g_usleep(WHITEBOARD_REGISTRATION_TIMEOUT);
	}
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER, 
			    "WhiteBoardNode registered successfully " \
			    "with WhiteBoard Daemon.\n");
#endif
    }

  whiteboard_log_debug_fe();
  return self->connection;
}

static void whiteboard_node_custom_command_response(WhiteBoardNode* self,
						    DBusMessage* msg)
{
  WhiteBoardCmdResponse* response = NULL;

  response = (WhiteBoardCmdResponse*) whiteboard_cmd_new_with_msg(msg);

  g_signal_emit(self,
		whiteboard_node_signals[SIGNAL_CUSTOM_COMMAND_RESPONSE],
		0,
		response);

  whiteboard_cmd_unref(response);
}

static DBusHandlerResult whiteboard_node_sib_access_handler(DBusConnection *conn,
							    DBusMessage *msg,
							    gpointer data)
{
  WhiteBoardNode *self = (WhiteBoardNode *) data;
  const gchar* member = NULL;
  gint type = 0;

  whiteboard_log_debug_fb();
  
  member = dbus_message_get_member(msg);
  type = dbus_message_get_type(msg);
  switch (type)
    {
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
      {
	whiteboard_log_warning("Unknown method return: %s\n", member);
      }
      break;
      
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
      whiteboard_log_warning("Unknown method call: %d\n", type);

      break;
    case DBUS_MESSAGE_TYPE_SIGNAL:
      if (!strcmp(member, WHITEBOARD_DBUS_NODE_SIGNAL_SIB_INSERTED))
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
			whiteboard_node_signals[SIGNAL_SIB_INSERTED],
			0,
			uuid,
			name);
	}
      else if (!strcmp(member, WHITEBOARD_DBUS_NODE_SIGNAL_SIB_REMOVED))
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
			whiteboard_node_signals[SIGNAL_SIB_REMOVED],
			0,
			uuid,
			name);
	}
      else if (!strcmp(member, WHITEBOARD_DBUS_NODE_SIGNAL_JOIN_COMPLETE))
	{
	  dbus_int32_t access_id=0;
	  //	  gint msgstatus= -1;
	  ssStatus_t status;
	  whiteboard_util_parse_message(msg,
					DBUS_TYPE_INT32, &access_id,
					DBUS_TYPE_INT32, &status,
					WHITEBOARD_UTIL_LIST_END);

	  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				"Got join complete signal for accessid %d, status %d.\n", access_id, status);
	  
	  if(status && self->sib)
	    {
	      g_free(self->sib);
	      self->sib=NULL;
	      whiteboard_log_debug("Join failed, Deletin self->sib\n");
	    }
	  else
	    {
	      whiteboard_log_debug("Join ok, self->sib: %s\n", self->sib);
	      self->joined = TRUE;
	    }
	  
	  g_signal_emit(self,
			whiteboard_node_signals[SIGNAL_JOIN_COMPLETE],
			0,
			status);
	}
      else if (!strcmp(member, WHITEBOARD_DBUS_NODE_SIGNAL_SUBSCRIPTION_IND))
	{
	  gchar *results_added  = NULL;
	  gchar *results_removed  = NULL;
	  gchar *subscription_id = NULL;
	  gint access_id = -1;
	  gint update_sequence = 0;
	  ssStatus_t status;

	  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				"Got subscription ind signal.\n");

	  whiteboard_util_parse_message(msg,
					DBUS_TYPE_INT32, &access_id,
					DBUS_TYPE_INT32, &update_sequence,
					DBUS_TYPE_STRING, &subscription_id,
					DBUS_TYPE_STRING, &results_added,
					DBUS_TYPE_STRING, &results_removed,
					WHITEBOARD_UTIL_LIST_END);

	  //no status was passed - instead, the scription_id string NULL indicates an error
	  //which is lumped with checking of other passed message parameters
	  status = ((access_id > 0) && (NULL != subscription_id) && (NULL != results_added)  && (NULL != results_removed))?
	    ss_StatusOK : ss_InternalError;
	  if(!status)
	    {
	      SubscriptionData  *sb = NULL;
	      sb = whiteboard_node_get_subscription_data(self, access_id);

	      if (!sb || sb->cb.s_template==NULL)
		{
		  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					"SubscriptionData not found or callback not set. ID: %s.\n", subscription_id);
		}
	      else
		{
		  //verify sequence number, if used.  always 0 or 1,2..(SSAP_IND_WRAP_NUM-1),1,2..
		  if (update_sequence==0
		      && sb->update_sequence==0) //sb->update_sequence initialized with 0
		    sb->update_sequence = -1;//lock 0 as only acceptable update_sequence

		  if (sb->update_sequence!=-1 && ++sb->update_sequence==SSAP_IND_WRAP_NUM)
		    sb->update_sequence = 1;

		  if ((update_sequence!=sb->update_sequence)
		      && !(update_sequence==0 && sb->update_sequence == -1))
		    status = ss_IndicationSequenceError;//passed in callback and used to effect empty lists (no parsing)

		  if(sb->type == QueryTypeTemplate)
		    {
		      GSList **triples_added = (GSList **)g_new0(GSList *,1); // allocate space for pointer and make it an empty list

		      GSList **triples_removed = (GSList **)g_new0(GSList *,1); // allocate space for pointer and make it an empty list

		      if (!status
			  &&
			  ((status = parseM3_triples (triples_added, results_added, sb->prefix_ns_map))
			   ||
			   (status = parseM3_triples (triples_removed, results_removed, sb->prefix_ns_map))
			  )
			 )
			whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					      "error, when trying to generating triples from results\n");
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "Calling subscription ind callback. Id: %s\n", subscription_id);
		      sb->cb.s_template(status, triples_added, triples_removed, sb->user_data);
		    }
		  else if(sb->type == QueryTypeWQLValues )
		    {
		      GSList **nodelist_added = (GSList **)g_new0(GSList *,1); // allocate space for pointer
		      GSList **nodelist_removed = (GSList **)g_new0(GSList *,1); // allocate space for pointer		  
		      if (!status
			  &&
			  ((status = parseM3_query_cnf_wql (nodelist_added, results_added))
			   ||
			   (status = parseM3_query_cnf_wql (nodelist_removed, results_removed))
			  )
			 )
			whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
						"Parse error, when trying to generating nodelist from results\n");
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "Calling subscription ind callback. Id: %s\n", subscription_id);
		      sb->cb.s_wql_values( status, nodelist_added, nodelist_removed, sb->user_data);
		    }
		  else if(sb->type == QueryTypeWQLRelated)
		    {
		      gboolean resultVal=TRUE+FALSE+2; //hehe.. that will kill the unassigned-warning
		      if(status)
			whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					      "error passed before parse to generate node list from results\n");

		      if (!status)
			{
			  if (0==strcmp (results_added, results_removed))
			    {
			      status = ss_ParsingError; // I.E. we dont know and thus can't assign the result value
			      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
						    "Subscription results added and removed, both: %s.\n", results_added);
			      status = ss_ParsingError; // I.E. we could guess, but shouldn't.
			    }
			  else if (0==strcmp(SIB_TRUE.txt, results_added))
			    resultVal = TRUE;
			  else if (0==strcmp(SIB_FALSE.txt, results_added))
			    resultVal = FALSE;
			  else
			    status = ss_ParsingError; // I.E. we dont know and thus can't assign the result value
			}
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "Calling query WQL callback for\n");
		      sb->cb.q_wql_boolean(status, resultVal, sb->user_data);
		    }
		  else if(sb->type == QueryTypeSPARQLSelect)
		    {
		      GSList **valRows_added = (GSList **)g_new0(GSList *,1); // allocate space for pointer
		      GSList **valRows_removed = (GSList **)g_new0(GSList *,1); // allocate space for pointer		  
		      if (!status
			  &&
			  ((status = parseM3_query_results_sparql_select (sb->selectedVariables, valRows_added, results_added, sb->prefix_ns_map))
			   ||
			   (status = parseM3_query_results_sparql_select (sb->selectedVariables, valRows_removed, results_removed, sb->prefix_ns_map))
			  )
			 )
			whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
						"Parse error, when trying to generating nodelist from results\n");
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "Calling subscription ind callback. Id: %s\n", subscription_id);
		      sb->cb.s_sparql_select (status, valRows_added, valRows_removed, sb->user_data);
		    }
		  else
		    {
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "Invalid Query type\n");  
		    }
		}
	    }  
	}
      else if (!strcmp(member, WHITEBOARD_DBUS_NODE_SIGNAL_UNSUBSCRIBE_COMPLETE))
	{
	  dbus_int32_t msgstatus=-1;
	  dbus_int32_t access_id = -1;
	  gint status = -1;
	  gchar *subscription_id=NULL;
	  whiteboard_util_parse_message(msg,
					DBUS_TYPE_INT32, &access_id,
					DBUS_TYPE_INT32, &msgstatus,
					DBUS_TYPE_STRING, &subscription_id,
					WHITEBOARD_UTIL_LIST_END);
	  if( (access_id != -1) &&  (msgstatus != -1) && ( subscription_id))
	    whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				  "Got unsubscribe complete signal got access_id:%d, /w status: %d, subscription_id: %s.\n", access_id, msgstatus, subscription_id);
	  else
	    whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				  "Got unsubscribe complete signal, but could not parse paratemers\n");
	  
	  if(msgstatus == MSG_E_OK)
	    {
	      status = 0;
	      whiteboard_node_remove_subscription_data(self, access_id);
	  
	      g_signal_emit(self,
			    whiteboard_node_signals[SIGNAL_UNSUBSCRIBE_COMPLETE],
			    0,
			    access_id,
			    status);
	    }
	  else
	    {
	      status = -1;
	      g_signal_emit(self,
			    whiteboard_node_signals[SIGNAL_UNSUBSCRIBE_COMPLETE],
			    0,
			    access_id,
			    status);
	    }
	}
      else if (!strcmp(member, WHITEBOARD_DBUS_NODE_METHOD_SUBSCRIBE))
	{
	  gint access_id = -1;
	  ssStatus_t status = -1;
	  gchar *subscription_id = NULL;
	  gchar *results = NULL;
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				"Got SUBSCRIBE method return.\n");
	  
	  whiteboard_util_parse_message(msg,
					DBUS_TYPE_INT32, &access_id,
					DBUS_TYPE_INT32, &status,
					DBUS_TYPE_STRING, &subscription_id,
					DBUS_TYPE_STRING, &results,
					WHITEBOARD_UTIL_LIST_END);
	  if( (access_id > 0) && (status == ss_StatusOK) && (NULL != subscription_id) && (NULL != results) )
	    {
	      SubscriptionData  *sb = NULL;
	      sb = whiteboard_node_get_subscription_data(self, access_id);
	      if (sb && sb->cb.s_template)
		{
		  //?? is the allocated subscription_id used? freed?
		  sb->subscription_id = g_strdup(subscription_id);
		  sb->flags = 0;
		}
	      else
		sb=NULL;

	      if (sb==NULL)
		{
		  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					"SubscriptionData not found or callback not set. ID: %s.\n", subscription_id);
		}
	      else if(sb->type == QueryTypeTemplate)
		{
		  GSList **initial_triples = (GSList **)g_new0(GSList *,1); // allocate space for pointer and make it an empty list
		  GSList **no_triples = (GSList **)g_new0(GSList *,1); // allocate space for pointer and make it an empty list

		  if(status)
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "error passed before parse to generate triples from results\n");
		  if (!status && /*and NOW changes*/
		      (status = parseM3_triples (initial_triples, results, sb->prefix_ns_map)))
		    whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					  "error, when trying to generating triples from results\n");
		  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					"Calling subscription ind callback. Id: %s\n", subscription_id);
		  sb->cb.s_template( status, initial_triples, no_triples, sb->user_data);
		}
	      else if (sb->type == QueryTypeSPARQLSelect)

		{
		  GSList **selectedVariables  = (GSList **)g_new0(GSList *,1); // allocate space for pointer and make it an empty list
		  GSList **valRows = (GSList **)g_new0(GSList *,1); // allocate space for pointer and make it an empty list

		  if(status)
		    whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					  "error passed before parse to generate results\n");
		  if(!status && /*and NOW changes*/
		     (status = parseM3_query_results_sparql_select (selectedVariables, valRows, results, sb->prefix_ns_map)))
		    whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					  "Parse error, when trying to generating nodelist from results\n");
		      
		  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					"Calling query WQL callback for\n");
		  sb->cb.q_sparql_select(status, selectedVariables, valRows, sb->user_data);
		}
	      else if(sb->type == QueryTypeWQLValues )
		{
		  GSList **nodelist = (GSList **)g_new0(GSList *,1); // allocate space for pointer and make it an empty list.
		  GSList **nullList = (GSList **)g_new0(GSList *,1); // allocate space for pointer and make it an empty list.

		  ssStatus_t status = parseM3_query_cnf_wql (nodelist, results);
		  if( status == ss_StatusOK )
		    {
		      if( (NULL != sb) &&
			  (NULL != sb->cb.s_wql_values) )
			{
			  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
						"Calling subscription ind callback. Id: %s\n", subscription_id);
			  sb->cb.s_wql_values( 0, nodelist, nullList, sb->user_data);
			}
		      else
			{
			  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
						"SubscriptionData not found or callback not set. ID: %s.\n", subscription_id);
			}
		    }
		  else
		    {
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "Parse error, when trying to generating nodelist from results\n");
		    }
		}
	      else if(sb->type == QueryTypeWQLRelated)
		{
		  gboolean resultVal=TRUE+FALSE+2; //hehe.. that will kill the unassigned-warning
		  if(status)
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "error passed before parse to generate node list from results\n");

		  if (!status)
		    {
		      if (0==strcmp(SIB_TRUE.txt, results))
			resultVal = TRUE;
		      else if (0==strcmp(SIB_FALSE.txt, results))
			resultVal = FALSE;
		      else
			status = ss_ParsingError; // I.E. we dont know and thus can't assign the result value
		    }
		  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					"Calling query WQL callback for\n");
		  sb->cb.q_wql_boolean(status, resultVal, sb->user_data);
		}
	      else
		{
		  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					"Invalid query type in subcriptionData\n");
		}
	    }  
	  else
	    {
	      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				    "Could not get subscribe response parameters\n"); 
	    }
	   
	  
	}
      else if (!strcmp(member, WHITEBOARD_DBUS_NODE_METHOD_QUERY))
	{
	  gint access_id = -1;
	  ssStatus_t status;
	  gchar *results = NULL;
	  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				"Got QUERY method return.\n");
	  
	  if(whiteboard_util_parse_message(msg,
					   DBUS_TYPE_INT32, &access_id,
					   DBUS_TYPE_INT32, &status,
					   DBUS_TYPE_STRING, &results,
					   WHITEBOARD_UTIL_LIST_END))
	    {	      
	      status = ( (status == ss_StatusOK) && (access_id > 0) && (NULL != results) )? ss_StatusOK : ss_InternalError;
#if 0 //testing sparql results
	      results = sampleSparqlresults;
	      status = ss_StatusOK;
#endif
	      if (status == ss_StatusOK)
		{
		  SubscriptionData  *sb = NULL;
		  sb = whiteboard_node_get_subscription_data(self, access_id);
		  //the call back should be made with or without triples at the first detectable failure
		  if (sb == NULL)
		    status = ss_NotEnoughResources;
		  else if (sb->cb.q_template == NULL) 
		    status = ss_InternalError; //it was a required parameter!
		  
		  if(status)
		    {
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "SubscriptionData not found or callback not set.\n");
		    }
		  else if(sb->type == QueryTypeTemplate)
		    { //noted *25.3.2010 - status must be 0, due to check above, - this block works, but was intended to take also non-zero status
		      GSList **triples = (GSList **)g_new0(GSList *,1); // allocate space for pointer and make it an empty list

		      if(status)
			whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					      "error passed before parse to generate triple list from results\n");
		      if(!status && /*and NOW changes*/
			 (status = parseM3_triples (triples, results, sb->prefix_ns_map)))
			whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					      "error, when trying to generating triples from results\n");
		      
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "Calling Query template callback\n");
		      sb->cb.q_template(status, triples, sb->user_data);
		    }
		  else if (sb->type == QueryTypeSPARQLSelect)
		    { //noted *25.3.2010
		      GSList **selectedVariables  = (GSList **)g_new0(GSList *,1); // allocate space for pointer and make it an empty list
		      GSList **valRows = (GSList **)g_new0(GSList *,1); // allocate space for pointer and make it an empty list

		      if(status)
			whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					      "error passed before parse to generate node list from results\n");
		      if(!status && /*and NOW changes*/
			 (status = parseM3_query_results_sparql_select (selectedVariables, valRows, results, sb->prefix_ns_map)))
			whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					      "Parse error, when trying to generating nodelist from results\n");
		      
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "Calling query WQL callback for\n");
		      sb->cb.q_sparql_select(status, selectedVariables, valRows, sb->user_data);
		      
		    }
		  else if ((sb->type == QueryTypeWQLValues) ||
			   (sb->type == QueryTypeWQLNodeTypes) )
		    { //noted *25.3.2010
		      GSList **nodelist = (GSList **)g_new0(GSList *,1); // allocate space for pointer and make it an empty list

		      if(status)
			whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					      "error passed before parse to generate node list from results\n");
		      if(!status && /*and NOW changes*/
			 (status = parseM3_query_cnf_wql (nodelist, results)))
			whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					      "Parse error, when trying to generating nodelist from results\n");
		      
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "Calling query WQL callback for\n");
		      sb->cb.q_wql_values(status, nodelist, sb->user_data);
		      
		    }
		  else if(sb->type == QueryTypeWQLRelated ||
			  sb->type == QueryTypeWQLIsType ||
			  sb->type == QueryTypeWQLIsSubType)
		    { //noted *25.3.2010
		      gboolean resultVal=TRUE+FALSE+2; //hehe.. that will kill the unassigned-warning
		      if(status)
			whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					      "error passed before parse to generate node list from results\n");
		      
		      if (!status)
			{
			  if (0==strcmp(SIB_TRUE.txt, results))
			    resultVal = TRUE;
			  else if (0==strcmp(SIB_FALSE.txt, results))
			    resultVal = FALSE;
			  else
			    status = ss_ParsingError; // I.E. we dont know and thus can't assign the result value
			}
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "Calling query WQL callback for\n");
		      sb->cb.q_wql_boolean(status, resultVal, sb->user_data);
		    }
		  else
		    {
		      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					    "Invalid query type\n");
		    }
		  // remove subscription id struct...
		  whiteboard_node_remove_subscription_data(self, access_id);
		  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					"SubscriptionData removed\n");
		}  
	      else
		{
		  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
					"Error with query, access_id: %d,status %d\n", access_id, status); 
		  if(access_id > 0)
		    {
		      SubscriptionData  *sb = NULL;
		      sb = whiteboard_node_get_subscription_data(self, access_id);
		      //the call back should be made with or without triples at the first detectable failure
		      if (sb  && sb->cb.q_template )
			{
			  switch(sb->type)
			    {
			    case QueryTypeWQLRelated:
			    case QueryTypeWQLIsType:
			    case QueryTypeWQLIsSubType:
			      sb->cb.q_wql_boolean(status, FALSE, sb->user_data);
			      break;
			    case QueryTypeTemplate:
			      sb->cb.q_template(status, NULL, sb->user_data);
			      break;
			    case QueryTypeWQLValues:
			    case QueryTypeWQLNodeTypes:
			      sb->cb.q_wql_values( status, NULL, sb->user_data);
			      break;
			    default:
			      break;
			    }
			}
		      else
			{
			  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
						"SubscriptionData not found or callback not set.\n");
			}
		    }
		}
	    }
	  else
	    {
	      whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				    "Could not get query response parameters\n"); 		  
	    }
	}
      else
	{
	  whiteboard_log_warning("Unknown signal type: %d\n", type);
	}
      break;
    default:
      
      whiteboard_log_warning("Unknown message type: %d\n", type);
      
      break;
    }
  return DBUS_HANDLER_RESULT_HANDLED;
}

static gboolean whiteboard_node_joined(WhiteBoardNode *self)
{
  g_return_val_if_fail(self != NULL , FALSE);
  return ( (self->sib!= NULL) && self->joined);
}

static DBusHandlerResult whiteboard_node_control_handler(DBusConnection *conn,
							 DBusMessage *msg,
							 gpointer data)
{
  WhiteBoardNode *self = (WhiteBoardNode *) data;
  const gchar* member = NULL;
  gint type = 0;

  whiteboard_log_debug_fb();

  member = dbus_message_get_member(msg);
  type = dbus_message_get_type(msg);

  switch (type)
    {
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:

      if (!strcmp(member, WHITEBOARD_DBUS_METHOD_CUSTOM_COMMAND))
	{
	  whiteboard_node_custom_command_response(self, msg);
	}
      else
	{
	  whiteboard_log_warning("Unknown method return: %s\n",
				 member);
	}

      break;

    case DBUS_MESSAGE_TYPE_METHOD_CALL:

      if (!strcmp(member, WHITEBOARD_DBUS_METHOD_CUSTOM_COMMAND))
	{
	  // whiteboard_node_custom_command_request(self, msg);
	}
      else
	{
	  whiteboard_log_warning("Unknown method call: %s\n",
				 member);
	}

      break;

    case DBUS_MESSAGE_TYPE_SIGNAL:

      whiteboard_log_warning("Unknown signal: %s\n", member);

      break;

    default:

      whiteboard_log_warning("Unknown message type: %d\n", type);

      break;
    }

  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult whiteboard_node_log_handler(DBusConnection *conn,
						     DBusMessage *msg,
						     gpointer data)
{
  WhiteBoardNode *self = (WhiteBoardNode *) data;
  const gchar* member = NULL;
  gint type = 0;

  whiteboard_log_debug_fb();

  member = dbus_message_get_member(msg);
  type = dbus_message_get_type(msg);

  switch (type)
    {
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:

      whiteboard_log_warning("Unknown method return: %s\n", member);

      break;

    case DBUS_MESSAGE_TYPE_METHOD_CALL:

      whiteboard_log_warning("Unknown method call: %s\n", member);

      break;

    case DBUS_MESSAGE_TYPE_SIGNAL:

      if (!strcmp(member, WHITEBOARD_DBUS_LOG_SIGNAL_MESSAGE))
	{
	  gint level = 0;
	  gint id = 0;
	  gchar *message = NULL;

	  whiteboard_log_debugc(WHITEBOARD_DEBUG_NODE,
				"Got log message signal.\n");

	  whiteboard_util_parse_message(msg,
					DBUS_TYPE_INT32, &level,
					DBUS_TYPE_INT32, &id,
					DBUS_TYPE_STRING, &message,
					DBUS_TYPE_INVALID);

	  g_signal_emit(self,
			whiteboard_node_signals[SIGNAL_LOG_MESSAGE],
			0,
			level,
			id,
			message);
	}
      else
	{
	  whiteboard_log_warning("Unknown signal: %s\n", member);
	}

      break;

    default:

      whiteboard_log_warning("Unknown message type: %d\n", type);

      break;
    }

  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult whiteboard_node_dispatch_message(DBusConnection *conn,
							  DBusMessage *msg,
							  gpointer data)
{
  const gchar* interface = NULL;

  whiteboard_log_debug_fb();

  whiteboard_log_debug("%s %s %d\n", dbus_message_get_interface(msg), dbus_message_get_member(msg), dbus_message_get_type(msg));

  interface = dbus_message_get_interface(msg);
  if(interface)
    {
      if (!strcmp(interface, WHITEBOARD_DBUS_NODE_INTERFACE))
	{
	  whiteboard_log_debug("Got sib_access message\n");
	  whiteboard_node_sib_access_handler(conn, msg, data);
	}
      else if(!strcmp(interface, WHITEBOARD_DBUS_CONTROL_INTERFACE))
	{
	  whiteboard_log_debug("Got control message\n");
	  whiteboard_node_control_handler(conn, msg, data);
	}
      else if (!strcmp(interface, WHITEBOARD_DBUS_LOG_INTERFACE))
	{
	  whiteboard_log_debug("Got log message\n");
	  whiteboard_node_log_handler(conn, msg, data);
	}
      else
	{
	  whiteboard_log_warning("Unknown interface: %s\n", interface);
	}
    }
  else
    {
      whiteboard_log_warning("Interface not set\n");
    }
  whiteboard_log_debug_fe();

  return DBUS_HANDLER_RESULT_HANDLED;
}

static gboolean invalidTriple (ssTriple_t *t, gboolean patternMatching)
{
  if (patternMatching)
    return
         !t->subject
      || !t->predicate
      || !t->object
      || !(t->subjType == ssElement_TYPE_URI)
      || !(t->objType  == ssElement_TYPE_URI || (t->object != ssMATCH_ANY && t->objType == ssElement_TYPE_LIT));

  else
    return
         t->subject   == ssMATCH_ANY
      || t->predicate == ssMATCH_ANY
      || t->object    == ssMATCH_ANY
      || t->predicate == NULL
      || !(t->subjType == ssElement_TYPE_BNODE || (t->subject &&  t->subjType == ssElement_TYPE_URI))
      || !(t->objType  == ssElement_TYPE_BNODE || (t->object  && (t->objType  == ssElement_TYPE_URI ||
                                                                  t->objType  == ssElement_TYPE_LIT)));
}

/*****************************************************************************
 * Initialization
 *****************************************************************************/

GType whiteboard_node_get_type()
{
  static GType type = 0;

  whiteboard_log_debug_fb();

  if (!type)
    {
      static const GTypeInfo info =
	{
	  sizeof(WhiteBoardNodeClass),
	  NULL,           /* base_init */
	  NULL,           /* base_finalize */
	  (GClassInitFunc) whiteboard_node_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof(WhiteBoardNode),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) whiteboard_node_init
	};

      type = g_type_register_static(G_TYPE_OBJECT,
				    "WhiteBoardNodeType", &info, 0);
    }

  whiteboard_log_debug_fe();

  return type;
}

GObject *whiteboard_node_new(GMainContext *main_context)
{
  GObject* object = NULL;
  WhiteBoardNode *self = NULL;
  gchar tmp[37];
  uuid_t u1;
  
  whiteboard_log_debug_fb();

  //g_return_val_if_fail(username != NULL, NULL);

  object = g_object_new(WHITEBOARD_NODE_TYPE, NULL);
  if (object == NULL)
    {
      whiteboard_log_error("Out of memory!\n");
      return NULL;
    }

  uuid_generate(u1);
  self = WHITEBOARD_NODE(object);
  uuid_unparse(u1, tmp);
  self->uuid = g_strdup(tmp);

  self->sib = NULL; /* not joined initially */
  self->joined = FALSE;
  
  self->subscription_map = g_hash_table_new(g_direct_hash, g_direct_equal);

  self->lock = g_mutex_new();
  
  if (main_context != NULL)
    self->main_context = main_context;
  else
    self->main_context = g_main_context_default();

  /* Register with WhiteBoard */
  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			"Attempting to register the WhiteBoardNode with WhiteBoard Daemon.\n");
  while (whiteboard_node_register(self) == FALSE)
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

static gboolean whiteboard_node_register(WhiteBoardNode* self)
{
  GError *gerror = NULL;
  DBusGConnection *connection = NULL;
  DBusMessage *reply = NULL;
  dbus_int32_t register_success = -1;
  gchar *address = NULL;
  gboolean retval = FALSE;
  /*
    DBusObjectPathVTable vtable = { &whiteboard_node_unregister_handler,
    &whiteboard_node_dispatch_message,
    NULL, NULL, NULL, NULL };
  */

  whiteboard_log_debug_fb();

  /* Discovering private address */
  if (whiteboard_util_discover(&address) == FALSE)
    {
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			    "Discovery failed, using default address\n");
      address = g_strdup("unix:path=/tmp/dbus-test");
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
    }    
  else
    {
      /* Get the actual connection pointer */
      self->connection = (DBusConnection*)
	dbus_g_connection_get_connection(connection);
      
      /* Registering this UI */
      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
			    "Sending UI registration request\n");
      
      /* TODO: Switch back to whiteboard_util_send_method_with_reply,
	 when WhiteBoard actually sends something */
      whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					     WHITEBOARD_DBUS_OBJECT,
					     WHITEBOARD_DBUS_REGISTER_INTERFACE,
					     WHITEBOARD_DBUS_REGISTER_METHOD_NODE,
					     self->connection,
					     &reply,
					     DBUS_TYPE_STRING,
					     &self->uuid,
					     WHITEBOARD_UTIL_LIST_END);
      
      if(reply)
	{
	  if(!whiteboard_util_parse_message(reply, 
					    DBUS_TYPE_INT32, &register_success,
					    WHITEBOARD_UTIL_LIST_END) )
	    {
	      whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
				    "Could not register node (reply message parse error)\n");
	    }
	  else 
	    {
	      if(register_success < 0)
		{
		  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
					"Could not register node (error in reply message)\n");
		}
	      else
		{
		  
		  whiteboard_log_debugc(WHITEBOARD_DEBUG_DISCOVER,
					"Node registered\n");
		  retval = TRUE;
		}
	      dbus_message_unref(reply);
	    }
	}
      
      if(retval)
	{
	  dbus_connection_add_filter(self->connection, 
				     &whiteboard_node_dispatch_message, self, NULL);
	  
	  /*
	    dbus_connection_register_fallback(self->connection,
	    WHITEBOARD_OBJECT,
	    &vtable,
	    self);
	  */
	  
	  dbus_connection_setup_with_g_main(self->connection,
					    self->main_context);
	  
	  
	}
    }
  g_free(address);
  
  whiteboard_log_debug_fe();
  
  return retval;
}

static void whiteboard_node_finalize(GObject* object)
{
  WhiteBoardNode* self = WHITEBOARD_NODE(object);

  whiteboard_log_debug_fb();

  g_return_if_fail(self != NULL);

  if( whiteboard_node_joined(self) )
    {
      whiteboard_node_sib_access_leave(self);
    }

  whiteboard_util_send_signal(WHITEBOARD_DBUS_OBJECT,
			      WHITEBOARD_DBUS_REGISTER_INTERFACE,
			      WHITEBOARD_DBUS_REGISTER_SIGNAL_UNREGISTER_NODE,
			      whiteboard_node_validate_connection(self),
			      DBUS_TYPE_STRING, &self->uuid,
			      WHITEBOARD_UTIL_LIST_END);
  
  self->main_context = NULL;

  dbus_connection_unref(self->connection);
  self->connection = NULL;

  g_free(self->uuid);
  self->uuid = NULL;

  if(self->sib)
    {
      g_free(self->sib);
      self->sib = NULL;
    }

  g_hash_table_destroy(self->subscription_map);
  
  whiteboard_log_debug_fe();
}

GMainContext *whiteboard_node_get_main_context(WhiteBoardNode *self)
{
  g_return_val_if_fail(self != NULL, NULL);
  return self->main_context;
}

const gchar *whiteboard_node_get_uuid(WhiteBoardNode *self)
{
  if (NULL == self)
    return NULL;

  return (const gchar*) self->uuid;
}

gint whiteboard_node_sib_access_join(WhiteBoardNode *self,
				     ssElement_ct udn)
{
  dbus_int32_t join_id = -1;
  DBusMessage *reply = NULL;
  whiteboard_log_debug_fb();
  g_return_val_if_fail(NULL != self, ss_InvalidParameter);
  g_return_val_if_fail(NULL != udn, ss_InvalidParameter);
  
  g_mutex_lock(self->lock);

  gint msgnum = ++(self->msgnumber);
  const gchar *nodeid = whiteboard_node_get_uuid(self);
  if( self->sib )
    {
      whiteboard_log_debug("Node (%s) already joined or in the middle of join process with SS: %s\n",nodeid, self->sib);
    }
  else
    {
      whiteboard_log_debug("Node (%s) joining SS: %s\n",nodeid, udn);
      
      whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					     WHITEBOARD_DBUS_OBJECT,
					     WHITEBOARD_DBUS_NODE_INTERFACE,
					     WHITEBOARD_DBUS_NODE_METHOD_JOIN,
					     whiteboard_node_validate_connection(self),
					     &reply,
					     DBUS_TYPE_STRING, &nodeid,
					     DBUS_TYPE_STRING, &udn,
					     DBUS_TYPE_INT32, &msgnum,
					     WHITEBOARD_UTIL_LIST_END);
      if(reply)
	{
	  whiteboard_util_parse_message(reply,
					DBUS_TYPE_INT32, &join_id,
					WHITEBOARD_UTIL_LIST_END);
	  dbus_message_unref(reply);
	}
      if(join_id > 0)
	{
	  self->sib = g_strdup((char *)udn); 
	}
      whiteboard_log_debug("Join reply for sib: %s, accessid: %d\n", ((join_id > 0)? self->sib : (char *)udn), join_id);
    }
  g_mutex_unlock(self->lock);
  whiteboard_log_debug_fe();
  return ((join_id > 0) ? ss_StatusOK:ss_OperationFailed);
}

gint whiteboard_node_sib_access_leave(WhiteBoardNode *self)
{
  DBusMessage *reply = NULL;
  ssStatus_t status;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(NULL != self, ss_InvalidParameter);
  g_mutex_lock(self->lock);
  gint msgnum = ++(self->msgnumber);
  const gchar *nodeid = whiteboard_node_get_uuid(self);

  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) not joined, can not leave\n",nodeid);
      g_mutex_unlock(self->lock);
      return ss_InvalidParameter;
    }
  else
    {
      whiteboard_log_debug("Node (%s) leaving\n", nodeid);
      
      whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					     WHITEBOARD_DBUS_OBJECT,
					     WHITEBOARD_DBUS_NODE_INTERFACE,
					     WHITEBOARD_DBUS_NODE_METHOD_LEAVE,
					     whiteboard_node_validate_connection(self),
					     &reply,
					     DBUS_TYPE_STRING, &nodeid,
					     DBUS_TYPE_INT32, &msgnum,
					     WHITEBOARD_UTIL_LIST_END);
      
      status = (reply)? ss_StatusOK : ss_InternalError;
      if(!status)
	{
	  whiteboard_util_parse_message(reply,
					DBUS_TYPE_INT32, &status,
					WHITEBOARD_UTIL_LIST_END);
	  dbus_message_unref(reply);
	}
      g_free(self->sib);
      self->sib = NULL;
      self->joined = FALSE;
      whiteboard_log_debug("leave reply, success: %d\n", status);
    }
  g_mutex_unlock(self->lock);
  whiteboard_log_debug_fe();
  return status;
}


gint whiteboard_node_sib_access_insert_graph(WhiteBoardNode *self,
					     gchar *graph,
					     EncodingType encoding)
{

}



gint whiteboard_node_sib_access_insert_M3Triples(WhiteBoardNode *self,
						 GSList *triples,
						 const gchar *namespace) //TBD
{
  const gchar *insert_message;
  EncodingType encoding = EncodingM3XML;
  gint success = ss_InternalError;
  gint response_success = -1;
  gchar *response  = NULL;
  DBusMessage *reply=NULL;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail(triples != NULL && triples->data!=NULL, ss_InvalidParameter);
  g_mutex_lock(self->lock);
  const char *nodeid = whiteboard_node_get_uuid(self);
  gint msgnum = ++(self->msgnumber);
  ssBufDesc_t *bd = NULL;
  GHashTable *prefix_ns_map=NULL;

  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) not joined, can not insert triples\n",nodeid);
    }
  else
    {
      GSList *l=triples;
      ssTriple_t *t;
      ssStatus_t status;

      //initializing status//
      status = (!namespace)? ss_StatusOK : new_prefix2ns_map(namespace, &prefix_ns_map);
      //g_return_val_if_fail (status==ss_StatusOK, status);
      if(status != ss_StatusOK)
	{
	  g_mutex_unlock(self->lock);
	  return status;
	}

      bd = ssBufDesc_new();
      if (!bd) {
	if(prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
	{
	  g_mutex_unlock(self->lock);
	  return ss_NotEnoughResources;
	}
      }

      status = addXML_start (bd, &SIB_TRIPLELIST, NULL, NULL, 0);
      
      while (status==ss_StatusOK && l && (t=(ssTriple_t *)l->data) && !invalidTriple(t,FALSE)) {
	status = addXML_templateTriple(t, prefix_ns_map, (gpointer)bd);
	l=l->next;
      }

      if (status!=ss_StatusOK)
	success = status;
      else if (l!=NULL)
	success = ss_InvalidTripleSpecification;
      else {

	addXML_end (bd, &SIB_TRIPLELIST);
      
	insert_message = ssBufDesc_GetMessage(bd);

	printf("Insert graph: %s\n", insert_message);


	whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					       WHITEBOARD_DBUS_OBJECT,
					       WHITEBOARD_DBUS_NODE_INTERFACE,
					       WHITEBOARD_DBUS_NODE_METHOD_INSERT,
					       whiteboard_node_validate_connection(self),
					       &reply,
					       DBUS_TYPE_STRING, &nodeid,
					       DBUS_TYPE_STRING, &self->sib,
					       DBUS_TYPE_INT32, &msgnum,
					       DBUS_TYPE_INT32, &encoding,
					       DBUS_TYPE_STRING, &insert_message,
					       WHITEBOARD_UTIL_LIST_END);
	if(reply)
	  {
	    whiteboard_util_parse_message(reply,
					  DBUS_TYPE_INT32, &response_success,
					  DBUS_TYPE_STRING, &response,
					  WHITEBOARD_UTIL_LIST_END);
	    if(NULL != response)
	      {
		// TODO: real parsing of the response message
		whiteboard_log_debug("Insert response: %s\n", response);

		if(response_success != 0)
		  {
		    whiteboard_log_debug("Insert failed\n");
		    success = -1;
		  }
		else
		  {
		    whiteboard_log_debug("Insert Ok\n");
		    success = ss_StatusOK;
		  }
	      }
	    else
	      {
		whiteboard_log_debug("Invalid insert response\n");
		success = -1;
	      }
	    dbus_message_unref(reply);
	  }
	//ssBufDesc_free(&desc);
      }
      ssBufDesc_free(&bd);
      if(prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
    }
  g_mutex_unlock(self->lock);
  whiteboard_log_debug_fe();
  return success;
  
}

gint whiteboard_node_sib_access_update_M3Triples(WhiteBoardNode *self,
						 GSList *insert_triples,
						 GSList *remove_triples,
						 const gchar *namespace)
{
  gchar *insertlist = NULL;
  gchar *removelist = NULL;
  EncodingType encoding = EncodingM3XML;
  ssStatus_t status;
  gchar *response  = NULL;
  DBusMessage *reply=NULL;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail(insert_triples != NULL || remove_triples!=NULL, ss_InvalidParameter);
  g_return_val_if_fail(insert_triples == NULL || insert_triples->data!=NULL, ss_InvalidParameter);
  g_return_val_if_fail(remove_triples == NULL || remove_triples->data!=NULL, ss_InvalidParameter);
  g_mutex_lock(self->lock);  
  const gchar *nodeid = whiteboard_node_get_uuid(self);
  gint msgnum = ++(self->msgnumber);

  ssBufDesc_t *bd_insert = NULL;
  ssBufDesc_t *bd_remove = NULL;
  GHashTable *prefix_ns_map=NULL;

  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) has not joined, can not update triples\n",nodeid);
    }
  else
    {
      GSList *l;
      ssTriple_t *t;

      //initializing status//
      status = (!namespace)? ss_StatusOK : new_prefix2ns_map(namespace, &prefix_ns_map);
      if (status!=ss_StatusOK)
	{
	  g_mutex_unlock(self->lock);  
	  return status;
	}

      bd_insert = ssBufDesc_new();
      bd_remove = ssBufDesc_new();
      if (!bd_insert || !bd_remove) {
	if (!bd_insert) //the first succeeded
	  ssBufDesc_free(&bd_insert);
	if (prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
	
	  g_mutex_unlock(self->lock);
	  return ss_NotEnoughResources;
	  
      }

      status = addXML_start (bd_insert, &SIB_TRIPLELIST, NULL, NULL, 0);

      l = insert_triples;
      while (status==ss_StatusOK && l && (t=(ssTriple_t *)l->data) && !invalidTriple(t,FALSE)) {
	status = addXML_templateTriple(t, prefix_ns_map, (gpointer)bd_insert);
	l=l->next;
      }
      if (!status && l!=NULL)
	status = ss_InvalidTripleSpecification;

      status = (status)?status : addXML_start (bd_remove, &SIB_TRIPLELIST, NULL, NULL, 0);

      l = remove_triples;
      while (status==ss_StatusOK && l && (t=(ssTriple_t *)l->data) && !invalidTriple(t,TRUE)) {
	status = addXML_templateTriple(t, prefix_ns_map, (gpointer)bd_remove);
	l=l->next;
      }
      if (!status && l!=NULL)
	status = ss_InvalidTripleSpecification;

      status = (status)?status : addXML_end (bd_insert, &SIB_TRIPLELIST);
      status = (status)?status : addXML_end (bd_remove, &SIB_TRIPLELIST);
      insertlist = ssBufDesc_GetMessage(bd_insert);
      removelist = ssBufDesc_GetMessage(bd_remove);

      if (!status)
	whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					       WHITEBOARD_DBUS_OBJECT,
					       WHITEBOARD_DBUS_NODE_INTERFACE,
					       WHITEBOARD_DBUS_NODE_METHOD_UPDATE,
					       whiteboard_node_validate_connection(self),
					       &reply,
					       DBUS_TYPE_STRING, &nodeid,
					       DBUS_TYPE_STRING, &self->sib,
					       DBUS_TYPE_INT32, &msgnum,
					       DBUS_TYPE_INT32, &encoding,
					       DBUS_TYPE_STRING, &insertlist,
					       DBUS_TYPE_STRING, &removelist,
					       WHITEBOARD_UTIL_LIST_END);

      if(!status && reply)
	{
	  whiteboard_util_parse_message(reply,
					DBUS_TYPE_INT32, &status,
					DBUS_TYPE_STRING, &response,
					WHITEBOARD_UTIL_LIST_END);
	  if(NULL != response)
	    {
	      // TODO: real parsing of the response message
	      whiteboard_log_debug("update response: %s\n", response);
	      
	      if(status != ss_StatusOK)
		{
		  whiteboard_log_debug("update failed\n");
		}
	      else
		{
		  whiteboard_log_debug("Update Ok\n");
		}
	    }
	  else
	    {
	      whiteboard_log_debug("Invalid update response\n");
	      status = ss_OperationFailed;
	    }
	  dbus_message_unref(reply);
	}
      else
	{
	  whiteboard_log_debug("no update REPLY\n");
	  status = (status)?status : ss_OperationFailed;
	}

      ssBufDesc_free(&bd_insert);
      ssBufDesc_free(&bd_remove);
      if(prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
    }
  g_mutex_unlock(self->lock);
  whiteboard_log_debug_fe();
  return status;
}

gint whiteboard_node_sib_access_remove_M3Triples(WhiteBoardNode *self,
						 GSList *triples,
						 const gchar *namespace)
{
  const gchar *removelist = NULL;
  ssStatus_t status;
  gchar *response = NULL;
  DBusMessage *reply=NULL;

  whiteboard_log_debug_fb();

  ssBufDesc_t *bd = NULL;
  GHashTable *prefix_ns_map=NULL;

  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail(triples!=NULL && triples->data!=NULL, ss_InvalidParameter);
  g_mutex_lock(self->lock);
  const gchar *nodeid = whiteboard_node_get_uuid(self);
  gint msgnum = ++(self->msgnumber);
  EncodingType encoding = EncodingM3XML;
        
  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) not joined, can not remove triples\n",nodeid);
    }
  else
    {
      GSList *l;
      ssTriple_t *t;

      //initializing status//
      status = (!namespace)? ss_StatusOK : new_prefix2ns_map(namespace, &prefix_ns_map);
      if(status!=ss_StatusOK)
	{
	  g_mutex_unlock(self->lock);
	  return status;
	}
      bd = ssBufDesc_new();
      if (!bd) {
	if(prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
	g_mutex_unlock(self->lock);
	return ss_NotEnoughResources;
      }

      status = addXML_start (bd, &SIB_TRIPLELIST, NULL, NULL, 0);
      
      l = triples;
      while (status==ss_StatusOK && l && (t=(ssTriple_t *)l->data) && !invalidTriple(t,FALSE)) {
	status = addXML_templateTriple(t, prefix_ns_map, (gpointer)bd);
	l=l->next;
      }
      if (!status && l!=NULL)
	status = ss_InvalidTripleSpecification;

      status = (status)?status : addXML_end (bd, &SIB_TRIPLELIST);
      removelist = ssBufDesc_GetMessage(bd);

      if (!status)
	whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					       WHITEBOARD_DBUS_OBJECT,
					       WHITEBOARD_DBUS_NODE_INTERFACE,
					       WHITEBOARD_DBUS_NODE_METHOD_REMOVE,
					       whiteboard_node_validate_connection(self),
					       &reply,
					       DBUS_TYPE_STRING, &nodeid,
					       DBUS_TYPE_STRING, &self->sib,
					       DBUS_TYPE_INT32, &msgnum,
					       DBUS_TYPE_INT32, &encoding,
					       DBUS_TYPE_STRING, &removelist,
					       WHITEBOARD_UTIL_LIST_END);
      if(!status && reply)
	{
	  whiteboard_util_parse_message(reply,
					DBUS_TYPE_INT32, &status,
                                        //DBUS_TYPE_INT32, &success,
					DBUS_TYPE_STRING, &response,
					WHITEBOARD_UTIL_LIST_END);
	  dbus_message_unref(reply);
	}

      ssBufDesc_free(&bd);
      if(prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
    }
  whiteboard_log_debug("Remove operation %s (status=%d)\n", (status)?"failed":"succeeded", status);
  g_mutex_unlock(self->lock);
  whiteboard_log_debug_fe();
  return status;  
}

gint whiteboard_node_sib_access_query_template(WhiteBoardNode *self,
					       GSList* templates,
					       const gchar *namespace,
					       WhiteBoardNodeQueryTemplateCB cb,
					       gpointer data)
{
  gchar *subscribe_message = NULL;
  gint access_id = -1;
  QueryType type = QueryTypeTemplate;
  DBusMessage *reply=NULL;
  ssBufDesc_t *desc=NULL;
  GHashTable *prefix_ns_map=NULL;

  whiteboard_log_debug_fb();
  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail( cb != NULL ,ss_InvalidParameter);
  g_return_val_if_fail( templates != NULL , ss_InvalidParameter);
  g_mutex_lock(self->lock);
  const gchar *nodeid = whiteboard_node_get_uuid(self);
  gint msgnum = ++(self->msgnumber);
  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) not joined, can not create query\n",nodeid);
      g_mutex_unlock(self->lock);
      return ss_InvalidParameter;
    }
  else
    {
      GSList *l=templates;
      ssTriple_t *t;
      ssStatus_t status;

      //initializing status//
      status = (!namespace)? ss_StatusOK : new_prefix2ns_map(namespace, &prefix_ns_map);
      if (status!=ss_StatusOK)
	{
	  g_mutex_unlock(self->lock);
	  return status;
	}
      desc = ssBufDesc_new();
      if (!desc) {
	if (prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
	g_mutex_unlock(self->lock);
	return ss_NotEnoughResources;
      }

      status = addXML_start (desc, &SIB_TRIPLELIST, NULL, NULL, 0);

      while (status==ss_StatusOK && l && (t=(ssTriple_t *)l->data) && !invalidTriple(t,TRUE)) {
	status = addXML_templateTriple(t, prefix_ns_map, (gpointer)desc);
	l=l->next;
      }
      if (!status && l!=NULL)
	status = ss_InvalidTripleSpecification;

      status=(status)?status : addXML_end (desc, &SIB_TRIPLELIST);

      if (status) {
	ssBufDesc_free(&desc);
	if (prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
	g_mutex_unlock(self->lock);
	return status;
      }

      subscribe_message = ssBufDesc_GetMessage(desc);
      
      whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					     WHITEBOARD_DBUS_OBJECT,
					     WHITEBOARD_DBUS_NODE_INTERFACE,
					     WHITEBOARD_DBUS_NODE_METHOD_QUERY,
					     whiteboard_node_validate_connection(self),
					     &reply,
					     DBUS_TYPE_STRING, &nodeid,
					     DBUS_TYPE_STRING, &self->sib,
					     DBUS_TYPE_INT32, &msgnum,
					     DBUS_TYPE_INT32, &type,
					     DBUS_TYPE_STRING, &subscribe_message,
					     WHITEBOARD_UTIL_LIST_END);
      if(reply)
	{
	  whiteboard_util_parse_message(reply,
					DBUS_TYPE_INT32, &access_id,
					WHITEBOARD_UTIL_LIST_END);
	  if(access_id < 0)
	    {
	      whiteboard_log_debug("Could not create query..\n");
	    }
	  else
	    {
	      whiteboard_log_debug("Got query access_id:%d\n", access_id);
	      SubscriptionData *sd = NULL;
	      sd = g_new0(SubscriptionData, 1);
	      sd->cb.q_template = cb;
	      sd->prefix_ns_map = prefix_ns_map;
	      sd->user_data = data;
	      sd->type = QueryTypeTemplate;
	      if(!whiteboard_node_add_subscription_data(self, access_id, sd))
		{
		  whiteboard_log_debug("Could not add subscription data to subscription map\n");
		  if (prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
		  g_free(sd);
		}
	    }
	  dbus_message_unref(reply);
	}
      ssBufDesc_free(&desc);
    }
  g_mutex_unlock(self->lock);
  whiteboard_log_debug_fe();
  return (access_id > 0) ? ss_StatusOK : ss_GeneralError;
}

ssStatus_t whiteboard_node_sib_access_query_sparql_select(WhiteBoardNode *self,
							  GSList* select,
							  GSList* where,
							  GSList* optional_lists,
							  const gchar *namespace,
							  WhiteBoardNodeQuerySPARQLselectCB cb,
							  gpointer data)
{
  gchar *subscribe_message = NULL;
  gint access_id = -1;
  QueryType type = QueryTypeSPARQLSelect;
  DBusMessage *reply=NULL;
  ssBufDesc_t *desc=NULL;
  GHashTable *prefix_ns_map=NULL;

  whiteboard_log_debug_fb();
  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail( cb != NULL ,ss_InvalidParameter);
  g_return_val_if_fail( where != NULL || optional_lists != NULL, ss_InvalidParameter);
  g_return_val_if_fail( optional_lists == NULL || optional_lists->data != NULL, ss_InvalidParameter);

  g_mutex_lock(self->lock);
  const gchar *nodeid = whiteboard_node_get_uuid(self);
  gint msgnum = ++(self->msgnumber);
  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) not joined, can not create query\n",nodeid);
      g_mutex_unlock(self->lock);
      return ss_InvalidParameter;
    }
  else
    {
      GSList *l = select;
      ssTriple_t *t;
      ssStatus_t status;

      //initializing status//
      status = (!namespace)? ss_StatusOK : new_prefix2ns_map(namespace, &prefix_ns_map);
      if (status!=ss_StatusOK)
	{
	  g_mutex_unlock(self->lock);
	  return status;
	}
      desc = ssBufDesc_new();
      if (!desc) {
	if (prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
	g_mutex_unlock(self->lock);
	return ss_NotEnoughResources;
      }

      status = generateSPARQLSelectQueryString(desc, select, where, optional_lists, prefix_ns_map);

      if (status) {
	ssBufDesc_free(&desc);
	if (prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
	g_mutex_unlock(self->lock);
	return status;
      }

      subscribe_message = ssBufDesc_GetMessage(desc);
      
      whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					     WHITEBOARD_DBUS_OBJECT,
					     WHITEBOARD_DBUS_NODE_INTERFACE,
					     WHITEBOARD_DBUS_NODE_METHOD_QUERY,
					     whiteboard_node_validate_connection(self),
					     &reply,
					     DBUS_TYPE_STRING, &nodeid,
					     DBUS_TYPE_STRING, &self->sib,
					     DBUS_TYPE_INT32, &msgnum,
					     DBUS_TYPE_INT32, &type,
					     DBUS_TYPE_STRING, &subscribe_message,
					     WHITEBOARD_UTIL_LIST_END);
      if(reply)
	{
	  whiteboard_util_parse_message(reply,
					DBUS_TYPE_INT32, &access_id,
					WHITEBOARD_UTIL_LIST_END);
	  if(access_id < 0)
	    {
	      whiteboard_log_debug("Could not create query..\n");
	    }
	  else
	    {
	      whiteboard_log_debug("Got query access_id:%d\n", access_id);
	      SubscriptionData *sd = NULL;
	      sd = g_new0(SubscriptionData, 1);
	      sd->cb.q_template = cb;
	      sd->prefix_ns_map = prefix_ns_map;
	      sd->user_data = data;
	      sd->type = type;
	      if(!whiteboard_node_add_subscription_data(self, access_id, sd))
		{
		  whiteboard_log_debug("Could not add subscription data to subscription map\n");
		  if (prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
		  g_free(sd);
		}
	    }
	  dbus_message_unref(reply);
	}

      ssBufDesc_free(&desc);
    }
  g_mutex_unlock(self->lock);
  whiteboard_log_debug_fe();
  return (access_id > 0) ? ss_StatusOK : ss_GeneralError;
}

ssStatus_t whiteboard_node_sib_access_query_wql_isSubclass(WhiteBoardNode *self,
						     const ssPathNode_t *subclassNode,
						     const ssPathNode_t *superclassNode,
						     WhiteBoardNodeQSCommonWQLbooleanCB cb,
						     gpointer data)
{
  gchar *query = NULL;
  gint access_id = -1;
  QueryType type = QueryTypeWQLIsSubType;
  DBusMessage *reply=NULL;
  ssStatus_t status;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail(subclassNode != NULL && subclassNode->string!=NULL && 
		       (subclassNode->nodeType==ssElement_TYPE_URI || subclassNode->nodeType==ssElement_TYPE_LIT), ss_InvalidParameter);
  g_return_val_if_fail(superclassNode != NULL && superclassNode->string!=NULL && 
		       (superclassNode->nodeType==ssElement_TYPE_URI || superclassNode->nodeType==ssElement_TYPE_LIT), ss_InvalidParameter);
  g_mutex_lock(self->lock);
  const gchar *nodeid = whiteboard_node_get_uuid(self);
  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) has not joined, can not update triples\n",nodeid);
      g_mutex_unlock(self->lock);
      return ss_InvalidParameter;
    }

  gint msgnum = ++(self->msgnumber);

  ssBufDesc_t *bD = ssBufDesc_new();
  if (!bD) {
    g_mutex_unlock(self->lock);
    return ss_NotEnoughResources;
  }

  status = addXML_query_w_wql_n_n (bD, type, subclassNode, superclassNode);
  if (status) {
    ssBufDesc_free(&bD);
    g_mutex_unlock(self->lock);
    return status;
  }

  query = ssBufDesc_GetMessage(bD);
  whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					 WHITEBOARD_DBUS_OBJECT,
					 WHITEBOARD_DBUS_NODE_INTERFACE,
					 WHITEBOARD_DBUS_NODE_METHOD_QUERY,
					 whiteboard_node_validate_connection(self),
					 &reply,
					 DBUS_TYPE_STRING, &nodeid,
					 DBUS_TYPE_STRING, &self->sib,
					 DBUS_TYPE_INT32, &msgnum,
					 DBUS_TYPE_INT32, &type,
					 DBUS_TYPE_STRING, &query,
					 WHITEBOARD_UTIL_LIST_END);

  status = (reply)? ss_StatusOK : ss_InternalError;
  if(!status)
    {
      whiteboard_util_parse_message(reply,
				    DBUS_TYPE_INT32, &access_id,
				    WHITEBOARD_UTIL_LIST_END);
      status = (access_id < 0)? ss_InternalError : ss_StatusOK;
      if(status)
	{
	  whiteboard_log_debug("Could not create query..\n");
	}
      else
	{
	  whiteboard_log_debug("Got query access_id:%d\n", access_id);
	  SubscriptionData *sd = g_new0(SubscriptionData, 1);
	  if (!sd)
	    status = ss_NotEnoughResources;
	  else
	    {
	      sd->cb.q_wql_boolean = cb;
	      sd->user_data = data;
	      sd->type = type;
	      if(!whiteboard_node_add_subscription_data(self, access_id, sd))
		{
		  status = ss_InternalError;
		  whiteboard_log_debug("Could not add query data to callback map\n");
		  g_free(sd);
		}
	    }
	}
      dbus_message_unref(reply);
    }

  ssBufDesc_free(&bD);
  g_mutex_unlock(self->lock);
  whiteboard_log_debug_fe();
  return status;
}

ssStatus_t whiteboard_node_sib_access_query_wql_ofClass(WhiteBoardNode *self,
							const ssPathNode_t *pathNode,
							const ssPathNode_t *classNode,
							WhiteBoardNodeQSCommonWQLbooleanCB cb,
							gpointer data)
{
  gchar *query = NULL;
  gint access_id = -1;
  QueryType type = QueryTypeWQLIsType;
  DBusMessage *reply=NULL;
  ssStatus_t status;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail(pathNode != NULL && pathNode->string!=NULL && 
		       (pathNode->nodeType==ssElement_TYPE_URI || pathNode->nodeType==ssElement_TYPE_LIT), ss_InvalidParameter);
  g_return_val_if_fail(classNode != NULL && classNode->string!=NULL && 
		       classNode->nodeType==ssElement_TYPE_URI, ss_InvalidParameter);
  g_mutex_lock(self->lock);
  const gchar *nodeid = whiteboard_node_get_uuid(self);
  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) has not joined, can not update triples\n",nodeid);
      g_mutex_unlock(self->lock);
      return ss_InvalidParameter;
    }

  gint msgnum = ++(self->msgnumber);

  ssBufDesc_t *bD = ssBufDesc_new();
  if (!bD) {
    g_mutex_unlock(self->lock);
    return ss_NotEnoughResources;
  }

  status = addXML_query_w_wql_n_n (bD, type, pathNode, classNode);
  if (status) {
    ssBufDesc_free(&bD);
    g_mutex_unlock(self->lock);
    return status;
  }

  query = ssBufDesc_GetMessage(bD);
  whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					 WHITEBOARD_DBUS_OBJECT,
					 WHITEBOARD_DBUS_NODE_INTERFACE,
					 WHITEBOARD_DBUS_NODE_METHOD_QUERY,
					 whiteboard_node_validate_connection(self),
					 &reply,
					 DBUS_TYPE_STRING, &nodeid,
					 DBUS_TYPE_STRING, &self->sib,
					 DBUS_TYPE_INT32, &msgnum,
					 DBUS_TYPE_INT32, &type,
					 DBUS_TYPE_STRING, &query,
					 WHITEBOARD_UTIL_LIST_END);

  status = (reply)? ss_StatusOK : ss_InternalError;
  if(!status)
    {
      whiteboard_util_parse_message(reply,
				    DBUS_TYPE_INT32, &access_id,
				    WHITEBOARD_UTIL_LIST_END);
      status = (access_id < 0)? ss_InternalError : ss_StatusOK;
      if(status)
	{
	  whiteboard_log_debug("Could not create query..\n");
	}
      else
	{
	  SubscriptionData *sd = NULL;
	  sd = g_new0(SubscriptionData, 1);
	  if (!sd)
	    status = ss_NotEnoughResources;
	  else
	    {
	      sd->cb.q_wql_boolean = cb;
	      sd->user_data = data;
	      sd->type = QueryTypeWQLIsType;
	      if(!whiteboard_node_add_subscription_data(self, access_id,sd))
		{
		  status = ss_InternalError;
		  whiteboard_log_debug("Could not add query data to callback map\n");
		  g_free(sd);
		}
	    }
	}
      dbus_message_unref(reply);
    }

  ssBufDesc_free(&bD);
  g_mutex_unlock(self->lock);
  return status;
}

ssStatus_t whiteboard_node_sib_access_subscribe_wql_related(WhiteBoardNode *self,
							    const ssPathNode_t *startPathNode,
							    const gchar *pathExpr,
							    const ssPathNode_t *endPathNode,
							    WhiteBoardNodeQSCommonWQLbooleanCB cb,
							    gint *subscription_id_p,
							    gpointer data)
{
  gchar *query = NULL;
  QueryType type = QueryTypeWQLRelated;
  DBusMessage *reply=NULL;
  ssStatus_t status;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail(startPathNode != NULL && startPathNode->string!=NULL && 
		       (startPathNode->nodeType==ssElement_TYPE_URI || startPathNode->nodeType==ssElement_TYPE_LIT), ss_InvalidParameter);
  g_return_val_if_fail(pathExpr!=NULL, ss_InvalidParameter);
  g_return_val_if_fail(endPathNode != NULL && endPathNode->string!=NULL && 
		       (endPathNode->nodeType==ssElement_TYPE_URI || endPathNode->nodeType==ssElement_TYPE_LIT), ss_InvalidParameter);
  g_return_val_if_fail( cb != NULL, ss_InvalidParameter);
  g_return_val_if_fail( subscription_id_p != NULL, ss_InvalidParameter);
  g_mutex_lock(self->lock);
  *subscription_id_p = -1;

  const gchar *nodeid = whiteboard_node_get_uuid(self);
  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) has not joined, can not query\n",nodeid);
      g_mutex_unlock(self->lock);
      return ss_InvalidParameter;
    }

  ssBufDesc_t *bD = ssBufDesc_new();
  if (!bD) {
    g_mutex_unlock(self->lock);
    return ss_NotEnoughResources;
  }

  status = addXML_query_w_wql_n_e_n (bD, type, startPathNode, pathExpr, endPathNode);
  if (status) {
    ssBufDesc_free(&bD);
    g_mutex_unlock(self->lock);
    return status;
  }

  gint msgnum = ++(self->msgnumber);
  query = ssBufDesc_GetMessage(bD);
  whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					 WHITEBOARD_DBUS_OBJECT,
					 WHITEBOARD_DBUS_NODE_INTERFACE,
					 WHITEBOARD_DBUS_NODE_METHOD_SUBSCRIBE,
					 whiteboard_node_validate_connection(self),
					 &reply,
					 DBUS_TYPE_STRING, &nodeid,
					 DBUS_TYPE_STRING, &self->sib,
					 DBUS_TYPE_INT32, &msgnum,
					 DBUS_TYPE_INT32, &type,
					 DBUS_TYPE_STRING, &query,
					 WHITEBOARD_UTIL_LIST_END);

  status = (reply)? ss_StatusOK : ss_InternalError;
  if(!status)
    {
      whiteboard_util_parse_message(reply,
				    DBUS_TYPE_INT32, subscription_id_p,
				    WHITEBOARD_UTIL_LIST_END);
      status = (*subscription_id_p < 0)? ss_InternalError : ss_StatusOK;
      if(status)
	{
	  whiteboard_log_debug("Could not create query..\n");
	}
      else
	{
	  whiteboard_log_debug("Got subscribe access_id:%d\n", *subscription_id_p);
	  SubscriptionData *sd = g_new0(SubscriptionData, 1);
	  if (!sd)
	    status = ss_NotEnoughResources;
	  else
	    {
	      sd->cb.q_wql_boolean = cb;
	      sd->user_data = data;
	      sd->type = type;
	      sd->flags = SUBSCRIBE_FLAGS_SUBSCRIBE;
	      if(!whiteboard_node_add_subscription_data(self, *subscription_id_p, sd))
		{
		  status = ss_InternalError;
		  whiteboard_log_debug("Could not add query data to callback map\n");
		  g_free(sd);
		}
	    }
	}
      dbus_message_unref(reply);
    }

  ssBufDesc_free(&bD);
  g_mutex_unlock(self->lock);
  whiteboard_log_debug_fe();
  return status;
}

ssStatus_t whiteboard_node_sib_access_query_wql_related (WhiteBoardNode *self,
						   const ssPathNode_t *startNode,
						   const gchar *expr,
						   const ssPathNode_t *endNode,
						   WhiteBoardNodeQSCommonWQLbooleanCB cb,
						   gpointer data)
{
  gchar *query = NULL;
  gint access_id = -1;
  QueryType type = QueryTypeWQLRelated;
  DBusMessage *reply=NULL;
  ssStatus_t status;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail(startNode != NULL && startNode->string!=NULL && 
		       (startNode->nodeType==ssElement_TYPE_URI || startNode->nodeType==ssElement_TYPE_LIT), ss_InvalidParameter);
  g_return_val_if_fail(expr!=NULL, ss_InvalidParameter);
  g_return_val_if_fail(endNode != NULL && endNode->string!=NULL && 
		       (endNode->nodeType==ssElement_TYPE_URI || endNode->nodeType==ssElement_TYPE_LIT), ss_InvalidParameter);
  g_mutex_lock(self->lock);
  const gchar *nodeid = whiteboard_node_get_uuid(self);
  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) has not joined, can not update triples\n",nodeid);
      g_mutex_unlock(self->lock);
      return ss_InvalidParameter;
    }

  gint msgnum = ++(self->msgnumber);

  ssBufDesc_t *bD = ssBufDesc_new();
  if (!bD) {
    g_mutex_unlock(self->lock);
    return ss_NotEnoughResources;
  }

  status = addXML_query_w_wql_n_e_n (bD, type, startNode, expr, endNode);
  if (status) {
    ssBufDesc_free(&bD);
    g_mutex_unlock(self->lock);
    return status;
  }

  query = ssBufDesc_GetMessage(bD);
  whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					 WHITEBOARD_DBUS_OBJECT,
					 WHITEBOARD_DBUS_NODE_INTERFACE,
					 WHITEBOARD_DBUS_NODE_METHOD_QUERY,
					 whiteboard_node_validate_connection(self),
					 &reply,
					 DBUS_TYPE_STRING, &nodeid,
					 DBUS_TYPE_STRING, &self->sib,
					 DBUS_TYPE_INT32, &msgnum,
					 DBUS_TYPE_INT32, &type,
					 DBUS_TYPE_STRING, &query,
					 WHITEBOARD_UTIL_LIST_END);

  status = (reply)? ss_StatusOK : ss_InternalError;
  if(!status)
    {
      whiteboard_util_parse_message(reply,
				    DBUS_TYPE_INT32, &access_id,
				    WHITEBOARD_UTIL_LIST_END);
      status = (access_id < 0)? ss_InternalError : ss_StatusOK;
      if(status)
	{
	  whiteboard_log_debug("Could not create query..\n");
	}
      else
	{
	  SubscriptionData *sd = NULL;
	  sd = g_new0(SubscriptionData, 1);
	  if (!sd)
	    status = ss_NotEnoughResources;
	  else
	    {
	      sd->cb.q_wql_boolean = cb;
	      sd->user_data = data;
	      sd->type = QueryTypeWQLRelated;
	      if(!whiteboard_node_add_subscription_data(self, access_id,sd))
		{
		  status = ss_InternalError;
		  whiteboard_log_debug("Could not add query data to callback map\n");
		  g_free(sd);
		}
	    }
	}
      dbus_message_unref(reply);
    }

  ssBufDesc_free(&bD);
  g_mutex_unlock(self->lock);
  return status;
}

gint whiteboard_node_sib_access_query_wql_values (WhiteBoardNode *self,
						  const ssPathNode_t *node,
						  const gchar *expr,
						  WhiteBoardNodeQueryWQLnodelistCB cb,
						  gpointer data)
{
  //? gchar *subscribe_message = NULL;
  gint access_id = -1;
  DBusMessage *reply=NULL;
  QueryType type = QueryTypeWQLValues;
  ssBufDesc_t *desc=NULL;
  gchar *query = NULL;

  whiteboard_log_debug_fb();
  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail( cb != NULL ,ss_InvalidParameter);
  g_return_val_if_fail( node != NULL , ss_InvalidParameter);
  g_return_val_if_fail( expr != NULL , ss_InvalidParameter);
  g_mutex_lock(self->lock);
  const gchar *nodeid = whiteboard_node_get_uuid(self);
  gint msgnum = ++(self->msgnumber);
  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) not joined, can not create query\n",nodeid);
      g_mutex_unlock(self->lock);
      return ss_InvalidParameter;
    }
  else
    {
      ssStatus_t status;
      desc = ssBufDesc_new();
      //insert_message =  whiteboard_insert_new_request(triples);
      status = addXML_query_w_wql_n_e (desc, type, node, expr);
      if (status) {
	ssBufDesc_free(&desc);
	g_mutex_unlock(self->lock);
	return status;
      }

      query = ssBufDesc_GetMessage(desc);
      whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					     WHITEBOARD_DBUS_OBJECT,
					     WHITEBOARD_DBUS_NODE_INTERFACE,
					     WHITEBOARD_DBUS_NODE_METHOD_QUERY,
					     whiteboard_node_validate_connection(self),
					     &reply,
					     DBUS_TYPE_STRING, &nodeid,
					     DBUS_TYPE_STRING, &self->sib,
					     DBUS_TYPE_INT32, &msgnum,
					     DBUS_TYPE_INT32, &type,
					     DBUS_TYPE_STRING, &query,
					     WHITEBOARD_UTIL_LIST_END);
      if(reply)
	{
	  whiteboard_util_parse_message(reply,
					DBUS_TYPE_INT32, &access_id,
					WHITEBOARD_UTIL_LIST_END);
	  if(access_id < 0)
	    {
	      whiteboard_log_debug("Could not create query..\n");
	    }
	  else
	    {
	      whiteboard_log_debug("Got query access_id:%d\n", access_id);
	      SubscriptionData *sd = NULL;
	      sd = g_new0(SubscriptionData, 1);
	      sd->cb.q_wql_values = cb;
	      sd->user_data = data;
	      sd->type = QueryTypeWQLValues;
	      if(!whiteboard_node_add_subscription_data(self, access_id,sd))
		{
		  whiteboard_log_debug("Could not add subscription data to subscription map\n");
		  g_free(sd);
		}
	    }
	  dbus_message_unref(reply);
	  //	  ssBufDesc_free(&desc);
	}
      ssBufDesc_free(&desc);
    }
  
  whiteboard_log_debug_fe();
  g_mutex_unlock(self->lock);
  return (access_id > 0) ? ss_StatusOK : ss_GeneralError;
}

gint whiteboard_node_sib_access_query_wql_nodeClasses (WhiteBoardNode *self,
						       const ssPathNode_t *pathNode,
						       WhiteBoardNodeQueryWQLnodelistCB cb,
						       gpointer data)
{
  gchar *query = NULL;
  gint access_id = -1;
  QueryType type = QueryTypeWQLNodeTypes;
  DBusMessage *reply=NULL;
  ssBufDesc_t *bD=NULL;
  ssStatus_t status;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail( pathNode != NULL && pathNode->string != NULL &&
			(pathNode->nodeType == ssElement_TYPE_URI || pathNode->nodeType == ssElement_TYPE_LIT),
			ss_InvalidParameter);
  g_return_val_if_fail( cb != NULL, ss_InvalidParameter);
  g_mutex_lock(self->lock);
  const gchar *nodeid = whiteboard_node_get_uuid(self);
  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) not joined, can not create query\n",nodeid);
      g_mutex_unlock(self->lock);
      return ss_InvalidParameter;
    }

  bD = ssBufDesc_new();
  if (!bD) {
    g_mutex_unlock(self->lock);
    return ss_NotEnoughResources;
  }

  status = addXML_query_w_wql_n (bD, type, pathNode);
  if (status) {
    ssBufDesc_free(&bD);
    g_mutex_unlock(self->lock);
    return status;
  }

  gint msgnum = ++(self->msgnumber);
  query = ssBufDesc_GetMessage(bD);
  whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					 WHITEBOARD_DBUS_OBJECT,
					 WHITEBOARD_DBUS_NODE_INTERFACE,
					 WHITEBOARD_DBUS_NODE_METHOD_QUERY,
					 whiteboard_node_validate_connection(self),
					 &reply,
					 DBUS_TYPE_STRING, &nodeid,
					 DBUS_TYPE_STRING, &self->sib,
					 DBUS_TYPE_INT32, &msgnum,
					 DBUS_TYPE_INT32, &type,
					 DBUS_TYPE_STRING, &query,
					 WHITEBOARD_UTIL_LIST_END);
  status = (reply)? ss_StatusOK : ss_InternalError;
  if(!status)
    {
      whiteboard_util_parse_message(reply,
				    DBUS_TYPE_INT32, &access_id,
				    WHITEBOARD_UTIL_LIST_END);
      status = (access_id < 0)? ss_InternalError : ss_StatusOK;
      if(status)
	{
	  whiteboard_log_debug("Could not create query..\n");
	}
      else
	{
	  whiteboard_log_debug("Got query access_id:%d\n", access_id);
	  SubscriptionData *sd = g_new0(SubscriptionData, 1);
	  if (!sd)
	    status = ss_NotEnoughResources;
	  else
	    {
	      sd->cb.q_wql_values = cb;
	      sd->user_data = data;
	      sd->type = type;
	      if(!whiteboard_node_add_subscription_data(self, access_id, sd))
		{
		  status = ss_InternalError;
		  whiteboard_log_debug("Could not add query data to callback map\n");
		  g_free(sd);
		}
	    }
	}
      dbus_message_unref(reply);
    }

  ssBufDesc_free(&bD);
  g_mutex_unlock(self->lock);
  whiteboard_log_debug_fe();
  return status;
}

ssStatus_t whiteboard_node_sib_access_subscribe_template(WhiteBoardNode *self,
							 GSList* templates,
							 const gchar *namespace,
							 WhiteBoardNodeSubscriptionIndTemplateCB cb,
							 gint *subscription_id_p,
							 gpointer data)
{
  gchar *subscribe_message = NULL;
  QueryType type = QueryTypeTemplate;
  DBusMessage *reply=NULL;
  ssStatus_t status;

  GHashTable *prefix_ns_map=NULL;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail( templates != NULL && templates->data != NULL, ss_InvalidParameter);
  g_return_val_if_fail( cb != NULL, ss_InvalidParameter);
  g_return_val_if_fail( subscription_id_p != NULL, ss_InvalidParameter);
  g_mutex_lock(self->lock);
  *subscription_id_p = -1;

  const gchar *nodeid = whiteboard_node_get_uuid(self);
  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) not joined, can not create subscription\n",nodeid);
      g_mutex_unlock(self->lock);
      return ss_InvalidParameter;
    }

  GSList *l=templates;
  ssTriple_t *t;

  //initializing status//
  status = (!namespace)? ss_StatusOK : new_prefix2ns_map(namespace, &prefix_ns_map);

  if (status!=ss_StatusOK)
    {
      g_mutex_unlock(self->lock);
      return status;
    }
  ssBufDesc_t *desc = ssBufDesc_new();
  if (!desc) {
    if (prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
    g_mutex_unlock(self->lock);
    return ss_NotEnoughResources;
  }

  status = addXML_start (desc, &SIB_TRIPLELIST, NULL, NULL, 0);

  while (status==ss_StatusOK && l && (t=(ssTriple_t *)l->data) && !invalidTriple(t,TRUE)) {
    status = addXML_templateTriple(t, prefix_ns_map, (gpointer)desc);
    l=l->next;
  }
  if (!status && l!=NULL)
    status = ss_InvalidTripleSpecification;
  
  status=(status)?status : addXML_end (desc, &SIB_TRIPLELIST);

  if (status) {
    ssBufDesc_free(&desc);
    if (prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
    g_mutex_unlock(self->lock);
    return -1; //must use -1 for now, not ssStatus_t, until a &subscriptionId parameter is used to pass back the value
  }

  gint msgnum = ++(self->msgnumber);
  subscribe_message = ssBufDesc_GetMessage(desc);
  whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					 WHITEBOARD_DBUS_OBJECT,
					 WHITEBOARD_DBUS_NODE_INTERFACE,
					 WHITEBOARD_DBUS_NODE_METHOD_SUBSCRIBE,
					 whiteboard_node_validate_connection(self),
					 &reply,
					 DBUS_TYPE_STRING, &nodeid,
					 DBUS_TYPE_STRING, &self->sib,
					 DBUS_TYPE_INT32, &msgnum,
					 DBUS_TYPE_INT32, &type,
					 DBUS_TYPE_STRING, &subscribe_message,
					 WHITEBOARD_UTIL_LIST_END);
  status = (reply)? ss_StatusOK : ss_InternalError;
  if(!status)
    {
      whiteboard_util_parse_message(reply,
				    DBUS_TYPE_INT32, subscription_id_p,
				    WHITEBOARD_UTIL_LIST_END);
      status = (*subscription_id_p < 0)? ss_InternalError : ss_StatusOK;
      if(status)
	{
	  whiteboard_log_debug("Could not create subscription...\n");
	}
      else
	{
	  whiteboard_log_debug("Got subscribe access_id:%d\n", *subscription_id_p);
	  SubscriptionData *sd = g_new0(SubscriptionData, 1);
	  if (!sd)
	    status = ss_NotEnoughResources;
	  else
	    {
	      sd->cb.s_template = cb;
	      sd->user_data = data;
	      sd->type = type;
	      sd->flags = SUBSCRIBE_FLAGS_SUBSCRIBE;
	      if(!whiteboard_node_add_subscription_data(self, *subscription_id_p, sd))
		{
		  whiteboard_log_debug("Could not add query data to callback map\n");
		  status = ss_InternalError;
		  if (prefix_ns_map) g_hash_table_destroy(prefix_ns_map);
		  g_free(sd);
		}
	    }
	}
      dbus_message_unref(reply);
    }

  ssBufDesc_free(&desc);
  g_mutex_unlock(self->lock);
  whiteboard_log_debug_fe();
  return status;
}


ssStatus_t whiteboard_node_sib_access_subscribe_wql_values(WhiteBoardNode *self,
							   const ssPathNode_t *pathNode,
							   const gchar *pathExpr,
							   WhiteBoardNodeSubscriptionIndWQLvaluesCB cb,
							   gint *subscription_id_p,
							   gpointer data)
{
  gchar *query = NULL;
  QueryType type = QueryTypeWQLValues;
  DBusMessage *reply=NULL;
  ssStatus_t status;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_return_val_if_fail(pathNode != NULL && pathNode->string!=NULL && 
		       (pathNode->nodeType==ssElement_TYPE_URI || pathNode->nodeType==ssElement_TYPE_LIT), ss_InvalidParameter);
  g_return_val_if_fail(pathExpr!=NULL, ss_InvalidParameter);
  g_return_val_if_fail( cb != NULL, ss_InvalidParameter);
  g_return_val_if_fail( subscription_id_p != NULL, ss_InvalidParameter);
  g_mutex_lock(self->lock);
  *subscription_id_p = -1;

  const gchar *nodeid = whiteboard_node_get_uuid(self);
  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) not joined, can not create subscription\n",nodeid);
      g_mutex_unlock(self->lock);
      return ss_InvalidParameter;
    }

  ssBufDesc_t *desc = ssBufDesc_new();
  if (!desc) {
    g_mutex_unlock(self->lock);
    return ss_NotEnoughResources;
  }

  status = addXML_query_w_wql_n_e (desc, type, pathNode, pathExpr);
  if (status) {
    g_mutex_unlock(self->lock);
    ssBufDesc_free(&desc);
    return status;
  }

  gint msgnum = ++(self->msgnumber);
  query = ssBufDesc_GetMessage(desc);
  whiteboard_util_send_method_with_reply(WHITEBOARD_DBUS_SERVICE,
					 WHITEBOARD_DBUS_OBJECT,
					 WHITEBOARD_DBUS_NODE_INTERFACE,
					 WHITEBOARD_DBUS_NODE_METHOD_SUBSCRIBE,
					 whiteboard_node_validate_connection(self),
					 &reply,
					 DBUS_TYPE_STRING, &nodeid,
					 DBUS_TYPE_STRING, &self->sib,
					 DBUS_TYPE_INT32, &msgnum,
					 DBUS_TYPE_INT32, &type,
					 DBUS_TYPE_STRING, &query,
					 WHITEBOARD_UTIL_LIST_END);
  status = (reply)? ss_StatusOK : ss_InternalError;
  if(!status)
    {
      whiteboard_util_parse_message(reply,
				    DBUS_TYPE_INT32, subscription_id_p,
				    WHITEBOARD_UTIL_LIST_END);
      status = (*subscription_id_p < 0)? ss_InternalError : ss_StatusOK;
      if(status)
	{
	  whiteboard_log_debug("Could not create subscription...\n");
	}
      else
	{
	  whiteboard_log_debug("Got subscribe access_id:%d\n", *subscription_id_p);
	  SubscriptionData *sd = g_new0(SubscriptionData, 1);
	  if (!sd)
	    status = ss_NotEnoughResources;
	  else
	    {
	      sd->cb.s_wql_values = cb;
	      sd->user_data = data;
	      sd->type = type;
	      sd->flags = SUBSCRIBE_FLAGS_SUBSCRIBE;
	      if(!whiteboard_node_add_subscription_data(self, *subscription_id_p, sd))
		{
		  status = ss_InternalError;
		  whiteboard_log_debug("Could not add query data to callback map\n");
		  g_free(sd);
		}
	    }
	}
      dbus_message_unref(reply);
    }

  ssBufDesc_free(&desc);
  g_mutex_unlock(self->lock);
  whiteboard_log_debug_fe();
  return status;
}

ssStatus_t whiteboard_node_sib_access_unsubscribe(WhiteBoardNode *self,
					    gint access_id)
{
  SubscriptionData *sb= NULL;

  whiteboard_log_debug_fb();
  g_return_val_if_fail(self!=NULL, ss_InvalidParameter);
  g_mutex_lock(self->lock);
  const gchar *nodeid = whiteboard_node_get_uuid(self);
  gint msgnum = ++(self->msgnumber);
  if( !whiteboard_node_joined(self))
    {
      whiteboard_log_debug("Node (%s) not joined, can not unsubscribe\n",nodeid);
      g_mutex_unlock(self->lock);
      return ss_InvalidParameter;
    }
  else
    {
      if((sb =  whiteboard_node_get_subscription_data(self, access_id)) != NULL)
	{
	  g_return_val_if_fail(sb->subscription_id != NULL, ss_InvalidParameter);
	  g_return_val_if_fail(sb->flags == 0, ss_InvalidParameter);
	  sb->flags = SUBSCRIBE_FLAGS_UNSUBSCRIBE;
	  whiteboard_util_send_signal(WHITEBOARD_DBUS_OBJECT,
				      WHITEBOARD_DBUS_NODE_INTERFACE,
				      WHITEBOARD_DBUS_NODE_SIGNAL_UNSUBSCRIBE,
				      whiteboard_node_validate_connection(self),
				      DBUS_TYPE_INT32, &access_id,
				      DBUS_TYPE_STRING, &nodeid,
				      DBUS_TYPE_STRING, &self->sib,
				      DBUS_TYPE_INT32, &msgnum,
				      DBUS_TYPE_STRING, &sb->subscription_id,
				      WHITEBOARD_UTIL_LIST_END);
	  //success = 0;
	}
      else
	{
	  whiteboard_log_debug("Subscription /w access_id: %d not found, can not unsubscribe\n", access_id);
	  g_mutex_unlock(self->lock);
	  return ss_InvalidParameter;
	}
    }
  whiteboard_log_debug_fe();
  g_mutex_unlock(self->lock);
  return ss_StatusOK;
}

/*****************************************************************************
 * Custom commands
 *****************************************************************************/

guint whiteboard_node_send_custom_command(WhiteBoardNode* self, WhiteBoardCmd* cmd)
{
  guint serial = 0;

  whiteboard_log_debug_fb();

  g_return_val_if_fail(self != NULL, 0);
  g_return_val_if_fail(cmd != NULL, 0);

  serial = whiteboard_cmd_send(cmd, self->connection);

  whiteboard_log_debug_fe();

  return serial;
}

static SubscriptionData *whiteboard_node_get_subscription_data(WhiteBoardNode *self, gint access_id)
{
  SubscriptionData* sd = NULL;
  
  whiteboard_log_debug_fb();
  g_return_val_if_fail(self != NULL, NULL);
  
  sd = (SubscriptionData*) g_hash_table_lookup(self->subscription_map,
					       GINT_TO_POINTER(access_id));
  
  whiteboard_log_debug_fe();
  
  return sd;
}

static gboolean whiteboard_node_add_subscription_data(WhiteBoardNode *self, gint access_id, SubscriptionData *sd)
{
  gboolean ret = FALSE;
  whiteboard_log_debug_fb();
  // check that not existing previously
  if( whiteboard_node_get_subscription_data(self, access_id) == NULL)
    {
      g_hash_table_insert(self->subscription_map, GINT_TO_POINTER(access_id), (gpointer)sd);
      ret = TRUE;
    }
  whiteboard_log_debug_fe();
  return ret;
}

static gboolean whiteboard_node_remove_subscription_data(WhiteBoardNode *self, gint access_id)
{
  gboolean ret = FALSE;
  SubscriptionData *sd = NULL;
  whiteboard_log_debug_fb();
  // check that not existing previously
  if( (sd = whiteboard_node_get_subscription_data(self, access_id) )!= NULL)
    {
      ret = g_hash_table_remove(self->subscription_map, GINT_TO_POINTER(access_id));
      // subscription data freed here...
      if(sd->subscription_id)
	g_free(sd->subscription_id);

      if (sd->prefix_ns_map)
	g_hash_table_destroy(sd->prefix_ns_map);

      g_free(sd);
    }
  else
    {
      whiteboard_log_debug("Can not remove subscription data, subscription id:%d not found\n", access_id);
    }
  whiteboard_log_debug_fe();
  return ret;
}
