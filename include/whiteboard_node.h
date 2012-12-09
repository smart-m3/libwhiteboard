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
/**
 * @file whiteboard_node.h
 * @brief  The WhiteBoardNode object implements NODE access to functionality of the
 * SmartSpace infrastucture. 
 *
 * When writing a SmartSpace Node application the developer must include the whiteboard_node.h
 * header file.
 *
 * @section initialize_sec Initializing WhiteBoardNode instance
 *
 * You create your Node and initialize its connection with the WhiteBoard by calling whiteboard_node_new() and casting its type with the WHITEBOARD_NODE macro:
 *
 * @code
 * WhiteBoardNode* node = NULL;
 * GMainContext* context = NULL;
 
 * context = g_main_context_default();
 * node = WHITEBOARD_NODE(whiteboard_node_new(context));	
 * ...
 * @endcode	
 *
 * The call to whiteboard_node_new() will block until a connection between it and the Whiteboard daemon "whiteboardd" is successfully established. The most common reason for not being able to establish a connection is not having the system DBus daemon running. In scratchbox environment running "af-sb-init.sh start" will take care of this. The whiteboard daemon is automatically started when needed.
 *
 * For the context, you can also use your own GMainContext if you wish, but usually that is not necessary, or wise - unless you absolutely know you are doing what you think you are doing, use the default.
 *
 * The WhiteBoardNode is able to receive and send messages thru the DBus only when a GMainLoop is running. If you are using GTK+, you can call gtk_main() as you would with any GTK+ application and you will have a working GMainLoop that way. GTK+ uses the default GMainContext and you should leave it that way. If, on the other hand, if you are using some other context that doesn't have its own GMainLoop, you should create one and start it manually. Note that you should use the same GMainContext for the GMainLoop and WhiteBoardNode, or at least make sure that the GMainContext of both are running in some GMainLoop.
 *
 * @code
 * GMainLoop* loop = NULL;
 * WhiteBoardNode* node = NULL;
 * GMainContext* context = NULL;
 *
 * context = g_main_context_default();
 * loop = g_main_loop_new(context, FALSE);
 *  ...
 * node = WHITEBOARD_NODE(whiteboard_node_new(context));
 * ...
 * g_main_loop_run(loop);
 * ...
 * @endcode
 *
 * @section discover_sec Discovering SmartSpaces

 *
 * A Node must know or discorver the URI of a Smart Space in order to join it. Discovery of existing SmartSpaces is done through the
 * WhiteBoardDiscovery class. This provides access to the WhiteBoard daemon's Discovery interface. The function whiteboard_discovery_get_sibs() will send the WHITEBOARD_DBUS_DISCOVERY_METHOD_GET_SIBS DBUS message to the Whiteboard daemon. After receiving this method call message, the Whiteboard daemon will send the WHITEBOARD_DBUS_DISCOVERY_SIGNAL_SIB signal with appropriate identifying information for each Smart Space the Whiteboard daemon is aware of. With each such message received by the WhiteBoardDiscovery instance, it emits a GLib signal, WHITEBOARD_DISCOVERY_SIGNAL_SIB. The Node application need not be concerned with the underlying DBUS message handling, but gets the data conveyed with the signals by connecting the signal with an appropriate callback.
 *

 * When a new Smart Space is registered with the Whiteboard daemon, the daemon sends the WHITEBOARD_NODE_SIGNAL_SIB_INSERTED signal to all registered WhiteBoard nodes. Likewise, when connection to a Smart Space is lost, the daemon sends the WHITEBOARD_NODE_SIGNAL_SIB_REMOVED signal to all register WhiteBoard nodes. In this way, the Node can follow the potentially changing list of available Smart Spaces.
 *
 * Note that the callback model is equivalent for all three signals.
 *
 *
 * @code
 * void sib_insert_callback (WhiteBoardDiscovery *context,
 *                           gchar *uuid,
 *                           gchar *name,
 *                           gpointer userdata)
 * {
 *   ... add to list of known Smart Spaces ...
 * }
 *
 * void sib_remove_callback (WhiteBoardDiscovery *context,
 *                           gchar *uuid,
 *                           gchar *name,
 *                           gpointer userdata)
 * {
 *   ... remove from list of known Smart Spaces ...
 * }
 * void sib_all_for_now_callback (WhiteBoardDiscovery *context,
 *                                gchar *uuid,
 *                                gchar *name,
 *                                gpointer userdata)
 * {
 *   ... All sib signals received.
 * }
 * 
 * void discover(WhiteBoardDiscovery *object)
 * {
 *    //listen for signalling of new Smart Spaces
 *    g_signal_connect(object,
 *                     WHITEBOARD_DISCOVERY_SIGNAL_SIB_INSERTED, 
 *                     (GCallback) sib_insert_callback,
 *                     NULL);
 *
 *    //listen for signalling of removed Smart Spaces
 *    g_signal_connect(object,
 *                     WHITEBOARD_DISCOVERY_SIGNAL_SIB_REMOVED,
 *                     (GCallback) sib_remove_callback,
 *                     NULL);
 *
 *    //listen for signalling of known Smart Spaces
 *    g_signal_connect(object,
 *                     WHITEBOARD_DISCOVERY_SIGNAL_SIB,
 *                     (GCallback) sib_insert_callback,
 *                     NULL);
 *
 *    //listen for signalling of known Smart Spaces
 *    g_signal_connect(object,
 *                     WHITEBOARD_DISCOVERY_SIGNAL_ALL_FOR_NOW,
 *                     (GCallback) sib_all_for_now_callback,
 *                     NULL);
 *
 *    whiteboard_discovery_get_sibs(object); //invoke signalling of known Smart Spaces
 * }
 * @endcode			 
 * 
 * @section join_sec Joining and leaving a SmartSpace
 *
 * After initialization of the WhiteBoardNode GObject and the URI of the desired SmartSpace is known (usually via discovery), the node is ready to join the SmartSpace. Within the WhiteBoard node infrastructure, the join procedure is asynchronous. The join operation is started by sending the WHITEBOARD_DBUS_NODE_METHOD_JOIN DBUS message to the Whiteboard daemon. For the Node application, this is done by the whiteboard_node_sib_access_join() function. After receiving the join result from the SmartSpace the Whiteboard daemon sends the result back toward the Node with the WHITEBOARD_DBUS_NODE_SIGNAL_JOIN_COMPLETE DBUS signal. When the WhiteBoardNode instance receives this, it emits the WHITEBOARD_NODE_SIGNAL_JOIN_COMPLETE GLib signal. The join opertation is then completed via a callback function. Before invoking whiteboard_node_sib_access_join(), the Node application uses the g_signal_connect() function to establishes connection to the signal and provide the proper callback function. The status parameter of the callback function indicates the join result.
 *
 * @code
 * void join_complete_callback(WhiteBoardNode *context,
 *                             ssStatus_t success,
 *                             gpointer userdata)
 * {
 *     if(success == ss_StatusOK)
 *     {
 *        ... sucessful join ...
 *     }
 *     else
 *     {
 *         ... handle error ...
 *     }
 * }
 * 
 * void join(WhiteBoardNode *node, gchar *uri)
 * {
 *
 *     g_signal_connect(node,
 *                      WHITEBOARD_NODE_SIGNAL_JOIN_COMPLETE,
 *                      (GCallback) join_complete_callback,
 *                      NULL); 
 *
 *     if( whiteboard_node_sib_access_join(node, uri, NULL, NULL,0) != ss_StatusOK)
 *     {
 *         ... handle error ...
 *     }
 * }
 * @endcode 
 * 
 * After the join is complete, the node instance is a valid reference parameter for subsequent SmartSpace operations: insert, remove, update, query, and subscribe, applicable to the joined SmartSpace, until used with the leave operation.  An instantiated WhiteBoardNode is thus associated at most with one SmartSpace at a time. For an application to access another SmartSpace at the same time, another WhiteBoardNode must be instantiated.
 *
 * A WhiteBoardNode GObject may be properly destroyed without leaving the Smart Space. Subscriptions are not passable from one Node instance to another and will be canceled when the WhiteBoardNode destructor is invoked.
 *
 * Leaving the joined SmartSpace is done by calling function whiteboard_node_sib_access_leave(), which sends the WHITEBOARD_DBUS_NODE_SIGNAL_LEAVE DBUS message to the Whiteboard daemon. There is no leave confirmation Glib signal emitted from the WhiteBoardNode instance to the user application, however the functions returns the status of sending the DBUS message.
 * 
 * @section insert_remove_sec Inserting, removing and updating triple information
 *
 * Information in the SmartSpace is represented as one graph of (set of zero or more) triples, described below. Insert, remove and update operations specify a graph of triples which usually is meant to change the exiting SmartSpace graph, but does so only if the change adds new information or removes existing information, or both. For these operations, this API supports two ways of specifying triple information to the SmartSpace, as list of ssTriple_t structures or serially encoded in a text string. There are serveral such text string encodings, so the string is accompanied by an EncodingType parameter. Currently, only EncodingRDFXML is supported.
 *
 * Correct syntax of an RDFXML triple\A0graph is not covered further here. Prior to inserting, deleting or updating triples using the triple list method, the Node application must create each triple relevant to the atomic operation and add it to a list. Each triple consists of a subject, predicate, and object with an associated subject and object type, all contained in an ssTriple_t structure. Whether one or more triples is involved, each is added to a GLib GSList structure and the Node application provides a pointer to the list as a parameter to the WhiteBoard library function. The library also provides utility functions, ssPrependTriple() to create one triple and prepend it to a triple list, and ssFreeTripleList() to delete and free triples from a triple list. Code examples include freeing triples, althougth where and when a triple is to be freed will depend on application design.
 *
 * Inserting triples into a SmartSpace is done with functions whiteboard_node_sib_access_insert_M3Triples() or whiteboard_node_sib_access_insert_graph(). The examples of both below insert the same information in the Smart Space. The generated GSList and corresponding string with EncodingRDFXML are representitive of parameters passed in the other examples below for removing, updating. Triple specification for queries (including subscriptions) are discussed separately. Note that blank nodes are used, which means that the SIB will give them a URI unique to the Smart Space. Note also that in this case, the RDFXML encoding can be done without the use of node IDs (rdf:nodeID). The triple list method must use a node ID to specify the same blank node used in more than one tripple, since each triple is specified separately.

The graph looks like this, where [] is a blank node, unless the elment type URI is enclosed, and LITERAL in enclosed in ():
 * @code
 *
 * //                 [someNs:#Mary]--[someNs:#lives]-->[]--[someNS:#locatorName]-->[someNs:#London]
 * //                        |
 * //       [someDefs:relationship/family#motherOf]
 * //                        |
 * //                        V
 * //                 [someNs:#John]--[someNs:#lives]-->[]--[someNS:#locatorName]-->[someNs:#NewYork]
 * //                        |                           |
 * //                        |                            --[someNs:#aka]-->(The Big Apple)
 * //                         --[someNs:#aka]-->(Die Hard)
 * 
 * void insert_byM3Triples(WhiteBoardNode *node)
 * {
 *   const gchar *nameSpace =                      "\
 *   xmlns:family=\"someDefs:relationship/family#\" \
 *   xmlns=\"someNs:#\""; //don't forget to escape the quotes (") in the namespace string
 *
 *   const gchar *lives = ":lives";
 *   const gchar *aka = ":aka";
 *   const gchar *motherOf = "family:motherOf";
 *   const gchar *locator = ":locatorName";
 *
 *   const gchar *john = ":John";
 *   const gchar *nyc = ":NewYork"; //space not allowed in uri syntax
 *   const gchar *johnAka = "Die Hard"; //space allowed in literal syntax
 * 
 *   const gchar *mary = ":Mary";
 *   const gchar *london = ":London";
 *   
 *   int badTriple = 0; //ss_StatusOK is fixed at 0
 *   badTriple |= GSList *triples = NULL;
 *   badTriple |= ssPrependTriple( &triples, john,   lives,    "bn01",          ssElement_TYPE_URI,   ssElement_TYPE_BNODE);
 *   badTriple |= ssPrependTriple( &triples, "bn01", locator,  nyc,             ssElement_TYPE_BNODE, ssElement_TYPE_URI);
 *   badTriple |= ssPrependTriple( &triples, "bn01", aka,      "The Big Apple", ssElement_TYPE_BNODE, ssElement_TYPE_LIT);
 *   badTriple |= ssPrependTriple( &triples, john,   aka,      johnAka,         ssElement_TYPE_URI,   ssElement_TYPE_LIT);
 *   badTriple |= ssPrependTriple( &triples, mary,   lives,    "bn02",          ssElement_TYPE_URI,   ssElement_TYPE_BNODE);
 *   badTriple |= ssPrependTriple( &triples, "bn02", locator,  london,          ssElement_TYPE_BNODE, ssElement_TYPE_URI);
 *   badTriple |= ssPrependTriple( &triples, mary,   motherOf, john,            0,                    0); //ssElement_TYPE_URI, the most common, is "conveniently" fixed 0
 * 
 *   if(badTriple ||
 *      whiteboard_node_sib_access_insert_M3Triples( node, triples, nameSpace) != ss_StatusOK)
 *   {
 *     ... handle error
 *     // discard internal triples 
 *     ssFreeTripleList(&triples);
 *   }
 *   else
 *   {
 *     ... save triple list pointer, triples, for later handling and release of internal triples.
 *   } 
 * }
 * @endcode
 *
 * @code
 * void insert_byRDFXML(WhiteBoardNode *node)
 * {
 *   const gchar *rdfXml =                                       "\
 * <rdf:RDF xmlns=\"someNs:#\"                                    \
 *     xmlns:family=\"someDefs:relationship/family#\"             \
 *     xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"> \
 *                                                                \
 *     <rdf:Description rdf:about="someNs:#John">                 \
 *         <aka>Die Hard</aka>                                    \
 *         <lives rdf:parseType="Resource">                       \
 *             <aka>The Big Apple</aka>                           \
 *             <locatorName rdf:resource="someNs:#NewYork"/>      \
 *         </lives>                                               \
 *     </rdf:Description>                                         \
 *                                                                \
 *     <rdf:Description rdf:about="someNs:#Mary">                 \
 *         <family:motherOf rdf:resource="someNs:#John"/>         \
 *         <lives rdf:parseType="Resource">                       \
 *             <locatorName rdf:resource="someNs:#London"/>       \
 *         </lives>                                               \
 *     </rdf:Description>                                         \
 *                                                                \
 * </rdf:RDF>                                                    ";
 *
 *   if(whiteboard_node_sib_access_insert_graph(node, rdfXml, EncodingRDFXML) != ss_StatusOK)
 *   {
 *     ... handle error
 *   }
 *   else
 *   {
 *     ... AOK, triple information has been asserted
 *   } 
 * }
 * @endcode
 *
 * Removing triples triples from a SmartSpace is done with functions whiteboard_node_sib_access_remove_M3Triples() (whiteboard_node_sib_access_remove_graph() is not yet supported). The information to be removed from the Smart Space is specified elsewhere and passed to the example function below. Triples to be deleted are "template" triples, which may include wildcards - template triples are described below with queries.
 *
 * @code
 * void remove_M3Triples_and_free(WhiteBoardNode *node, GSList *triples, gchar *namespace)
 * {
 * 
 *   if( whiteboard_node_sib_access_remove_M3Triples( node, triples, namespace) != ss_StatusOK)
 *   {
 *     ... handle error
 *   }
 *   // in all cases, free internal triples
 *   ssFreeTripleList(&triples);
 * }
 * @endcode
 *
 * @section query_sec Query and subscribe
 *

 * Finding information stored in the SmartSpace is pursued by querying. A query is an operation where a search requested by the Node is performed once and its results are reported back to the node immediately. The search criteria can be either template based, including possible wildcards, or constructed by using more sophisticated query languages, like WQL (Wilbur Query Language) or SPARQL. The current version of the WhiteBoard library supports template based queries and WQL queries.

 * Template based queries are done by constructing a list of one or more triples, as described above for the insert, remove and update operations. Each template triple inquires whether any matching triples exists in the Smart Space Whiteboard. Thus, the result of a single triple template query is a set of zero or more matching triples. If the query list has more than one triple template, the result is the union of the result set for each template. 
 *
 * Template triples are similar to  triples, except that they may not contain blank nodes and they may, and most often do, contain wildcards. A wildcard a special uri (defined by ssMATCH_ANY), not valid for Whiteboard triple content, in any of the subject, predicate or object elements, which matches any value (URI or literal) of the same element of a triple in the SmartSpace. For example, if you already know there is a [:John] and a [:Mary], you can find the values of their predicate aliases [:aka], if any, by constructing a triple template query, using the default namespace "someNs:#", as follows:
 * @code
 *  GSList *triples = NULL;
 *  ssPrependTriple( &triples, ":John", ":aka" ssMATCH_ANY, ssElement_TYPE_URI, 0);
 *  ssPrependTriple( &triples, ":Mary", ":aka" ssMATCH_ANY, ssElement_TYPE_URI, 0);
 *  //when using ssMATCH_ANY for subject and/or object elements, the corresding type parameter is ignored.
 * @endcode
 *
 * Template triple queries are weak because each triple specifies a separate match, with no relationship between multiple templates. Applied against the example insert above, the results will be a list with one triple, namely: {uri:"someNs:#John", "someNs:#aka", lit:"Die Hard"}. Mary (i.e. lit:"someNs:#Mary") doesn't have an alias. A query with one template triple, {ssMATCH_ANY, "someNs:#aka", ssMATCH_ANY}, for the example insert, besides telling us John's alias and that Mary doesn't have one, would have returned also a second triple in the list, namely: {uri:*, "someNS:#aka", lit:"The Big Apple"], where * is a uri assigned to the blank node by the smart space.
 *
 * Subscriptions are persistent queries which are performed initially, when submitted, and every time the contents of the Whiteboard of the SmartSpace changes, until the subscription is cancelled. Results are returned initially, and thereafter only when triggered searches have different results. Results are reported to the Whiteboard library infrastucture in subscription indication messages, causing a user provided callback function to be called to handle the query results. For this callback, the application Node does not connect to any GLib signal.
 *
 * Subscriptions must be canceled with the unsubscribe function when the Node no longer wants to receive results for a given subscription. As a response to sucessful cancellation of a subscription or a subscription with expiration time, the WhiteBoard library sends the WHITEBOARD_NODE_UNSUBSCRIBE_COMPLETE GLib signal to the Node application when the subscription is expired.
 *
 * Template based queries and subscriptions are initiated using functions whiteboard_node_sib_access_query_template() and whiteboard_node_sib_access_subscribe_template(), respectively. In addition to the search criteria, these functions take pointers to a callback function and optional user data as parameters. The callback function is called by the WhiteBoard library when the results of the query or subsciption are received and forwarded by the Whiteboard daemon. The response to a template based query/subscription is a list of triples. The WhiteBoard library allocates memory for the triple stuctures and provides a pointer to a GSList list structure for the node application to access contents of the triples. However, the node application is responsible for freeing the memory allocated for the triples, GSList structures and the GSList pointer.
 *
 * Using query languages such as Wilbur (WQL), one can create more sophisticated search criteria for queries than using template based search criteria. WQL queries and subscriptions are initiated with the functions whiteboard_node_sib_access_query_wql<type>() and whiteboard_node_sib_access_subscribe_wql_<type>(), respectively, where _<type> is one of the five Wilbur query types. There are two response types, list and boolean, which depends on the type of Wilbur query or subscriptionspecified.
 *
 * The five Wilbur query types, with query parameters and corresponding response type, with response parameters, are:
 * _values(node,path) returns a list of reachable nodes.
 * _nodeClasses(node) returns a list of classes.
 * _related(startNode,path,endNode) returns the boolean value of: endNode is reachable from startNode via path.
 * _ofClass(node,class) returns the boolean value of: node is of class
 * _isSubclass(class, superclass) returns the boolean value of: class is subclass of superclass
 *
 * only _values and _related have subcription forms.
 *
 * In the above, a node and class are graph nodes, represented by the ssPathNode_t structure. The nodeType of node is either uri  (ssElement_TYPE_URI) or literal (ssElement_TYPE_LIT). The nodeType of class should always be uri. A path is a Wilbur Path Language string - not further documented here. Currently, the use of namespace prefixes in the path and node/class strings is not supported.
 *
 * The list response callback function for a _values or _nodeClasses WQL query/subscription looks similiar to the callback function declaration for template based queries/subscriptions, the result lists provided are different in content. In the case of WQL list responses, the GSList **results parameter of the callback provides access to a list of pointers to ssPathNode_t structures, while in the case of template based query/subscription, the list contains pointers to ssTriple_t structures. In both cases, the user must free memory relavent to the GSList ** parameter, and utility functions are provided to help - see examples. Note, 
 *
 * @code
 * void find_all_newyorkers_query_cb( ssStatus_t status, GSList **results, gpointer userdata)
 * {
 *   if(status == ss_StatusOK)
 *   {
 *     GSList *resultTriples = *results;
 *     int queryPartResults = (int)userdata;
 *     switch (queryPartResults) {
 *     case 1:
 *       {// construct second query to find who :lives at such places with predicate :locatorName assigned :NewYork
 *         ssTriple_t *resultTriple;
 *         GSList *queryTriples = NULL;
 *
 *         while (resultTriples && (resultTriple = resultTriples->data)) {
 *           ssPrependTriple( &queryTriples, ssMATCH_ANY, ":lives", resultTriple->subject, 0, resultTriple->subjType);
 *           resultTriples = resultTriples->next;
 *         }
 *         if (queryTriples) {
 *           if (whiteboard_node_sib_access_query_template(node, queryTriples, "xmlns=\"someNs:#\"", &find_all_newyorkers_query_cb, 2) != ss_StatusOK)
 *           {
 *             ... handle error ...
 *           }
 *           ssFreeTripleList(&queryTriples); // Free template triples
 *         }
 *       } break;

 *     case 2:
 *       { //handle resultTriples for "?who :lives there", where there was determined by results in part 1
 *       } break;
 *     }
 *   }
 *   ssFreeTripleList(results); // Free result triples - address of original pointer, not of modified local pointer, resultTriples
 *   g_free(results); // Free results pointer also
 * }
 *
 * void find_all_newyorkers(WhiteBoardNode *node)
 * {
 *   GSList *triples = NULL;
 *   // construct first query to find blank nodes with predicate :locatorName assigned :NewYork
 *   ssPrependTriple( &triples, ssMATCH_ANY, ":locatorName", ":NewYork", 0, ssElement_TYPE_URI);
 *   if( whiteboard_node_sib_access_query_template(node, triples, "xmlns=\"someNs:#\"", &find_all_newyorkers_query_cb, 1) != ss_StatusOK)
 *   {
 *    ... handle error ...
 *   }
 *   ssFreeTripleList(&triples); // Free template triples
 * }
 * @endcode
 *
 * Copyright 2009 Nokia Corporation
 */

