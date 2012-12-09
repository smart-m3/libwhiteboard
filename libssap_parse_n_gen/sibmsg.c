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
 * sssib.c
 *
 * Provides the a C API for access to a Smart Space (SS) Semantic Information Broker (SIB)
 * based on M3 Program Deliverable DEL201: Smart Space Access Protocol
 * Uses glib and Expat libraries to read an XML message from the SS SIB
 *
 * Must be used with Expat compiled for UTF-8 output.
 */

#include <glib.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define _NODE_LIB
#define _TESTING 1

#include "sibmsg.h"
#include "ssap_sib_tokens.h"
#ifndef NO_WHITEBOARD
#include "whiteboard_log.h"
#else
#define whiteboard_log_debug_fb()
#define whiteboard_log_debug_fe()
#endif

ssStatus_t ssBufDesc_CreateJoinMessage(ssBufDesc_t *desc,
				       ssElement_ct ssId,
				       ssElement_ct  nodeName,
				       gint msgnumber)
{
  charStr cont;
  desc->datLen = 0;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  whiteboard_log_debug_fb();

  stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_REQUEST, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_JOIN, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t ssBufDesc_CreateLeaveMessage(ssBufDesc_t *desc,
					ssElement_ct ssId,
					ssElement_ct nodeName,
					gint msgnumber,
					gboolean confirm)
{
  ssStatus_t stat;
  GString *trid = g_string_new("");
  charStr cont;
  whiteboard_log_debug_fb();

    stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
    g_return_val_if_fail(stat == ss_StatusOK, stat);

    stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_REQUEST, 1);
    g_return_val_if_fail(stat == ss_StatusOK, stat);

    stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_LEAVE, 1);
    g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
    stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
    g_return_val_if_fail(stat == ss_StatusOK, stat);

    cont.txt = (char *)nodeName;
    cont.len = strlen(cont.txt);
    stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
    g_return_val_if_fail(stat == ss_StatusOK, stat);

    cont.txt = (char *)ssId;
    cont.len = strlen(cont.txt);
    stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
    g_return_val_if_fail(stat == ss_StatusOK, stat);



    stat = addXML_end (desc, &SIB_MESSAGE);
    g_return_val_if_fail(stat == ss_StatusOK, stat);
    g_string_free(trid, TRUE);

    whiteboard_log_debug_fe();
    return ss_StatusOK;
}

