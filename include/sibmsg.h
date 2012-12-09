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
#ifndef SIBMSG_H
#define SIBMSG_H

#include <glib.h>

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "sibdefs.h"

ssBufDesc_t *ssBufDesc_new(void);

gchar *ssBufDesc_GetMessage(ssBufDesc_t *desc);

gint ssBufDesc_GetMessageLen(ssBufDesc_t *desc);

void ssBufDesc_free(ssBufDesc_t **_bd);

ssStatus_t addXML_open_element (ssBufDesc_t *bD, charStr *el);

ssStatus_t addXML_append_attribute (ssBufDesc_t *bD, attrStr *attr);

ssStatus_t addXML_close_element (ssBufDesc_t *bD);

ssStatus_t addXML_close_element_append_content_end (ssBufDesc_t *bD, 
						    charStr *contains, 
						    charStr *el);

ssStatus_t addXML_pathNode (ssBufDesc_t *bD, 
			    const ssPathNode_t *node, 
			    charStr *nameVal);

ssStatus_t addXML_templateTriple (gpointer _triple, 
				  GHashTable *prefix_uri_map, 
				  gpointer _itd);


ssStatus_t addXML_query_w_wql_n  ( ssBufDesc_t *desc, 
				   QueryType type, 
				   const ssPathNode_t *pathNode);

ssStatus_t addXML_query_w_sparql( ssBufDesc_t *desc, 
				  gpointer something);

ssStatus_t addXML_query_w_wql_n_e  ( ssBufDesc_t *desc, 
				     QueryType type, 
				     const ssPathNode_t *node, 
				     const gchar *expr);

ssStatus_t addXML_query_w_wql_n_e_n( ssBufDesc_t *desc, 
				     QueryType type, 
				     const ssPathNode_t *node, 
				     const gchar *expr, 
				     const ssPathNode_t *endNode);

ssStatus_t addXML_query_w_wql_n_n  ( ssBufDesc_t *desc, 
				     QueryType type, 
				     const ssPathNode_t *pathNode, 
				     const ssPathNode_t *node);

ssStatus_t generateSPARQLSelectQueryString( ssBufDesc_t *desc,
					    GSList *selectVars,//list of (ssPathNode_t *) where nodeType is ssElement_TYPE_BNODE
					    GSList *where,//list of (sparqlTriple_t *)
					    GSList *optionals,//list of (GSList *) list of (sparqlTriple_t *)
					    GHashTable *namespaces);// xmlns:x="y:z#" hashed=> KEY=x<->VALUE=y:z# 


/* addXML_start @:
   where: element.txt="element";
          attribute.name.txt="name";
          attribute.defined.txt="defined";
	  content.txt="content";
          #define CLOSE (1)
   @(&desc, &element, NULL,       NULL,     !CLOSE) => '<element>'
   @(&desc, &element, NULL,       NULL,      CLOSE) => '<element></element>'
   @(&desc, &element, NULL,       &content,  CLOSE) => '<element>content</element>'
   @(&desc, &element, &attribute, &content,  CLOSE) => '<element name="defined">content</element>'
   @(&desc, &element, &attribute, NULL,      CLOSE) => '<element name="defined"/>'
*/

/* addRDFXML_templates @:
    where nameSpace = "xmlns:x=\"a:b#\""
    any triple in list, e.g.  
    ((ssTriple_t *)triples.data)->subject = "a:b#MrT";
    ((ssTriple_t *)triples.data)->predicate = "x:drinks";
    ((ssTriple_t *)triples.data)->object = "a:b#beer";
    ((ssTriple_t *)triples.data)->objType = ssOBJ_TYPE_URI;
    
    @(&desc, triples) => not necessarily with linefeed or indentation '
    <rdf:RDF xmlns:x="a:b#" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
    <rdf:Description rdf:about="a:b#MrT">
    <x:drinks rdf:resource="a:b#beer"/>
    </rdf:Description>
    </rdf:RDF>
    '
    alternatively, changing:
    ((ssTriple_t *)triples.data)->object = "never";
    ((ssTriple_t *)triples.data)->objType = ssOBJ_TYPE_LIT;
    
    @(&desc, triples) => not necessarily with linefeed or indentation '
    <rdf:RDF xmlns:x="a:b#" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
    <rdf:Description rdf:about="a:b#MrT">
    <x:drinks>never</x:drinks>
    </rdf:Description>
    </rdf:RDF>
    '
*/