/** @mainpage WhiteBoard library for Linux platform
 *
 * @section intro_sec Introduction
 *
 * The Whiteboard library is a utility library providing access to the Smart Space infrastructure. The utilities in the library provide an API for a Smart Space Node to discover available Smart Spaces, join and leave one at a time, and while joined to a given Smart Space to insert, remove, query and subscribe for triple information posted on its Whiteboard. The API thus provides a Node access point to the SmartSpace infrastructure, allowing applicaitons with Nodes joined to the same Smart Space to share information and coordinate activities. An application (program) may be designed to have more than one Node access point.
 *
 * Depending on the given Smart Space, the infrastructure may span multiple devices. The Whiteboard library provides the utilities to coordinate the functions of the infrastructure within a single device and interface any external infrastructure, providing device independent access to the Whiteboard for all Nodes joined to the same Smart Space.
 *
 *The device infrastructure consists of three components, namely node, sib-access and Whiteboard daemon. A SIB  is a access server handling SmartSpace operations for nodes. The Whiteboard daemon is a background process to which all node and sib-access processes are registered. The sib-access process provides access to different SIBs (Symantic Information Broker) of the SmartSpaces  that may be running locally and remotely. Currently only TCP/IP is supported as an access method for remote SIBs. Local SIBs are not implemented at the moment.
 *
 *If you are writing an application with Node access to the Smart Space infrastructure, all you need to read is section \ref node_sec. The remaining material here describes installing and configuring the Whiteboard library and the rest of the local device infrastructure.
 *
 * @section discovery_sec Discovery API
 *
 *   * The Node needs the URI of a Smart Space in order to join it. If this is not already know, the application can can register to be informed of known Smart Space URIs. A callback function of type WhiteBoardDiscoverySIBCB is registered for the Node association with the WHITEBOARD_DISCOVERY_SIGNAL_SIB signal using the g_signal_connect function. Additionally, the whiteboard_discovery_get_sibs function must then be called in order to start the signaling. With each signal, the callback function receives the friendly name and URI of a Smart Space. It is up to the user to recognize the appropriate Smart Space to join. When all known Smart Spaces have been conveyed in this way, WHITEBOARD_DISCOVERY_ALL_FOR_NOW signal is emitted and no further WHITEBOARD_DISCOVERY_SIGNAL_SIB signals will be emitted from the Discovery object.
 *   * The Node can follow the potentially changing list by also registering callbacks for its association with the WHITEBOARD_DISCOVERY_SIGNAL_SIB_INSERTED and WHITEBOARD_DISCOVERY_SIGNAL_SIB_REMOVED signals. These signals are emitted spontaneously to all registered discovery objects according to the event, and thus there is no function to be called to start them.
 *
 * See whiteboard_discovery.h
 *
 * @section node_sec Node API
 *
 * A Smart Space Node is implemented as a GObject, WhiteBoardNode, each instance of which implements an access interface to the Smart Space infrastructure via its data and methods. Most data is returned to the object through callback methods associated with signals emitted from the infrastructure\'s D-Bus component. The objects are scheduled and run from a GMainLoop context. The Whiteboard library header files documents the details of Smart Space Node access for the application developer. A brief outline for the startup of a Node is given below. For more detail, see whiteboard_node.h and the \"more\" link there, also GObject documentation [http://library.gnome.org/devel/gobject/stable/], if needed. 
 *
 *  * A Node is dependent on communication with the Whiteboard daemon (whiteboardd) which much be running in the local device.
 *
 *  * A Node object is created using the WHITEBOARD_NODE macro to cast the value returned by whiteboard_node_new whose parameters define the Node Id and object context. Normally, the default context is the right context to use. The Node Id is a string defined by the user and must be unique among other Nodes that have joined (are members of) the same Smart Space.
 *
 *   * A Node requests to join a Smart Space using whiteboard_node_sib_access_join after defining its call back association with the WHITEBOARD_NODE_SIGNAL_JOIN_COMPLETE signal. When the signal is emiteed, the callback function receives the status the request. Only with success (status==0) can further interaction with the Smart Space take place. 
 *
 *  See whiteboard_node.h  
 *
 * @section sibaccess_sec SIB access API
 *  See whiteboard_sib_access.h
 *
 * @section building_sec Building and installing the WhiteBoard library
 *
 * The build environment and configure script file are generated by running autogen.sh shell script in the lib directory. The makefiles are generated by running configure script. The configure takes various parameters. See ./configure --help for detailed information on different parameters. In the following example --with-debug parameter enables debug log messages and --prefix parameter defines base directory where the whiteboard library is installed. The SSAP message parser (sssib.c) uses expat library for parsing XML messages, so the expat must be installed properly prior building the Whiteboard library.
 *
 * > ./autogen.sh
 *
 * > ./configure --with-debug --prefix=/usr/local
 *
 * > make
 *
 * > make install
 *
 * @section compiling_sec Compiling application that uses WhiteBoard library
 * You can get proper compiler include directories and linker settings using pkg-config:
 *
 * > CFLAGS += `pkg-config --cflags libwhiteboard`
 *
 * > LIBS += `pkg-config --libs libwhiteboard`
 *
 * If pkg-config does not find package libwhiteboard make sure that you have included proper path to PKG_CONFIG_PATH environmental variable. The libwhiteboard pkg-config
 * files are installed to PREFIX/lib/pkgconfig directory, where PREFIX is path given to
 * configure script or default installation base directory (/usr/local/ in most systems).
 *
 * The current release of the WhiteBoard library has been tested with Ubuntu in standard laptops and Maemo SDK v3.1 and with corresponding softwave version in N800 devices.
 * 
 * @section seealso_sec See also
 *
 * <A HREF="../../../../common/trunk/doxygen/html/main.html">Whiteboard daemon documentation</A>
 *
 * <A HREF="../../../../sib-access/trunk/doxygen/html/main.html">SIB Access documentation</A>
 * 
 **/