ssStatus_t ssBufDesc_CreateInsertMessage(ssBufDesc_t *desc,
					 ssElement_ct ssId,
					 ssElement_ct nodeName,
					 gint msgnumber,
					 EncodingType encoding,
					 const guchar *tripleStr,
					 gboolean confirm)
{
  attrStr attr;
  charStr cont;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  whiteboard_log_debug_fb();

  desc->datLen = 0;

    stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

    stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_REQUEST, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

    stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_INSERT, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;

  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);


  attr.name = &SIB_NAME;
  attr.defined = &SIB_PARAMCONFIRM;
  if(confirm)
    stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_TRUE, 1);
  else
    stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_FALSE, 1);

  g_return_val_if_fail(stat == ss_StatusOK, stat);


  attr.name = &SIB_NAME;
  attr.defined = &SIB_INSERTGRAPH;

  stat = addXML_open_element (desc, &SIB_PARAMETER);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  stat = addXML_append_attribute (desc, &attr);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  cont.txt = " ";
  cont.len = 1;
  stat = addXML_append_str(desc, &cont);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  attr.name = &SIB_ENCODING;
  switch(encoding)
    {
    case EncodingM3XML:
      attr.defined = &SIB_TYPE_M3XML;
      break;
    case EncodingRDFXML:
      attr.defined = &SIB_TYPE_RDFXML;
      break;
    default:
      g_return_val_if_reached(ss_InternalError);
      break;
    }
  stat = addXML_append_attribute (desc, &attr);

  g_return_val_if_fail(stat == ss_StatusOK, stat);
  stat = addXML_close_element (desc);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (gchar*) tripleStr;
  cont.len = strlen( (gchar *)tripleStr);
  stat = addXML_append_str(desc, &cont);
  g_return_val_if_fail(stat == ss_StatusOK, stat);


  stat = addXML_end (desc, &SIB_PARAMETER);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t ssBufDesc_CreateUpdateMessage(ssBufDesc_t *desc,
					 ssElement_ct ssId,
					 ssElement_ct nodeName,
					 gint msgnumber,
					 EncodingType encoding,
					 const guchar *insTripleStr,
					 const guchar *remTripleStr,
					 gboolean confirm)
{
  attrStr attr;
  charStr cont;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  whiteboard_log_debug_fb();

  desc->datLen = 0;

    stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

    stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_REQUEST, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

    stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_UPDATE, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

    g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;

  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
    cont.len = strlen(cont.txt);
    stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);


  attr.name = &SIB_NAME;
  attr.defined = &SIB_PARAMCONFIRM;
  if(confirm)
    stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_TRUE, 1);
  else
    stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_FALSE, 1);

  g_return_val_if_fail(stat == ss_StatusOK, stat);


  attr.name = &SIB_NAME;
  attr.defined = &SIB_INSERTGRAPH;

  stat = addXML_open_element (desc, &SIB_PARAMETER);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  stat = addXML_append_attribute (desc, &attr);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = " ";
  cont.len = 1;
  stat = addXML_append_str(desc, &cont);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  attr.name = &SIB_ENCODING;
  switch(encoding)
    {
    case EncodingM3XML:
      attr.defined = &SIB_TYPE_M3XML;
      break;
    case EncodingRDFXML:
      attr.defined = &SIB_TYPE_RDFXML;
      break;
    default:
      g_return_val_if_reached(ss_InternalError);
      break;
    }
  stat = addXML_append_attribute (desc, &attr);

  g_return_val_if_fail(stat == ss_StatusOK, stat);
  stat = addXML_close_element (desc);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (gchar *)insTripleStr;
  cont.len = strlen( (gchar *)insTripleStr);
  stat = addXML_append_str(desc, &cont);
  g_return_val_if_fail(stat == ss_StatusOK, stat);


  stat = addXML_end (desc, &SIB_PARAMETER);
  g_return_val_if_fail(stat == ss_StatusOK, stat);


  attr.name = &SIB_NAME;
  attr.defined = &SIB_REMOVEGRAPH;

  stat = addXML_open_element (desc, &SIB_PARAMETER);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  stat = addXML_append_attribute (desc, &attr);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = " ";
  cont.len = 1;
  stat = addXML_append_str(desc, &cont);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_ENCODING;
  switch(encoding)
    {
    case EncodingM3XML:
      attr.defined = &SIB_TYPE_M3XML;
      break;
    case EncodingRDFXML:
      attr.defined = &SIB_TYPE_RDFXML;
      break;
    default:
      g_return_val_if_reached(ss_InternalError);
      break;
    }

  stat = addXML_append_attribute (desc, &attr);

  g_return_val_if_fail(stat == ss_StatusOK, stat);
  stat = addXML_close_element (desc);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt =  (gchar *)remTripleStr;
  cont.len = strlen( (gchar *)remTripleStr);
  stat = addXML_append_str(desc, &cont);
  g_return_val_if_fail(stat == ss_StatusOK, stat);


  stat = addXML_end (desc, &SIB_PARAMETER);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;

}

ssStatus_t ssBufDesc_CreateRemoveMessage(ssBufDesc_t *desc,
					 ssElement_ct ssId,
					 ssElement_ct nodeName,
					 gint msgnumber,
					 EncodingType encoding,
					 const guchar *rdfxml)
{
  attrStr attr;
  charStr cont;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  whiteboard_log_debug_fb();

  desc->datLen = 0;

    stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

    stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_REQUEST, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_REMOVE, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
    cont.len = strlen(cont.txt);
    stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_NAME;
  attr.defined = &SIB_REMOVEGRAPH;
  stat = addXML_open_element (desc, &SIB_PARAMETER);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_append_attribute (desc, &attr);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
    /***** create RDFXML from triple templates **********/
    /*addRDFXML_templates(bD, nameSpaceStr, tripleList);*/
  cont.txt = " ";
  cont.len = 1;
  stat = addXML_append_str(desc, &cont);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_ENCODING;
  switch(encoding)
    {
    case EncodingM3XML:
      attr.defined = &SIB_TYPE_M3XML;
      break;
    case EncodingRDFXML:
      attr.defined = &SIB_TYPE_RDFXML;
      break;
    default:
      g_return_val_if_reached(ss_InternalError);
      break;
    }

  stat = addXML_append_attribute (desc, &attr);

  g_return_val_if_fail(stat == ss_StatusOK, stat);
  stat = addXML_close_element (desc);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (gchar *)rdfxml;
  cont.len = strlen( (gchar *)rdfxml);
  stat = addXML_append_str(desc, &cont);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  /***************************/
  stat = addXML_end (desc, &SIB_PARAMETER);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;

}

