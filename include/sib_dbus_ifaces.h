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
 * @file sib_dbus_ifaces.h
 * @brief DBUS interface definitions with method and signal definitions supported by Sib daemon.
 *
 * Copyright 2007 Nokia Corporation
 **/

#ifndef SIB_DBUS_IFACES_H
#define SIB_DBUS_IFACES_H

/*****************************************************************************
 * SIB Common Service & Object names
 ******************************************************************************/
/**
 * Sib DBUS interface define.
 **/
#define SIB_DBUS_INTERFACE "com.nokia.sib"

/**
 * Sib DBUS service define
 **/
#define SIB_DBUS_SERVICE "com.nokia.sib"

/**
 * Sib DBUS object define
 **/
#define SIB_DBUS_OBJECT "/com/nokia/sib"

/**
 * Sib DBUS discovery method. Discovery method is used to get the DBUS address of Sib daemon
 **/
#define SIB_DBUS_METHOD_DISCOVERY "discover"

/*****************************************************************************
 * SIB Registration interface, methods & signals
 ******************************************************************************/

/* Interface */
/**
 * Definition of Sib register interface
 **/
#define SIB_DBUS_REGISTER_INTERFACE SIB_DBUS_INTERFACE ".register"

/* Methods */
/**
 * Definition of register control method. This method is used when registering control object. Register only one control object per process.
 **/
#define SIB_DBUS_REGISTER_METHOD_CONTROL "control"
/**
 * Definition of register node method. This method is used when registering node object to the Sib. 
 **/
#define SIB_DBUS_REGISTER_METHOD_KP "kp"


/* Signals */
/**
 ** Definition of register sib signal. This signal is used when a node wishes to unregister from Sib. 
 */
#define SIB_DBUS_REGISTER_SIGNAL_UNREGISTER_KP "unregister_kp"


/*****************************************************************************
 * SIB Control interface, methods & signals
 ******************************************************************************/

/* Interface */
/**
 * Definition of Sib control interface
 **/
#define SIB_DBUS_CONTROL_INTERFACE SIB_DBUS_INTERFACE ".control"

/* Methods */
/**
 * Definition of shutdown method. Sent by the Sib daemon when it wishes to shutdown/restart all processes registered to to sib.
 **/
#define SIB_DBUS_CONTROL_METHOD_SHUTDOWN "shutdown"
/**
 * Definition of refresh method. 
 **/

#define SIB_DBUS_CONTROL_METHOD_HEALTHCHECK "healthcheck"

#define SIB_DBUS_CONTROL_METHOD_REFRESH "refresh"

#define SIB_DBUS_CONTROL_METHOD_REGISTER_SIB "register_sib"

/* Signals */
#define SIB_DBUS_CONTROL_SIGNAL_STARTING "starting"

/*****************************************************************************
 * SIB NODE interface, methods & signals
 *****************************************************************************/

/* Interface */
#define SIB_DBUS_KP_INTERFACE SIB_DBUS_INTERFACE ".kp"

/* Methods */

#define SIB_DBUS_KP_METHOD_REFRESH_NODE SIB_DBUS_CONTROL_METHOD_REFRESH
#define SIB_DBUS_KP_METHOD_GET_DESCRIPTION SIB_DBUS_CONTROL_METHOD_GET_DESCRIPTION
#define SIB_DBUS_KP_METHOD_JOIN "join"
#define SIB_DBUS_KP_METHOD_LEAVE "leave"
#define SIB_DBUS_KP_METHOD_INSERT "insert"
#define SIB_DBUS_KP_METHOD_UPDATE "update"
#define SIB_DBUS_KP_METHOD_QUERY "query"
#define SIB_DBUS_KP_METHOD_REMOVE "remove"
#define SIB_DBUS_KP_METHOD_SUBSCRIBE "subscribe"
#define SIB_DBUS_KP_METHOD_UNSUBSCRIBE "unsubscribe"

/* Signals */

#define SIB_DBUS_KP_SIGNAL_JOIN_COMPLETE "join_complete"
#define SIB_DBUS_KP_SIGNAL_SUBSCRIPTION_IND "subscription_ind"
#define SIB_DBUS_KP_SIGNAL_UNSUBSCRIBE_IND "unsubscribe_ind"
#define SIB_DBUS_KP_SIGNAL_LEAVE_IND "leave_ind"

/*****************************************************************************
 * SIB Log interface, methods & signals
 *****************************************************************************/

/* Interface */
#define SIB_DBUS_LOG_INTERFACE SIB_DBUS_INTERFACE ".log"

/* Methods */
#define SIB_DBUS_LOG_SIGNAL_MESSAGE "message"

#endif /* SIB_DBUS_IFACES_H */