#ifndef WHITEBOARD_NODE_H
#define WHITEBOARD_NODE_H

#include <glib.h>
#include <glib-object.h>
#include <whiteboard_util.h>
#include <whiteboard_command.h>
#include <sibdefs.h>
G_BEGIN_DECLS

/******************************************************************************
 * Type checks and casts
 ******************************************************************************/

#define WHITEBOARD_NODE_TYPE whiteboard_node_get_type()

#define WHITEBOARD_NODE(object) G_TYPE_CHECK_INSTANCE_CAST(object, \
						       WHITEBOARD_NODE_TYPE, \
						       WhiteBoardNode)

#define WHITEBOARD_NODE_CLASS(object) G_TYPE_CHECK_CLASS_CAST(object, \
							  WHITEBOARD_NODE_TYPE, \
							  WhiteBoardNodeClass)

#define IS_WHITEBOARD_NODE(object) G_TYPE_CHECK_INSTANCE_TYPE(object, \
							  WHITEBOARD_NODE_TYPE)

#define IS_WHITEBOARD_NODE_CLASS(object) G_TYPE_CHECK_CLASS_TYPE(object, \
							     WHITEBOARD_NODE_TYPE)

/******************************************************************************
 * Type definitions
 ******************************************************************************/

struct _WhiteBoardNode;
typedef struct _WhiteBoardNode WhiteBoardNode;