ssStatus_t ssBufDesc_CreateQueryMessage(ssBufDesc_t *desc,
				       ssElement_ct ssId,
				       ssElement_ct nodeName,
				       gint msgnumber,
					gint type,
				       const guchar *rdfxml)
{
  whiteboard_log_debug_fb();

  attrStr attr;
  charStr cont;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  desc->datLen = 0;

    stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

    stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_REQUEST, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

    stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_QUERY, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
    stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);

  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);

  g_return_val_if_fail(stat == ss_StatusOK, stat);

  switch (type )
    {
    case QueryTypeTemplate:
      cont.txt = SIB_TYPE_M3XML.txt;
      cont.len = SIB_TYPE_M3XML.len;
      break;
    case QueryTypeWQLValues:
      cont.txt = SIB_TYPE_WQLVALUES.txt;
      cont.len = SIB_TYPE_WQLVALUES.len;
      break;
    case QueryTypeWQLNodeTypes:
      cont.txt = SIB_TYPE_WQLNODETYPES.txt;
      cont.len = SIB_TYPE_WQLNODETYPES.len;
      break;
    case QueryTypeWQLRelated:
      cont.txt = SIB_TYPE_WQLRELATED.txt;
      cont.len = SIB_TYPE_WQLRELATED.len;
      break;
    case QueryTypeWQLIsType:
      cont.txt = SIB_TYPE_WQLISTYPE.txt;
      cont.len = SIB_TYPE_WQLISTYPE.len;
      break;
    case QueryTypeWQLIsSubType:
      cont.txt = SIB_TYPE_WQLISSUBTYPE.txt;
      cont.len = SIB_TYPE_WQLISSUBTYPE.len;
      break;
    case QueryTypeSPARQLSelect:
      cont.txt = SIB_TYPE_SPARQL.txt;
      cont.len = SIB_TYPE_SPARQL.len;
      break;
    default:
      g_return_val_if_reached(ss_InternalError);
    }

  attr.name = &SIB_NAME;
  attr.defined = &SIB_TYPE;

  stat = addXML_start (desc, &SIB_PARAMETER, &attr, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_NAME;
  attr.defined = &SIB_QUERYSTRING;
  stat = addXML_open_element (desc, &SIB_PARAMETER);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_append_attribute (desc, &attr);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_close_element (desc);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt =  (gchar *)rdfxml;
  cont.len = strlen( (gchar *)rdfxml);
  stat = addXML_append_str(desc, &cont);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  /***************************/
  stat = addXML_end (desc, &SIB_PARAMETER);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;

}


