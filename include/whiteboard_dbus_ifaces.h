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
 * @file whiteboard_dbus_ifaces.h
 * @brief DBUS interface definitions with method and signal definitions supported by WhiteBoard daemon.
 *
 * Copyright 2007 Nokia Corporation
 **/

#ifndef WHITEBOARD_DBUS_IFACES_H
#define WHITEBOARD_DBUS_IFACES_H

/*****************************************************************************
 * WHITEBOARD Common Service & Object names
 ******************************************************************************/
/**
 * WhiteBoard DBUS interface define.
 **/
#define WHITEBOARD_DBUS_INTERFACE "com.nokia.whiteboard"

/**
 * WhiteBoard DBUS service define
 **/
#define WHITEBOARD_DBUS_SERVICE "com.nokia.whiteboard"

/**
 * WhiteBoard DBUS object define
 **/
#define WHITEBOARD_DBUS_OBJECT "/com/nokia/whiteboard"

/**
 * WhiteBoard DBUS discovery method. Discovery method is used to get the DBUS address of Whiteboard daemon
 **/
#define WHITEBOARD_DBUS_METHOD_DISCOVERY "discover"

/**
 * WhiteBoard DBUS custom command method
 **/
#define WHITEBOARD_DBUS_METHOD_CUSTOM_COMMAND "custom_command"

/*****************************************************************************
 * WHITEBOARD Registration interface, methods & signals
 ******************************************************************************/

/* Interface */
/**
 * Definition of WhiteBoard register interface
 **/
#define WHITEBOARD_DBUS_REGISTER_INTERFACE WHITEBOARD_DBUS_INTERFACE ".register"

/* Methods */
/**
 * Definition of register control method. This method is used when registering control object. Register only one control object per process.
 **/
#define WHITEBOARD_DBUS_REGISTER_METHOD_CONTROL "control"
/**
 * Definition of register node method. This method is used when registering node object to the WhiteBoard. 
 **/
#define WHITEBOARD_DBUS_REGISTER_METHOD_NODE "node"
/**
 ** Definition of register sib method. This method is used when registering sib object to the WhiteBoard. 
 */
#define WHITEBOARD_DBUS_REGISTER_METHOD_SIB "sib"

#define WHITEBOARD_DBUS_REGISTER_METHOD_DISCOVERY "discovery"

/* Signals */
/**
 ** Definition of register sib signal. This signal is used when a node wishes to unregister from WhiteBoard. 
 */
#define WHITEBOARD_DBUS_REGISTER_SIGNAL_UNREGISTER_NODE "unregister_node"

/* Interface */

#define WHITEBOARD_DBUS_DISCOVERY_INTERFACE WHITEBOARD_DBUS_INTERFACE ".discovery"

/**
 * Definition of WhiteBoard SIB discovery interface
 **/

#define WHITEBOARD_DBUS_DISCOVERY_METHOD_GET_SIBS "get_sibs"
#define WHITEBOARD_DBUS_DISCOVERY_SIGNAL_SIB "sib"
#define WHITEBOARD_DBUS_DISCOVERY_SIGNAL_ALL_FOR_NOW "all_for_now"

#define WHITEBOARD_DBUS_DISCOVERY_SIGNAL_SIB_INSERTED "sib_inserted" 
#define WHITEBOARD_DBUS_DISCOVERY_SIGNAL_SIB_REMOVED "sib_removed"



/*****************************************************************************
 * WHITEBOARD Control interface, methods & signals
 ******************************************************************************/

/* Interface */
/**
 * Definition of WhiteBoard control interface
 **/
#define WHITEBOARD_DBUS_CONTROL_INTERFACE WHITEBOARD_DBUS_INTERFACE ".control"

/* Methods */
/**
 * Definition of shutdown method. Sent by the WhiteBoard daemon when it wishes to shutdown/restart all processes registered to to whiteboard.
 **/
#define WHITEBOARD_DBUS_CONTROL_METHOD_SHUTDOWN "shutdown"
/**
 * Definition of refresh method. 
 **/
#define WHITEBOARD_DBUS_CONTROL_METHOD_REFRESH "refresh"
#define WHITEBOARD_DBUS_CONTROL_METHOD_GET_DESCRIPTION "get_description"
#define WHITEBOARD_DBUS_CONTROL_METHOD_HEALTHCHECK "healthcheck"
#define WHITEBOARD_DBUS_CONTROL_METHOD_CUSTOM_COMMAND WHITEBOARD_DBUS_METHOD_CUSTOM_COMMAND

/* Signals */
#define WHITEBOARD_DBUS_CONTROL_SIGNAL_STARTING "starting"


/*****************************************************************************
 * WHITEBOARD NODE interface, methods & signals
 *****************************************************************************/

