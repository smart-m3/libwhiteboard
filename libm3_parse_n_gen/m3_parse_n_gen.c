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
/*****************************************************************
 * m3_parse_n_gen.c
 *
 * Provides parsers and generators for components of the XML messages exchanged with a Smart Space (SS) Semantic Information Broker (SIB)
 * related to SIB operations (SSAP message "payloads" handled by the KP API).
 * Uses glib and Expat libraries to read an XML message from the SS SIB
 * 
 * Must be used with Expat compiled for UTF-8 output.
 */

#include "config.h"

//#ifdef SIB_ROLE
// the compiler swtich -DSIB_ROLE defines the SIB role in the SSAP message exchanges
// and should not use the #else part in order to allow both roles, for testing
//#endif
//#ifdef SIBUSER_ROLE
// the compiler swtich -DSIBUSER_ROLE defines the SIB-user role in the SSAP message exchanges
// and should not use the #else part in order to allow both roles, for testing
//#endif
//#ifdef SSAP_BOTH_ROLES
//#define SIB_ROLE
//#define SIBUSER_ROLE
//#endif

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef SIB_ROLE
#include <uuid.h>
#endif

#define _TESTING 1

#include "sibmsg.h"
#include "m3_sib_tokens.h"
#include "whiteboard_log.h"


#define BUFF_INCREMENT 2048
#define PRINT_TRACE 0
#define PRINT_MSG 0
#define REMOVE_TESTING 1
#if defined(__amigaos__) && defined(__USE_INLINE__)
#include <proto/expat.h>
#else
#include <expat.h>
#endif

#ifdef XML_LARGE_SIZE
#if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
#define XML_FMT_INT_MOD "I64"
#else
#define XML_FMT_INT_MOD "ll"
#endif
#else
#define XML_FMT_INT_MOD "l"
#endif

gchar *errorContentNotInBinding = "unexpected character content outside of <uri> or <literal>";

typedef struct {
  ssStatus_t parseStatus;
  XML_Parser  p;
} commonParseBlk_t;

typedef struct {
  commonParseBlk_t c;
  gboolean list_on;
  GSList **results;
  gboolean inUri;
  gboolean inLiteral;

  //  gpointer *pp;
  gpointer pp;
#ifdef SIB_ROLE
  gboolean inExpr;
  ssWqlDesc_t *qD;
#endif
} wqlParseBlk_t;

typedef struct {
  commonParseBlk_t c;
  int inTripleList; //was: rdfns_specified; i.e. start of triple list XML doc.
  charStr inUseSibNsUri;
  int depth;
  GHashTable *sibs_prefix_ns_map;
  GHashTable *usrs_prefix_ns_map;
  GSList ** tripleList_pp;
  ssTriple_t *currentTriple;
  ssElement_t currentSubject; //assigned only in STD_RDF parse mode
#ifdef SIB_ROLE
  GHashTable * bnode_name2uri_map;
#endif
  ssBufDesc_t *bD_bNodeUriListXML;
  int predicateDepth; /* to limit mistaking deep stuff as subject's predicate */
  ssElement_t *tripleComponentToParse;
  enum {STD_RDF, SIB_REIFIED} parseMode;
} ParseTriplesBlk;

typedef struct {
  commonParseBlk_t c;
  GSList **selectedVariables;
  GSList **results;
  GHashTable *usrs_prefix_ns_map;
  gboolean inResults,
    inResult,
    inBindingsList,
    inBinding,
    inHead,
    inVariable,
    doneVariableList;
  gint valueIndex;
  gchar *parsedCharTxt;
} ParseSparqlSelectBlk;

/*=============================================================================*/
#ifndef WHITEBOARD_NODE_H
//These should be already in whiteboard_node.h
/*-----------------------------------------------------------------------------*/
ssStatus_t ssPrependTriple (GSList **currentList, ssElement_ct subject, ssElement_ct predicate, ssElement_ct object, ssElementType_t subjType, ssElementType_t objType);
void ssFreeTriple( ssTriple_t *triple);
void ssFreePathNode( ssPathNode_t *pathNode);
void ssFreeTripleList (GSList **tripleList);
void ssFreePathNodeList (GSList **pathNodeList);
ssStatus_t new_prefix2ns_map(const gchar *ns, GHashTable **map);
gchar *fullUri (const gchar *s, int sLen, GHashTable *prefix_ns_map);
void nsLocal2prefixLocal (gchar **g_heap_str, GHashTable *prefix2ns_map);
#endif
#ifndef SIBMSG_H
//These should be already in sibmsg.h
/*-----------------------------------------------------------------------------*/
ssStatus_t addXML_templateTriple(gpointer _triple, GHashTable *prefix_ns_map, gpointer _bd);
ssStatus_t addXML_start (ssBufDesc_t *bD, charStr *el, attrStr *attr, charStr *value, int complete);
ssStatus_t addXML_end   (ssBufDesc_t *bD, charStr *el);
ssStatus_t addXML_append_attribute (ssBufDesc_t *bD, attrStr *attr);
ssStatus_t addXML_append_str(ssBufDesc_t *desc, charStr *str);
ssStatus_t addXML_close_element (ssBufDesc_t *bD);
ssStatus_t addXML_close_element_append_content_end (ssBufDesc_t *bD, charStr *content, charStr *el);
ssStatus_t addXML_pathNode (ssBufDesc_t *bD, const ssPathNode_t *node, charStr *nameVal);
ssStatus_t generateSPARQLSelectQueryString(ssBufDesc_t *bD, GSList *selectVars, GSList *where, GSList *optionals, GHashTable *namespaces);
ssStatus_t addXML_query_w_wql_n_e (ssBufDesc_t *bD, QueryType type, const ssPathNode_t *node, const gchar *expr);
ssStatus_t addXML_query_w_wql_n_e (ssBufDesc_t *bD, QueryType type, const ssPathNode_t *node, const gchar *expr);
ssStatus_t addXML_query_w_wql_n_e_n (ssBufDesc_t *bD, QueryType type, const ssPathNode_t *startNode, const gchar *expr, const ssPathNode_t *endNode);
ssStatus_t addXML_query_w_wql_n_n( ssBufDesc_t *bD, QueryType type, const ssPathNode_t *pathNode1, const ssPathNode_t *pathNode2);
ssStatus_t addXML_query_w_wql_n( ssBufDesc_t *bD, QueryType type, const ssPathNode_t *pathNode);
ssStatus_t addXML_open_element (ssBufDesc_t *bD, charStr *el);
ssBufDesc_t *ssBufDesc_new(void);
void ssBufDesc_free(ssBufDesc_t **_bd);
gchar *ssBufDesc_GetMessage(ssBufDesc_t *desc);
gint ssBufDesc_GetMessageLen(ssBufDesc_t *desc);
ssStatus_t parseM3_query_cnf_wql(GSList **results, const gchar * response);
ssStatus_t parseM3_triples(GSList ** list_pp, const char * m3XML_triples_str, GHashTable *prefix_ns_map);
ssStatus_t parseM3_query_results_sparql_select(GSList **selectedVariables, GSList **valRows, const gchar *resultXML, /*const*/ GHashTable *prefix_ns_map);
#endif
/*-----------------------------------------------------------------------------*/
typedef struct {
  char **uri_p;
  gboolean done;
} nsLocal2prefix_ctrlBlk;
//statics herein:
static ssStatus_t ssBufDesc_buf_realloc(ssBufDesc_t *bD, gint newDatLen);
#define SUFFIXeqSTR(PREFIXcharStr,SUFFIXstr,str) (g_str_has_prefix(str, PREFIXcharStr.txt) && 0==strcmp(&str[PREFIXcharStr.len], SUFFIXstr))
static void XMLCALL ns2hash_hndl (void *data, const XML_Char *prefix, const XML_Char *uri);
static ssStatus_t xmlns_str_2_hash(const gchar *ns, GHashTable *prefix2ns_map);
static ssStatus_t ssBufDesc_buf_realloc(ssBufDesc_t *bD, gint newDatLen);
#ifdef SIBUSER_ROLE
static void XMLCALL queryWQLCnfHandler (void *data, const char *s, int sLen);
#endif
#ifdef SIB_ROLE
static void XMLCALL queryWQLReqHandler (void *data, const char *s, int sLen);
#endif
static void XMLCALL queryWQLStart (void *data, const char *el, const char **attr);
static void XMLCALL queryWQLEnd (void *data, const char *el);
static void XMLCALL startNameSpaceHndl(void *data, const XML_Char *prefix, const XML_Char *uri);
static void XMLCALL endNameSpaceHndl (void *data, const XML_Char *prefix);
static void nsLocal2prefixLocal_hndl (gpointer _prefix, gpointer _ns, gpointer data);
static void XMLCALL charhndl(void *data, const char *s, int sLen);
static void XMLCALL start(void *data, const char *el, const char **attr);
static void XMLCALL end (void *data, const char *el);
static ssStatus_t parseM3XML_triples_start (ParseTriplesBlk *blk);
static void parseM3XML_triples_end (ParseTriplesBlk *blk);
static gchar *enclose_CDATA_section( gchar *str);

static void XMLCALL CDATA_start(void* data);
static void XMLCALL CDATA_end(void* data);
static void XMLCALL sparqlSelect_charhndl(void *data, const char *s, int sLen);
//end of statics
/*-----------------------------------------------------------------------------*/
/*=============================================================================*/