ssStatus_t ssBufDesc_CreateSubscribeMessage(ssBufDesc_t *desc,
					    ssElement_ct ssId,
					    ssElement_ct nodeName,
					    gint msgnumber,
					    gint type,
					    const guchar *rdfxml)
{
  whiteboard_log_debug_fb();

  attrStr attr;
  charStr cont;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  desc->datLen = 0;

  stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_REQUEST, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_SUBSCRIBE, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);


  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);


    switch (type )
    {
    case QueryTypeTemplate:
      cont.txt = SIB_TYPE_M3XML.txt;
      cont.len = SIB_TYPE_M3XML.len;
      break;
    case QueryTypeWQLValues:
      cont.txt = SIB_TYPE_WQLVALUES.txt;
      cont.len = SIB_TYPE_WQLVALUES.len;
      break;
    case QueryTypeWQLNodeTypes:
      cont.txt = SIB_TYPE_WQLNODETYPES.txt;
      cont.len = SIB_TYPE_WQLNODETYPES.len;
      break;
    case QueryTypeWQLRelated:
      cont.txt = SIB_TYPE_WQLRELATED.txt;
      cont.len = SIB_TYPE_WQLRELATED.len;
      break;
    case QueryTypeWQLIsType:
      cont.txt = SIB_TYPE_WQLISTYPE.txt;
      cont.len = SIB_TYPE_WQLISTYPE.len;
      break;
    case QueryTypeWQLIsSubType:
      cont.txt = SIB_TYPE_WQLISSUBTYPE.txt;
      cont.len = SIB_TYPE_WQLISSUBTYPE.len;
      break;
    case QueryTypeSPARQLSelect:
      cont.txt = SIB_TYPE_SPARQL.txt;
      cont.len = SIB_TYPE_SPARQL.len;
      break;
    default:
      g_return_val_if_reached(ss_InternalError);
    }

    attr.name = &SIB_NAME;
    attr.defined = &SIB_TYPE;

    stat = addXML_start (desc, &SIB_PARAMETER, &attr, &cont, 1);
    g_return_val_if_fail(stat == ss_StatusOK, stat);

    /******** create RDFXML from triple templates **********/
    /*addRDFXML_templates(bD, nameSpaceStr, tripleList);*/

      attr.name = &SIB_NAME;
  attr.defined = &SIB_QUERYSTRING;
  stat = addXML_open_element (desc, &SIB_PARAMETER);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_append_attribute (desc, &attr);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_close_element (desc);
  g_return_val_if_fail(stat == ss_StatusOK, stat);


  cont.txt =  (gchar *)rdfxml;
  cont.len = strlen( (gchar *)rdfxml);
  stat = addXML_append_str(desc, &cont);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  /***************************/
  stat = addXML_end (desc, &SIB_PARAMETER);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_return_val_if_fail(stat == ss_StatusOK, stat);
  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;

}


ssStatus_t ssBufDesc_CreateUnsubscribeMessage(ssBufDesc_t *desc,
					      ssElement_ct ssId,
					      ssElement_ct nodeName,
					      gint msgnumber,
					      ssElement_ct rdfxml)
{
  whiteboard_log_debug_fb();

  attrStr attr;
  charStr cont;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  desc->datLen = 0;

  stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_REQUEST, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_UNSUBSCRIBE, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
    cont.len = strlen(cont.txt);
    stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);


  cont.txt = (char *)rdfxml;
  cont.len = strlen((char *)rdfxml);
    attr.name = &SIB_NAME;
    attr.defined = &SIB_SUBSCRIPTIONID;
    stat = addXML_start (desc, &SIB_PARAMETER, &attr, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  /***************************/

  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;

}

