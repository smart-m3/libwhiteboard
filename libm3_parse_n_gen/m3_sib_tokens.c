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

sib_(NS_SIB_PREFIX,    "sib",               3);
sib_(NS_SIB,           "sib:www.nokia.com/NRC/M3/SIB#",        29);

sib_(SIB_NAME,         "name",              4);
sib_(SIB_TYPE,         "type",              4);

sib_(SIB_TRIPLELIST,   "triple_list",      11);
sib_(SIB_NODE_LIST,    "node_list",         9);

sib_(SIB_SPARQLQUERY,  "sparql_query",      12);
sib_(SIB_WQLQUERY,         "wql_query",      9);
sib_(SIB_PATH_NODE,        "node",           4);
sib_(SIB_PATH_NODE_START,  "start",          5);
sib_(SIB_PATH_NODE_END,    "end",            3);
sib_(SIB_PATH_NODE_SUPERTYPE,  "supertype",  9);
sib_(SIB_PATH_NODE_SUBTYPE,"subtype",        7);
sib_(SIB_PATH_EXPRESSION,"path_expression",15);

sib_(SIB_TRIPLE,       "triple",            6);
sib_(SIB_SUBJECT,      "subject",           7);
sib_(SIB_PREDICATE,    "predicate",         9);
sib_(SIB_OBJECT,       "object",            6);
sib_(SIB_URI,          "uri",               3);
sib_(SIB_URICAPS,      "URI",               3);
sib_(SIB_LITERAL,      "literal",           7);
sib_(SIB_BNODE,        "bnode",             5);
sib_(SIB_MATCH_ANY,    "sib:any",           7);

sib_(SIB_TRUE,         "TRUE"    ,          4);
sib_(SIB_FALSE,        "FALSE"   ,          5);

//below: also in ssap_sib_tokens.c  - reorganization 'n role definitions clean-up needed
sib_(SIB_TAG,          "tag",               3);
sib_(SIB_URILIST,      "urilist",           7);
sib_(SIB_CDATA_START,  "<![CDATA[",         9);
sib_(SIB_CDATA_END,    "]]>",               3);