/*********** for RECEIVED messages / example "pseudo" code shown */
/*
A struct is defined which contains all possible parameter content 
for all messages. (because: there is little variation in the kinds 
of parameters - of course values change!)

  const NodeMsgContent *msgParsed;

When a message is received, a NodeMsgContent is allocated:

  msgParsed = parseSSAPmsg_new (); //NULL is no allocation

The user provides the message in segments or whole. One or more calls
are made to parse the segment(s) passing the pointer and length of the
segment, and setting done to !0 with the last segment. The allocated
NodeMsgContent block is always included with each call.. upon return,
a setting gives the state of the parse.

  parseSSAPmsg_section (msgParsed, 
                        msgSegment, 
                        msgSegmentLen, 
                        (moreSegemnts)?0:1)
  if (msgParsed->parseState==MSG_S_NOK) {
     ..error processing..
  }

when last segment is given, the parseState will be either MSG_S_NOK or
MSG_S_OK and when the finished processing the message parameter
content, the struct and its content should be freed with:

  parseSSAPmsg_free (&msgParsed);

*/

typedef enum  {
  MSG_T_NSET=0,
  MSG_T_REQ,
  MSG_T_CNF,
  MSG_T_IND
} msgType_t;

typedef enum  {
  MSG_N_NSET=0,
  MSG_N_JOIN,
  MSG_N_LEAVE,
  MSG_N_INSERT,
  MSG_N_UPDATE,
  MSG_N_REMOVE,
  MSG_N_QUERY,
  MSG_N_SUBSCRIBE,
  MSG_N_UNSUBSCRIBE,
} msgName_t;

typedef enum {
  MSG_G_NSET=0, 
  MSG_G_TMPL, 
  MSG_G_N3, 
  MSG_G_RDF,
} graphStyle_t;

typedef enum {
  MSG_Q_NSET=0, 
  MSG_Q_TMPL, 
  MSG_Q_N3, 
  MSG_Q_SPRQL,
  MSG_Q_WQLfirst,
  MSG_Q_WQL_VALUES=MSG_Q_WQLfirst,
  MSG_Q_WQL_NODETYPES,
  MSG_Q_WQL_RELATED,
  MSG_Q_WQL_ISTYPE,
  MSG_Q_WQL_ISSUBTYPE,
  MSG_Q_WQLlast=MSG_Q_WQL_ISSUBTYPE
} queryStyle_t;

typedef enum {
  MSG_E_NSET=0, 
  MSG_E_NOK, 
  MSG_E_OK,
  MSG_E_NOT_FOUND,
} msgStatus_t;

typedef struct {
  ssStatus_t parseStatus;
  msgType_t type;
  msgName_t name;
  gboolean confirmReq;
  //  EncodingType encoding;
  graphStyle_t graphStyle;
  queryStyle_t queryStyle;
  msgStatus_t status;
  int msgNumber;
  int indUpdateSequence;
  gchar *queryids_MSTR; // also used for tripleids in case of insert_cnf
  char *nodeid_MSTR;
  char *spaceid_MSTR;
  //embedded in main "result" string:  char *headResults_MSTR; //used only for SPARQL results <header>
  char *m3XML_MSTR; // general purpose/main string, more or less used to pass query results, or added results w/ subscribe ind 
  char *removedResults_MSTR; // used only for removed results w/ subscribe ind
  char *expireTime_MSTR;
  char *icv_MSTR;
  gchar *credentials_MSTR;
} NodeMsgContent_t;

/** about NodeMsgContent It is assumed, of course, that the message
segment(s) are in plain text, as in DEL201.  Note that parametners:
type, name and msgNumber are converted into binary values, which may
be more directly useable internally.. there is little question about
how to do this. The others are newly allocated string copies of the
corresponding message part. These will all be freed along with the
struct by paraseSSAPmsg_free. Thus, especially the m3XML_MSTR string
must be preserved (or copied) for later processing. 

Some of these, instead of being allocated
strings, might rather be convert to (or select) internal binary
values, if we discuss the means.

more discussion later - changes to the structures won't change basic 
parsing syntax.
**/

NodeMsgContent_t *parseSSAPmsg_new(void);

ssStatus_t parseSSAPmsg_section(NodeMsgContent_t *msgParBlk, 
				char *buff, 
				int buffLen, 
				int done);

gint parseSSAPmsg_parsedbytecount(NodeMsgContent_t *msgParBlk);

ssStatus_t parseM3_triples_SIB(GSList ** list_pp, 
			       const char * rdfXMLstr, 
			       GHashTable *prefix_uri_map, 
			       gchar **bNodeUriList_str);

#define parseM3_triples(list_pp, rdfXMLstr, prefix_uri_map) \
  parseM3_triples_SIB(list_pp, rdfXMLstr, prefix_uri_map, NULL)


#ifdef SIBUSER_ROLE
ssStatus_t parseM3_query_cnf_wql(GSList **results, const gchar *response);

ssStatus_t parseM3_query_results_sparql_select(GSList **selectedVariables, GSList **valRows, const gchar *resultXML, const GHashTable *prefix_ns_map);
#endif /* SIBUSER_ROLE */

#ifdef SIB_ROLE
ssWqlDesc_t *ssWqlDesc_new(queryStyle_t queryStyle);

