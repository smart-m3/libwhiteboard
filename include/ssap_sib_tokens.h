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
#ifndef SSSAP_SIB_TOKENS_H
#define SSAP_SIB_TOKENS_H

#include "sibdefs.h"

extern charStr SIB_MESSAGE;
extern charStr SIB_MSGTYPE;
extern charStr SIB_REQUEST;
extern charStr SIB_CONFIRM;
extern charStr SIB_INDICATION;
extern charStr SIB_INDSEQNUM;
extern charStr SIB_MSGNAME;
extern charStr SIB_MSGNUMBER;
extern charStr SIB_NODEID;
extern charStr SIB_SPACEID;
extern charStr SIB_PARAMETER;
extern charStr SIB_NAME;
extern charStr SIB_ICV;

extern charStr SIB_JOIN;
extern charStr SIB_LEAVE;
extern charStr SIB_INSERT;
extern charStr SIB_REMOVE;
extern charStr SIB_UPDATE;
extern charStr SIB_QUERY;
extern charStr SIB_SUBSCRIBE;
extern charStr SIB_UNSUBSCRIBE;

extern charStr SIB_PARAMCONFIRM;
extern charStr SIB_TYPE;
extern charStr SIB_TYPE_RDFXML;
extern charStr SIB_TYPE_M3XML;
extern charStr SIB_TYPE_SPARQL;
extern charStr SIB_TYPE_WQLVALUES;
extern charStr SIB_TYPE_WQLNODETYPES;
extern charStr SIB_TYPE_WQLRELATED;
extern charStr SIB_TYPE_WQLISTYPE;
extern charStr SIB_TYPE_WQLISSUBTYPE;

extern charStr SIB_TRUE;
extern charStr SIB_FALSE;

extern charStr SIB_INSERTGRAPH;
extern charStr SIB_REMOVEGRAPH;
extern charStr SIB_QUERYSTRING;
extern charStr SIB_CREDENTIALS;

extern charStr SIB_QUERYID;
extern charStr SIB_SUBSCRIPTIONID;
extern charStr SIB_ENCODING;

extern charStr SIB_TRIPLES;
extern charStr SIB_RESULTS;
extern charStr SIB_RESULTS_ADDED;
extern charStr SIB_RESULTS_REMOVED;
extern charStr SIB_BNODES;

extern charStr SIB_STATUS;
extern charStr SIB_STATUSOK;
extern charStr SIB_STATUS_NOTIF_RESET;
extern charStr SIB_STATUS_NOTIF_CLOSING;
extern charStr SIB_STATUS_SIB_ERROR;
extern charStr SIB_STATUS_SIB_FAIL_ACCESSDENIED;
extern charStr SIB_STATUS_SIB_FAIL_NOTIMPLEMENTED;
extern charStr SIB_STATUS_KP_ERROR;
extern charStr SIB_STATUS_KP_ERROR_REQUEST;
extern charStr SIB_STATUS_KP_ERROR_MSG_INCOMPLETE;
extern charStr SIB_STATUS_KP_ERROR_MSG_SYNTAX;
extern charStr SIB_STATUS_SIB_PROTECTION_FAULT;  /* DAN - ARCES */
#endif