/* Interface */
#define WHITEBOARD_DBUS_NODE_INTERFACE WHITEBOARD_DBUS_INTERFACE ".node"

/* Methods */

#define WHITEBOARD_DBUS_NODE_METHOD_REFRESH_NODE WHITEBOARD_DBUS_CONTROL_METHOD_REFRESH
#define WHITEBOARD_DBUS_NODE_METHOD_GET_DESCRIPTION WHITEBOARD_DBUS_CONTROL_METHOD_GET_DESCRIPTION
#define WHITEBOARD_DBUS_NODE_METHOD_JOIN "join"
#define WHITEBOARD_DBUS_NODE_METHOD_LEAVE "leave"
#define WHITEBOARD_DBUS_NODE_METHOD_INSERT "insert"
#define WHITEBOARD_DBUS_NODE_METHOD_UPDATE "update"
#define WHITEBOARD_DBUS_NODE_METHOD_QUERY "query"
#define WHITEBOARD_DBUS_NODE_METHOD_REMOVE "remove"
#define WHITEBOARD_DBUS_NODE_METHOD_SUBSCRIBE "subscribe"


/* Signals */

#define WHITEBOARD_DBUS_NODE_SIGNAL_JOIN_COMPLETE "join_complete"
#define WHITEBOARD_DBUS_NODE_SIGNAL_SUBSCRIPTION_IND "subscription_ind"

#define WHITEBOARD_DBUS_NODE_SIGNAL_UNSUBSCRIBE "unsubscribe"
#define WHITEBOARD_DBUS_NODE_SIGNAL_UNSUBSCRIBE_COMPLETE "unsubscribe_complete"

#define WHITEBOARD_DBUS_NODE_SIGNAL_SIB_INSERTED "sib_inserted" 
#define WHITEBOARD_DBUS_NODE_SIGNAL_SIB_REMOVED "sib_removed"

/*****************************************************************************
 * WHITEBOARD Log interface, methods & signals
 *****************************************************************************/

/* Interface */
#define WHITEBOARD_DBUS_LOG_INTERFACE WHITEBOARD_DBUS_INTERFACE ".log"

/* Methods */
#define WHITEBOARD_DBUS_LOG_SIGNAL_MESSAGE "message"

/* Signals */

/*****************************************************************************
 * WHITEBOARD SIB ACCESS component interface, methods & signals
 *****************************************************************************/

/* Interface */
#define WHITEBOARD_DBUS_SIB_ACCESS_INTERFACE WHITEBOARD_DBUS_INTERFACE ".sib_access"

/* Methods */
#define WHITEBOARD_DBUS_SIB_ACCESS_METHOD_GET_DESCRIPTION WHITEBOARD_DBUS_CONTROL_METHOD_GET_DESCRIPTION
#define WHITEBOARD_DBUS_SIB_ACCESS_METHOD_INSERT WHITEBOARD_DBUS_NODE_METHOD_INSERT
#define WHITEBOARD_DBUS_SIB_ACCESS_METHOD_UPDATE WHITEBOARD_DBUS_NODE_METHOD_UPDATE
#define WHITEBOARD_DBUS_SIB_ACCESS_METHOD_QUERY WHITEBOARD_DBUS_NODE_METHOD_QUERY
#define WHITEBOARD_DBUS_SIB_ACCESS_METHOD_REMOVE WHITEBOARD_DBUS_NODE_METHOD_REMOVE
#define WHITEBOARD_DBUS_SIB_ACCESS_METHOD_SUBSCRIBE WHITEBOARD_DBUS_NODE_METHOD_SUBSCRIBE



/* Signals */
#define WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_SIB_REMOVED WHITEBOARD_DBUS_NODE_SIGNAL_SIB_REMOVED
#define WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_JOIN WHITEBOARD_DBUS_NODE_METHOD_JOIN
#define WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_LEAVE WHITEBOARD_DBUS_NODE_METHOD_LEAVE
#define WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_JOIN_COMPLETE WHITEBOARD_DBUS_NODE_SIGNAL_JOIN_COMPLETE
#define WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_SUBSCRIPTION_IND WHITEBOARD_DBUS_NODE_SIGNAL_SUBSCRIPTION_IND
#define WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_UNSUBSCRIBE WHITEBOARD_DBUS_NODE_SIGNAL_UNSUBSCRIBE
#define WHITEBOARD_DBUS_SIB_ACCESS_SIGNAL_UNSUBSCRIBE_COMPLETE WHITEBOARD_DBUS_NODE_SIGNAL_UNSUBSCRIBE_COMPLETE

#endif /* WHITEBOARD_DBUS_IFACES_H */