ssWqlDesc_t *ssWqlDesc_new_jh(QueryType query_type);

ssStatus_t parseM3_query_req_wql(ssWqlDesc_t *qD, const gchar *qXml);

void ssWqlDesc_free (ssWqlDesc_t ** ssWqlDesc);
#endif /* SIB_ROLE */

NodeMsgContent_t  *nodemsgcontent_new_join_rsp( guchar *spaceid,
						guchar *nodeid,
						gint msgnum,
						gint success);

NodeMsgContent_t  *nodemsgcontent_new_insert_rsp( guchar *spaceid,
						  guchar *nodeid,
						  gint msgnum,
						  gint success,
						  guchar *bNodes);

NodeMsgContent_t  *nodemsgcontent_new_leave_rsp( guchar *spaceid,
						 guchar *nodeid,
						 gint msgnum,
						 gint status);

NodeMsgContent_t  *nodemsgcontent_new_remove_rsp( guchar *spaceid,
						  guchar *nodeid,
						  gint msgnum,
						  gint success);

NodeMsgContent_t  *nodemsgcontent_new_update_rsp( guchar *spaceid,
						  guchar *nodeid,
						  gint msgnum,
						  gint success,
						  guchar *bNodes);

NodeMsgContent_t  *nodemsgcontent_new_query_rsp( guchar *spaceid,
						 guchar *nodeid,
						 gint msgnum,
						 gint success,
						 guchar *results);

NodeMsgContent_t  *nodemsgcontent_new_subscribe_rsp( guchar *spaceid,
						     guchar *nodeid,
						     gint msgnum,
						     gint success,
						     guchar *subscription_id,
						     guchar *results);

NodeMsgContent_t  *nodemsgcontent_new_subscription_ind( guchar *spaceid,
							guchar *nodeid,
							gint msgnum,
							gint seqnum,
							guchar *subscription_id,
							guchar *results_new,
							guchar *results_obsolete);

NodeMsgContent_t  *nodemsgcontent_new_unsubscribe_rsp( guchar *spaceid,
						       guchar *nodeid,
						       gint msgnum,
						       gint success,
						       guchar *subsription_id);

void parseSSAPmsg_free  (NodeMsgContent_t ** msgParBlk);
ssStatus_t parseSSAPmsg_get_status(NodeMsgContent_t  *msg); // parsing status
msgType_t parseSSAPmsg_get_type(NodeMsgContent_t  *msg);
msgName_t parseSSAPmsg_get_name(NodeMsgContent_t  *msg);
const gchar *parseSSAPmsg_get_M3XML(NodeMsgContent_t  *msg);
const gchar *parseSSAPmsg_get_nodeid(NodeMsgContent_t  *msg);
const gchar *parseSSAPmsg_get_spaceid(NodeMsgContent_t  *msg);
msgStatus_t parseSSAPmsg_get_msg_status(NodeMsgContent_t  *msg);
queryStyle_t parseSSAPmsg_get_queryStyle(NodeMsgContent_t  *msg);
graphStyle_t parseSSAPmsg_get_graphStyle(NodeMsgContent_t  *msg);
const char *parseSSAPmsg_get_queryids( NodeMsgContent_t  *msg);
const char *parseSSAPmsg_get_subscriptionid( NodeMsgContent_t  *msg);
const char *parseSSAPmsg_get_results_added( NodeMsgContent_t  *msg);
const char *parseSSAPmsg_get_results_removed( NodeMsgContent_t  *msg);
const char *parseSSAPmsg_get_insert_graph( NodeMsgContent_t  *msg);
const char *parseSSAPmsg_get_remove_graph( NodeMsgContent_t  *msg);
const gchar *parseSSAPmsg_get_credentials( NodeMsgContent_t  *msg);
gint parseSSAPmsg_get_msgnumber( NodeMsgContent_t  *msg);
gint parseSSAPmsg_get_update_sequence( NodeMsgContent_t  *msg);

void nodemsgcontent_free  (NodeMsgContent_t ** msgParBlk);

ssStatus_t ssBufDesc_CreateJoinMessage(ssBufDesc_t *desc,
				       ssElement_ct ssId,
				       ssElement_ct nodeId,
				       gint msgnumber);

ssStatus_t ssBufDesc_CreateLeaveMessage(ssBufDesc_t *desc,
					ssElement_ct ssId,
					ssElement_ct nodeName,
					gint msgnumber,
					gboolean confirm);

ssStatus_t ssBufDesc_CreateInsertMessage(ssBufDesc_t *desc,
					 ssElement_ct ssId,
					 ssElement_ct nodeName,
					 gint msgnumber,
					 EncodingType encoding,
					 const guchar *tripleStr,
					 gboolean confirm);