struct _WhiteBoardNodeClass;
typedef struct _WhiteBoardNodeClass WhiteBoardNodeClass;

/*****************************************************************************
 * Source callback prototypes
 *****************************************************************************/

/**
 * Type definition for callback that is called when cancellation of a subscription is complete.
 *
 * @param context The signaling WhiteBoardNode instance.
 * @param subscription_id The id of the subscription.
 * @param success Success of the unsibscription. If subscription successfully canceled the value if success is 0, otherwise != ss_StatusOK.
 */
typedef void (*WhiteBoardNodeUnsubscribeCompleteCB) (WhiteBoardNode *context,
						     gint subscription_id,
						     ssStatus_t status,
						     gpointer userdata);

/**
 * Use this identifier with g_signal_connect() to receive unsubscribe complete signals.
 */
#define WHITEBOARD_NODE_SIGNAL_UNSUBSCRIBE_COMPLETE "unsubscribe_complete"

/**
 * Type definition for callback that is called when joining a smartspace is complete.
 *
 * @param context the signaling WhiteBoardNode instance.
 * @param success Success code, 0 if join was successful, otherwise != ss_StatusOK.
 * 
 */
typedef void (*WhiteBoardNodeJoinCompleteCB) (WhiteBoardNode *context,
					      ssStatus_t status,
					      gpointer userdata);