ssStatus_t ssBufDesc_CreateJoinResponse(ssBufDesc_t *desc,
					ssElement_ct nodeName,
					ssElement_ct ssId,
					gint msgnumber,
					msgStatus_t status)
{
  attrStr attr;
  charStr cont;

  desc->datLen = 0;
  ssStatus_t stat;
  //GString *trid = g_string_new("");
  GString *trid = g_string_new("");
  whiteboard_log_debug_fb();

  stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_CONFIRM, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_JOIN, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_printf(trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;

  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_NAME;
  attr.defined = &SIB_STATUS;
  if(status == MSG_E_OK)
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUSOK, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }
  else
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_ERROR, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }

  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t ssBufDesc_CreateLeaveResponse(ssBufDesc_t *desc,
					 ssElement_ct nodeName,
					 ssElement_ct ssId,
					 gint msgnumber,
					 msgStatus_t status)
{
  attrStr attr;
  charStr cont;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  desc->datLen = 0;

  whiteboard_log_debug_fb();

  stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_CONFIRM, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_LEAVE, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_NAME;
  attr.defined = &SIB_STATUS;
  if(status == MSG_E_OK)
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUSOK, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }
  else
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_ERROR, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }

  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t ssBufDesc_CreateSubscribeResponse(ssBufDesc_t *desc,
					     ssElement_ct nodeName,
					     ssElement_ct ssId,
					     gint msgnumber,
					     msgStatus_t status,
					     ssElement_ct subId,
					     guchar *rdfxml)
{
  attrStr attr;
  charStr cont;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  desc->datLen = 0;

  whiteboard_log_debug_fb();

  stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_CONFIRM, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_SUBSCRIBE, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_NAME;
  attr.defined = &SIB_STATUS;
  if(status == MSG_E_OK)
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUSOK, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }
  else
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_ERROR, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }
  attr.name = &SIB_NAME;
  attr.defined = &SIB_SUBSCRIPTIONID;

  cont.txt = (char *)subId;
  cont.len = strlen(cont.txt);
  stat =   addXML_start(desc, &SIB_PARAMETER, &attr, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  if(rdfxml)
    {
       attr.name = &SIB_NAME;
       attr.defined = &SIB_RESULTS;
       cont.txt =  (gchar *)rdfxml;
       cont.len = strlen( (gchar *)rdfxml);
       stat =   addXML_start(desc, &SIB_PARAMETER, &attr, &cont, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }

  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t ssBufDesc_CreateUnsubscribeResponse(ssBufDesc_t *desc,
					       ssElement_ct nodeName,
					       ssElement_ct ssId,
					       gint msgnumber,
					       msgStatus_t status,
					       ssElement_ct subId)
{
  attrStr attr;
  charStr cont;
  GString *trid = g_string_new("");

  desc->datLen = 0;
  ssStatus_t stat;
  whiteboard_log_debug_fb();

  stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_CONFIRM, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_UNSUBSCRIBE, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_NAME;
  attr.defined = &SIB_STATUS;
  if(status == MSG_E_OK)
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUSOK, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }
  else
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_ERROR, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }
  attr.name = &SIB_NAME;
  attr.defined = &SIB_SUBSCRIPTIONID;

  if(subId)
    {
      cont.txt = (char *)subId;
      cont.len = strlen(cont.txt);
      stat =   addXML_start(desc, &SIB_PARAMETER, &attr, &cont, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }

  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;
}


ssStatus_t ssBufDesc_CreateInsertResponse(ssBufDesc_t *desc,
					  ssElement_ct nodeName,
					  ssElement_ct ssId,
					  gint msgnumber,
					  msgStatus_t status,
					  ssElement_ct rdfxml)
{
  attrStr attr;
  charStr cont;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  desc->datLen = 0;

  whiteboard_log_debug_fb();

  stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_CONFIRM, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_INSERT, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_NAME;
  attr.defined = &SIB_STATUS;
  //SIB_STATUSOK look into ssap_sib_tokens.c
  /*DAN ARCES*///I need to parse every enumeration (see sibdefs.h)
  /*if(status == MSG_E_OK)
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUSOK, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }
  else
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_ERROR, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }
  */
  /*DAN ARCES*/
  //printf("Status code for INSERT is %d", status);
  switch(status)
   {/*
    // STATUS CODES USED BY SIB
     */
    case ss_NotifReset:
    case ss_NotifClosing:
    case ss_StatusOK:
         stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUSOK, 1);
         g_return_val_if_fail(stat == ss_StatusOK, stat);
         break;

    case ss_SIBFailAccessDenied:
    case ss_SIBProtectionFault: //AD-ARCES
        stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_PROTECTION_FAULT, 1);
    	//stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_FAIL_ACCESSDENIED, 1);
        g_return_val_if_fail(stat == ss_StatusOK, stat);
        break;

    case ss_OperationFailed:
    case ss_SIBError:
    case ss_SIBFailNotImpl:
    case ss_KPError:
    case ss_KPErrorRequest:
    case ss_KPErrorMsgInComplete:
    case ss_KPErrorMsgSyntax:

    default:

    	stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_ERROR, 1);
        g_return_val_if_fail(stat == ss_StatusOK, stat);

   }//switch(status)




  if(rdfxml)
    {
      attr.name = &SIB_NAME;
      attr.defined = &SIB_BNODES;

      stat =   addXML_start(desc, &SIB_PARAMETER, &attr, NULL, 0);
      g_return_val_if_fail(stat == ss_StatusOK, stat);

      cont.txt = (char *)rdfxml;
      cont.len = strlen((char *)rdfxml);
      stat = addXML_append_str(desc, &cont);
      g_return_val_if_fail(stat == ss_StatusOK, stat);


      stat =   addXML_end(desc, &SIB_PARAMETER);

    }

  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;
}



