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
#include "ssap_sib_tokens.h"

sib_(SIB_MESSAGE,      "SSAP_message",     12);
sib_(SIB_SPACEID,      "space_id",          8);
sib_(SIB_MSGTYPE,      "message_type",     12);
sib_(SIB_ICV,          "icv",               3);

sib_(SIB_JOIN,         "JOIN",              4);
sib_(SIB_LEAVE,        "LEAVE",             5);
sib_(SIB_INSERT,       "INSERT",            6);
sib_(SIB_REMOVE,       "REMOVE",            6);
sib_(SIB_QUERY,        "QUERY",             5);
sib_(SIB_SUBSCRIBE,    "SUBSCRIBE",         9);
sib_(SIB_UPDATE,       "UPDATE",            6);

sib_(SIB_SUBSCRIPTIONID,"subscription_id", 15);
sib_(SIB_UNSUBSCRIBE,  "UNSUBSCRIBE",      11);

sib_(SIB_PARAMETER,    "parameter",         9);
sib_(SIB_ENCODING,      "encoding",        8);
sib_(SIB_INSERTGRAPH,   "insert_graph",    12);
sib_(SIB_REMOVEGRAPH,   "remove_graph",    12);
sib_(SIB_CREDENTIALS,   "credentials",     11);

sib_(SIB_QUERYID,      "query_id",          8);

sib_(SIB_RESULTS,      "results",           7);
sib_(SIB_RESULTS_ADDED,  "new_results",     11);
sib_(SIB_RESULTS_REMOVED,"obsolete_results",16);
sib_(SIB_BNODES,       "bnodes",            6);

sib_(SIB_TRIPLES,      "triples",           7);

sib_(SIB_QUERYSTRING,  "query",             5);
sib_(SIB_TYPE_RDFXML,  "RDF-XML",           7);
sib_(SIB_TYPE_M3XML,   "RDF-M3",            6);
sib_(SIB_TYPE_SPARQL,  "sparql",            6);
sib_(SIB_TYPE_WQLVALUES,   "WQL-VALUES",    10);
sib_(SIB_TYPE_WQLNODETYPES,"WQL-NODETYPES", 13);
sib_(SIB_TYPE_WQLRELATED,  "WQL-RELATED",   11);
sib_(SIB_TYPE_WQLISTYPE,   "WQL-ISTYPE",    10);
sib_(SIB_TYPE_WQLISSUBTYPE,"WQL-ISSUBTYPE", 13);

sib_(SIB_STATUS,       "status",            6);
sib_(SIB_STATUSOK,                        "m3:Success",                    10);
sib_(SIB_STATUS_KP_ERROR,                 "m3:KP.Error",                   11);
sib_(SIB_STATUS_NOTIF_CLOSING,            "m3:SIB.Notification.Closing",   27);
sib_(SIB_STATUS_SIB_FAIL_ACCESSDENIED,    "m3:SIB.Failure.AccessDenied",   27);
sib_(SIB_STATUS_KP_ERROR_REQUEST,         "m3:KP.Error.Request",           19);
sib_(SIB_STATUS_KP_ERROR_MSG_SYNTAX,      "m3:KP.Error.Message.Syntax",    26);
sib_(SIB_STATUS_KP_ERROR_MSG_INCOMPLETE,  "m3:KP.Error.Message.Incomplete",30);
sib_(SIB_STATUS_SIB_ERROR,                "m3:SIB.Error",                  12);
sib_(SIB_STATUS_SIB_FAIL_NOTIMPLEMENTED,  "m3:SIB.Failure.NotImplemented", 29);
sib_(SIB_STATUS_NOTIF_RESET,              "m3:SIB.Notification.Reset",     25);

/* DAN - ARCES */
sib_(SIB_STATUS_SIB_PROTECTION_FAULT,     "m3:SIB.Failure.ProtectionFault", 30);


sib_(SIB_REQUEST,      "REQUEST",           7);
sib_(SIB_CONFIRM,      "CONFIRM",           7);
sib_(SIB_PARAMCONFIRM, "confirm"  ,         7);
sib_(SIB_INDICATION,   "INDICATION",       10);
sib_(SIB_MSGNAME,      "transaction_type", 16);
sib_(SIB_MSGNUMBER,    "transaction_id",   14);
sib_(SIB_NODEID,       "node_id",           7);
sib_(SIB_INDSEQNUM,    "ind_sequence",     12);

sib_(SIB_TAG,          "tag",               3);
sib_(SIB_URILIST,      "urilist",           7);