/**
 * Use this identifier with g_signal_connect() to receive join complete signals.
 */
#define WHITEBOARD_NODE_SIGNAL_JOIN_COMPLETE "join_complete"

/**
 * Type definition for callback that is called when results from a template based subscription are received. Pointer to the callback is given to the library in whiteboard_node_sib_access_subscribe_template call.
 *
 * @param status 0 if no errors were found when parsing the indication.
 * @param results_added Pointer to pointer of list of results added since last indication. The memory for the pointer and possible list of ssTriple_t structures is allocated by the WhiteBoard library. MUST be freed by the node application (and any list content pointed to).
 * @param results_removed Pointer to pointer of list of results removed since last indication. The memory for the pointer and possible list of ssTriple_t structures is allocated by the WhiteBoard library. MUST be freed by the node application (and any list content pointed to). 
 * @param userdata pointer to userdata given in corresponding whiteboard_node_sib_access_subscribe call.
 */
typedef void (*WhiteBoardNodeSubscriptionIndTemplateCB) (ssStatus_t status,
							 GSList **results_added,
							 GSList **results_removed,
							 gpointer userdata);

/**
 * Type definition for callback that is called when results from a WQL-values subscription are received. Pointer to the callback is given to the library in a corresponding whiteboard_node_sib_access_subscribe_wql_values call.
 *
 * @param status 0 if no errors were found when parsing the indication.
 * @param nodelist_added Pointer to pointer of list of nodes added since last indication. The memory for the pointer and possible list of ssPathNode_t structures is allocated by the WhiteBoard library. MUST be freed by the node application (and any list content pointed to).
  * @param nodelist_removed Pointer to pointer of list of nodes removed since last indication. The memory of the pointer and possible list of ssPathNode_t structures is allocated by the WhiteBoard library. MUST be freed by the node application (and any list content pointed to).
 * @param userdata pointer to userdata given in corresponding whiteboard_node_sib_access_subscribe_wql_values call.
 */
typedef void (*WhiteBoardNodeSubscriptionIndWQLvaluesCB) (ssStatus_t status,
						          GSList **nodelist_added,
						          GSList **nodelist_removed,
						          gpointer userdata);

/**
 * Type definition for callback that is called when boolean results from a WQL subscription or query are received. Pointer to the callback is given to the library in a corresponding whiteboard_node_sib_access_subscribe_wql_related, whiteboard_node_sib_access_query_wql_related, whiteboard_node_sib_access_query_wql_isClass or whiteboard_node_sib_access_query_wql_isSubclass call.
 *
 * @param status 0 if no errors were found when parsing the indication.
 * @param result Boolean value indicating the result of a query or an update result since the last indication for a subscription.
 * @param userdata pointer to userdata given in corresponding query or subcription call.
 */
typedef void (*WhiteBoardNodeQSCommonWQLbooleanCB) (ssStatus_t status,
						    gboolean result,
						    gpointer userdata);

/**
 * Type definition for callback that is called when results from a template based query are received. Pointer to the callback is given to the library in a corresponding whiteboard_node_sib_access_query call.
 *
 * @param status 0 if no errors were found when parsing the indication.
 * @param results Pointer to pointer of list of results. The memory for the pointer and possible list of ssTriples_t structures is  allocated by the WhiteBoard library. MUST be freed by the node application (and any list content pointed to).
 * @param userdata pointer to userdata given in a corresponding whiteboard_node_sib_access_query call.
 */
typedef void (*WhiteBoardNodeQueryTemplateCB) (gint status,
					       GSList **result,
					       gpointer userdata);

/**
 * Type definition for callback that is called when results from a WQL-values or WQL-nodeClasses query are received. Pointer to the callback is given to the library in a corresponding whiteboard_node_sib_access_query_wql_values or whiteboard_node_sib_access_query_wql_nodeClasses call.
 *
 * @param status 0 if no errors were found when parsing the query response.
 * @param nodelist Pointer to pointer to list of results. The memory for the pointer and possible list of ssPathNode_t structures is allocated by the WhiteBoard library. MUST be freed by the node application (and any list content pointed to).
 * @param userdata pointer to userdata given in a corresponding whiteboard_node_sib_access_query_wql_values or whiteboard_node_sib_access_query_wql_nodeClasses call.
 */
typedef void (*WhiteBoardNodeQueryWQLnodelistCB) (ssStatus_t status,
					          GSList **nodelist,
					          gpointer userdata);

/**
 * Type definition for callback that is called when results from a SPARQL-select are received. Pointer to the callback is given to the library in a corresponding whiteboard_node_sib_access_query_sparql_select call.
 *
 * @param status 0 if no errors were found when receiving the query response.
 * @param selectedVariables Pointer to pointer to list of selected variables (variable is: char *). The memory for the pointer and possible list of strings (char *) is allocated by the WhiteBoard library. MUST be freed by the node application (and any list content pointed to).
 * @param valueRows Pointer to pointer to list of query solution rows (row is GSList of pointers to values (value is: ssPathNode_t). Each value is bound to the variable of corresponding position in the list of selected variables. The memory for the pointer and possible list of lists of ssPathNode_t structures is allocated by the WhiteBoard library. MUST be freed by the node application (and any list content pointed to).
 * @param userdata pointer to userdata given in a corresponding whiteboard_node_sib_access_query_wql_values or whiteboard_node_sib_access_query_wql_nodeClasses call.
 */