ssStatus_t ssBufDesc_CreateUpdateResponse(ssBufDesc_t *desc,
					  ssElement_ct nodeName,
					  ssElement_ct ssId,
					  gint msgnumber,
					  msgStatus_t status,
					  ssElement_ct xml)
{
  attrStr attr;
  charStr cont;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  desc->datLen = 0;

  whiteboard_log_debug_fb();

  stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_CONFIRM, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_UPDATE, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_NAME;
  attr.defined = &SIB_STATUS;
  /*if(status == MSG_E_OK)
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUSOK, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }
  else
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_ERROR, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }*/

  /*DAN ARCES*/
  //printf("Status code for UPDATE is %d", status);
  switch(status)
   {/*
    // STATUS CODES USED BY SIB
     */
    case ss_NotifReset:
    case ss_NotifClosing:
    case ss_StatusOK:
         stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUSOK, 1);
         g_return_val_if_fail(stat == ss_StatusOK, stat);
         break;

    case ss_SIBFailAccessDenied:
    case ss_SIBProtectionFault: //AD-ARCES
        stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_PROTECTION_FAULT, 1);
    	//stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_FAIL_ACCESSDENIED, 1);
        g_return_val_if_fail(stat == ss_StatusOK, stat);
        break;

    case ss_OperationFailed:
    case ss_SIBError:
    case ss_SIBFailNotImpl:
    case ss_KPError:
    case ss_KPErrorRequest:
    case ss_KPErrorMsgInComplete:
    case ss_KPErrorMsgSyntax:

    default:

    	stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_ERROR, 1);
        g_return_val_if_fail(stat == ss_StatusOK, stat);

   }//switch(status)


  if(xml)
    {
      attr.name = &SIB_NAME;
      attr.defined = &SIB_BNODES;

      stat =   addXML_start(desc, &SIB_PARAMETER, &attr, NULL, 0);
      g_return_val_if_fail(stat == ss_StatusOK, stat);

      cont.txt = (char*)xml;
      cont.len = strlen((char*)xml);
      stat = addXML_append_str(desc, &cont);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
      stat =   addXML_end(desc, &SIB_PARAMETER);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }

  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t ssBufDesc_CreateRemoveResponse(ssBufDesc_t *desc,
					  ssElement_ct nodeName,
					  ssElement_ct ssId,
					  gint msgnumber,
					  msgStatus_t status)
{
  attrStr attr;
  charStr cont;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  desc->datLen = 0;

  whiteboard_log_debug_fb();

  stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_CONFIRM, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_REMOVE, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_NAME;
  attr.defined = &SIB_STATUS;

  /*if(status == MSG_E_OK)
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUSOK, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }
  else
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_ERROR, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }*/

  /*DAN ARCES*/
  //printf("Status code for REMOVE is %d", status);
  switch(status)
   {/*
    // STATUS CODES USED BY SIB
     */
    case ss_NotifReset:
    case ss_NotifClosing:
    case ss_StatusOK:
         stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUSOK, 1);
         g_return_val_if_fail(stat == ss_StatusOK, stat);
         break;

    case ss_SIBFailAccessDenied:
    case ss_SIBProtectionFault: //AD-ARCES
        stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_PROTECTION_FAULT, 1);
    	//stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_FAIL_ACCESSDENIED, 1);
        g_return_val_if_fail(stat == ss_StatusOK, stat);
        break;

    case ss_OperationFailed:
    case ss_SIBError:
    case ss_SIBFailNotImpl:
    case ss_KPError:
    case ss_KPErrorRequest:
    case ss_KPErrorMsgInComplete:
    case ss_KPErrorMsgSyntax:

    default:

    	stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_ERROR, 1);
        g_return_val_if_fail(stat == ss_StatusOK, stat);

   }//switch(status)


  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);
  whiteboard_log_debug_fe();
  return ss_StatusOK;
}

