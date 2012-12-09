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
 * ssap_parse_n_gen.c
 *
 * Copyright 2007 Nokia Corporation
 */

/* #ifdef SSAP_BOTH_ROLES */
/* #define SIB_ROLE */
/* #define SIBUSER_ROLE */
/* #endif */

/* #ifdef SIB_ROLE */
// the compiler swtich -DSIB_ROLE defines the SIB role in SSAP message exchanges
//#else
// prefer defining SIBUSER_ROLE
//#endif
//#ifdef SIBUSER_ROLE
// the compiler swtich -DSIBUSER_ROLE defines the SIB-user role in SSAP message exchanges
//#else
// prefer defining SIB_ROLE
//#endif

//#ifndef SIB_ROLE
//#ifndef SIBUSER_ROLE
//#if 0 //1=>lenient/guessing
//#define SIBUSER_ROLE
//#else
//NIETHER ROLE DEFINED!
//#endif
//#endif
//#endif

//#define _STAND_ALONE_TESTING

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include "sibmsg.h"
#include "ssap_sib_tokens.h"
#if WHITEBOARD_DEBUG==1
#include "whiteboard_log.h"
#else
#define whiteboard_log_debug_fb()
#define whiteboard_log_debug_fe()
#define whiteboard_log_debug(x,...)
#define whiteboard_log_error(x,...)
#endif

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

#define BUFF_INCREMENT 2048

typedef union {
  NodeMsgContent_t c;
  struct {
    NodeMsgContent_t c;
    /* plus internals: */
    int depth;
    XML_Parser p;
    const char *partxt;
    enum {EL_FLG_NSET=0, EL_FLG_SET} el_selfclosed;
    enum {PAR_N_NSET=0, PAR_N_TRIPLES, PAR_N_TRIPLEIDS, PAR_N_QSTYLE, PAR_N_STATUS, PAR_N_RESULTS, PAR_N_QUERYID, PAR_N_QUERYIDS, PAR_N_SUBSCRIPTIONID, PAR_N_INSERT_GRAPH, PAR_N_REMOVE_GRAPH, PAR_N_RESULTS_ADDED, PAR_N_RESULTS_REMOVED, PAR_N_BNODES, PAR_N_PARAMCONFIRM, PAR_N_CREDENTIALS, PAR_N_INDSEQNUM } paramName;
    enum {SCN_OFF=0, SCN_ON} strScan_state;
    ssBufDesc_t strScan;
    gboolean inCdata;
  } i;
} ParseBlk;

/*-----------------------------------------------------------------------------*/
//statics herein:
static void XMLCALL ssapXML_start  (void *data, const char *el, const char **attr);
static void XMLCALL ssapXML_end    (void *data, const char *el);
static void XMLCALL ssapXML_scanStartOnGap(void *data, const char *s, int sLen);
static void XMLCALL ssapXML_scanStartOnEl(void *data, const char *el, const char **attr);
static void XMLCALL ssapXML_strScan_end  (void *data, const char *el);
static void XMLCALL ssapCDATA_dataHndlr( void *userData, const XML_Char *s,int len);
inline void ssapXML_setQuitParse (ParseBlk * blk, ssStatus_t status);
inline void ssapXML_StopParser (ParseBlk *blk);
inline void ssapXML_strScanStartOnNextGap(ParseBlk * blk, const char *untilEl);
static void ssapStrContentHndl(void *data, const char *s, int sLen);
static void ssapCDATA_start( void *userData);
static void ssapCDATA_end( void *userData);
//end of statics
/*-----------------------------------------------------------------------------*/
static void XMLCALL
ssapXML_strScan_end  (void *data, const char *el)
{
  ParseBlk *blk = data;
  if (blk->i.el_selfclosed == EL_FLG_SET) {
    blk->i.el_selfclosed = EL_FLG_NSET;
    return;
  }
  else
    if (0!=strcmp(el, blk->i.partxt)) {
      int offset, size, len;
      const char *inputBuff = XML_GetInputContext(blk->i.p,
						  &offset,
						  &size);
      whiteboard_log_debug("offset: %d, size %d\n", offset, size);
      /* the paser KNOWS there is a full element read in.. "<someElement ... > ..." */
      for (len=0, size-=offset ; inputBuff[offset+len++]!='>' && len<size;);
      if (len <= size)
	ssapXML_scanStartOnGap (data, &inputBuff[offset], len);
      else
	{ssapXML_setQuitParse (blk, ss_InternalError);return;}

      return;
    }

  blk->i.depth--;

  ssapStrContentHndl (blk, blk->i.strScan.buf, blk->i.strScan.datLen);
  if (blk->c.parseStatus != ss_ParsingInProgress)
    XML_SetElementHandler(blk->i.p, NULL,        NULL);
  else
    XML_SetElementHandler(blk->i.p, ssapXML_start, ssapXML_end);
  XML_SetCharacterDataHandler(blk->i.p, NULL);

  blk->i.strScan.datLen = 0;
  //  blk->i.strScan_state = SCN_OFF;
}

inline void
ssapXML_setQuitParse (ParseBlk * blk, ssStatus_t status)
/* sets the handlers used here to NULL to quit parsing
   - which works only if not reassigned before returning
   control to the parser
*/
{
  whiteboard_log_debug_fb();

  XML_SetCharacterDataHandler(blk->i.p, NULL);
  XML_SetElementHandler(blk->i.p, NULL, NULL);
  if ( blk->c.parseStatus!=ss_ParsingInProgress)//sic.: not expecting ss_StatusOK
    whiteboard_log_error("*** error %i overwrites previously set error %i\n", status, blk->c.parseStatus);
  blk->c.parseStatus = status;

  whiteboard_log_debug_fe();
}

static void XMLCALL
ssapXML_scanStartOnGap(void *data, const char *s, int sLen)
{
  ParseBlk *blk = data;
#if _TESTING // testing
  //int offset, size;
  //  const char *inputBuff = XML_GetInputContext(blk->i.p,
  //					      &offset,
  //				      &size);
  //
  //printf ("*** TESTING offset=%d size=%d\n", offset, size);
#endif
  {
    gchar *deb=g_strndup(s,sLen);
    whiteboard_log_debug("*** TESTING len=%d, s=%s\n", sLen, deb);
    g_free(deb);
  }
  if ((sLen+1) > (blk->i.strScan.bufLen-blk->i.strScan.datLen)) {
    blk->i.strScan.buf = (char *)realloc (blk->i.strScan.buf, (blk->i.strScan.bufLen += BUFF_INCREMENT));
    if (!blk->i.strScan.buf)
      {ssapXML_setQuitParse (blk, ss_NotEnoughResources);return;}
  }
  strncpy (&blk->i.strScan.buf[blk->i.strScan.datLen], s, sLen);
  blk->i.strScan.datLen += sLen;
  blk->i.strScan.buf[blk->i.strScan.datLen]=0;
}