typedef void (*WhiteBoardNodeQuerySPARQLselectCB) (ssStatus_t status,
						   GSList **selectedVariables,
						   GSList **valueRows,
						   gpointer userdata);

/**
 * Type definition for callback that is called when results from a SPARQL-select based subscription are received. Pointer to the callback is given to the library in whiteboard_node_sib_access_subscribe_sparql_select call.
 *
 * Note: Sucessful subscription updates depend on the selected variables list returned in the initial callback (WhiteBoardNodeQuerySPARQLselectCB). The selected variables list should not be modified or freed until the subscription is cancelled with a call to whiteboard_node_sib_access_unsubscribe.
 *
 * @param status 0 if no errors were found when receiving the indication.
 * @param valRows_added Pointer to pointer to list of query solution rows added since last indication. The memory for the pointer and possible list of lists of ssPathNode_t structures is allocated by the WhiteBoard library. MUST be freed by the node application (and any list content pointed to).
 * @param valRows_removed Pointer to pointer to list of query solution rows removed since last indication. The memory for the pointer and possible list of lists of ssPathNode_t structures is allocated by the WhiteBoard library. MUST be freed by the node application (and any list content pointed to).
 * @param userdata pointer to userdata given in corresponding whiteboard_node_sib_access_subscribe call.
 */
typedef void (*WhiteBoardNodeSubscriptionIndSPARQLselectCB) (ssStatus_t status,
							     GSList **valRows_added,
							     GSList **valRows_removed,
							     gpointer userdata);


/*****************************************************************************
 * Custom command callback types
 *****************************************************************************/

/** 
 * Defines callback function for a custom command request 
 *
 * @param context The signaling WhiteBoardNode instance.
 * @param request A WhiteBoardCmdRequest instance that holds the command data.
 * @param user_data Optional userdata pointer.
 *
 * @warning NOT IMPLEMENTED
 */