ssStatus_t ssBufDesc_CreateSubscriptionIndMessage(ssBufDesc_t *desc,
						  ssElement_ct nodeName,
						  ssElement_ct ssId,
						  gint msgnumber,
						  gint seqnumber,
						  const guchar *subscription_id,
						  const guchar *rdfxml_added,
						  const guchar *rdfxml_removed)
{
  attrStr attr;
  charStr cont;
  GString *trid = g_string_new("");
  GString *seqstr = g_string_new("");
  desc->datLen = 0;
  ssStatus_t stat;
  whiteboard_log_debug_fb();

  stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_INDICATION, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_SUBSCRIBE, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);


  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_string_free(trid, TRUE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( seqstr, "%d", seqnumber);
  attr.name = &SIB_NAME;
  attr.defined = &SIB_INDSEQNUM;

  cont.txt = seqstr->str;
  cont.len = seqstr->len;
  stat = addXML_start (desc, &SIB_PARAMETER, &attr, &cont, 1);
  g_string_free(seqstr, TRUE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_NAME;
  attr.defined = &SIB_SUBSCRIPTIONID;
  cont.txt = (char *)subscription_id;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_PARAMETER, &attr, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);


  attr.name = &SIB_NAME;
  attr.defined = &SIB_RESULTS_ADDED;
  cont.txt =  (gchar *)rdfxml_added;
  cont.len = strlen( (gchar *)rdfxml_added);
  stat = addXML_start (desc, &SIB_PARAMETER, &attr, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_NAME;
  attr.defined = &SIB_RESULTS_REMOVED;
  cont.txt =  (gchar *)rdfxml_removed;
  cont.len = strlen( (gchar *)rdfxml_removed);
  stat = addXML_start (desc, &SIB_PARAMETER, &attr, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  stat = addXML_end (desc, &SIB_MESSAGE);

  g_return_val_if_fail(stat == ss_StatusOK, stat);


  whiteboard_log_debug_fe();
  return ss_StatusOK;
}
ssStatus_t ssBufDesc_CreateQueryResponse(ssBufDesc_t *desc,
					 ssElement_ct nodeName,
					 ssElement_ct ssId,
					 gint msgnumber,
					 msgStatus_t status,
					 const guchar *results)
{
  attrStr attr;
  charStr cont;
  ssStatus_t stat;
  GString *trid = g_string_new("");
  whiteboard_log_debug_fb();

  stat = addXML_start (desc, &SIB_MESSAGE, NULL, NULL, 0);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGTYPE, NULL, &SIB_CONFIRM, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  stat = addXML_start (desc, &SIB_MSGNAME, NULL, &SIB_QUERY, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  g_string_printf( trid, MSGNRO_FORMAT, msgnumber);
  cont.txt = trid->str;
  cont.len = trid->len;
  stat = addXML_start (desc, &SIB_MSGNUMBER, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)ssId;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_SPACEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  cont.txt = (char *)nodeName;
  cont.len = strlen(cont.txt);
  stat = addXML_start (desc, &SIB_NODEID, NULL, &cont, 1);
  g_return_val_if_fail(stat == ss_StatusOK, stat);

  attr.name = &SIB_NAME;
  attr.defined = &SIB_STATUS;
  if(status== MSG_E_OK)
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUSOK, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);


      attr.name = &SIB_NAME;
      attr.defined = &SIB_RESULTS;
      cont.txt = (char *)results;
      cont.len = strlen( (gchar *)results);

      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &cont, 1);
    }
  else
    {
      stat = addXML_start (desc, &SIB_PARAMETER, &attr, &SIB_STATUS_SIB_ERROR, 1);
      g_return_val_if_fail(stat == ss_StatusOK, stat);
    }

  g_return_val_if_fail(stat == ss_StatusOK, stat);
    /********* create RDFXML from triple templates *********/
    /*addRDFXML_templates(bD, nameSpaceStr, tripleList);*/

  stat = addXML_end (desc, &SIB_MESSAGE);
  g_return_val_if_fail(stat == ss_StatusOK, stat);
  g_string_free(trid, TRUE);

  whiteboard_log_debug_fe();
  return ss_StatusOK;
}