static void XMLCALL
ssapXML_scanStartOnEl(void *data, const char *el, const char **attr)
{
  ParseBlk *blk = data;
  int offset, size, len;
  const char *inputBuff = XML_GetInputContext(blk->i.p,
					      &offset,
					      &size);
  whiteboard_log_debug("offset: %d, size %d\n", offset, size);
  /* the paser KNOWS there is a full element read in.. "<someElement ... > ..." */
  for (len=0, size-=offset ; inputBuff[offset+len++]!='>' && len<size;);
  if (len <= size) {
    if (inputBuff[offset+len-2]=='/')
      blk->i.el_selfclosed = EL_FLG_SET;
    ssapXML_scanStartOnGap (data, &inputBuff[offset], len);
  }
  else
    {ssapXML_setQuitParse (blk, ss_InternalError);return;}
}

static void
ssapStrContentHndl(void *data, const char *s, int sLen)
/* in spite of looking looking like an XMLCALL charater handler, this
   is NOT one. This one requires s to be a zero terminated string, and
   sLen is provided as a convenience. Also, here s is expected to be
   the complete object.
*/
{
  ParseBlk *blk = data;
  whiteboard_log_debug_fb();
  //#if _TESTING // testing
  {
    gchar *deb = g_strndup(s, sLen);
  whiteboard_log_debug ("*** TESTING parsed value string len=%d:\n%s\n***\n", sLen, deb);
  g_free(deb);
  }
  //#endif
  if (blk->i.partxt == SIB_MSGTYPE.txt
#if _TESTING //testing
      ){ //*** for testing - allowing repeats
#else
    && blk->c.type == MSG_T_NSET) {
#endif
#ifdef SIB_ROLE
    if (sLen == SIB_REQUEST.len && 0==strncmp(SIB_REQUEST.txt, s, sLen))
      blk->c.type = MSG_T_REQ;
    else
#endif
#ifdef SIBUSER_ROLE
      if (sLen == SIB_CONFIRM.len && 0==strncmp(SIB_CONFIRM.txt, s, sLen))
	blk->c.type = MSG_T_CNF;
      else
	if (sLen == SIB_INDICATION.len && 0==strncmp(SIB_INDICATION.txt, s, sLen))
	  blk->c.type = MSG_T_IND;
	else
	  //	  if (blk->i.partxt == SIB_INDSEQNUM.txt && blk->c.indUpdateSequence == 0) {
	  //	    blk->c.indUpdateSequence = atoi(s);
	  //}
	  // else
#endif
	  {ssapXML_setQuitParse (blk, ss_ParsingError);return;}
  }
  else
    if (blk->i.partxt == SIB_MSGNAME.txt && blk->c.name == MSG_N_NSET) {
      if (sLen == SIB_JOIN.len && 0==strncmp(SIB_JOIN.txt, s, sLen))
	blk->c.name = MSG_N_JOIN;
      else
	if (sLen == SIB_LEAVE.len && 0==strncmp(SIB_LEAVE.txt, s, sLen))
	  blk->c.name = MSG_N_LEAVE;
	else
	  if (sLen == SIB_INSERT.len && 0==strncmp(SIB_INSERT.txt, s, sLen))
	    blk->c.name = MSG_N_INSERT;
	  else
	    if (sLen == SIB_UPDATE.len && 0==strncmp(SIB_UPDATE.txt, s, sLen))
	      blk->c.name = MSG_N_UPDATE;
	    else
	      if (sLen == SIB_REMOVE.len && 0==strncmp(SIB_REMOVE.txt, s, sLen))
		blk->c.name = MSG_N_REMOVE;
	      else
		if (sLen == SIB_SUBSCRIBE.len && 0==strncmp(SIB_SUBSCRIBE.txt, s, sLen))
		  blk->c.name = MSG_N_SUBSCRIBE;
		else
		  if (sLen == SIB_UNSUBSCRIBE.len && 0==strncmp(SIB_UNSUBSCRIBE.txt, s, sLen))
		    blk->c.name = MSG_N_UNSUBSCRIBE;
		  else
		    if (sLen == SIB_QUERY.len && 0==strncmp(SIB_QUERY.txt, s, sLen))
		      blk->c.name = MSG_N_QUERY;
      //    else
      //    if (sLen == SIB_SUBSCRIBEIND.len && 0==strncmp(SIB_SUBSCRIBEIND.txt, s, sLen))
      //blk->c.name = MSG_N_SUBSCRIBE;
		    else
		      {ssapXML_setQuitParse (blk, ss_ParsingError);return;}
    }
    else
      if (blk->i.partxt == SIB_MSGNUMBER.txt && blk->c.msgNumber == 0) {
	blk->c.msgNumber = atoi(s);
      }
      else
	if (blk->i.partxt == SIB_NODEID.txt && !blk->c.nodeid_MSTR) {
	  blk->c.nodeid_MSTR = g_strdup(s);
	}
	else
	  if (blk->i.partxt == SIB_SPACEID.txt && !blk->c.spaceid_MSTR) {
	    blk->c.spaceid_MSTR = g_strdup(s);
	  }
	  else
	    if (blk->i.partxt == SIB_PARAMETER.txt)
	      {
#ifdef SIB_ROLE
		if (blk->i.paramName == PAR_N_QSTYLE)
		  {
		    if (blk->c.queryStyle != MSG_Q_NSET) {
		      //second time here - so goes the syntax on this one..
		      if (!blk->c.m3XML_MSTR)
			blk->c.m3XML_MSTR = g_strdup(s);
		      else
			{ssapXML_setQuitParse (blk, ss_ParsingError);return;}
		    }
		    else if (sLen == SIB_TYPE_M3XML.len && 0==strncmp(SIB_TYPE_M3XML.txt, s, sLen))
		      blk->c.queryStyle = MSG_Q_TMPL;
		    else if (sLen == SIB_TYPE_WQLVALUES.len && 0==strncmp(SIB_TYPE_WQLVALUES.txt, s, sLen))
		      blk->c.queryStyle = MSG_Q_WQL_VALUES;
		    else if (sLen == SIB_TYPE_WQLNODETYPES.len && 0==strncmp(SIB_TYPE_WQLNODETYPES.txt, s, sLen))
		      blk->c.queryStyle = MSG_Q_WQL_NODETYPES;
		    else if (sLen == SIB_TYPE_WQLRELATED.len && 0==strncmp(SIB_TYPE_WQLRELATED.txt, s, sLen))
		      blk->c.queryStyle = MSG_Q_WQL_RELATED;
		    else if (sLen == SIB_TYPE_WQLISTYPE.len && 0==strncmp(SIB_TYPE_WQLISTYPE.txt, s, sLen))
		      blk->c.queryStyle = MSG_Q_WQL_ISTYPE;
		    else if (sLen == SIB_TYPE_WQLISSUBTYPE.len && 0==strncmp(SIB_TYPE_WQLISSUBTYPE.txt, s, sLen))
		      blk->c.queryStyle = MSG_Q_WQL_ISSUBTYPE;

		    //ARCES
		    else if (sLen == SIB_TYPE_SPARQL.len && 0==strncmp(SIB_TYPE_SPARQL.txt, s, sLen))
		      blk->c.queryStyle = MSG_Q_SPRQL;

		    // -- ARCES
		    else
		      {ssapXML_setQuitParse (blk, ss_ParsingError);return;}
		  }
		else if (blk->i.paramName == PAR_N_PARAMCONFIRM)
		  {
		    if (sLen == SIB_TRUE.len && 0==strncmp(SIB_TRUE.txt, s, sLen))
		      blk->c.confirmReq = TRUE;
		    else if (sLen == SIB_FALSE.len && 0==strncmp(SIB_FALSE.txt, s, sLen))
		      blk->c.confirmReq = FALSE;
		    else
		      {ssapXML_setQuitParse (blk, ss_ParsingError);return;}
		  }
		else if( (blk->i.paramName == PAR_N_INSERT_GRAPH)  &&
			 (!blk->c.m3XML_MSTR) )
		  {
		    blk->c.m3XML_MSTR = g_strndup(s, sLen);
		    whiteboard_log_debug("Got insertresults: %s\n",blk->c.m3XML_MSTR);
		  }
		else if( (blk->i.paramName == PAR_N_REMOVE_GRAPH)  &&
			 (!blk->c.removedResults_MSTR) )
		  {
		    blk->c.removedResults_MSTR = g_strndup(s, sLen);
		    whiteboard_log_debug("Got Removeresults: %s\n",blk->c.removedResults_MSTR);
		  }
		else if ((blk->i.paramName == PAR_N_CREDENTIALS)&&
			 (!blk->c.credentials_MSTR) )
		  {
		    blk->c.credentials_MSTR = g_strndup(s, sLen);
		  }
		else
#endif
		  if( ((blk->i.paramName == PAR_N_TRIPLES) ||
		       //apr09obsolete (blk->i.paramName == PAR_N_TRIPLEIDS) ||
		       (blk->i.paramName == PAR_N_RESULTS) ||
		       (blk->i.paramName == PAR_N_RESULTS_ADDED) ||
		       (blk->i.paramName == PAR_N_BNODES)
		       //apr09obsolete ||
		       //apr09obsolete (blk->i.paramName == PAR_N_REMOVED_TRIPLES)
		       )  &&
		      (!blk->c.m3XML_MSTR) )
		    {
		      blk->c.m3XML_MSTR = g_strndup(s, sLen);
		      whiteboard_log_debug("Got results: %s\n",blk->c.m3XML_MSTR);
		    }
		  else
		    if (blk->i.paramName == PAR_N_RESULTS_REMOVED)
		      {
			blk->c.removedResults_MSTR = g_strndup(s, sLen);
			whiteboard_log_debug("Got Removedresults: %s\n",blk->c.removedResults_MSTR);
		      }
		    else
		      if (blk->i.paramName == PAR_N_STATUS && blk->c.status == MSG_E_NSET) {
			if (sLen==SIB_STATUSOK.len && 0==strncmp (s, SIB_STATUSOK.txt, sLen))
			  blk->c.status = MSG_E_OK;
			else if ((sLen==SIB_STATUS_NOTIF_RESET.len &&
				  0==strncmp (s, SIB_STATUS_NOTIF_RESET.txt, sLen)) ||
				 (sLen==SIB_STATUS_NOTIF_CLOSING.len &&
				  0==strncmp (s, SIB_STATUS_NOTIF_CLOSING.txt, sLen)) ||
				 (sLen==SIB_STATUS_SIB_ERROR.len &&
				  0==strncmp (s, SIB_STATUS_SIB_ERROR.txt, sLen)) ||
				 (sLen==SIB_STATUS_SIB_FAIL_ACCESSDENIED.len &&
				  0==strncmp (s, SIB_STATUS_SIB_FAIL_ACCESSDENIED.txt, sLen)) ||
				 (sLen==SIB_STATUS_SIB_FAIL_NOTIMPLEMENTED.len &&
				  0==strncmp (s, SIB_STATUS_SIB_FAIL_NOTIMPLEMENTED.txt, sLen)) ||
				 (sLen==SIB_STATUS_KP_ERROR.len &&
				  0==strncmp (s, SIB_STATUS_KP_ERROR.txt, sLen)) ||
				 (sLen==SIB_STATUS_KP_ERROR_REQUEST.len &&
				  0==strncmp (s, SIB_STATUS_KP_ERROR_REQUEST.txt, sLen)) ||
				 (sLen==SIB_STATUS_KP_ERROR_MSG_INCOMPLETE.len &&
				  0==strncmp (s, SIB_STATUS_KP_ERROR_MSG_INCOMPLETE.txt, sLen)) ||
				 (sLen==SIB_STATUS_KP_ERROR_MSG_SYNTAX.len &&
				  0==strncmp (s, SIB_STATUS_KP_ERROR_MSG_SYNTAX.txt, sLen)))
			  blk->c.status = MSG_E_NOK;
			else
			  {ssapXML_setQuitParse (blk, ss_ParsingError);return;}
		      }
		      else if (((blk->i.paramName == PAR_N_QUERYID) || (blk->i.paramName == PAR_N_QUERYIDS) ||
				(blk->i.paramName == PAR_N_SUBSCRIPTIONID)) &&
			       !blk->c.queryids_MSTR) {
			blk->c.queryids_MSTR = g_strdup(s);
		      }
		      else if( blk->i.paramName == PAR_N_INDSEQNUM )
			{
			  blk->c.indUpdateSequence = atoi(s);
			}
		      else
			{ssapXML_setQuitParse (blk, ss_ParsingError);return;}
	      }
	    else if (blk->i.partxt == SIB_ICV.txt && !blk->c.icv_MSTR) {
	      blk->c.icv_MSTR = g_strdup(s);
	    }
	    else
	      {ssapXML_setQuitParse (blk, ss_ParsingError);return;}
  whiteboard_log_debug_fe();
}