typedef void (*WhiteBoardNodeCustomCommandRequestCB) (WhiteBoardNode *context,
						      WhiteBoardCmdRequest *request,
						      gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive custom command
 * request signals.
 */
#define WHITEBOARD_NODE_SIGNAL_CUSTOM_COMMAND_REQUEST "custom_command_request"

/**
 * Defines callback function for a custom command response
 *
 * @param context The signaling WhiteBoardNode instance
 * @param response A WhiteBoardCmdResponse instance that holds the command data
 * @param user_data Optional userdata pointer
 *
 * @warning NOT IMPLEMENTED
 */
typedef void (*WhiteBoardNodeCustomCommandResponseCB) (WhiteBoardNode *context,
						  WhiteBoardCmdResponse *response,
						  gpointer user_data);

/**
 * Use this identifier with g_signal_connect() to receive custom command
 * response signals
 */
#define WHITEBOARD_NODE_SIGNAL_CUSTOM_COMMAND_RESPONSE "custom_command_response"

/*****************************************************************************
 * Log messaging callback types
 *****************************************************************************/

/**
 * Type definition for callback that is called when new log message
 * arrives.
 *
 * Node developers should use only message id's and
 * when informing user about exceptions/infos.
 *
 * @param level Log message serverity
 * @param id Specific id given to this error message
 * @param message Actual log message (Not localized!)
 */
typedef void (*WhiteBoardLogMessageCB) (WhiteBoardNode *context,
					gint level,
					gint id,
					gchar *message,
					gpointer userdata);

/**
 * Use this identifier with g_signal_connect() to receive log message signals
 */
#define WHITEBOARD_NODE_SIGNAL_LOG_MESSAGE "log_message"

/*****************************************************************************
 * Initialization
 *****************************************************************************/

/**
 * Get the GLib type of WhiteBoardNode
 */
GType whiteboard_node_get_type();

/**
 * Create a new WhiteBoardNode instance. Note that unlike whiteboard_sib_new(),
 * this function does NOT create a new GMainLoop if the
 * mainloop parameter is omitted.
 *
 * @param maincontext A pre-allocated GMainContext (usually obtained with
 *                    g_main_context_default(), unless you know better)
 * @return Pointer to a new WhiteBoardNode instance
 */
GObject *whiteboard_node_new(GMainContext *maincontext);

/**
 * Get the WhiteBoardNode's mainloop, you can let whiteboard_node to create the
 * mainloop, then fetch it and run it externally.
 *
 * @param self The WhiteBoardNode, whose mainloop to get
 * @return Pointer to the mainloop.
 */
GMainContext *whiteboard_node_get_main_context(WhiteBoardNode *self);

/**
 * Get the UUID of an WhiteBoardNode instance
 *
 * @param self A WhiteBoardNode instance
 * @return A string containing the instance UUID (must NOT be freed)
 */
const gchar *whiteboard_node_get_uuid(WhiteBoardNode *self);

/*****************************************************************************
 * SIB ACCESS functions
 *****************************************************************************/

/**
 * Join a smartspace. 
 *
 * @param self A WhiteBoardNode instance
 * @param udn The identifier of the smartspace.
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_join(WhiteBoardNode *self,
				     ssElement_ct udn);

/**
 * Leave a smartspace.
 *
 * @param self A WhiteBoardNode instance
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_leave(WhiteBoardNode *self);

/**
 * Insert graph represented by a list of triple structures into the smartspace the node is currently joined to. Triple elements (subect, predicat, object) may use prefixes for XML namespaces if a namespace string is provided. Since the namespace syntax includes "-characters, these must be escaped in a source code string.
 *
 * The namespace "xmlns:A=\"X:Y\"" allows an element "X:Yz", in full, to be "A:z" in the ssTriple_t structure. The namespace string can contain more than one namespace, separated with whitespace, one of which can be the default namespace. The default namespace "xmlns=\"X:Y\"" allows an element "X:Yz", in full, to be ":z" in the ssTriple_t structure.
 *
 * @param self A WhiteBoardNode instance
 * @param triples Pointer to the list of triples to insert (Blank Nodes allowed, Wildcards not allowed)
 * @param nameSpace NULL if no namespace, else one or more namespaces corresponding to prefixes used in the the triple element strings
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_insert_M3Triples(WhiteBoardNode *self,
						 GSList *triples,
						 const gchar *nameSpace);

#if 0 //NOT SUPPORTED
/**
 * Insert graph in string into the smartspace the node is currently joined to.
 *
 * @param self A WhiteBoardNode instance
 * @param string with encoded text of graph to insert
 * @param encoding type used for the graph (only EncodingRDFXML is supported)
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
#endif
ssStatus_t whiteboard_node_sib_access_insert_graph(WhiteBoardNode *self,
					     gchar *graph,
					     EncodingType encoding);


/**
 * Update information into the smartspace the node is currently joined to.
 *
 * @param self A WhiteBoardNode instance
 * @param add_triples Pointer to the list of triples to add (Blank Nodes allowed, Wildcards not allowed)
 * @param remove_triples Pointer to the list of triples to remove (Blank Nodes not allowed, Wildcards allowed)
 * @param nameSpace NULL if no namespace, else one or more namespaces corresponding to prefixes used in the the triple element strings
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_update_M3Triples(WhiteBoardNode *self,
						 GSList *add_triples,
						 GSList *remove_triples,
						 const gchar *nameSpace);

#if 0 //NOT SUPPORTED
/**
 * Update information into the smartspace the node is currently joined to.
 *
 * @param self A WhiteBoardNode instance
 * @param add_graph string with encoded text of graph to be added.
 * @param remove_graph string with encoded text of graph to be removed.
 * @param encoding type used for both graphs
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
#endif
ssStatus_t whiteboard_node_sib_access_update_graph(WhiteBoardNode *self,
					     gchar *insert_graph,
					     gchar *remove_graph,
					     EncodingType encoding);


/**
 * Remove triples from the smartspace the node is currently joined to.
 * 
 * @param self A WhiteBoardNode instance
 * @param triples Pointer to the list of triples to remove (Blank Nodes not allowed, Wildcards allowed)
 * @param nameSpace NULL if no namespace, else one or more namespaces corresponding to prefixes used in the the triple element strings
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_remove_M3Triples(WhiteBoardNode *self,
						       GSList *triples,
						       const gchar *nameSpace);
#if 0 //NOT SUPPORTED
/**
 * Remove triples from the smartspace the node is currently joined to.
 * 
 * @param self A WhiteBoardNode instance
 * @param string with encoded text graph to be removed
 * @param encoding type used for the graph
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
#endif

ssStatus_t whiteboard_node_sib_access_remove_graph(WhiteBoardNode *self,
					     gchar *graph,
					     EncodingType encoding);


/**
 * Query smartspace the node is joined to. This is asynchronous. 
 *
 * @param self A WhiteBoardNode instance
 * @param templates Pointer to the list of template triples to be matched (Blank Nodes not allowed, Wildcards allowed)
 * @param nameSpace NULL if no namespace, else one or more namespaces corresponding to prefixes used in the the triple element strings
 * @param cb Pointer to the callback function that is called when results for the query have been received.
 * @param data Pointer to user data for the callback function, NULL if none.
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_query_template(WhiteBoardNode *self,
					       GSList* templates,
					       const gchar *namespace,
					       WhiteBoardNodeQueryTemplateCB cb,
					       gpointer data);

/**
 * Query smartspace the node is joined to. This is asynchronous.
 * 
 * A SPARQL select query specification is graph specification consisting of the combined list of path nodes in the "where" and "optional" lists. A path node is: ssPathNode_t, where string is assigned with a non-zero length character string. If nodeType is appropriately ssElement_TYPE_URI or ssElement_TYPE_LIT, the node specifies an exact match in the SIB information. If nodeType is ssElement_TYPE_BNODE, the node is a variable which can match any SIB information in common with other uses of the variable in the query specification. If used only once, the variable is a wildcard, ie. matches any SIB information. 
 * 
 *  Variables can be "selected" or not (see select parameter). In the query resonse, all variables, as well as use of the general wildcard, ssMATCH_ANY, qualify the soultions to the query specification, but only solutions for selected variables are returned in the query results. In a solution, selected variables are bound to a path node (matching one in the SIB) in a query solution row, in the column corresponding to the matched variable poisition in the returned selected variable list. Note the difference in type between the select parameter here and the returned selected variable list. Each selected variable here has a corresponding entry in the returned list, but not necessarily in the same order.
 * 
 * The where part specifies a required graph to be matched in the SIB. If no such graph is found, the returned list of solution rows will be empty. Otherwise, there is one row for each differnt solution to bindings of the selected variables. An optional list specifies a graph which may or not be matched, and allows selected variables in the query specification to be bound, which were not already bound by the where specification. Row solutions will have a NULL for unbound selected variables.
 *
 * @param self A WhiteBoardNode instance
 * @param select Pointer to a list of pointers to the selected variables, as described above. If select is NULL, all named blank nodes in the query specification are selected. An error is returned if a varible pointer is NULL, or does not point to a named blank node. Be aware that if a valid variable in the select list is not in, or matching one in the query specification, it will have no binding in the results - i.e. will always be NULL for all result rows, if any.
 * @param where Pointer to a list of pointers to triples of path nodes in that part of the query specification which must be matched (Blank Nodes and wildcards allowed).
 * @param optional_lists Pointer to a list of pointers to a list of pointers to path nodes. NULL if no optional lists, otherwise 
 * @param nameSpace NULL if no namespace, else one or more namespaces corresponding to prefixes used in the the triple element strings
 * @param cb Pointer to the callback function that is called when results for the query have been received.
 * @param data Pointer to user data for the callback function, NULL if none.
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 *
 * Note: where and optional lists are lists of sparqlTriple_t, not ssTriple_t, since the later do not support variables in the predicate position.
 */
ssStatus_t whiteboard_node_sib_access_query_sparql_select(WhiteBoardNode *self,
							  GSList* select,
							  GSList* where,
							  GSList* optional_lists,
							  const gchar *namespace,
							  WhiteBoardNodeQuerySPARQLselectCB cb,
							  gpointer data);

/**
 * Query smartspace the node is joined to. This is asynchronous. 
 *
 * @param self A WhiteBoardNode instance
 * @param node Pointer to the ssPathNode_t structure specifying the starting node for the WQL-values query.
 * @param pathExpr Pointer to the string containing the path expression for the WQL-values query
 * @param cb Pointer to the callback function that is called when results for the query have been received.
 * @param data Optional pointer to user data for the callback function.
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_query_wql_values(WhiteBoardNode *self,
						 const ssPathNode_t *node,
						 const gchar *pathExpr,
						 WhiteBoardNodeQueryWQLnodelistCB cb,
						 gpointer data);
/**
 * Create subscription. This is asynchronous. 
 *
 * @param self A WhiteBoardNode instance
 * @param templates Pointer to the list of template triples to be matched (Blank Nodes not allowed, Wildcards allowed)
 * @param nameSpace NULL if no namespace, else one or more namespaces corresponding to prefixes used in the the triple element strings
 * @param cb Pointer to the callback function that is called when results for the query have been received.
 * @param data Pointer to user data for the callback function, NULL if none.
 * @param address of subscription_id. Assigned value > 0 if ss_StatusOK is returned, otherwise -1. Needed later to cancel a successfully set subscription
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_subscribe_template(WhiteBoardNode *self,
							 GSList* templates,
							 const gchar *namespace,
							 WhiteBoardNodeSubscriptionIndTemplateCB cb,
							 gint *subscription_id,
							 gpointer data);

/**
 * Create subscription. This is asynchronous. 
 *
 * @param self A WhiteBoardNode instance
 * @param node Pointer to the ssPathNode_t structure specifying the starting node for the WQL-values query.
 * @param pathExpr Pointer to the string containing the path expression for the WQL-values query
 * @param cb Pointer to the callback function that is called when results from SIB have been received.
 * @param data Optional pointer to user data for the callback function.
 * @param address of subscription_id. Assigned value > 0 if ss_StatusOK is returned, otherwise -1. Needed later to cancel a successfully set subscription
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_subscribe_wql_values(WhiteBoardNode *self,
							   const ssPathNode_t *pathNode,
							   const gchar *pathExpr,
							   WhiteBoardNodeSubscriptionIndWQLvaluesCB cb,
							   gint *subscription_id,
							   gpointer data);


/**
 * Create subscription. This is asynchronous. 
 *
 * @param self A WhiteBoardNode instance
 * @param startNode Pointer to the ssPathNode_t structure specifying the start node for the WQL-values query.
 * @param pathExpr Pointer to the string containing the expression of the path from the start to end node for the WQL-values query
 * @param endNode Pointer to the ssPathNode_t structure specifying the end node for the WQL-values query.
 * @param cb Pointer to the callback function that is called when results from SIB have been received.
 * @param data Optional pointer to user data for the callback function.
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_query_wql_related(WhiteBoardNode *self,
						  const ssPathNode_t *startNode,
						  const gchar *pathExpr,
						  const ssPathNode_t *endNode,
						  WhiteBoardNodeQSCommonWQLbooleanCB cb,
						  gpointer data);


/**
 * Create subscription. This is asynchronous. 
 *
 * @param self A WhiteBoardNode instance
 * @param startNode Pointer to the ssPathNode_t structure specifying the start node for the WQL-values query.
 * @param pathExpr Pointer to the string containing the expression of the path from the start to end node for the WQL-values query
 * @param endNode Pointer to the ssPathNode_t structure specifying the end node for the WQL-values query.
 * @param cb Pointer to the callback function that is called when results from SIB have been received.
 * @param data Optional pointer to user data for the callback function.
 * @param address of subscription_id. Assigned value > 0 if ss_StatusOK is returned, otherwise -1. Needed later to cancel a successfully set subscription
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_subscribe_wql_related(WhiteBoardNode *self,
							    const ssPathNode_t *startNode,
							    const gchar *pathExpr,
							    const ssPathNode_t *endNode,
							    WhiteBoardNodeQSCommonWQLbooleanCB cb,
							    gint *subscription_id,
							    gpointer data);


/**
 * Query smartspace the node is joined to. This is asynchronous. 
 *
 * @param self A WhiteBoardNode instance
 * @param node Pointer to the ssPathNode_t structure specifying the node for which the class list is requested.
 * @param cb Pointer to the callback function that is called when results for the query have been received.
 * @param data Optional pointer to user data for the callback function.
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_query_wql_nodeClasses(WhiteBoardNode *self,
						      const ssPathNode_t *node,
						      WhiteBoardNodeQueryWQLnodelistCB cb,
						      gpointer data);

/**
 * Query smartspace the node is joined to. This is asynchronous. 
 *
 * @param self A WhiteBoardNode instance
 * @param node Pointer to the ssPathNode_t structure specifying the node reference.
 * @param class Pointer to the ssPathNode_t structure specifying the class reference.
 * @param cb Pointer to the callback function that is called when results for the query have been received.
 * @param data Optional pointer to user data for the callback function.
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_query_wql_ofClass(WhiteBoardNode *self,
						  const ssPathNode_t *node,
						  const ssPathNode_t *class,
						  WhiteBoardNodeQSCommonWQLbooleanCB cb,
						  gpointer data);

/**
 * Query smartspace the node is joined to. This is asynchronous. 
 *
 * @param self A WhiteBoardNode instance
 * @param subclass Pointer to the ssPathNode_t structure specifying the subclass reference.
 * @param superclass Pointer to the ssPathNode_t structure specifying the superclass reference.
 * @param cb Pointer to the callback function that is called when results for the query have been received.
 * @param data Optional pointer to user data for the callback function.
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_query_wql_isSubclass(WhiteBoardNode *self,
						     const ssPathNode_t *subclass,
						     const ssPathNode_t *superclass,
						     WhiteBoardNodeQSCommonWQLbooleanCB cb,
						     gpointer data);

/**
 * Cancel a subscription. This is asynchronous. 
 *
 * @param self A WhiteBoardNode instance
 * @param subscription_id Identifier of the subscription to cancel - obtained when subscription was made..
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t whiteboard_node_sib_access_unsubscribe(WhiteBoardNode *self, gint subscription_id);

/*****************************************************************************
 * Utilities
 *****************************************************************************/

/**
 * Create new triple and prepend it to triple list. All strings passed to the function are duplicated. When creating triple for template query, one or more components of the triple are filled with wildcard uri \"sib:any\". Note: tripleId field of the triples is left empty. 
 * @param currentList Pointer to list start.
 * @param subject Pointer to the string containing the subject of the created triple.
 * @param predicate Pointer to the string containing the predicate of the created triple.
 * @param object Pointer to the string containing the object of the created triple.
 * @param subjType Subject type (URI, literal or blanknode - use 0 if subject is ssMATCH_ANY)
 * @param objType Object type (URI, literal or blanknode - use 0 if object is ssMATCH_ANY)
 * @return ssStatus_t.
 */
ssStatus_t ssPrependTriple (GSList **currentList, ssElement_ct subject, ssElement_ct predicate, ssElement_ct object,
			    ssElementType_t subjType, ssElementType_t objType);


/**
 * Create new triple and copy it's content from src.
 * @param src Pointer to source triple.
 * @param dst Pointer to newly allocated triple. 
 * @return ssStatus_t.
 */
ssStatus_t ssCopyTriple( ssTriple_t *src, ssTriple_t **dst );


/**
 * Free memory allocated for the triples from a list of triples.
 *
 * @param tripleList Pointer to pointer of list to free; set NULL upon return.
 */
void ssFreeTripleList (GSList **tripleList);

/**
 * Free memory allocated for a single triple
 *
 * @param triple Pointer to triple structure.
 */
void ssFreeTriple( ssTriple_t *triple);

/**
 * Free memory allocated for a list of path-nodes and zero the list pointer.
 *
 * @param pathNodeList Pointer to pointer of list to free; set NULL upon return.
 */
void ssFreePathNodeList (GSList **pathNodeList);

/**
 * Free memory allocated for a single path-node.
 *
 * @param pathNode Pointer to pointer of ssPathNode_t structure.
 */
void ssFreePathNode( ssPathNode_t *pathNode);

/**
 * Create a prefix to namespace hash table
 *
 * @param ns char string with XML namespace syntax. NULL is not a valid parameter, "" ??








. Quotes within the namespace string must be escaped For example "xmlns:prefix1=\"ns1#\" xmlns=\"ns/Default#\"", gives two mapppings: "prefix1"<->"ns1#" and ""<->"ns/Default#".
 * @param prefix2ns_map address of GHashTable pointer to be assigned if ss_StatusOK is returned, otherwise unchanged. The pointer must be set NULL when passed in. An allocated table should be freed using g_hash_table_destroy(prefix2ns_map).
 * @return ss_StatusOK (zero) if the operation was successful, otherwise a non-zero ssStatus_t value.
 */
ssStatus_t new_prefix2ns_map(const gchar *ns, GHashTable **prefix2ns_map);

/**
 * Allocate full uri string for prefixed reference string.
 * 
 * @param s string with prefix ending with ":" separator - the remainder of the string is the "local" reference.
 * @param sLen length of s. I.e. s does not have to be a zero terminated string.
 * @param prefix2ns_map a GHashTable returned by new_prefix2ns_map()
 * @return NULL if no map is found, otherwise a string which must be freed with g_free(). For example, using the hash table assigned by the example namespace given for new_prefix2ns_map(), "prefix1:local1" would return "ns1#local1" and ":local2" would return "ns/Default#local2"
 */
gchar *fullUri (const gchar *s, int sLen, GHashTable *prefix2ns_map);

/**
 * Replace full uri string with prefixed name.
 *
 * @param g_heap_str address to pointer of uri string allocated from the g-heap. If the uri starts with a namespace in the table, the string is freed with g_free() and replaced with newly allocated string with the corresponding prefix separated from the local name with ":", otherwise *g_heap_str is unchanged.
 * @param prefix2ns_map a GHashTable returned by new_prefix2ns_map()
 * @return none.
 */
void nsLocal2prefixLocal (ssElement_t *g_heap_str, GHashTable *prefix2ns_map);
G_END_DECLS

#endif