ssStatus_t ssPrependTriple (GSList **currentList, ssElement_ct subject, ssElement_ct predicate, ssElement_ct object, ssElementType_t subjType, ssElementType_t objType)
{
  whiteboard_log_debug_fb();

  //ssPrependTriple may be used to build triple lists for new information or to match existing information.
  //Different rules apply in each case for acceptable subject, predicate, and object NULL values and 
  //corresponding subject and object types. THUS, check here is only for common restrictions.
  g_return_val_if_fail(currentList
		       && predicate
		       && (subjType==ssElement_TYPE_URI || subjType==ssElement_TYPE_BNODE)
		       && ( objType==ssElement_TYPE_URI ||  objType==ssElement_TYPE_BNODE ||
			   (object && objType==ssElement_TYPE_LIT)),
		       ss_InvalidParameter);

  ssTriple_t *triple = (ssTriple_t *)g_new0(ssTriple_t,1);
  g_return_val_if_fail(triple, ss_NotEnoughResources);
  triple->subject =     (subject==ssMATCH_ANY)?(ssElement_t)ssMATCH_ANY //not to be freed!
                                              :(ssElement_t)g_strdup((char *)subject);
  triple->predicate = (predicate==ssMATCH_ANY)?(ssElement_t)ssMATCH_ANY //not to be freed!
                                              :(ssElement_t)g_strdup((char *)predicate);
  triple->object = (object==ssMATCH_ANY)?(ssElement_t)ssMATCH_ANY //not to be freed!
                                        :(ssElement_t)g_strdup((char *)object);

  triple->subjType = subjType;
  triple->objType = objType;
  
  *currentList = g_slist_prepend(*currentList, triple);
  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t ssCopyTriple( ssTriple_t *src, ssTriple_t **dst)
{
  ssTriple_t *triple = (ssTriple_t *)g_new0(ssTriple_t,1);
  g_return_val_if_fail(triple, ss_NotEnoughResources);

  triple->subject =     (src->subject==ssMATCH_ANY)?(ssElement_t)ssMATCH_ANY //not to be freed!
    :(ssElement_t)g_strdup((char *)src->subject);
  triple->predicate = (src->predicate==ssMATCH_ANY)?(ssElement_t)ssMATCH_ANY //not to be freed!
    :(ssElement_t)g_strdup((char *)src->predicate);
  triple->object =       (src->object==ssMATCH_ANY)?(ssElement_t)ssMATCH_ANY //not to be freed!
    :(ssElement_t)g_strdup((char *)src->object);
  triple->subjType = src->subjType;
  triple->objType = src->objType;
  
  *dst = triple;
  return ss_StatusOK;
}

void ssFreeTriple( ssTriple_t *triple)
{
  if(!triple)
    return; //nothing to do
  
  if (triple->subject && triple->subject!=ssMATCH_ANY) g_free (triple->subject);
  if (triple->predicate && triple->predicate!=ssMATCH_ANY) g_free (triple->predicate);
  if (triple->object && triple->object!=ssMATCH_ANY) g_free (triple->object);

  g_free (triple);
}

void ssFreePathNode( ssPathNode_t *pathNode)
{
  if(!pathNode)
    return; //nothing to do

  if (pathNode->string && pathNode->string!=ssMATCH_ANY) g_free (pathNode->string);
  g_free (pathNode);
}

void ssFreeTripleList (GSList **tripleList)
{
  if (!tripleList || !*tripleList)
    return; //nothing to do

  GSList *list = *tripleList;
  ssTriple_t *triple;
  whiteboard_log_debug_fb();
  while (list)
    {
      triple = list->data;
      list = g_slist_remove(list,triple);
      ssFreeTriple(triple);
    }
  *tripleList = NULL;
  whiteboard_log_debug_fe();
}

void ssFreePathNodeList (GSList **pathNodeList)
{
  if(!pathNodeList || !*pathNodeList)
    return; //nothing to do

  GSList *list = *pathNodeList;
  ssPathNode_t *pathNode;
  whiteboard_log_debug_fb();
  while (list)
    {
      pathNode = list->data;
      list = g_slist_remove(list,pathNode);
      ssFreePathNode(pathNode);
    }
  *pathNodeList = NULL;
  whiteboard_log_debug_fe();
}

static void XMLCALL
ns2hash_hndl (void *data,
	      const XML_Char *prefix,
	      const XML_Char *uri)
{
  whiteboard_log_debug_fb();
  g_return_if_fail (data);
  g_hash_table_insert((GHashTable *)data, (prefix)?g_strdup(prefix):g_strdup(""), (gpointer)g_strdup(uri));
  whiteboard_log_debug_fe();
}

static ssStatus_t xmlns_str_2_hash(const gchar *ns, GHashTable *prefix2ns_map)
{ 
  ssStatus_t status;
  XML_Parser ns_parser;
  
  whiteboard_log_debug_fb();
  g_return_val_if_fail (ns && prefix2ns_map, ss_InvalidParameter);

  ns_parser = XML_ParserCreateNS(NULL, 0);
  if(!ns_parser)
    return ss_NotEnoughResources;

  XML_SetNamespaceDeclHandler(ns_parser, ns2hash_hndl, NULL);
  XML_SetUserData(ns_parser, prefix2ns_map);

  status = (          !XML_Parse(ns_parser, "<ns ", 4, 0))?     ss_ParsingError:0;
  status = (status || !XML_Parse(ns_parser, ns, strlen(ns), 0))?ss_ParsingError:0;
  status = (status || !XML_Parse(ns_parser, "></ns>", 6, 1))?   ss_ParsingError:0;

  XML_ParserFree (ns_parser);
  whiteboard_log_debug_fe();
  return status;
}

ssStatus_t new_prefix2ns_map(const gchar *ns, GHashTable **map)
{
  g_return_val_if_fail (ns!=NULL && *map==NULL, ss_InvalidParameter);
  ssStatus_t status;
  
  *map = g_hash_table_new_full(g_str_hash,g_str_equal, g_free, g_free);
  status = (*map)? xmlns_str_2_hash(ns, *map) : ss_NotEnoughResources;
  if (status && *map) {
    g_hash_table_destroy (*map);
    *map=NULL;
  }

  g_return_val_if_fail (status==ss_StatusOK, status); //make some noise
  return status;
}

//the returned string, if not NULL, must be freed with g_free (as in many other places.. :-O
gchar *fullUri (const gchar *s, int sLen, GHashTable *prefix_ns_map)
{
  whiteboard_log_debug_fb();
  if(s==NULL)
    {
        whiteboard_log_debug_fe();
	return NULL;
    }
  
  gchar *result;
  int i=0,l=0;
  char *uri=NULL;

  if (prefix_ns_map)
    {
      //get i, the index of ':' in s, assuming it delimits a prefix, assign the matching uri 
      for (i=0; i<sLen && s[i]!=':';i++);
      if (i<sLen-1) 
	{ //valid prefix only if something comes after ':' 
	  char *prefix = g_strndup(s,i);
	  if( strncmp(prefix, "http",4) != 0)
	    {
	      uri = (char *)g_hash_table_lookup(prefix_ns_map, prefix);
	    }
	  g_free(prefix);
	}
      
      if (uri)
	{
	  i++;//index of local name, after ':'
	  l=strlen(uri);//size of uri
	}
      else 
	{
	  //printf("fullUri: found no uri\n");
	  i = 0;//no prefix
	  l = 0;//no uri
	}
    }
  //printf("fullUri: allocating string of size %d\n", l+sLen-i+1);
  result = g_new0(gchar, l+sLen-i+1);
  if (l)
    strncpy(result, uri, l);

  strncpy(result+l, s+i, sLen-i);
 
  //are you SURE this is not needed? result[sLen+l-i] = 0; /* NULL-terminate the string */
  whiteboard_log_debug_fe();
  return result;
}

ssStatus_t addXML_templateTriple(gpointer _triple, GHashTable *prefix_ns_map, gpointer _bd)
{
  whiteboard_log_debug_fb();
  ssBufDesc_t *bD = (ssBufDesc_t *)_bd;
  ssTriple_t *triple = _triple;
  attrStr attr;
  charStr _str0;
  ssStatus_t status;
  
  status = addXML_start (bD, &SIB_TRIPLE, NULL, NULL, 0);
  g_return_val_if_fail(status == ss_StatusOK, status);

  //this coding serves BOTH cases of expicit and match specification of triples.
  //the first can have no wildcards, the second no blanknodes.
  //checking for correct specifications according to the case SHOULD be done where handled.

  attr.name = &SIB_TYPE;
  if (triple->subject) 
    {
      if( triple->subjType==ssElement_TYPE_URI) //default and most common(?) case
	attr.defined = &SIB_URI;
      else if( triple->subjType==ssElement_TYPE_LIT )
	attr.defined = &SIB_LITERAL;
      else if(triple->subjType == ssElement_TYPE_BNODE )
	attr.defined = &SIB_BNODE;
      else
	whiteboard_log_warning("invalid subject type");
    }
  //else somebody didn't check the validity!
  else
    {
      if (triple->subjType==ssElement_TYPE_BNODE)
	attr.defined = &SIB_BNODE;
      else
	attr.defined = &SIB_URI;
    }
  
  if (triple->subject || triple->subjType==ssElement_TYPE_BNODE) 
    {
      char *full_uri = (!prefix_ns_map || !triple->subject)?NULL:fullUri((char *)triple->subject, strlen((char *)triple->subject), prefix_ns_map);
      _str0.txt = (full_uri)?full_uri:(char *)triple->subject;
      _str0.len = (_str0.txt)? strlen(_str0.txt):0;
#if 0 //long form
      status = addXML_start (bD, &SIB_SUBJECT, &attr, &_str0, 1);
#else //short form
      status = addXML_start (bD, &SIB_SUBJECT, &attr, (_str0.txt)?&_str0:NULL, 1);
#endif
      if (full_uri)
	g_free(full_uri);
    }
  else
    {
      status = addXML_start (bD, &SIB_SUBJECT, &attr, &SIB_MATCH_ANY, 1);
    }
  g_return_val_if_fail(status == ss_StatusOK,status);

  if (triple->predicate)
    {
    _str0.txt = fullUri((char *)triple->predicate, strlen((char *)triple->predicate), prefix_ns_map);
    _str0.len = strlen(_str0.txt);
    status = addXML_start (bD, &SIB_PREDICATE, NULL, &_str0, 1);
    if (_str0.txt)
      g_free((gchar *)_str0.txt);
    }
  else
    {
      status = addXML_start (bD, &SIB_PREDICATE, &attr, &SIB_MATCH_ANY, 1);
    }

  g_return_val_if_fail(status == ss_StatusOK,status);
  
  if (triple->object) 
    {
      if (triple->objType==ssElement_TYPE_URI) //default and most common(?) case
	attr.defined = &SIB_URI;
      else if (triple->objType==ssElement_TYPE_LIT)
	attr.defined = &SIB_LITERAL;
      else if (triple->objType==ssElement_TYPE_BNODE)
	attr.defined = &SIB_BNODE;
      else
	whiteboard_log_warning("invalid object type");
    }
  else
    {
      if (triple->objType==ssElement_TYPE_BNODE)
	attr.defined = &SIB_BNODE;
      else
	attr.defined = &SIB_URI;
    }

  if (triple->object || triple->objType==ssElement_TYPE_BNODE) 
    {
      gchar *full_uri = (!prefix_ns_map || !triple->object)?NULL:fullUri((char *)triple->object, strlen((char *)triple->object), prefix_ns_map);
      
      if(triple->objType==ssElement_TYPE_LIT)
	{
	  gchar *tmp = (gchar *)((full_uri)?full_uri:(char *)triple->object);
	  
	  //status = addXML_start (bD, &SIB_OBJECT, &attr, &SIB_CDATA_START, 0);
	  status = addXML_start (bD, &SIB_OBJECT, &attr, NULL, 0);
	  g_return_val_if_fail(status == ss_StatusOK,status);

	  gchar *legalstring = enclose_CDATA_section(tmp);
	  if(legalstring)
	    {
	      _str0.txt = (char *)legalstring;
	      _str0.len = strlen( (char *)legalstring);
	    }
	  else
	    {
	      _str0.txt=NULL;
	      _str0.len=0;
	    }
	  
	  status = addXML_append_str( bD, &_str0 );
	  
	  if(legalstring)
	    g_free(legalstring);
	  
	  g_return_val_if_fail(status == ss_StatusOK,status);
	  //	  status = addXML_append_str( bD, &SIB_CDATA_END);
	  //	  g_return_val_if_fail(status == ss_StatusOK,status);
	  status = addXML_end(bD, &SIB_OBJECT);
	  g_return_val_if_fail(status == ss_StatusOK,status);
	}
      else
	{
	  _str0.txt = (full_uri)?full_uri:(char *)triple->object;
	  _str0.len = (_str0.txt)? strlen(_str0.txt):0;

	  status = addXML_start (bD, &SIB_OBJECT, &attr, (_str0.txt)?&_str0:NULL, 1);
	}
      
      if (full_uri)
	g_free(full_uri);
    }
  else
    {
      status = addXML_start (bD, &SIB_SUBJECT, &attr, &SIB_MATCH_ANY, 1);
    }
  
  g_return_val_if_fail(status == ss_StatusOK,status);
  
  status = addXML_end (bD, &SIB_TRIPLE);
  whiteboard_log_debug_fb();
  g_return_val_if_fail(status == ss_StatusOK,status);
  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

gchar *enclose_CDATA_section( gchar *str)
{
  gchar *illegal = "]]>";
  gchar *legal = "]]]]><![CDATA[>";
  gchar *result = NULL;
  if( strlen(str) > 0)
    {
      gchar **splitted = g_strsplit( str, illegal,-1);

      int i = 0;
      int count = g_strv_length( splitted );
      GString *legalstring = g_string_new("<![CDATA[");
      
      for( i = 0; i < count; i++)
	{
	  if( (i == 0) && (strlen( splitted[i] ) == 0) ) // starts with illegal 
	{
	  legalstring = g_string_append(legalstring, legal);
	}
      else if( strlen(splitted[i]) > 0 )
	{
	  legalstring = g_string_append(legalstring, splitted[i] );
	  if( (i+1 < count) )
	    {
	      legalstring = g_string_append(legalstring, legal);
	    }
	}	
	}
      legalstring = g_string_append(legalstring,"]]>");
      result = legalstring->str;
      g_strfreev(splitted);
      g_string_free( legalstring,FALSE);
    }
  return result;
}

ssStatus_t addXML_start (ssBufDesc_t *bD, charStr *el, attrStr *attr, charStr *value, int complete)
{
  int newLen;
  int terminationLen;
  gint newDatLen;
  whiteboard_log_debug_fb();
  if (!el)
    return ss_InternalError;
  
  if (complete && value)
    terminationLen = 1 // ">"
      + value->len
      + 2 // "</"
      + el->len
      + 1 // ">"
#if PRINT_MSG
      + 1 // '\n'
#endif
      ;
  else if (complete && attr)
    terminationLen = 2 // "/>"
#if PRINT_MSG
      + 1 // '\n'
#endif
      ;
  else if (!complete)
    terminationLen = 1 // ">"
#if PRINT_MSG
      + 1 // '\n'
#endif
      + ((value)? value->len:0)
      ;
  else
    return ss_InternalError;

  newLen =
  1 // <
  + el->len // element
  + ((attr)?(1 // ' '
	     + attr->name->len
	     + 2 // ="
	     + attr->defined->len
	     + 1 // "
	     ):0)
      + terminationLen;

  newDatLen = bD->datLen + newLen +1;
  if(ssBufDesc_buf_realloc( bD, newDatLen) != ss_StatusOK)
    {
      return ss_NotEnoughResources;
    }  
  char *p = &bD->buf[bD->datLen];
  bD->datLen += newLen;

  *p++ = '<';
  strncpy (p, el->txt, el->len);
  p += el->len;

  if (attr) {
    *p++ = ' ';
    strncpy (p, attr->name->txt, attr->name->len);
    p += attr->name->len;
    *p++ = '=';
    *p++ = '"';
    strncpy (p, attr->defined->txt, attr->defined->len);
    p += attr->defined->len;
    *p++ = '"';
  }
  if (complete && value) {
    *p++ = '>';
    strncpy (p, value->txt, value->len);
    p += value->len;
    *p++ = '<';
    *p++ = '/';
    strncpy (p, el->txt, el->len);
    p += el->len;
    *p++ = '>';
#if PRINT_MSG
    *p++ = '\n';
#endif
  }
  else if (complete && attr) {
    *p++ = '/';
    *p++ = '>';
#if PRINT_MSG
    *p++ = '\n';
#endif
    }
  else if (!complete) {
    *p++ = '>';
#if PRINT_MSG
    *p++ = '\n';
#endif
    if (value) {
    strncpy (p, value->txt, value->len);
    p += value->len;
    }
  }

  *p = 0;
    whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t addXML_end   (ssBufDesc_t *bD, charStr *el)
{
  int newLen;
  int newDatLen;
  whiteboard_log_debug_fb();
  if (!el)
    return ss_InternalError;
  
  newLen =
    + 2 // "</"
    + el->len
    + 1 // ">"
#if PRINT_MSG
    + 1 // '\n';
#endif
    ;

  newDatLen = bD->datLen + newLen +1;
  if(ssBufDesc_buf_realloc( bD, newDatLen) != ss_StatusOK)
    {
      return ss_NotEnoughResources;
    }


  char *p = &bD->buf[bD->datLen];
  bD->datLen += newLen;

  *p++ = '<';
  *p++ = '/';
  strncpy (p, el->txt, el->len);
  p += el->len;
  *p++ = '>';
#if PRINT_MSG
  *p++ = '\n';
#endif

  *p = 0;
  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t addXML_append_attribute (ssBufDesc_t *bD, attrStr *attr)
{
  int newLen;
  gint newDatLen;
  whiteboard_log_debug_fb();
  if (!attr)
    return ss_InternalError;

  newLen =
  + 1 // ' '
  + attr->name->len
  + 2 // ="
  + attr->defined->len
  + 1 // "
  ;
  newDatLen = bD->datLen + newLen +1;
  if(ssBufDesc_buf_realloc( bD, newDatLen) != ss_StatusOK)
    {
      return ss_NotEnoughResources;
    }
  char *p = &bD->buf[bD->datLen];
  bD->datLen += newLen;

  *p++ = ' ';
  strncpy (p, attr->name->txt, attr->name->len);
  p += attr->name->len;
  *p++ = '=';
  *p++ = '"';
  strncpy (p, attr->defined->txt, attr->defined->len);
  p += attr->defined->len;
  *p++ = '"';

  *p = 0;
  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t addXML_append_str(ssBufDesc_t *desc, charStr *str)
{
  gint newLen = str->len;
  gint newDatLen;
  whiteboard_log_debug_fb();
  newDatLen = desc->datLen + newLen +1;
  if(ssBufDesc_buf_realloc( desc, newDatLen) != ss_StatusOK)
    {
      return ss_NotEnoughResources;
    }
  
  strncpy(desc->buf + desc->datLen, str->txt, str->len);
  desc->datLen += str->len;
  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t addXML_close_element (ssBufDesc_t *bD)
{
  int newLen;
  gint newDatLen;
  whiteboard_log_debug_fb();
  newLen =
    1 // >
#if PRINT_MSG
    + 1 // '\n'
#endif
  ;
  newDatLen = bD->datLen + newLen +1;
  if(ssBufDesc_buf_realloc( bD, newDatLen) != ss_StatusOK)
    {
      return ss_NotEnoughResources;
    }
  
  char *p = &bD->buf[bD->datLen];
  bD->datLen += newLen;

    *p++ = '>';
#if PRINT_MSG
    *p++ = '\n';
#endif

  *p = 0;
  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t addXML_close_element_append_content_end (ssBufDesc_t *bD, charStr *content, charStr *el)
{
  ssStatus_t status;

  status = addXML_close_element(bD);
#if PRINT_MSG
  //remove newline and rezero
  bD->buf[--bD->datLen]=0;
#endif
  status = (status)?status : addXML_append_str(bD, content);
  status = (status)?status : addXML_end (bD, el);
  return status;
}

ssStatus_t addXML_pathNode (ssBufDesc_t *bD, const ssPathNode_t *node, charStr *nameVal)
{
  ssStatus_t status;
  attrStr attr;
  charStr cont;
  
  status = addXML_open_element(bD, &SIB_PATH_NODE);
  g_return_val_if_fail(status == ss_StatusOK, status);

  if (nameVal) {
    attr.name = &SIB_NAME;
    attr.defined = nameVal;
    status = addXML_append_attribute(bD, &attr);
    g_return_val_if_fail(status == ss_StatusOK, status);
  }

  attr.name = &SIB_TYPE;
  attr.defined = (node->nodeType==ssElement_TYPE_LIT)?
    &SIB_LITERAL : &SIB_URI;
  status = addXML_append_attribute(bD, &attr);
  g_return_val_if_fail(status == ss_StatusOK, status);

  if(node->nodeType==ssElement_TYPE_LIT)
    {
      status = addXML_close_element(bD);
      gchar *tmp = enclose_CDATA_section ( (gchar *)node->string);
      if(tmp)
	{
	  cont.txt = tmp;
	  cont.len = strlen(tmp);
	}
      else
	{
	  cont.txt = NULL;
	  cont.len = 0;
	}
      status = addXML_append_str (bD, &cont);

      if(tmp)
	 g_free(tmp);

      g_return_val_if_fail(status == ss_StatusOK, status);
      status = addXML_end( bD, &SIB_PATH_NODE);
      g_return_val_if_fail(status == ss_StatusOK, status);
      
    }
  else
    {
      cont.txt = (char *)node->string;
      cont.len = (!cont.txt)?0:strlen(cont.txt);
      status = addXML_close_element_append_content_end (bD, &cont, &SIB_PATH_NODE);
    }
  g_return_val_if_fail(status == ss_StatusOK, status);

  return status;
}

ssStatus_t addXML_queryResultNode(ssBufDesc_t *bD, const ssPathNode_t *node)
{
  /* Added by jho. Generates a query response string from a single node */
  ssStatus_t status;
  charStr cont;
  
  if (node->nodeType==ssElement_TYPE_LIT)
    status = addXML_start(bD, &SIB_LITERAL, NULL, NULL, 0);
  else
    status = addXML_open_element(bD, &SIB_URI);
  g_return_val_if_fail(status == ss_StatusOK, status);

  if (node->nodeType==ssElement_TYPE_LIT)
    {
      gchar *tmp = enclose_CDATA_section ( (gchar *)node->string);
      if( tmp)
	{
	  cont.txt = tmp;
	  cont.len = strlen(tmp);
	}
      else
	{
	  cont.txt=NULL;
	  cont.len=0;
	}
      status = addXML_append_str (bD, &cont);

      if(tmp)
	g_free(tmp);

      g_return_val_if_fail(status == ss_StatusOK, status);
      status = addXML_end( bD, &SIB_LITERAL);
      g_return_val_if_fail(status == ss_StatusOK, status);
    }
  else
    {
      cont.txt = (char *)node->string;
      cont.len = (!cont.txt)?0:strlen(cont.txt);

      status = addXML_close_element_append_content_end (bD, &cont, &SIB_URI);
    }
  g_return_val_if_fail(status == ss_StatusOK, status);
  return status;
}

ssStatus_t addXML_query_w_wql_n_e (ssBufDesc_t *bD, QueryType type, const ssPathNode_t *node, const gchar *expr)
{
  ssStatus_t status;
  charStr cont;
  
  whiteboard_log_debug_fb();
  g_return_val_if_fail(type == QueryTypeWQLValues, ss_InternalError);

  status = addXML_start (bD, &SIB_WQLQUERY, NULL, NULL, 0);
  g_return_val_if_fail(status == ss_StatusOK, status);

  status = addXML_pathNode (bD, node, &SIB_PATH_NODE_START);
  g_return_val_if_fail(status == ss_StatusOK, status);

  cont.txt = expr;
  cont.len = (!cont.txt)?0:strlen(expr);
  status = addXML_start(bD, &SIB_PATH_EXPRESSION, NULL, &cont,1);
  g_return_val_if_fail(status == ss_StatusOK, status);

  status = addXML_end (bD, &SIB_WQLQUERY);
  g_return_val_if_fail(status == ss_StatusOK, status);
  
  whiteboard_log_debug_fe();
  return status;
  }

ssStatus_t addXML_query_w_wql_n_e_n (ssBufDesc_t *bD, QueryType type, const ssPathNode_t *startNode, const gchar *expr, const ssPathNode_t *endNode)
{
  ssStatus_t status;
  charStr cont;
  
  whiteboard_log_debug_fb();
  g_return_val_if_fail(type == QueryTypeWQLRelated, ss_InternalError);

  status = addXML_start (bD, &SIB_WQLQUERY, NULL, NULL, 0);
  g_return_val_if_fail(status == ss_StatusOK, status);
  
  status = addXML_pathNode (bD, startNode, &SIB_PATH_NODE_START);
  g_return_val_if_fail(status == ss_StatusOK, status);

  cont.txt = expr;
  cont.len = (!cont.txt)?0:strlen(expr);
  status = addXML_start(bD, &SIB_PATH_EXPRESSION, NULL, &cont,1);
  g_return_val_if_fail(status == ss_StatusOK, status);

  status = addXML_pathNode (bD, endNode, &SIB_PATH_NODE_END);
  g_return_val_if_fail(status == ss_StatusOK, status);

  status = addXML_end (bD, &SIB_WQLQUERY);
  g_return_val_if_fail(status == ss_StatusOK, status);
  
  whiteboard_log_debug_fe();
  return status;
}

ssStatus_t addXML_query_w_wql_n_n( ssBufDesc_t *bD, QueryType type, const ssPathNode_t *pathNode1, const ssPathNode_t *pathNode2)
{
  ssStatus_t status;
  
  whiteboard_log_debug_fb();
  g_return_val_if_fail(type == QueryTypeWQLIsType || type == QueryTypeWQLIsSubType, ss_InternalError);

  status = addXML_start (bD, &SIB_WQLQUERY, NULL, NULL, 0);
  g_return_val_if_fail(status == ss_StatusOK, status);
  
  //??  status = addXML_pathNode (bD, pathNode1, (type==QueryTypeWQLIsType)? NULL : &SIB_PATH_NODE_SUBTYPE);
  status = addXML_pathNode (bD, pathNode1, (type==QueryTypeWQLIsType)? &SIB_PATH_NODE_START : &SIB_PATH_NODE_SUBTYPE);
  g_return_val_if_fail(status == ss_StatusOK, status);

  status = addXML_pathNode (bD, pathNode2, (type==QueryTypeWQLIsType)? &SIB_TYPE : &SIB_PATH_NODE_SUPERTYPE);
  g_return_val_if_fail(status == ss_StatusOK, status);

  status = addXML_end (bD, &SIB_WQLQUERY);
  g_return_val_if_fail(status == ss_StatusOK, status);
  
  whiteboard_log_debug_fe();
  return status;
}

ssStatus_t addXML_query_w_wql_n( ssBufDesc_t *bD, QueryType type, const ssPathNode_t *pathNode)
{
  ssStatus_t status;
  
  whiteboard_log_debug_fb();
  g_return_val_if_fail(type == QueryTypeWQLNodeTypes, ss_InternalError);

  status = addXML_start (bD, &SIB_WQLQUERY, NULL, NULL, 0);
  g_return_val_if_fail(status == ss_StatusOK, status);
  
  status = addXML_pathNode (bD, pathNode, NULL);
  g_return_val_if_fail(status == ss_StatusOK, status);

  status = addXML_end (bD, &SIB_WQLQUERY);
  g_return_val_if_fail(status == ss_StatusOK, status);
  
  whiteboard_log_debug_fe();
  return status;
}

ssStatus_t addXML_open_element (ssBufDesc_t *bD, charStr *el)
{
  ssStatus_t status;
  whiteboard_log_debug_fb();
  status = addXML_start (bD, el, NULL, NULL, 0);
  g_return_val_if_fail(status == ss_StatusOK, status);
  
  bD->datLen = bD->datLen - 1
#if PRINT_MSG
      - 1 // '\n'
#endif
    ;
  bD->buf[bD->datLen] = 0;
  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t generateSPARQLSelectQueryString(ssBufDesc_t *bD, GSList *selectVars, GSList *where, GSList *optionals, GHashTable *namespaces)
{
  charStr _charstr;

  inline charStr *CHARSTR(char *str, int len)
    { _charstr.txt=str;
      _charstr.len=(len>0)? len: strlen(str);
      return &_charstr;
    }

  ssStatus_t status = 0;
  whiteboard_log_debug_fb();

  status = addXML_start (bD, &SIB_SPARQLQUERY, NULL, &SIB_CDATA_START, 0);
  if(status)
    return status;

  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, namespaces);
  while (!status && g_hash_table_iter_next (&iter, &key, &value)) 
    {
      status = addXML_append_str(bD, CHARSTR("PREFIX ", 7));      
      if (!status) status = addXML_append_str(bD, CHARSTR(key,0));      
      if (!status) status = addXML_append_str(bD, CHARSTR(" <",2));      
      if (!status) status = addXML_append_str(bD, CHARSTR(value,0));      
      if (!status) status = addXML_append_str(bD, CHARSTR(">\n",2));
    }
  if (status) 
    return status;

  ssPathNode_t *pn;
  status = addXML_append_str(bD, CHARSTR("SELECT",6));
  if (!status && !selectVars)
    status = addXML_append_str(bD, CHARSTR("*",1)); //select all variables
  else for ( ; !status && selectVars; selectVars = selectVars->next)
    {
      pn = selectVars->data;
      if (!pn || strlen((char *)pn->string) < 1 || pn->nodeType != ssElement_TYPE_BNODE)
	return ss_InvalidParameter;
      status = addXML_append_str(bD, CHARSTR(" ?",2));
      if (!status) status = addXML_append_str(bD, CHARSTR((char*)pn->string,0));
    }

  if (!status) status = addXML_append_str(bD, CHARSTR("\n",1));
  if (status) 
    return status;

  sparqlTriple_t *t;
  status = addXML_append_str(bD, CHARSTR("WHERE {\n",8));
  for ( ; !status && where; where = where->next)
    {
      t = where->data;
      if (!t)
	return ss_InvalidParameter;

      status = addXML_append_str(bD, CHARSTR(" ",1));
      if (status) 
	return status;

      int i;
      for (i = 0; i < 3; i++)
	{
	  switch(i) {
	  case 0: pn = &(t->subject); break;
	  case 1: pn = &(t->predicate); break;
	  case 2: pn = &(t->object); break;
	  }

	  if (pn->nodeType == ssElement_TYPE_BNODE)
	    {
	      status = addXML_append_str(bD, CHARSTR(" ?",2));
	      if (!status) status = addXML_append_str(bD, CHARSTR((char*)pn->string,0));
	    }
	  else if (i==2 && pn->nodeType == ssElement_TYPE_LIT)
	    {
	      status = addXML_append_str(bD, CHARSTR(" \"",2));
	      if (!status) status = addXML_append_str(bD, CHARSTR((char*)pn->string,0));
	      if (!status) status = addXML_append_str(bD, CHARSTR("\"",1));
	    }
	  else if (pn->nodeType == ssElement_TYPE_URI)
	    {
	      gchar *namespace = NULL;
	      gchar **prefix_p = g_strsplit((const gchar*)pn->string, ":", 2);
	      if (*prefix_p)
		namespace = g_hash_table_lookup (namespaces, *prefix_p);
	      if (namespace)
		{
		  status = addXML_append_str(bD, CHARSTR(" ",1));
		  if (!status) status = addXML_append_str(bD, CHARSTR((char*)pn->string,0));
		}
	      else
		{
		  status = addXML_append_str(bD, CHARSTR(" <",2));
		  if (!status) status = addXML_append_str(bD, CHARSTR((char*)pn->string,0));
		  if (!status) status = addXML_append_str(bD, CHARSTR(">",1));
		}
	    }
	  else
	    return ss_InvalidTripleSpecification;

	  if (!status && i==2)
	    status = addXML_append_str(bD, CHARSTR(" .\n",3));
	}
    }

  GSList *optional;

  for ( ; !status && optionals; optionals = optionals->next)
    {
      optional = optionals->data;
      if (!optional)
	return ss_InvalidParameter;

      status = addXML_append_str(bD, CHARSTR("  OPTIONAL {\n",13));

      for ( ; !status && optional; optional = optional->next)
	{
	  t = optional->data;
	  if (!t)
	    return ss_InvalidParameter;
	      
	  status = addXML_append_str(bD, CHARSTR("   ",3));
	  if (status) 
	    return status;

	  int i;
	  for (i = 0; i < 3; i++)
	    {
	      switch(i) {
	      case 0: pn = &(t->subject); break;
	      case 1: pn = &(t->predicate); break;
	      case 2: pn = &(t->object); break;
	      }

	      if (pn->nodeType == ssElement_TYPE_BNODE)
		{
		  status = addXML_append_str(bD, CHARSTR(" ?",2));
		  if (!status) status = addXML_append_str(bD, CHARSTR((char*)pn->string,0));
		}
	      else if (i==2 && pn->nodeType == ssElement_TYPE_LIT)
		{
		  status = addXML_append_str(bD, CHARSTR(" \"",2));
		  if (!status) status = addXML_append_str(bD, CHARSTR((char*)pn->string,0));
		  if (!status) status = addXML_append_str(bD, CHARSTR("\"",1));
		}
	      else if (pn->nodeType == ssElement_TYPE_URI)
		{
		  gchar *namespace = NULL;
		  gchar **prefix_p = g_strsplit( (const gchar*) pn->string, ":", 2);
		  if (*prefix_p)
		    namespace = g_hash_table_lookup (namespaces, *prefix_p);
		  if (namespace)
		    {
		      status = addXML_append_str(bD, CHARSTR(" ",1));
		      if (!status) status = addXML_append_str(bD, CHARSTR((char*)pn->string,0));
		    }
		  else
		    {
		      status = addXML_append_str(bD, CHARSTR(" <",2));
		      if (!status) status = addXML_append_str(bD, CHARSTR((char*)pn->string,0));
		      if (!status) status = addXML_append_str(bD, CHARSTR(">",1));
		    }
		}
	      else
		return ss_InvalidTripleSpecification;
	      
	      if (!status && i==2)
		status = addXML_append_str(bD, CHARSTR(" .\n",3));
	    }
	}

      if (!status) status = addXML_append_str(bD, CHARSTR("  }\n",4));
    }
  
  if (!status) status = addXML_append_str (bD, CHARSTR("}\n",2));
  if (!status) status = addXML_append_str (bD, &SIB_CDATA_END);
  if (!status) status = addXML_end (bD, &SIB_SPARQLQUERY);
  
  whiteboard_log_debug_fe();
  return status;
}


ssBufDesc_t *ssBufDesc_new()
{
  ssBufDesc_t *buf = NULL;
  whiteboard_log_debug_fb();
  buf = g_new0( ssBufDesc_t, 1);
  whiteboard_log_debug_fe();
  return buf;
}

static ssStatus_t ssBufDesc_buf_realloc(ssBufDesc_t *bD, gint newDatLen)
{
  gint numBufs;
  whiteboard_log_debug_fb();
  if (newDatLen > bD->bufLen)
    {
      /* Modification to avoid overtly large realloc --jho */
      numBufs = (newDatLen / BUFF_INCREMENT ) + 1; 
      /* numBufs = (newDatLen % BUFF_INCREMENT ) + 1; */
      bD->buf = (gchar *)g_try_realloc (bD->buf, (bD->bufLen += (numBufs*BUFF_INCREMENT)));
    }
  whiteboard_log_debug_fe();
  if (!bD->buf)
    return ss_NotEnoughResources;
  else
    return ss_StatusOK;
}

void ssBufDesc_free(ssBufDesc_t **_bd)
{
  ssBufDesc_t *bd = *_bd;
  whiteboard_log_debug_fb();
  if (bd->buf)
    g_free( bd->buf );
  bd->buf = NULL;
  bd->datLen=0;
  bd->bufLen=0;
  g_free(bd);
  *_bd = NULL;
  whiteboard_log_debug_fe();
}

gchar *ssBufDesc_GetMessage(ssBufDesc_t *desc)
{
  return( desc->buf);
}

gint ssBufDesc_GetMessageLen(ssBufDesc_t *desc)
{
  return( desc->datLen);
}

//============================ ^- Generators -^  v- Parsers -v ===============================

inline void
parseM3_setQuitParse (commonParseBlk_t * blk, ssStatus_t status)
/* sets the handlers used here to NULL to quit parsing
   - which works only if not reassigned before returning
     control to the parser
*/ 
{
  whiteboard_log_debug_fb();
  XML_SetCharacterDataHandler(blk->p, NULL);
  XML_SetElementHandler(blk->p, NULL, NULL);
  if ( blk->parseStatus!=ss_StatusOK)
    whiteboard_log_error("*** error %i overwrites previously set error %i\n", status, blk->parseStatus);
  blk->parseStatus = status;
  whiteboard_log_debug_fe();
}

#ifdef SIBUSER_ROLE
static void XMLCALL
queryWQLCnfHandler (void *data, const char *s, int sLen)
{
  whiteboard_log_debug_fb();
#if WHITEBOARD_DEBUG==1
  gchar *dbg_txt = g_strndup(s,sLen);
  whiteboard_log_debug("%s", dbg_txt);
  g_free(dbg_txt);
#endif
  wqlParseBlk_t *blk = data;
  if( blk->pp )
    {
      whiteboard_log_debug("Appending to existing blk->pp\n");
      ssPathNode_t *pN = blk->pp;
      gchar *new = g_strndup(s,sLen);
      gchar *old =  (gchar *) pN->string;
      pN->string = (guchar *)g_strconcat( old, new, NULL);
      g_free(old);
      g_free(new);
      whiteboard_log_debug("After append: %s\n", pN->string);
    }
  else
    {
      whiteboard_log_debug("Creating new blk->pp\n");
      ssPathNode_t *pN = g_new0(ssPathNode_t, 1);
      pN->string = (ssElement_t)g_strndup(s,sLen);

      if (blk->inLiteral) //default for uri
	pN->nodeType = ssElement_TYPE_LIT;
      
      blk->pp = pN;
    }

  //  XML_SetCharacterDataHandler(blk->c.p, NULL);
  whiteboard_log_debug_fe();
}
#endif

#ifdef SIB_ROLE
static void XMLCALL
queryWQLReqHandler (void *data, const char *s, int sLen)
{
  whiteboard_log_debug_fb();

#if WHITEBOARD_DEBUG==1
  gchar *dbgtxt = g_strndup(s,sLen);
  whiteboard_log_debug("Parsing: %s\\n", dbgtxt);
  g_free(dbgtxt);
#endif

  wqlParseBlk_t *blk = data;
  if(!blk || !blk->pp ||
     ((blk->inUri)?1:0 + 
      (blk->inLiteral)?1:0 + 
      (blk->inExpr)?1:0)
     != 1) {
    whiteboard_log_debug("queryWQLReqHandler, bad input\n");
    parseM3_setQuitParse(&blk->c, ss_InternalError);
    return;
  }

  if (!blk->inExpr)
    {
      if( blk->pp )
	{ 
	  whiteboard_log_debug("Appending to existing pathnode\n");
	  ssPathNode_t *pN = (ssPathNode_t *)(blk->pp);
	  guchar *old = pN->string;
	  gchar *new = g_strndup(s,sLen);
	  pN->string = (guchar *) g_strconcat((const gchar *)old, new, NULL);
	  g_free(old);
	  g_free(new);
	  whiteboard_log_debug("After append: %s\n", (gchar *)(pN->string));
	}
      else
	{
	  ssPathNode_t *pN = g_new0(ssPathNode_t, 1);
	  pN->string = (ssElement_t)g_strndup(s,sLen);
	  if (blk->inLiteral) //default for uri
	    pN->nodeType = ssElement_TYPE_LIT;
	  blk->pp = pN;
	}
    }
  else
    { 
      if( blk->pp )
	{
	  gchar *new =  g_strndup(s,sLen);
	  gchar *old = blk->pp;
	  whiteboard_log_debug("Appending to existing string:s\n", old);
	  blk->pp = g_strconcat( old, new, NULL);
	  g_free(old);
	  g_free(new);
	  whiteboard_log_debug("After append: %s\n", (char *)(blk->pp));
	}
      else
	{
	  whiteboard_log_debug("Creating new string\n");
	  gchar *tmp = g_strndup(s,sLen);
	  blk->pp = tmp;
	  whiteboard_log_debug("After create: %s\n", (char *)(blk->pp));
	}
    }
  
  //XML_SetCharacterDataHandler(blk->c.p, NULL);
  whiteboard_log_debug_fe();
}
#endif

static void XMLCALL
queryWQLStart (void *data, const char *el, const char **attr)
{
  wqlParseBlk_t *blk = data;

  whiteboard_log_debug_fb();
  g_return_if_fail (blk);
  whiteboard_log_debug("queryWQLStart: Parsing element: %s\n", el);

  if (!blk->list_on ) {
#ifdef SIBUSER_ROLE
    if( 0==strcmp(el, SIB_NODE_LIST.txt))
      {
	blk->list_on = TRUE;
	return;
      }
    else
#endif
#ifdef SIB_ROLE
      if( 0==strcmp(el, SIB_WQLQUERY.txt))
	{
	  blk->list_on = TRUE;
	  return;
	}
      else
#endif
	{
	  whiteboard_log_debug("ERROR: element name: %s\n", el);
	  parseM3_setQuitParse(&blk->c, ss_ParsingError);
	}
  }
  
  if (!blk->list_on)
    return;

#ifdef SIB_ROLE
  //g_return_if_fail (blk->qD);
#endif

#ifdef SIBUSER_ROLE
  if(( (0==strcmp(el, SIB_URICAPS.txt)) ||(0==strcmp(el, SIB_URI.txt)) ) && !(blk->inUri || blk->inLiteral))
    {
      blk->inUri = TRUE;
      ssPathNode_t *pn = g_new0(ssPathNode_t ,1);
      pn->string = (guchar *)g_strdup("");
      pn->nodeType = ssElement_TYPE_URI;
      blk->pp = pn;
      XML_SetCharacterDataHandler(blk->c.p, queryWQLCnfHandler);
    }
  else if( (0==strcmp(el, SIB_LITERAL.txt)) && !blk->inUri && !(blk->inUri || blk->inLiteral))
    {
      blk->inLiteral = TRUE;
      ssPathNode_t *pn = g_new0(ssPathNode_t ,1);
      pn->string = (guchar *)g_strdup("");
      pn->nodeType = ssElement_TYPE_LIT;
      blk->pp = pn;
      XML_SetCharacterDataHandler(blk->c.p, queryWQLCnfHandler);
    }
  else 
#endif
#ifdef SIB_ROLE
    if (0==strcmp(el, SIB_PATH_NODE.txt))
      {
	const char *ppName=NULL, *ppType=NULL;
	/* Default to URI if type not stated --jho */
	blk->inUri = TRUE;
	if (attr[0]) {
	  if (0==strcmp(attr[0], SIB_NAME.txt))
	    ppName = attr[1];
	  else if (0==strcmp(attr[0], SIB_TYPE.txt))
	    ppType = attr[1];

	  if (attr[2]) {
	    if (0==strcmp(attr[2], SIB_NAME.txt))
	      ppName = attr[3];
	    else if (0==strcmp(attr[2], SIB_TYPE.txt))
	      ppType = attr[3];
	  }

	  if (ppType && 0==strcmp(ppType, SIB_LITERAL.txt))
	    {
	      blk->inLiteral = TRUE;
	      blk->inUri = FALSE;
	    }
	  /*
	    if (ppType && 0==strcmp(ppType, SIB_URI.txt))
	    blk->inUri = TRUE;
	    else if (ppType && 0==strcmp(ppType, SIB_LITERAL.txt))
	    blk->inLiteral = TRUE;
	    else
	    blk->inUri = TRUE;
	  */
	}

	XML_SetCharacterDataHandler(blk->c.p, queryWQLReqHandler);
	//error if blk->pp and one of blk->inLiteral or blk->inUri not assigned
      
	switch (blk->qD->qType) {
	case QueryTypeWQLValues:
	  if (ppName && (blk->inUri || blk->inLiteral))
	    {
	      if (0==strcmp(ppName, SIB_PATH_NODE_START.txt))
		{
		  blk->qD->wqlType.values.startNode = g_new0(ssPathNode_t,1 );
		  if(  blk->inLiteral )
		    {
		      blk->qD->wqlType.values.startNode->nodeType = ssElement_TYPE_LIT;
		    }
		  else
		    {
		      blk->qD->wqlType.values.startNode->nodeType = ssElement_TYPE_URI;
		    }
		
		  blk->qD->wqlType.values.startNode->string = (guchar *)g_strdup("");
		  blk->pp = blk->qD->wqlType.values.startNode;
		}
	      else
		{
		  whiteboard_log_debug("ERROR: element name: %s\n", el);
		  parseM3_setQuitParse(&blk->c, ss_ParsingError); 
		}
	    }
	  else if (!ppName && (blk->inUri || blk->inLiteral))
	    {
	      /* If values query does not have a name, treat the node as startNode
		 This is unambiguous as there is only one node for wql values --jh */
	      blk->qD->wqlType.values.startNode = g_new0(ssPathNode_t,1 );
	      if(  blk->inLiteral )
		{
		  blk->qD->wqlType.values.startNode->nodeType = ssElement_TYPE_LIT;
		}
	      else
		{
		  blk->qD->wqlType.values.startNode->nodeType = ssElement_TYPE_URI;
		}
	    
	      blk->qD->wqlType.values.startNode->string = (guchar *)g_strdup("");
	      blk->pp = blk->qD->wqlType.values.startNode;
	    }
	  else
	    {
	      whiteboard_log_debug("ERROR: element name: %s\n", el);
	      parseM3_setQuitParse(&blk->c, ss_ParsingError);
	    }
	  break;
	case QueryTypeWQLNodeTypes:
	  if (!ppName && (blk->inUri || blk->inLiteral))
	    blk->pp = blk->qD->wqlType.nodeTypes.node;
	  else
	    {
	      whiteboard_log_debug("ERROR: element name: %s\n", el);
	      parseM3_setQuitParse(&blk->c, ss_ParsingError);
	    }
	  break;
	case QueryTypeWQLRelated:
	  if (ppName && (blk->inUri || blk->inLiteral))
	    {
	      if (0==strcmp(ppName, SIB_PATH_NODE_START.txt))
		{
		  blk->qD->wqlType.related.startNode= g_new0(ssPathNode_t,1 );
		  blk->qD->wqlType.related.startNode->string = (ssElement_t) g_strdup("");
		   blk->qD->wqlType.related.startNode->nodeType = 
		     (blk->inLiteral ?  ssElement_TYPE_LIT:ssElement_TYPE_URI);
		  blk->pp = blk->qD->wqlType.related.startNode;
		}
	      else if (0==strcmp(ppName, SIB_PATH_NODE_END.txt))
		{
		  blk->qD->wqlType.related.endNode= g_new0(ssPathNode_t,1 );
		  blk->qD->wqlType.related.endNode->string = (ssElement_t) g_strdup("");
		  blk->qD->wqlType.related.endNode->nodeType = 
		     (blk->inLiteral ?  ssElement_TYPE_LIT:ssElement_TYPE_URI);
		  blk->pp = blk->qD->wqlType.related.endNode;
		}
	    }
	  else
	    {
	      whiteboard_log_debug("ERROR: element name: %s\n", el);
	      parseM3_setQuitParse(&blk->c, ss_ParsingError);
	    }
	  break;
	case QueryTypeWQLIsType:
	  if (blk->inUri || blk->inLiteral)
	    {
	      if (!ppName)
		{
		  blk->qD->wqlType.isType.node = g_new0(ssPathNode_t,1 );
		  blk->qD->wqlType.isType.node->string = (ssElement_t) g_strdup("");
		  blk->qD->wqlType.isType.node->nodeType = 
		     (blk->inLiteral ?  ssElement_TYPE_LIT:ssElement_TYPE_URI);
		  blk->pp = blk->qD->wqlType.isType.node;
		}
	      else if (0==strcmp(ppName, SIB_TYPE.txt))
		{
		  blk->qD->wqlType.isType.typeNode = g_new0(ssPathNode_t,1 );
		  blk->qD->wqlType.isType.typeNode->string = (ssElement_t) g_strdup("");
		  blk->qD->wqlType.isType.typeNode->nodeType = 
		     (blk->inLiteral ?  ssElement_TYPE_LIT:ssElement_TYPE_URI);
		  blk->pp = blk->qD->wqlType.isType.typeNode;
		}
	    }
	  else
	    {
	      whiteboard_log_debug("ERROR: element name: %s\n", el);
	      parseM3_setQuitParse(&blk->c, ss_ParsingError);
	    }
	  break;
	case QueryTypeWQLIsSubType:
	  if (ppName && (blk->inUri || blk->inLiteral))
	    {
	      if (0==strcmp(ppName, SIB_PATH_NODE_SUPERTYPE.txt))
		{
		  blk->qD->wqlType.isSubType.superTypeNode = g_new0(ssPathNode_t,1 );
		  blk->qD->wqlType.isSubType.superTypeNode->string = (ssElement_t) g_strdup("");
		  blk->qD->wqlType.isSubType.superTypeNode->nodeType = 
		     (blk->inLiteral ?  ssElement_TYPE_LIT:ssElement_TYPE_URI);
		  blk->pp = blk->qD->wqlType.isSubType.superTypeNode;
		}
	      else if (0==strcmp(ppName, SIB_PATH_NODE_SUBTYPE.txt))
		{
		  blk->qD->wqlType.isSubType.subTypeNode = g_new0(ssPathNode_t,1 );
		  blk->qD->wqlType.isSubType.subTypeNode->string = (ssElement_t) g_strdup("");
		   blk->qD->wqlType.isSubType.subTypeNode->nodeType = 
		     (blk->inLiteral ?  ssElement_TYPE_LIT:ssElement_TYPE_URI);
		   blk->pp = blk->qD->wqlType.isSubType.subTypeNode;
		}
	    }
	  else
	    {
	      whiteboard_log_debug("ERROR: element name: %s\n", el);
	      parseM3_setQuitParse(&blk->c, ss_ParsingError);
	    }
	  break;
	default:;
	}

	if (!blk->pp ) {
	  whiteboard_log_debug("ERROR: element name: %s\n", el);
	  parseM3_setQuitParse(&blk->c, ss_ParsingError);
	}
      }
    else if( !blk->pp && (0==strcmp(el, SIB_PATH_EXPRESSION.txt)) && !attr[0])
      {
	blk->inExpr = TRUE;
	XML_SetCharacterDataHandler(blk->c.p, queryWQLReqHandler);

	switch (blk->qD->qType) {
	case QueryTypeWQLValues:
	case QueryTypeWQLRelated:
	  blk->qD->wqlType.values.pathExpr = g_strdup("");
	  blk->pp = blk->qD->wqlType.values.pathExpr;
	  whiteboard_log_debug("allocatied memory for pathexpression\n");
	  break;
	default:
	  whiteboard_log_debug("ERROR: element name: %s\n", el);
	  parseM3_setQuitParse(&blk->c, ss_ParsingError);
	}

	if (!blk->pp) {
	  whiteboard_log_debug("ERROR: element name: %s\n", el);
	  parseM3_setQuitParse(&blk->c, ss_ParsingError);
	}
      }
    else
#endif
      {
	whiteboard_log_debug("ERROR: element name: %s\n", el);
	parseM3_setQuitParse(&blk->c, ss_ParsingError);
      }
  whiteboard_log_debug_fe();
}

static void XMLCALL
queryWQLEnd (void *data, const char *el)
{
  wqlParseBlk_t *blk = data;
  whiteboard_log_debug_fb();
  whiteboard_log_debug("Rached element end: %s",el);

  if(blk->list_on)
    {
#ifdef SIBUSER_ROLE
      if( blk->inUri && blk->pp && (0 == strcmp(el, SIB_URI.txt)))
	{
 	  // SMART--11
	  //ssPathNode_t *t = blk->pp;
	  whiteboard_log_debug("Prepending URI (%s) to result set\n", blk->pp);
	  blk->inUri = FALSE;
	  *(blk->results) = g_slist_prepend(*(blk->results), (blk->pp));
	  blk->pp = NULL;
	}
      else if(blk->inLiteral && blk->pp && (0 == strcmp(el, SIB_LITERAL.txt)))
	{
	  // SMART--11
	  //ssPathNode_t *t = blk->pp;
	  blk->inLiteral = FALSE;
	  whiteboard_log_debug("Prepending literal (%s) to result set\n", blk->pp);
	  *(blk->results) = g_slist_prepend(*(blk->results), (blk->pp));
	  blk->pp = NULL;
	}
      else if( (0 == strcmp(el, SIB_NODE_LIST.txt) ) &&
	       !blk->inUri && !blk->inLiteral )
	{
	  blk->list_on= FALSE;
	}
      else
#endif
#ifdef SIB_ROLE
	if( 0 == strcmp(el, SIB_PATH_NODE.txt) && blk->pp )
	  {
	    blk->inUri = FALSE;
	    blk->inLiteral = FALSE;
	    blk->pp = NULL;
	  }
	else if( 0 == strcmp(el, SIB_PATH_EXPRESSION.txt) && blk->pp )
	  {
	    blk->inExpr = FALSE;
	    blk->qD->wqlType.values.pathExpr = blk->pp;
	    blk->pp = NULL;
	  }
	else if( 0 == strcmp(el, SIB_WQLQUERY.txt))
	  {
	    blk->list_on= FALSE;
	  }
	else
#endif
	  {
	    whiteboard_log_debug("queryWQLEnd: Invalid element end: %s or uri/literal tag not ended\n", el);
	    parseM3_setQuitParse(&blk->c, ss_ParsingError);
	    return;
	  }
      XML_SetCharacterDataHandler(blk->c.p, NULL );
    }
  else
    {
      whiteboard_log_debug("queryListEnd: end received, but not started!! %s\n", el);
      parseM3_setQuitParse(&blk->c, ss_ParsingError);
      return;
    }
  whiteboard_log_debug_fe();
}

static
inline gint str_list_findByIndex (GSList *lst, /*gchar*/const char *str)
{
  GSList *l;
  gint i;
  for (l=lst,i=0; l; l=l->next,i++)
    if (strcmp(str, l->data)==0)
      return i;
  return -1;
}

// SMART-11
static void XMLCALL
elementSPARQLSelect_start (void *data, const char *el, const char **attr)
{
  ParseSparqlSelectBlk *blk = data;
  whiteboard_log_debug_fb();
  whiteboard_log_debug("parsing SPARQL Select results: element start: %s\n",el);

  if (!blk->inBindingsList)
  {
    if( strcmp(el, "sparql_results")==0 )
    {
	blk->inBindingsList = TRUE;
	return;
    }
    else
    {
	whiteboard_log_debug("Parsed message must start with element sparql_bindings_list, not with: %s", el);
	parseM3_setQuitParse(&blk->c, ss_ParsingError);
	return;
    }
  }

  if(!blk->doneVariableList)
  {
    if (!blk->inHead)
    {
      if( strcmp(el, "head")==0 )
      {
        blk->inHead = TRUE;
	return;
      }
      else
      {
        whiteboard_log_debug("1st element within first (only) <sparql_results> must be <head>, not: ", el);
	parseM3_setQuitParse(&blk->c, ss_ParsingError);
	return;
      }

      if( !blk->inVariable && strcmp(el,"variable")==0 && (attr[0]&&!attr[2]) && strcmp(attr[0],"name")==0 )
      {
        blk->inVariable = TRUE;
	*blk->selectedVariables = g_slist_append (*blk->selectedVariables, g_strdup(attr[1]));
	return;
      }
      else
      {
        if (blk->inVariable)
	  whiteboard_log_debug("new element without closing previous <variable> element");
	else if (strcmp(el,"variable")!=0)
	  whiteboard_log_debug("starting something other than <variable> element in <head> list");
	else if (!attr[0])
	  whiteboard_log_debug("missing \"name\" attribute");
	else if (strcmp(attr[0],"name")!=0)
	  whiteboard_log_debug("only \"name\" attribute allowed in <variable> element");
	  parseM3_setQuitParse(&blk->c, ss_ParsingError);
	  return;
      }
    }
  }

  if (!blk->inResults)
  {
    if( strcmp(el,"results")==0 )
    {
      blk->inResults = TRUE;
      return;
    }
    else
    {
      whiteboard_log_debug("expected element <results>, not : %s",el);
      parseM3_setQuitParse(&blk->c, ss_ParsingError);
      return;
    }
  }

  if (!blk->inResult)
  {
    if( strcmp(el,"result")==0 )
    { //initialize a new result row with NULLs; replaced if appropriate <binding..> is parsed.
      GSList *list = NULL;
      GSList *tmp = *blk->selectedVariables;
      for ( ; tmp; tmp=tmp->next)
	list = g_slist_prepend(list,NULL);

      *blk->results = g_slist_prepend(*blk->results, list);
      blk->inResult = TRUE;
      return;
    }
    else
    {
      whiteboard_log_debug("expected element <result>, not : %s",el);
      parseM3_setQuitParse(&blk->c, ss_ParsingError);
      return;
    }
  }

  if (!blk->inBinding && strcmp(el,"binding")==0 && (attr[0]&&!attr[2]) &&
      strcmp(attr[0],"name")==0 && (blk->valueIndex=str_list_findByIndex(*blk->selectedVariables,attr[1])) != -1)
  {
    blk->inBinding = TRUE;
    return;
  }
  else
  {
    if (blk->inBinding)
    {
      if (blk->parsedCharTxt==NULL && ( strcmp(el,"uri")==0 || strcmp(el,"literal")==0 ))
        return;
      else if (blk->parsedCharTxt!=NULL)
	whiteboard_log_debug("%s",errorContentNotInBinding);
      else
	whiteboard_log_debug("new element withing <binding> which repeats or is not <uri> or <literal>");
	parseM3_setQuitParse(&blk->c, ss_ParsingError);
	return;
    }
    else if (strcmp(el,"binding")!=0) 
    {
      whiteboard_log_debug("starting <%s> when <binding> is expected after <result>",el);
    }
    else if (!attr[0])
      whiteboard_log_debug("missing \"name\" attribute");
    else if (strcmp(attr[0],"name")!=0)
      whiteboard_log_debug("only \"name\" attribute allowed in <binding> element");
    else
      whiteboard_log_debug("attribute value is not a selected variable");
      parseM3_setQuitParse(&blk->c, ss_ParsingError);
      return;
  }

  //should have returned before this
  parseM3_setQuitParse(&blk->c, ss_InternalError);

  whiteboard_log_debug_fe();
}

// SMART-11
static void XMLCALL
elementSPARQLSelect_end (void *data, const char *el)
{
  ParseSparqlSelectBlk *blk = data;
  whiteboard_log_debug_fb();
  whiteboard_log_debug("parsing SPARQL Select results: reached element end: %s\n",el);

  if(blk->inBindingsList)
  {
    if(!blk->doneVariableList)
    {
      if (blk->inHead && blk->inVariable && strcmp(el,"variable") == 0)
      {
	 blk->inVariable = FALSE;
	 return;
      }
      else if(blk->inHead && !blk->inVariable && strcmp(el,"head") == 0)
      {
        blk->inHead = FALSE;
	blk->doneVariableList = TRUE;
	return;
      }
      else //not in <head>
      {
	 parseM3_setQuitParse(&blk->c, ss_ParsingError);
	 return;
      }
    }

    //doing required bindings
    //first, internal check..
    if (blk->inResult && (*blk->results==NULL || g_slist_length((GSList*)(*blk->results)->data) != g_slist_length(*blk->selectedVariables)))
    {
      whiteboard_log_debug("internal error, results table improperly initialized");
      parseM3_setQuitParse(&blk->c, ss_InternalError);
      return;
    }
    if (blk->inResults)
    {
      if(blk->inResult && blk->inBinding && blk->parsedCharTxt!=NULL && (strcmp(el,"uri")==0 || strcmp(el,"literal")==0))
      { //binding has been defined for defined triple element value
	//so, add a triple element to the internal triple element list (=results, here) and
	//update (overwrite NULL), for the current (blk->valueIndex) variable's value with the current (last on list) triple address
	ssPathNode_t *pN = g_new0(ssPathNode_t, 1);
	if (strcmp(el,"uri")==0)
	{
	  pN->nodeType = ssElement_TYPE_URI;
	  //someday, in a perfect world, the sib may use namespace forms.. 
	  //they are independent of the user's namespace, and URIs should be expanded to full form before collapsing in the user's namespace.
	  //for now, URIs are assumed already in full form.

	  //translate, if needed, to user namespace form
	  if (blk->usrs_prefix_ns_map) 
          {
	    nsLocal2prefix_ctrlBlk nsLocal2prefix_ctrl;
	    nsLocal2prefix_ctrl.uri_p = &blk->parsedCharTxt;
	    nsLocal2prefix_ctrl.done = FALSE;
	    g_hash_table_foreach(blk->usrs_prefix_ns_map, nsLocal2prefixLocal_hndl, &nsLocal2prefix_ctrl);
	  }
	  pN->string = (ssElement_t) blk->parsedCharTxt;
        }
	else //"literal"
	{
	  pN->nodeType = ssElement_TYPE_LIT;
	  pN->string = (ssElement_t) blk->parsedCharTxt;
	}

	GSList *l = *blk->results;//last pushed (prepended) row
	gint i;
	for (i=0, l = l->data; // column list
	     i<blk->valueIndex && l;
	     i++, l=l->next);
	  if(!l || l->data != NULL) {
	    parseM3_setQuitParse(&blk->c, ss_InternalError);
	    return;
	  }
	  l->data = pN;
	  blk->parsedCharTxt = NULL;
	  blk->valueIndex = -1;
	  return;
      }
      else if(blk->inResult && blk->inBinding && strcmp(el,"binding")==0)
      {
        blk->inBinding = FALSE;
	return;
      }
      else if(blk->inResult && strcmp(el,"result")==0)
      {
	blk->inResult = FALSE;
	return;
      }
      else if(strcmp(el,"results")==0)
      {
	blk->inResults = FALSE;
	return;
      }
      else
      {
        parseM3_setQuitParse(&blk->c, ss_ParsingError);
	return;
      }
    }
    if( strcmp(el,"sparql_results")==0)
    {
      blk->inBindingsList = FALSE;
      return;
    }
    else
    {
      parseM3_setQuitParse(&blk->c, ss_ParsingError);
      return;
    }
  }

  parseM3_setQuitParse(&blk->c, ss_InternalError);
  return;

  whiteboard_log_debug_fe();
}

ssStatus_t parseM3_query_results_sparql_select(GSList **selectedVariables, GSList **results, const gchar * response, /*const*/ GHashTable *prefix_ns_map)
{
  whiteboard_log_debug_fb();
  ParseSparqlSelectBlk blk;

  blk.results = results;
  blk.c.parseStatus = ss_StatusOK;

  blk.selectedVariables = selectedVariables;
  blk.results = results;
  blk.usrs_prefix_ns_map = prefix_ns_map; // TODO SMART-11
  blk.parsedCharTxt = NULL;
  blk.inBindingsList = FALSE;
  blk.inBinding = FALSE;
  blk.inResults = FALSE;
  blk.inResult = FALSE;
  blk.inHead = FALSE;
  blk.inVariable = FALSE;
  blk.doneVariableList = FALSE;
  blk.valueIndex = -1;

  blk.c.p = XML_ParserCreate(NULL);
  if (! blk.c.p) {
    whiteboard_log_error("Couldn't allocate memory for parser\n");
    whiteboard_log_debug_fe();
    return ss_NotEnoughResources;
  }
  XML_SetElementHandler(blk.c.p, elementSPARQLSelect_start, elementSPARQLSelect_end);
  XML_SetUserData(blk.c.p, &blk);
  XML_SetCharacterDataHandler(blk.c.p, sparqlSelect_charhndl);

  XML_Parse(blk.c.p, response, strlen(response), 1);
  XML_ParserFree(blk.c.p);
  whiteboard_log_debug_fe();
  return blk.c.parseStatus;
  whiteboard_log_debug_fe();
}

ssStatus_t parseM3_query_cnf_wql(GSList **results, const gchar * response)
{
  whiteboard_log_debug_fb();
  wqlParseBlk_t blk;
  blk.list_on = FALSE;
  blk.c.parseStatus = ss_StatusOK;
  blk.results = results;
  blk.inUri = FALSE;
  blk.inLiteral = FALSE;

  blk.pp = NULL;

  blk.c.p = XML_ParserCreate(NULL);
  if (! blk.c.p) {
    whiteboard_log_error("Couldn't allocate memory for parser\n");
    whiteboard_log_debug_fe();
    return ss_NotEnoughResources;
  }
#ifdef SIB_ROLE
  blk.qD = NULL;
#endif
  XML_SetElementHandler(blk.c.p, queryWQLStart, queryWQLEnd);
  XML_SetUserData(blk.c.p, &blk);

  XML_Parse(blk.c.p, response, strlen(response), 1);
  XML_ParserFree(blk.c.p);

  whiteboard_log_debug_fe();
  return blk.c.parseStatus;
}

static void XMLCALL
startNameSpaceHndl(void *data,
	           const XML_Char *prefix,
		   const XML_Char *uri)
{
  ParseTriplesBlk *blk = (ParseTriplesBlk*)data;
#if PRINT_TRACE
  fprintf (stderr,"%d:ns: prefix=%s uri=%s\n", blk->depth, prefix, uri);
#endif
  blk->depth++;

  if (0==strcmp(uri, NS_SIB.txt))
    blk->inUseSibNsUri=NS_SIB; //inTripleList = !0;
  else {  //apr09new (p.s. above never was in use
    if (!blk->sibs_prefix_ns_map)
      blk->sibs_prefix_ns_map = g_hash_table_new_full(g_str_hash,g_str_equal, g_free, g_free);
    g_hash_table_insert(blk->sibs_prefix_ns_map, (gpointer)g_strdup(prefix), (gpointer)g_strdup(uri));
  }
}

static void XMLCALL
endNameSpaceHndl (void *data,
                  const XML_Char *prefix)
{
  ParseTriplesBlk *blk = (ParseTriplesBlk*)data;
#if PRINT_TRACE
  fprintf (stderr,"%d:e ns: prefix=%s:\n", blk->depth, prefix);
#endif
  if (blk->sibs_prefix_ns_map) {
    g_hash_table_destroy(blk->sibs_prefix_ns_map);
    blk->sibs_prefix_ns_map=NULL;
  }
  blk->depth--;
}

static void nsLocal2prefixLocal_hndl (gpointer _prefix, gpointer _ns, gpointer data)
{
  nsLocal2prefix_ctrlBlk *ctrlblk = data;
  if (ctrlblk->done)
    return;

  char *uri = *ctrlblk->uri_p;
  char *ns = _ns;
  char *prefix = _prefix;
  gint nsLen = strlen(ns);
  if (g_strstr_len (uri, nsLen, ns)==uri) {
    gchar * prefix_local = g_strjoin(":", prefix, uri+nsLen, NULL);
    g_free (uri);
    *ctrlblk->uri_p = prefix_local;
    ctrlblk->done=TRUE;
  }
}

void nsLocal2prefixLocal (gchar **g_heap_str, GHashTable *prefix2ns_map)
{
  if (!prefix2ns_map)
    return; //nothing to do - its ok!

    nsLocal2prefix_ctrlBlk nsLocal2prefix_ctrl;
    nsLocal2prefix_ctrl.uri_p = g_heap_str;
    nsLocal2prefix_ctrl.done = FALSE;
    g_hash_table_foreach(prefix2ns_map, nsLocal2prefixLocal_hndl, &nsLocal2prefix_ctrl);
}


static void XMLCALL
sparqlSelect_charhndl(void *data, const char *s, int sLen)
{
  int i;
  for (i = 0; i < sLen && g_ascii_isspace(s[i]); i++);
  
  if ( i<sLen)//s not empty
    {
      ParseSparqlSelectBlk *blk = (ParseSparqlSelectBlk*)data;

      if (!blk->inBinding)
	{
	  whiteboard_log_debug("%s",errorContentNotInBinding);
	  parseM3_setQuitParse(&blk->c, ss_ParsingError);
	  return;
	}
      gchar *tmp2 = g_strndup(s, sLen);
      if (!blk->parsedCharTxt)
	blk->parsedCharTxt = tmp2;
      else
	{
	  gchar *tmp1 = blk->parsedCharTxt;
	  blk->parsedCharTxt = g_strconcat(tmp1,tmp2,NULL); 
	  g_free(tmp1);
	  g_free(tmp2);
	}
   }
  return;
}

static void XMLCALL
charhndl(void *data, const char *s, int sLen)
{
  whiteboard_log_debug_fb();
  ParseTriplesBlk *blk = (ParseTriplesBlk*)data;
  if (blk->c.parseStatus)
    return;

  //#if PRINT_TRACE
  //int i;
  //fprintf(stderr,"depth: %d, len: %d, s: ", blk->depth, sLen);
  //for (i = 0; i < sLen; i++)
    //fprintf (stderr,"%c", s[i]);
  //fprintf(stderr,"\n");
  //#endif
  //gchar* tmp_str;
  if( *blk->tripleComponentToParse )
    {
      //printf("appending to current component string\n");
      gchar *old = (gchar *) (*blk->tripleComponentToParse);
      gchar *new = fullUri(s, sLen, blk->sibs_prefix_ns_map);
      *blk->tripleComponentToParse = (ssElement_t)g_strconcat( old, new, NULL);
      g_free(old);
      g_free(new);
    }
  else
    {
      //printf("creating new component string\n");
      *blk->tripleComponentToParse = (ssElement_t)fullUri(s, sLen, blk->sibs_prefix_ns_map); //allocates a string from g-heap
    }
  // now to see if it can be translated back to a user prefix
  // as advised in the hash table documentation, this might not be so efficient.
  //Also the expansion to full URI before back to user prefix, could be avoided with some reworking ...

  //if we have a map (I personnally believe, some'ah people out there DON'T have maps!)
  //for all subj, pred and for all obj of URI type: (?? subj cant be lit?)
  if (blk->usrs_prefix_ns_map &&
      (blk->tripleComponentToParse != &blk->currentTriple->object || blk->currentTriple->objType == ssElement_TYPE_URI)) {
    //  gint x = g_hash_table_size(blk->usrs_prefix_ns_map);
    //  if (x) //if zero, has it been destroyed, and not nulled?
    nsLocal2prefix_ctrlBlk nsLocal2prefix_ctrl;
    nsLocal2prefix_ctrl.uri_p = (char **)blk->tripleComponentToParse;
    nsLocal2prefix_ctrl.done = FALSE;
    g_hash_table_foreach(blk->usrs_prefix_ns_map, nsLocal2prefixLocal_hndl, &nsLocal2prefix_ctrl);
  }
  whiteboard_log_debug_fe();
}

static void XMLCALL CDATA_start(void* data)
{ 
  ParseTriplesBlk *blk = (ParseTriplesBlk*)data;
  XML_SetCharacterDataHandler(blk->c.p, charhndl);
  return;
}

static void XMLCALL CDATA_end(void* data)
{
  ParseTriplesBlk *blk = (ParseTriplesBlk*)data;
  XML_SetCharacterDataHandler(blk->c.p, NULL);
  return;
}

static void XMLCALL
start(void *data, const char *el, const char **attr)
{
  whiteboard_log_debug_fb();
  ParseTriplesBlk *blk = (ParseTriplesBlk*)data;
#if PRINT_TRACE
  int i;
  //***  printf("\ns:");
  for (i = 0; i < blk->depth; i++)
    fprintf(stderr,"^");
  fprintf(stderr,"s:<");

  fprintf(stderr,"%s>: ", el);

  for (i = 0; attr[i]; i += 2) {
    fprintf(stderr," %s='%s'\n", attr[i], attr[i + 1]);
  }
#endif
  if (blk->c.parseStatus)
    {
      whiteboard_log_debug_fe();
      return;
    }

  blk->depth++;
  if (blk->predicateDepth && blk->depth > blk->predicateDepth) {
    blk->c.parseStatus = ss_ParsingError;
  whiteboard_log_debug_fe();
    return; /* not going down there! */
  }

  if (!blk->inTripleList) 
    {
      if (SUFFIXeqSTR(blk->inUseSibNsUri,SIB_TRIPLELIST.txt,el))
	blk->inTripleList = !0; //NOW the list is on
      else
	parseM3_setQuitParse(&blk->c, ss_ParsingError); 
      
      whiteboard_log_debug_fe();
      return;
    }
  
  if (attr[0] /* zero or one parameter, but don't do complicated stuff */ && attr[2])
    return;

  if (blk->currentTriple) {
    if (attr[0] && attr[2])
      {
	parseM3_setQuitParse(&blk->c, ss_ParsingError);
	whiteboard_log_debug_fe();
	return;
      }

    XML_SetCharacterDataHandler(blk->c.p, charhndl);

    if (SUFFIXeqSTR(blk->inUseSibNsUri,SIB_SUBJECT.txt,el)) 
      {
#ifdef SIB_ROLE
	if(attr[0]) 
	  {
	    if (0==strcmp(SIB_URI.txt, attr[1]) || (0==strcmp(SIB_URICAPS.txt, attr[1])))
	      ;//type is URI by default
	    else if (0==strcmp(SIB_BNODE.txt, attr[1])) {
	      blk->currentTriple->subjType = ssElement_TYPE_BNODE;
	    }
	    else
	      {
		parseM3_setQuitParse(&blk->c, ss_ParsingError); 
		  whiteboard_log_debug_fe();
		  return;
	      }
	  }
      //type is URI by default
#endif
	blk->tripleComponentToParse = &blk->currentTriple->subject;
	whiteboard_log_debug("Current component to parse: subject\n");
      }
    else if (SUFFIXeqSTR(blk->inUseSibNsUri,SIB_PREDICATE.txt,el) && !attr[0]) 
      {
	blk->tripleComponentToParse = &blk->currentTriple->predicate;
	whiteboard_log_debug("Current component to parse: predicate\n");
	//	XML_SetCharacterDataHandler(blk->c.p, charhndl);
      }
    else if (SUFFIXeqSTR(blk->inUseSibNsUri,SIB_OBJECT.txt,el) && (!attr[0] || 0==strcmp(SIB_TYPE.txt, attr[0]))) 
      {
	if( !attr[0] || (0==strcmp(SIB_URI.txt, attr[1])) || (0==strcmp(SIB_URICAPS.txt, attr[1])))
	  blk->currentTriple->objType = ssElement_TYPE_URI;
	else if (0==strcmp(SIB_LITERAL.txt, attr[1]))
	  blk->currentTriple->objType = ssElement_TYPE_LIT;
#ifdef SIB_ROLE
	else if (0==strcmp(SIB_BNODE.txt, attr[1]))
	  blk->currentTriple->objType = ssElement_TYPE_BNODE;
#endif
	else
	  {
	    parseM3_setQuitParse(&blk->c, ss_ParsingError); 
	    whiteboard_log_debug_fe();
	    return;
	  }

	whiteboard_log_debug("Current component to parse: object\n");
	blk->tripleComponentToParse = &blk->currentTriple->object;
	if(blk->currentTriple->objType == ssElement_TYPE_LIT)
	  {
	    blk->currentTriple->object = (guchar *)g_strdup("");
	  }
      }
    else
      {
	blk->c.parseStatus = ss_ParsingError;
	XML_SetCharacterDataHandler(blk->c.p, NULL);
      }
    whiteboard_log_debug_fe();
    return;
  }
  
  /**************** below for new triple, above for current triple ************/

  if (!SUFFIXeqSTR(blk->inUseSibNsUri,SIB_TRIPLE.txt,el))
    {
      whiteboard_log_debug_fe();
      return;
    }

  /* so, we have proper triple start */
  blk->parseMode = SIB_REIFIED; //for now the only one we understand

  blk->currentTriple = (ssTriple_t *)g_new0(ssTriple_t,1);
  if (!blk->currentTriple)
    {
      parseM3_setQuitParse(&blk->c, ss_NotEnoughResources); 
        whiteboard_log_debug_fe();
	return;
    }
  
  blk->predicateDepth = blk->depth + 1;

  whiteboard_log_debug_fe();
}

static void XMLCALL
end (void *data, const char *el)
{
  whiteboard_log_debug_fb();
  ParseTriplesBlk *blk = (ParseTriplesBlk*)data;
  blk->depth--;
#if PRINT_TRACE
  fprintf(stderr,"%d:<%s>e\n",blk->depth, el);
#endif
  XML_SetCharacterDataHandler(blk->c.p, NULL);
  //  if (currentTriple && g_str_has_prefix(el, rdfns) && 0==strcmp(&el[rdfns_l], "Description")) {
  if (blk->predicateDepth == ((blk->parseMode==SIB_REIFIED)?blk->depth+2:blk->depth+1)) {

#ifndef SIB_ROLE
    if (blk->currentTriple->subject && blk->currentTriple->predicate && blk->currentTriple->object) {
#endif
#ifdef SIB_ROLE
    if ((blk->currentTriple->subject || blk->currentTriple->subjType==ssElement_TYPE_BNODE) 
	&&  blk->currentTriple->predicate
	&& (blk->currentTriple->object || blk->currentTriple->objType==ssElement_TYPE_BNODE)) {
      uuid_t u1;
      gchar tmp[37];
      gchar *uri;
      ssElement_t *target_pp = NULL;
      ssElementType_t *type_p = NULL;

      int n;
      for (n=0; n<2; n++) {
	switch (n) {
	case 0:
	  target_pp=&blk->currentTriple->subject;
	  type_p=&blk->currentTriple->subjType;
	  break;
	case 1:
	  target_pp=&blk->currentTriple->object;
	  type_p=&blk->currentTriple->objType;
	}

	if (*type_p == ssElement_TYPE_BNODE) {
	  uri=NULL;
	  if (*target_pp)
	    uri = (gchar *)g_strdup(g_hash_table_lookup(blk->bnode_name2uri_map,
							 *target_pp));
	  if (!uri) {//newly defined
	    uuid_generate(u1);
	    uuid_unparse(u1, tmp);
	    uri = g_strdup(tmp);

	    if (*target_pp) {//named
	      //remember the associated uri
	      g_hash_table_insert(blk->bnode_name2uri_map,
				  g_strdup((gchar *)*target_pp),
				  (gpointer)g_strdup(uri));
	      if(blk->bD_bNodeUriListXML) {//creating list string
		attrStr attr;
		charStr str0,str1;
		attr.name = &SIB_TAG;
		str0.txt = (gchar *)*target_pp;
		str0.len = strlen ((gchar *)*target_pp);
		attr.defined = &str0;
		str1.txt = uri;
		str1.len = strlen(uri);
		addXML_start(blk->bD_bNodeUriListXML, &SIB_URI, &attr, &str1, 1);
	      }
	    }
	  }
	  
	  if (*target_pp)
	    g_free (*target_pp);
	
	  //finally!
	  *target_pp = (ssElement_t)uri;
	  *type_p = ssElement_TYPE_URI;
	}
      }
#endif
      *blk->tripleList_pp = g_slist_prepend (*blk->tripleList_pp, (gpointer)blk->currentTriple);
    }
    else
      {
	parseM3_setQuitParse(&blk->c, ss_ParsingError); 
	whiteboard_log_debug_fe();
	return;
      }
    
    blk->currentTriple = NULL;
    //    blk->currentTagInx = -1;
  }

  if (blk->predicateDepth > (blk->depth + 1)) { //i.e. end of rdf:Description
    blk->predicateDepth = 0;
    blk->currentSubject = NULL;
  }
  whiteboard_log_debug_fe();
}

static ssStatus_t
parseM3XML_triples_start (ParseTriplesBlk *blk)
{
  blk->depth = 0;
  blk->inTripleList = 0;
  blk->inUseSibNsUri.txt = "";
  blk->inUseSibNsUri.len = 0;
  blk->tripleComponentToParse = NULL;
  blk->c.p = XML_ParserCreateNS(NULL, 0);
  if (! blk->c.p) {
    whiteboard_log_error("Couldn't allocate memory for parser\n");
    return ss_NotEnoughResources;
  }
#ifdef SIB_ROLE
  blk->bnode_name2uri_map = g_hash_table_new_full(g_str_hash,g_str_equal, g_free, g_free);
  if (!blk->bnode_name2uri_map) {
    whiteboard_log_error("Couldn't allocate memory for bnode processing\n");
    return ss_NotEnoughResources;
  }
#endif

  XML_SetNamespaceDeclHandler(blk->c.p, startNameSpaceHndl, endNameSpaceHndl);
  XML_SetElementHandler(blk->c.p, start, end);
  XML_SetCdataSectionHandler( blk->c.p, CDATA_start, CDATA_end);
  XML_SetUserData(blk->c.p, blk);
  return ss_StatusOK;
}

static void
parseM3XML_triples_end (ParseTriplesBlk *blk)
{
  if (!blk)
    return;
  if(blk->c.p)
    XML_ParserFree(blk->c.p);
  ssFreeTriple(blk->currentTriple);
#ifdef SIB_ROLE
  if(blk->bnode_name2uri_map)
    g_hash_table_destroy (blk->bnode_name2uri_map);
  if(blk->bD_bNodeUriListXML)
    ssBufDesc_free(&blk->bD_bNodeUriListXML);
#endif
  if(blk->sibs_prefix_ns_map)
    g_hash_table_destroy (blk->sibs_prefix_ns_map);
  //note: the usrs_prefix_ns_map, if any, is a copy, to be freed elsewhere.
  g_free(blk);
}

ssStatus_t
parseM3_triples_SIB (GSList ** list_pp, const char * m3XML_triples_str, GHashTable *prefix_ns_map, gchar **bNodeUriList_str)
{
  g_return_val_if_fail(list_pp!=NULL && m3XML_triples_str!=NULL, ss_InvalidParameter);
#ifdef SIB_ROLE
  if (bNodeUriList_str != NULL)
    g_return_val_if_fail (*bNodeUriList_str==NULL, ss_InvalidParameter);
#else
  g_return_val_if_fail(bNodeUriList_str==NULL, ss_InvalidParameter);
#endif

  ParseTriplesBlk *blk = g_new0(ParseTriplesBlk, 1);
  if (!blk) {
    whiteboard_log_error("Couldn't allocate memory for triple parsing\n");
    return ss_NotEnoughResources;
  }

  blk->tripleList_pp = list_pp;
  blk->usrs_prefix_ns_map = prefix_ns_map;
  if (bNodeUriList_str) {
    blk->bD_bNodeUriListXML = ssBufDesc_new();
    if (!blk->bD_bNodeUriListXML) {
      whiteboard_log_error("Couldn't allocate memory for bnode processing\n");
      parseM3XML_triples_end(blk);
      return ss_NotEnoughResources;
    }
    addXML_start(blk->bD_bNodeUriListXML, &SIB_URILIST, NULL, NULL, 0);
  }
  
  ssStatus_t ssS = parseM3XML_triples_start (blk);
  if  (!ssS) {
    whiteboard_log_debug("Parsing triples: %s\n, len %d",m3XML_triples_str, strlen(m3XML_triples_str));

    //borrowing ssS, will be 0 if XML_Parse finds error
    ssS = XML_Parse(blk->c.p, m3XML_triples_str, strlen(m3XML_triples_str), 1);
    if (ssS==0)
      ssS = ss_ParsingError;
    else
      ssS = blk->c.parseStatus;
  }

  if (bNodeUriList_str) {
    addXML_end (blk->bD_bNodeUriListXML, &SIB_URILIST);
    *bNodeUriList_str = blk->bD_bNodeUriListXML->buf;
    blk->bD_bNodeUriListXML->buf = NULL;
  }
    
  
  parseM3XML_triples_end (blk);
  return ssS;
}

#ifdef SIB_ROLE
/*-------------------------------------------------------------------------------------*/

ssWqlDesc_t *ssWqlDesc_new(queryStyle_t qStyle)
{
  g_return_val_if_fail(qStyle >= MSG_Q_WQLfirst && qStyle <= MSG_Q_WQLlast, NULL);
  ssWqlDesc_t *wqlD = g_new0(ssWqlDesc_t, 1);
  g_return_val_if_fail (wqlD, NULL);
  switch (qStyle) {
  case MSG_Q_WQL_VALUES:    wqlD->qType = QueryTypeWQLValues;    break;
  case MSG_Q_WQL_NODETYPES: wqlD->qType = QueryTypeWQLNodeTypes; break;
  case MSG_Q_WQL_RELATED:   wqlD->qType = QueryTypeWQLRelated;   break;
  case MSG_Q_WQL_ISTYPE:    wqlD->qType = QueryTypeWQLIsType;    break;
  case MSG_Q_WQL_ISSUBTYPE: wqlD->qType = QueryTypeWQLIsSubType; break;
  default:;
  }

  return wqlD;
}

ssWqlDesc_t *ssWqlDesc_new_jh(QueryType qtype)
{
  g_return_val_if_fail(qtype >=  QueryTypeWQLValues && qtype <= QueryTypeWQLIsSubType, NULL);
  ssWqlDesc_t *wqlD = g_new0(ssWqlDesc_t, 1);
  g_return_val_if_fail (wqlD, NULL);
  wqlD->qType = qtype;
  return wqlD;
}

ssStatus_t parseM3_query_req_wql(ssWqlDesc_t *qD, const gchar *qXml)
{
  whiteboard_log_debug_fb();
  wqlParseBlk_t blk;

  blk.list_on = FALSE;
  blk.c.parseStatus = ss_StatusOK;
  blk.qD = qD;
  blk.inUri = FALSE;
  blk.inLiteral = FALSE;
  blk.inExpr = FALSE;
  blk.pp = NULL;
  blk.c.p = XML_ParserCreate(NULL);
  if (! blk.c.p) {
    whiteboard_log_error("Couldn't allocate memory for parser\n");
    whiteboard_log_debug_fe();
    return ss_NotEnoughResources;
  }


  XML_SetElementHandler(blk.c.p, queryWQLStart, queryWQLEnd);
  //  XML_SetCdataSectionHandler( blk.c.p, queryWQLCDATAstart, queryWQLCDATAned);
  XML_SetUserData(blk.c.p, &blk);

  XML_Parse(blk.c.p, qXml, strlen(qXml), 1);
  XML_ParserFree(blk.c.p);
  
  whiteboard_log_debug_fe();
  return blk.c.parseStatus;
}

void ssWqlDesc_free (ssWqlDesc_t ** wqlDp)
{
  if (!wqlDp || !*wqlDp)
    return;

  ssWqlDesc_t *p=*wqlDp;

  switch (p->qType) {
  case QueryTypeWQLValues:
      ssFreePathNode (p->wqlType.values.startNode);
      if (p->wqlType.values.pathExpr) g_free(p->wqlType.values.pathExpr);
      break;
  case QueryTypeWQLNodeTypes:
      ssFreePathNode (p->wqlType.nodeTypes.node);
      break;
  case QueryTypeWQLRelated:
      ssFreePathNode (p->wqlType.related.startNode);
      if (p->wqlType.related.pathExpr) g_free(p->wqlType.related.pathExpr);
      ssFreePathNode (p->wqlType.related.endNode);
      break;
  case QueryTypeWQLIsType:
      ssFreePathNode (p->wqlType.isType.node);
      ssFreePathNode (p->wqlType.isType.typeNode);
      break;
  case QueryTypeWQLIsSubType:;
      ssFreePathNode (p->wqlType.isSubType.subTypeNode);
      ssFreePathNode (p->wqlType.isSubType.superTypeNode);
      break;
  default:;
  }
  g_free(p);
}
#endif //SIB_ROLE