inline void
ssapXML_strScanStartOnNextGap(ParseBlk * blk, const char *untilEl)
{
  blk->i.partxt = untilEl;
  XML_SetCharacterDataHandler(blk->i.p, ssapXML_scanStartOnGap);
  XML_SetElementHandler(blk->i.p, ssapXML_scanStartOnEl, ssapXML_strScan_end);
}

inline void ssapXML_StopParser (ParseBlk *blk)
{
  g_return_if_fail( blk->i.p != NULL);
  XML_StopParser(blk->i.p, 1); // resumable
}

static void XMLCALL
ssapXML_start(void *data, const char *el, const char **attr)
{
  ParseBlk *blk = data;
  whiteboard_log_debug_fb();

  g_return_if_fail (blk->c.parseStatus == ss_ParsingInProgress);

  if ( ++(blk->i.depth) > 3)
    {
      ssapXML_setQuitParse (blk, ss_ParsingError);
      whiteboard_log_debug_fe();
      return;
    }

  if (blk->i.depth==1)
    {
      if (0==strcmp(el, SIB_MESSAGE.txt) && !attr[0])
	{
	  whiteboard_log_debug("Start of new SSAP_message\n");
	  whiteboard_log_debug_fe();
	  return;
	}
      else
	{
	  ssapXML_setQuitParse (blk, ss_ParsingError);
	  whiteboard_log_debug_fe();
	  return;
	}
    }

  /* element specific */
  if (0==strcmp(el, SIB_MSGTYPE.txt) && !attr[0])
    ssapXML_strScanStartOnNextGap (blk, SIB_MSGTYPE.txt);

  else if (0==strcmp(el, SIB_MSGNAME.txt) && !attr[0])
    ssapXML_strScanStartOnNextGap (blk, SIB_MSGNAME.txt);

  else if (0==strcmp(el, SIB_MSGNUMBER.txt) && !attr[0])
    ssapXML_strScanStartOnNextGap (blk, SIB_MSGNUMBER.txt);

  //  else if (0==strcmp(el, SIB_INDSEQNUM.txt) && !attr[0])
  //  ssapXML_strScanStartOnNextGap (blk, SIB_INDSEQNUM.txt);

  else if (0==strcmp(el, SIB_NODEID.txt) && !attr[0])
    ssapXML_strScanStartOnNextGap (blk, SIB_NODEID.txt);

  else if (0==strcmp(el, SIB_SPACEID.txt) && !attr[0])
    ssapXML_strScanStartOnNextGap (blk, SIB_SPACEID.txt);

  else if( (0==strcmp(el, SIB_PARAMETER.txt)) && attr[1] )  // parameter element with at least 1 attribute
    {
      int name_index = -1;
      int encoding_index = -1;
      int i = 0;
      // find the "name" attribute
      while( attr[i] && attr[i+1] && (name_index < 0 ) )
	{
	  if(0 == strcmp(attr[i],  SIB_NAME.txt) )
	    {
	      name_index = i;
	      whiteboard_log_debug("Found name attribute, index: %d\n", name_index);
	    }
	  i += 2;
	}
      if(name_index<0)
	{
	  whiteboard_log_debug("Name attribute not found\n");
	  ssapXML_setQuitParse (blk, ss_ParsingError);
	  whiteboard_log_debug_fe();
	  return;
	}
      // find the "encoding" attribute
      i = 0;
      while( attr[i] && attr[i+1] && (encoding_index < 0 ) )
	{
	  if((i != name_index ) && (0 == strcmp(attr[i],  SIB_ENCODING.txt) ))
	    {
	      encoding_index = i;
	      whiteboard_log_debug("Found encoding attribute, index: %d\n",encoding_index);
	    }
	  i += 2;
	}

      if (FALSE)
	;// for else-if style.....    // arces: questo è un pazzo furioso.
#ifdef SIB_ROLE
      else if( (name_index >= 0) && (encoding_index >= 0)  &&
	       (0==strcmp(attr[name_index+1], SIB_INSERTGRAPH.txt)) &&
	       (0==strcmp(attr[encoding_index+1], SIB_TYPE_M3XML.txt)) )
	{
	  blk->c.graphStyle = MSG_G_TMPL;
	  blk->i.paramName = PAR_N_INSERT_GRAPH;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if(  (name_index >= 0) && (encoding_index >= 0)  &&
		(0==strcmp(attr[name_index+1], SIB_REMOVEGRAPH.txt)) &&
		(0==strcmp(attr[encoding_index+1], SIB_TYPE_M3XML.txt)) )
	{
	  blk->c.graphStyle = MSG_G_TMPL;
	  blk->i.paramName = PAR_N_REMOVE_GRAPH;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
//ARCES

      else if( (name_index >= 0) && (encoding_index >= 0)  &&
	       (0==strcmp(attr[name_index+1], SIB_INSERTGRAPH.txt)) &&
	       (0==strcmp(attr[encoding_index+1], SIB_TYPE_RDFXML.txt)) )
	{
	  blk->c.graphStyle = MSG_G_RDF;
	  blk->i.paramName = PAR_N_INSERT_GRAPH;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if(  (name_index >= 0) && (encoding_index >= 0)  &&
		(0==strcmp(attr[name_index+1], SIB_REMOVEGRAPH.txt)) &&
		(0==strcmp(attr[encoding_index+1], SIB_TYPE_RDFXML.txt)) )
	{
	  blk->c.graphStyle = MSG_G_RDF;
	  blk->i.paramName = PAR_N_REMOVE_GRAPH;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}

//ARCES End

#endif
      else if (attr[2]) //no more two attribute elements below
	{
	  ssapXML_setQuitParse (blk, ss_ParsingError);
	  whiteboard_log_debug_fe();
	  return;
	}
#ifdef SIB_ROLE
      else if (0==strcmp(attr[1], SIB_QUERYSTRING.txt))
	{
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if (0==strcmp(attr[1], SIB_PARAMCONFIRM.txt))
	{
	  blk->i.paramName = PAR_N_PARAMCONFIRM;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if (0==strcmp(attr[1], SIB_CREDENTIALS.txt))
	{
	  blk->i.paramName = PAR_N_CREDENTIALS;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
#endif
      else if (0==strcmp(attr[1], SIB_TRIPLES.txt))
	{
	  blk->i.paramName = PAR_N_TRIPLES;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if (0==strcmp(attr[1], SIB_TYPE.txt))
	{
	  blk->i.paramName = PAR_N_QSTYLE;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if (0==strcmp(attr[1], SIB_STATUS.txt))
	{
	  blk->i.paramName = PAR_N_STATUS;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if (0==strcmp(attr[1], SIB_QUERYID.txt))
	{
	  blk->i.paramName = PAR_N_QUERYID;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if (0==strcmp(attr[1], SIB_SUBSCRIPTIONID.txt))
	{
	  blk->i.paramName = PAR_N_SUBSCRIPTIONID;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if (0==strcmp(attr[1], SIB_RESULTS.txt))
	{
	  blk->i.paramName = PAR_N_RESULTS;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if (0==strcmp(attr[1], SIB_RESULTS_ADDED.txt))
	{
	  blk->i.paramName = PAR_N_RESULTS_ADDED;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if (0==strcmp(attr[1], SIB_RESULTS_REMOVED.txt))
	{
	  blk->i.paramName = PAR_N_RESULTS_REMOVED;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if (0==strcmp(attr[1], SIB_BNODES.txt))
	{
	  blk->i.paramName = PAR_N_BNODES;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else if (0==strcmp(attr[1], SIB_INDSEQNUM.txt))
	{
	  blk->i.paramName = PAR_N_INDSEQNUM;
	  ssapXML_strScanStartOnNextGap (blk, SIB_PARAMETER.txt);
	}
      else
	{
	  ssapXML_setQuitParse (blk, ss_ParsingError);
	  whiteboard_log_debug_fe();
	  return;
	}
    }
  else if (0==strcmp(el, SIB_ICV.txt) && !attr[0])
    {
      ssapXML_strScanStartOnNextGap (blk, SIB_ICV.txt);
    }
  else
    {
      ssapXML_setQuitParse (blk, ss_ParsingError);
      whiteboard_log_debug_fe();
      return;
    }
  whiteboard_log_debug_fe();
}

static void XMLCALL
ssapXML_end  (void *data, const char *el)
{
  ParseBlk *blk = data;
  whiteboard_log_debug_fb();
  g_return_if_fail (blk->c.parseStatus == ss_ParsingInProgress);

  blk->i.depth--;
  if(blk->i.depth == 0)
    {
      whiteboard_log_debug("Depth 0 reached\n");
      ssapXML_StopParser (blk);
    }
  else if(  blk->i.depth < 0)
    {
      ssapXML_setQuitParse (blk, ss_ParsingError);
      return;
    }
  whiteboard_log_debug_fe();
}

static void XMLCALL
ssapCDATA_dataHndlr( void *userData, const XML_Char *s,int sLen)
{
  ParseBlk *blk = (ParseBlk *)userData;
  whiteboard_log_debug_fb();
  if(blk)
    {
      if ((sLen+1) > (blk->i.strScan.bufLen-blk->i.strScan.datLen))
	{
	  blk->i.strScan.buf = (char *)realloc (blk->i.strScan.buf, (blk->i.strScan.bufLen += BUFF_INCREMENT));
	  if (!blk->i.strScan.buf)
	    {ssapXML_setQuitParse (blk, ss_NotEnoughResources);return;}
	}
      strncpy (&blk->i.strScan.buf[blk->i.strScan.datLen], s, sLen);
      blk->i.strScan.datLen += sLen;
      blk->i.strScan.buf[blk->i.strScan.datLen]=0;
    }
  whiteboard_log_debug_fe();
}

NodeMsgContent_t  *parseSSAPmsg_new()
{
  whiteboard_log_debug_fb();
  ParseBlk *blk = g_new0( ParseBlk, 1);
  if (blk)
    {
    blk->i.p = XML_ParserCreate(NULL);
    blk->i.inCdata = FALSE;
    }
  if (!blk || !blk->i.p) {
    whiteboard_log_error("*** Couldn't allocate memory for parsing\n");
    if (blk)
      g_free(blk);
    whiteboard_log_debug_fe();
    return NULL;
  }

XML_SetUserData(blk->i.p, blk);
  XML_SetElementHandler(blk->i.p, ssapXML_start, ssapXML_end);
  XML_SetCdataSectionHandler( blk->i.p, ssapCDATA_start, ssapCDATA_end);

  whiteboard_log_debug_fe();
  return &blk->c;
}

void ssapCDATA_start(void *userData)
{
  whiteboard_log_debug_fb();
  ParseBlk *blk = (ParseBlk *)userData;
  if(blk)
    {
      blk->i.inCdata = TRUE;
      if ((10) > (blk->i.strScan.bufLen-blk->i.strScan.datLen))  /* length of "<![CDATA[" +1*/
	{
	  blk->i.strScan.buf = (char *)realloc (blk->i.strScan.buf, (blk->i.strScan.bufLen += BUFF_INCREMENT));
	  if (!blk->i.strScan.buf)
	    {ssapXML_setQuitParse (blk, ss_NotEnoughResources);return;}
	}
      strncpy (&blk->i.strScan.buf[blk->i.strScan.datLen], "<![CDATA[", 9);
      blk->i.strScan.datLen += 9;
      blk->i.strScan.buf[blk->i.strScan.datLen]=0;
      XML_SetCharacterDataHandler( blk->i.p, ssapCDATA_dataHndlr);
    }
  whiteboard_log_debug_fe();
}

void ssapCDATA_end(void *userData)
{
  whiteboard_log_debug_fb();
  ParseBlk *blk = (ParseBlk *)userData;
  if(blk)
    {
 if ((4) > (blk->i.strScan.bufLen-blk->i.strScan.datLen))  /* length of "]]>" +1*/
	{
	  blk->i.strScan.buf = (char *)realloc (blk->i.strScan.buf, (blk->i.strScan.bufLen += BUFF_INCREMENT));
	  if (!blk->i.strScan.buf)
	    {ssapXML_setQuitParse (blk, ss_NotEnoughResources);return;}
	}
      strncpy (&blk->i.strScan.buf[blk->i.strScan.datLen], "]]>", 3);
      blk->i.strScan.datLen += 3;
      blk->i.strScan.buf[blk->i.strScan.datLen]=0;
      blk->i.inCdata = FALSE;
      XML_SetCharacterDataHandler( blk->i.p, ssapXML_scanStartOnGap);
    }
  whiteboard_log_debug_fe();
}

NodeMsgContent_t  *nodemsgcontent_new_join_rsp( guchar *spaceid,
						guchar *nodeid,
						gint msgnum,
						gint success)
{
  NodeMsgContent_t  *self = g_try_new0(NodeMsgContent_t,1);
  whiteboard_log_debug_fb();
  if(self)
    {
      self->name = MSG_N_JOIN;
      self->type= MSG_T_CNF;
      self->spaceid_MSTR = g_strdup((gchar*)spaceid);
      self->nodeid_MSTR = g_strdup((gchar*)nodeid);
      self->msgNumber = msgnum;
      self->status = (success == 0 ? MSG_E_OK: MSG_E_NOK);
    }
  whiteboard_log_debug_fe();
  return self;
}

NodeMsgContent_t  *nodemsgcontent_new_leave_rsp( guchar *spaceid,
						 guchar *nodeid,
						 gint msgnum,
						 gint success)
{
  NodeMsgContent_t  *self = g_try_new0(NodeMsgContent_t,1);
  whiteboard_log_debug_fb();
  if(self)
    {
      self->name = MSG_N_LEAVE;
      self->type= MSG_T_CNF;
      self->spaceid_MSTR = g_strdup((gchar*) spaceid);
      self->nodeid_MSTR = g_strdup((gchar*)nodeid);
      self->msgNumber = msgnum;
      self->status = (success == 0 ? MSG_E_OK: MSG_E_NOK);
    }
  whiteboard_log_debug_fe();
  return self;
}

NodeMsgContent_t  *nodemsgcontent_new_insert_rsp( guchar *spaceid,
						  guchar *nodeid,
						  gint msgnum,
						  gint success,
						  guchar *bNodes)
{
  NodeMsgContent_t  *self = g_try_new0(NodeMsgContent_t,1);
  whiteboard_log_debug_fb();
  if(self)
    {
      self->name = MSG_N_INSERT;
      self->type= MSG_T_CNF;
      self->spaceid_MSTR = g_strdup((gchar*)spaceid);
      self->nodeid_MSTR = g_strdup((gchar*)nodeid);
      if(bNodes)
	self->m3XML_MSTR = g_strdup((gchar*)bNodes);

      self->msgNumber = msgnum;
      /*self->status = (success == 0 ? MSG_E_OK: MSG_E_NOK);*/    /*DAN ARCES , see sibdefs.h*/
      self->status = success;
    }
  whiteboard_log_debug_fe();
  return self;
}

NodeMsgContent_t  *nodemsgcontent_new_remove_rsp( guchar *spaceid,
						  guchar *nodeid,
						  gint msgnum,
						  gint success)
{
  NodeMsgContent_t  *self = g_try_new0(NodeMsgContent_t,1);
  whiteboard_log_debug_fb();
  if(self)
    {
      self->name = MSG_N_REMOVE;
      self->type= MSG_T_CNF;
      self->spaceid_MSTR = g_strdup((gchar*)spaceid);
      self->nodeid_MSTR = g_strdup((gchar*)nodeid);
      self->msgNumber = msgnum;
      /*self->status = (success == 0 ? MSG_E_OK: MSG_E_NOK);*/    /*DAN ARCES*/
      self->status = success;
    }
  whiteboard_log_debug_fe();
  return self;
}

NodeMsgContent_t  *nodemsgcontent_new_update_rsp( guchar *spaceid,
						  guchar *nodeid,
						  gint msgnum,
						  gint success,
						  guchar *bNodes)
{
  NodeMsgContent_t  *self = g_try_new0(NodeMsgContent_t,1);
  whiteboard_log_debug_fb();
  if(self)
    {
      self->name = MSG_N_UPDATE;
      self->type= MSG_T_CNF;
      self->spaceid_MSTR = g_strdup((gchar*)spaceid);
      self->nodeid_MSTR = g_strdup((gchar*)nodeid);
      if(bNodes)
	self->m3XML_MSTR = g_strdup((gchar*)bNodes);

      self->msgNumber = msgnum;
      /*self->status = (success == 0 ? MSG_E_OK: MSG_E_NOK);*/    /*DAN ARCES*/
      self->status = success;
    }
  whiteboard_log_debug_fe();
  return self;
}

NodeMsgContent_t  *nodemsgcontent_new_query_rsp( guchar *spaceid,
						 guchar *nodeid,
						 gint msgnum,
						 gint success,
						 guchar *results)
{
  NodeMsgContent_t  *self = g_try_new0(NodeMsgContent_t,1);
  whiteboard_log_debug_fb();
  if(self)
    {
      self->name = MSG_N_QUERY;
      self->type= MSG_T_CNF;
      self->spaceid_MSTR = g_strdup((gchar*)spaceid);
      self->nodeid_MSTR = g_strdup((gchar*)nodeid);
      if(results)
	self->m3XML_MSTR = g_strdup((gchar*)results);

      self->msgNumber = msgnum;
      self->status = (success == 0 ? MSG_E_OK: MSG_E_NOK);
    }
  whiteboard_log_debug_fe();
  return self;
}

NodeMsgContent_t  *nodemsgcontent_new_subscribe_rsp( guchar *spaceid,
						     guchar *nodeid,
						     gint msgnum,
						     gint success,
						     guchar *subscription_id,
						     guchar *results)
{
  NodeMsgContent_t  *self = g_try_new0(NodeMsgContent_t,1);
  whiteboard_log_debug_fb();
  if(self)
    {
      self->name = MSG_N_SUBSCRIBE;
      self->type= MSG_T_CNF;
      self->spaceid_MSTR = g_strdup((gchar*)spaceid);
      self->nodeid_MSTR = g_strdup((gchar*)nodeid);
      if(results)
	self->m3XML_MSTR = g_strdup((gchar*)results);

      if(subscription_id)
	self->queryids_MSTR = g_strdup((gchar*)subscription_id);

      self->msgNumber = msgnum;
      self->status = (success == 0 ? MSG_E_OK: MSG_E_NOK);
    }
  whiteboard_log_debug_fe();
  return self;
}

NodeMsgContent_t  *nodemsgcontent_new_subscription_ind( guchar *spaceid,
							guchar *nodeid,
							gint msgnum,
							gint seqnum,
							guchar *subscription_id,
							guchar *results_new,
							guchar *results_obsolete)
{
  NodeMsgContent_t  *self = g_try_new0(NodeMsgContent_t,1);
  whiteboard_log_debug_fb();
  if(self)
    {
      self->name = MSG_N_SUBSCRIBE;
      self->type= MSG_T_IND;
      self->spaceid_MSTR = g_strdup((gchar*)spaceid);
      self->nodeid_MSTR = g_strdup((gchar*)nodeid);

      self->queryids_MSTR = g_strdup((gchar*)subscription_id);

      if(results_new)
	self->m3XML_MSTR = g_strdup((gchar*)results_new);

      if(results_obsolete)
	self->removedResults_MSTR = g_strdup((gchar*)results_obsolete);

      self->msgNumber = msgnum;
      self->indUpdateSequence = seqnum;
    }
  whiteboard_log_debug_fe();
  return self;
}

NodeMsgContent_t  *nodemsgcontent_new_unsubscribe_rsp( guchar *spaceid,
						       guchar *nodeid,
						       gint msgnum,
						       gint success,
						       guchar *subscription_id)
{
  NodeMsgContent_t  *self = g_try_new0(NodeMsgContent_t,1);
  whiteboard_log_debug_fb();
  if(self)
    {
      self->name = MSG_N_UNSUBSCRIBE;
      self->type= MSG_T_CNF;
      self->spaceid_MSTR = g_strdup((gchar*)spaceid);
      self->nodeid_MSTR = g_strdup((gchar*)nodeid);

      self->queryids_MSTR = g_strdup((gchar*)subscription_id);

      self->msgNumber = msgnum;
      self->status = (success == 0 ? MSG_E_OK: MSG_E_NOK);
    }
  whiteboard_log_debug_fe();
  return self;
}

void nodemsgcontent_free(NodeMsgContent_t  **_msg)
{
  NodeMsgContent_t  *msg = *_msg;

  g_return_if_fail(msg != NULL);
  if(msg->nodeid_MSTR)
    g_free(msg->nodeid_MSTR);
  if(msg->spaceid_MSTR)
    g_free(msg->spaceid_MSTR);
  if(msg->m3XML_MSTR)
    g_free( msg->m3XML_MSTR);
  if(msg->removedResults_MSTR)
    g_free(msg->removedResults_MSTR);
  if(msg->expireTime_MSTR)
    g_free( msg->expireTime_MSTR);
  if(msg->icv_MSTR)
    g_free( msg->icv_MSTR);
  if(msg->queryids_MSTR)
    g_free(msg->queryids_MSTR);

  if(msg->credentials_MSTR)
    g_free(msg->credentials_MSTR);

  g_free(msg);
  //that's our local copy   msg=NULL;
  *_msg=NULL; //the user's cop
}

void parseSSAPmsg_free(NodeMsgContent_t  **_msg)
{
  whiteboard_log_debug_fb();
  g_return_if_fail(_msg != NULL);
  NodeMsgContent_t  *msg = *_msg;
  ParseBlk *blk = (ParseBlk *)msg;
  g_return_if_fail(msg != NULL);
  if(msg->nodeid_MSTR)
    g_free(msg->nodeid_MSTR);
  if(msg->spaceid_MSTR)
    g_free(msg->spaceid_MSTR);
  if(msg->m3XML_MSTR)
    g_free( msg->m3XML_MSTR);
  if(msg->expireTime_MSTR)
    g_free( msg->expireTime_MSTR);
  if(msg->icv_MSTR)
    g_free( msg->icv_MSTR);
  if(msg->queryids_MSTR)
    g_free(msg->queryids_MSTR);
  //also internals
  if(msg->removedResults_MSTR)
    g_free(msg->removedResults_MSTR);

  if(msg->credentials_MSTR)
    g_free(msg->credentials_MSTR);

  char *buf = blk->i.strScan.buf;
  if (buf)
    g_free(buf);

  if(blk->i.p)
    {
      XML_ParserFree( blk->i.p);
    }
  g_free((/* in reality */ParseBlk *)msg);
  //that's our local copy   msg=NULL;
  *_msg=NULL; //the user's copy
  whiteboard_log_debug_fe();
}

ssStatus_t parseSSAPmsg_section (NodeMsgContent_t  *msgParsed,
				 char *msgSegment,
				 gint msgSegmentLen,
				 gboolean lastSegment)
{
  whiteboard_log_debug_fb();
  g_return_val_if_fail(msgParsed != NULL, ss_InvalidParameter);
  g_return_val_if_fail(msgSegment != NULL, ss_InvalidParameter);
  enum XML_Status status;
  enum XML_Error errorcode;
  XML_ParsingStatus parsingstatus;
  ParseBlk *blk = (ParseBlk *)msgParsed;
  if(blk->c.parseStatus == ss_StatusOK )
    {
      blk->c.parseStatus = ss_ParsingInProgress;
    }

  if ( (status = XML_Parse(blk->i.p, msgSegment, msgSegmentLen, lastSegment)) == XML_STATUS_ERROR)
    {
      errorcode = XML_GetErrorCode( blk->i.p);
      whiteboard_log_debug("Parse error : %d\n", errorcode);
      blk->c.parseStatus = (blk->c.parseStatus != ss_ParsingInProgress)?ss_InternalError:ss_ParsingError;
    }
  else if(blk->i.depth==0)
    {
      XML_GetParsingStatus( blk->i.p, &parsingstatus);
      whiteboard_log_debug("parse finished, depth = 0, status: %d\n",parsingstatus.parsing);
      if( (parsingstatus.parsing == XML_FINISHED) ||
	  (parsingstatus.parsing == XML_SUSPENDED) )
	{
	  blk->c.parseStatus = ss_StatusOK;
	}
    }
  whiteboard_log_debug_fe();
  return blk->c.parseStatus;
}

gint parseSSAPmsg_parsedbytecount(NodeMsgContent_t *msgParBlk)
{
  ParseBlk *blk = (ParseBlk *)msgParBlk;
  if(blk->i.p )
    {
      return  XML_GetCurrentByteIndex(blk->i.p);
    }
  else
    return -1;
}

const gchar *parseSSAPmsg_get_M3XML(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, NULL);
  return (msg->m3XML_MSTR);
}

gint parseSSAPmsg_get_msgnumber(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, -1);
  return (msg->msgNumber);
}

gint parseSSAPmsg_get_update_sequence(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, -1);
  return (msg->indUpdateSequence);
}

const gchar *parseSSAPmsg_get_subscriptionid(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, NULL);
  return (msg->queryids_MSTR);
}

const char *parseSSAPmsg_get_results_added( NodeMsgContent_t  *msg)
{
  return parseSSAPmsg_get_M3XML(msg);
}

const char *parseSSAPmsg_get_results_removed( NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, NULL);
  return (msg->removedResults_MSTR);
}

const char *parseSSAPmsg_get_insert_graph( NodeMsgContent_t  *msg)
{
  return parseSSAPmsg_get_M3XML(msg);
}

const char *parseSSAPmsg_get_remove_graph( NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, NULL);
  return (msg->removedResults_MSTR);
}

const gchar *parseSSAPmsg_get_spaceid(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, NULL);
  return (msg->spaceid_MSTR);
}

const gchar *parseSSAPmsg_get_nodeid(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, NULL);
  return (msg->nodeid_MSTR);
}

const gchar *parseSSAPmsg_get_credentials(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, NULL);
  return (msg->credentials_MSTR);
}

msgType_t parseSSAPmsg_get_type(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, MSG_T_NSET);
  return (msg->type);
}

msgName_t parseSSAPmsg_get_name(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, MSG_N_NSET);
  return (msg->name);
}

ssStatus_t parseSSAPmsg_get_status(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, ss_InvalidParameter);
  return (msg->parseStatus);
}

queryStyle_t parseSSAPmsg_get_queryStyle(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail
    (msg != NULL &&
     (msg->name==MSG_N_QUERY || msg->name==MSG_N_SUBSCRIBE),
     MSG_Q_NSET);
  return (msg->queryStyle);
}

graphStyle_t parseSSAPmsg_get_graphStyle(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail
    (msg != NULL &&
     (msg->name==MSG_N_INSERT || msg->name==MSG_N_UPDATE || msg->name==MSG_N_REMOVE),
     MSG_G_NSET);
  return (msg->graphStyle);
}

msgStatus_t parseSSAPmsg_get_msg_status(NodeMsgContent_t  *msg)
{
  g_return_val_if_fail(msg != NULL, MSG_E_NSET);
  return (msg->status);
}

#ifdef _STAND_ALONE_TESTING
//==================================== for testing:
/************* compiled with:
gcc -DHAVE_CONFIG_H -I. -I.. -I../include -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -Wall -g -O0 -Di686 ssap_parse_n_gen.c -lexpat -pthread -lgthread-2.0 -lrt -lglib-2.0 -lm3_parse_n_gen -lssap_parse_n_gen -lwhiteboard-util -DSSAP_BOTH_ROLES -luuid -D_STAND_ALONE_TESTING
*************/

#include <whiteboard_node.h>
int main(int argc, char *argv[])
{
  char line[80];
  int len=argc;//just testin
  gboolean lastSegment;

  g_thread_init(NULL);//for logging

  whiteboard_log_debug_fb();

  gchar *uriList_str=NULL;

  NodeMsgContent_t  *msgParsed = parseSSAPmsg_new();
  ssStatus_t status;

  GSList * tripleList = NULL;
#ifdef SIB_ROLE
  ssWqlDesc_t *wqlD = NULL;
#endif

  do {//take segements from stdin

    lastSegment = (NULL==fgets(line,sizeof(line),stdin));
    len=strlen(line);

    status =
      parseSSAPmsg_section (msgParsed,
			    line,
			    len,
			    lastSegment);
  } while (status==ss_ParsingInProgress && !lastSegment);

  if (!status)
    {
      if (parseSSAPmsg_get_type(msgParsed)==MSG_T_CNF)
	{
#ifdef SIBUSER_ROLE
	  if (parseSSAPmsg_get_name(msgParsed)==MSG_N_INSERT)
	    status = parseM3_triples(&tripleList, parseSSAPmsg_get_M3XML(msgParsed), NULL);
#endif
	}
      else if (parseSSAPmsg_get_type(msgParsed)==MSG_T_REQ)
	{
#ifdef SIB_ROLE
	  if (parseSSAPmsg_get_name(msgParsed)==MSG_N_INSERT)
	    status = parseM3_triples_SIB(&tripleList, parseSSAPmsg_get_M3XML(msgParsed), NULL, &uriList_str);
	  else if (parseSSAPmsg_get_name(msgParsed)==MSG_N_QUERY ||
		   parseSSAPmsg_get_name(msgParsed)==MSG_N_SUBSCRIBE)
	    {
	      queryStyle_t qStyle = parseSSAPmsg_get_queryStyle(msgParsed);
	      wqlD = ssWqlDesc_new(qStyle);

	      if (qStyle==MSG_Q_TMPL)
		status = parseM3_triples_SIB(&tripleList, msgParsed->m3XML_MSTR, NULL, NULL);
	      else if (wqlD) //any of the WQL queries (set by test above)
		status = parseM3_query_req_wql(wqlD, parseSSAPmsg_get_M3XML(msgParsed));
	    }
#endif
	}
    }

  printf ("\n\n************* end of parse ************\n\
parse status %sOk (status=%i)\n\n", (!status)?"":"NOT ", status);

  parseSSAPmsg_free(&msgParsed);

  if (uriList_str) {
    printf ("Uri List:\n%s\n", uriList_str);
    g_free(uriList_str);
  }

  ssTriple_t *t;
  GSList *l=tripleList;//NULL if none; carry on..

  for (; l && (t=l->data); l=l->next)
    printf("triple: [%i]:%s\n            %s\n        [%i]:%s\n", t->subjType, t->subject, t->predicate, t->objType, t->object);

  ssFreeTripleList (&tripleList);

#ifdef SIB_ROLE
  if (wqlD)
    switch (wqlD->qType) {
    case QueryTypeWQLValues:
      printf("Wilbur Query: Values?,\n  node[%s]=%s\n  expression=%s\n\n", (wqlD->wqlType.values.startNode->nodeType==ssElement_TYPE_URI)?"URI":"LIT", wqlD->wqlType.values.startNode->string, wqlD->wqlType.values.pathExpr);
      break;
    case QueryTypeWQLNodeTypes:
      printf("Wilbur Query: NodeTypes?,\n  node[%s]=%s\n\n", (wqlD->wqlType.nodeTypes.node->nodeType==ssElement_TYPE_URI)?"URI":"LIT", wqlD->wqlType.nodeTypes.node->string);
      break;
    case QueryTypeWQLRelated:
      printf("Wilbur Query: Related?,\n  startNode[%s]=%s\n  expression=%s\n  endNode[%s]=%s\n\n", (wqlD->wqlType.related.startNode->nodeType==ssElement_TYPE_URI)?"URI":"LIT", wqlD->wqlType.related.startNode->string, wqlD->wqlType.related.pathExpr, (wqlD->wqlType.related.endNode->nodeType==ssElement_TYPE_URI)?"URI":"LIT", wqlD->wqlType.related.endNode->string);
      break;
    case QueryTypeWQLIsType:
      printf("Wilbur Query: isType?,\n  node[%s]=%s\n  typeNode[%s]=%s\n\n", (wqlD->wqlType.isType.node->nodeType==ssElement_TYPE_URI)?"URI":"LIT", wqlD->wqlType.isType.node->string, (wqlD->wqlType.isType.typeNode->nodeType==ssElement_TYPE_URI)?"URI":"LIT", wqlD->wqlType.isType.typeNode->string);
      break;
    case QueryTypeWQLIsSubType:
      printf("Wilbur Query: isSubType?,\n  subTypeNode[%s]=%s\n  superTypeNode[%s]=%s\n\n", (wqlD->wqlType.isSubType.subTypeNode->nodeType==ssElement_TYPE_URI)?"URI":"LIT", wqlD->wqlType.isSubType.subTypeNode->string, (wqlD->wqlType.isSubType.superTypeNode->nodeType==ssElement_TYPE_URI)?"URI":"LIT", wqlD->wqlType.isSubType.superTypeNode->string);
      break;
    default:;
    }

  ssWqlDesc_free (&wqlD);//ok if already NULL
#endif

  whiteboard_log_debug_fe();
  exit(0);
}
#endif
