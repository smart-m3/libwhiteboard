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
#ifndef SIBDEFS_H
#define SIBDEFS_H
#include <glib.h>

/*
  these should be in the Node application header - ie. stuff
  common to the lib and lib user
*/

typedef enum {
  EncodingInvalid = 0,
  EncodingM3XML   = 1,
  EncodingRDFXML  = 2,
} EncodingType;

typedef enum {
  QueryTypeInvalid      = 0,
  QueryTypeTemplate     = 1,
  QueryTypeWQLValues    = 2,
  QueryTypeWQLNodeTypes = 3,
  QueryTypeWQLRelated   = 4,
  QueryTypeWQLIsType    = 5,
  QueryTypeWQLIsSubType = 6,
  QueryTypeSPARQLSelect = 7,
} QueryType;


typedef struct {
  const char * txt;
  int len;
}charStr;

typedef struct {
  charStr *name;
  charStr *defined;
} attrStr;

typedef struct {
  gchar *buf;
  unsigned int datLen;
  unsigned int bufLen;
} ssBufDesc_t;

typedef enum {
  ss_InternalError = -1,
    ss_StatusOK = 0,
  ss_GeneralError, /*probably local problem */
  ss_OperationFailed, /*may include SIB interaction problem*/
  ss_InvalidTripleSpecification,
  ss_SubscriptionNotInvolved, //ARCES 
  ss_ParsingError,
  ss_ParsingInProgress, /* message not fully parsed, i.e. buffer */
			/* did not contain full message */
  ss_InvalidParameter,
  ss_NotEnoughResources,
  /* STATUS CODES USED BY SIB */
  ss_NotifReset,
  ss_NotifClosing,
  ss_SIBError,
  ss_SIBFailAccessDenied,
  ss_SIBFailNotImpl,
  ss_KPError,
  ss_KPErrorRequest,
  ss_KPErrorMsgInComplete,
  ss_KPErrorMsgSyntax,
  ss_IndicationSequenceError,
  ss_SIBProtectionFault /*AD-ARCES*/
} ssStatus_t;

typedef enum {ssElement_TYPE_URI=0, ssElement_TYPE_LIT,
	      ssElement_TYPE_BNODE, ssElement_TYPE_eot} ssElementType_t;

typedef unsigned char * ssElement_t;
typedef const unsigned char * ssElement_ct; //read only

typedef struct {
  ssElement_t     subject,
                  predicate,
                  object;
  ssElementType_t subjType, /* never: ssElement_TYPE_LIT*/
  /* predicate type is always uri */
                  objType;
} ssTriple_t;

typedef struct {
  ssElement_t     string;
  ssElementType_t nodeType; /*never: ssElement_TYPE_BNODE */
} ssPathNode_t;

typedef struct { // for SPARQL WHERE and OPTIONAL lists of triples with variables
  ssPathNode_t subject;
  ssPathNode_t predicate;
  ssPathNode_t object;
} sparqlTriple_t;

typedef struct _insertTripleData
{
  ssBufDesc_t *bd;
} insertTripleData_t;

extern charStr SIB_MATCH_ANY;
#define ssMATCH_ANY ((ssElement_ct)SIB_MATCH_ANY.txt)

#define sib_(token,txt,size) charStr token ={txt,size}


typedef struct {
  ssPathNode_t *startNode;
  gchar *pathExpr;
} QueryTypeWQLValues_Parsed_t;

typedef struct {
  ssPathNode_t *node;
} QueryTypeWQLNodeTypes_Parsed_t;

typedef struct {
  ssPathNode_t *startNode;
  gchar *pathExpr;
  ssPathNode_t *endNode;
} QueryTypeWQLRelated_Parsed_t;

typedef struct {
  ssPathNode_t *node;
  ssPathNode_t *typeNode;
} QueryTypeWQLIsType_Parsed_t;

typedef struct {
  ssPathNode_t *subTypeNode;
  ssPathNode_t *superTypeNode;
} QueryTypeWQLIsSubType_Parsed_t;

typedef struct {
  QueryType qType;
  union {
    QueryTypeWQLValues_Parsed_t values;
    QueryTypeWQLNodeTypes_Parsed_t nodeTypes;
    QueryTypeWQLRelated_Parsed_t related;
    QueryTypeWQLIsType_Parsed_t isType;
    QueryTypeWQLIsSubType_Parsed_t isSubType;
  } wqlType;
} ssWqlDesc_t;

#endif