ssStatus_t ssBufDesc_CreateUpdateMessage(ssBufDesc_t *desc,
					 ssElement_ct ssId,
					 ssElement_ct nodeName,
					 gint msgnumber,
					 EncodingType encoding,
					 const guchar *insTripleStr,
					 const guchar *remTripleStr,
					 gboolean confirm);

ssStatus_t ssBufDesc_CreateRemoveMessage(ssBufDesc_t *desc,
					 ssElement_ct ssId,
					 ssElement_ct nodeName,
					 gint msgnumber,
					 EncodingType encoding,
					 const guchar *rdfxml);

ssStatus_t ssBufDesc_CreateQueryMessage(ssBufDesc_t *desc,
					ssElement_ct ssId,
					ssElement_ct nodeName,
					gint msgnumber,
					gint type,
					const guchar *rdfxml);
ssStatus_t ssBufDesc_CreateSubscribeMessage(ssBufDesc_t *desc,
					    ssElement_ct ssId,
					    ssElement_ct nodeName,
					    gint msgnumber,
					    gint type,
					    const guchar *rdfxml);

ssStatus_t ssBufDesc_CreateUnsubscribeMessage(ssBufDesc_t *desc,
					      ssElement_ct ssId,
					      ssElement_ct nodeName,
					    gint msgnumber,
					      ssElement_ct subid);


/* responses */
ssStatus_t ssBufDesc_CreateJoinResponse(ssBufDesc_t *desc,
					ssElement_ct nodeName,
					ssElement_ct ssId,
					gint msgnumber,
					msgStatus_t status);

ssStatus_t ssBufDesc_CreateLeaveResponse(ssBufDesc_t *desc,
					ssElement_ct nodeName,
					ssElement_ct ssId,
					 gint msgnumber,
					 msgStatus_t status);

ssStatus_t ssBufDesc_CreateSubscribeResponse(ssBufDesc_t *desc,
					     ssElement_ct nodeName,
					     ssElement_ct ssId,
					     gint msgnumber,
					     msgStatus_t status,
					     ssElement_ct subId,
					     guchar *rdfxml);
#if 0
ssStatus_t ssBufDesc_CreateUnsubscribeResponse(ssBufDesc_t *desc,
					       ssElement_ct nodeName,
					       ssElement_ct ssId,
					       gint msgnumber,
					       gboolean status,
					       ssElement_ct subId);
#endif
ssStatus_t ssBufDesc_CreateInsertResponse(ssBufDesc_t *desc,
					  ssElement_ct nodeName,
					  ssElement_ct ssId,
					  gint msgnumber,
					  msgStatus_t status,
					  ssElement_ct rdfxml);

ssStatus_t ssBufDesc_CreateUpdateResponse(ssBufDesc_t *desc,
					  ssElement_ct nodeName,
					  ssElement_ct ssId,
					  gint msgnumber,
					  msgStatus_t status,
					  ssElement_ct xml);

ssStatus_t ssBufDesc_CreateRemoveResponse(ssBufDesc_t *desc,
					  ssElement_ct nodeName,
					  ssElement_ct ssId,
					  gint msgnumber,
					  msgStatus_t status);

ssStatus_t ssBufDesc_CreateSubscriptionIndMessage(ssBufDesc_t *desc,
						  ssElement_ct nodeName,
						  ssElement_ct ssId,
						  gint msgnumber,
						  gint seqnumber,
						  const guchar *subscription_id,
						  const guchar *rdfxml_added,
						  const guchar *rdfxml_removed);

ssStatus_t ssBufDesc_CreateQueryResponse(ssBufDesc_t *desc,
					 ssElement_ct nodeName,
					 ssElement_ct ssId,
					 gint msgnumber,
					 msgStatus_t status,
					 const guchar *results);

ssStatus_t ssBufDesc_CreateUnsubscribeResponse(ssBufDesc_t *desc,
					       ssElement_ct nodeName,
					       ssElement_ct ssId,
					       gint msgnumber,
					       msgStatus_t status,
					       ssElement_ct subscription_id);

ssStatus_t addXML_start (ssBufDesc_t *desc, 
			 charStr *el, 
			 attrStr *attr, 
			 charStr *contains, 
			 int complete);

ssStatus_t addXML_end   (ssBufDesc_t *desc, 
			 charStr *el);

ssStatus_t addXML_append_str(ssBufDesc_t *desc, 
			     charStr *str);

ssStatus_t addXML_queryResultNode (ssBufDesc_t *bD, 
				   const ssPathNode_t *node);

ssStatus_t addXML_templateTriple(gpointer _triple, 
				 GHashTable *prefix_ns_map, 
				 gpointer _bd);

//#define MAX_MSGNRO        99
#define MAX_MSGNRO_DIGITS 10
#define MSGNRO_FORMAT "%d"
#define SSAP_IND_WRAP_NUM 10000
#endif
