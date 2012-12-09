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
#ifndef WHITEBOARD_DISCOVERY_H
#define WHITEBOARD_DISCOVERY_H



#include <glib.h>
#include <glib-object.h>
G_BEGIN_DECLS

/******************************************************************************
 * Type checks and casts
 ******************************************************************************/

#define WHITEBOARD_DISCOVERY_TYPE whiteboard_discovery_get_type()

#define WHITEBOARD_DISCOVERY(object) G_TYPE_CHECK_INSTANCE_CAST(object, \
						       WHITEBOARD_DISCOVERY_TYPE, \
						       WhiteBoardDiscovery)

#define WHITEBOARD_DISCOVERY_CLASS(object) G_TYPE_CHECK_CLASS_CAST(object, \
							  WHITEBOARD_DISCOVERY_TYPE, \
							  WhiteBoardDiscoveryClass)

#define IS_WHITEBOARD_DISCOVERY(object) G_TYPE_CHECK_INSTANCE_TYPE(object, \
							  WHITEBOARD_DISCOVERY_TYPE)

#define IS_WHITEBOARD_DISCOVERY_CLASS(object) G_TYPE_CHECK_CLASS_TYPE(object, \
							     WHITEBOARD_DISCOVERY_TYPE)

/******************************************************************************
 * Type definitions
 ******************************************************************************/

struct _WhiteBoardDiscovery;
typedef struct _WhiteBoardDiscovery WhiteBoardDiscovery;

struct _WhiteBoardDiscoveryClass;
typedef struct _WhiteBoardDiscoveryClass WhiteBoardDiscoveryClass;

/*****************************************************************************
 * Source callback prototypes
 *****************************************************************************/

/**
 * Type definition for callback that is called when results from
 * a whiteboard_discovery_get_nodes() request is available.
 *
 * @param context the signaling WhiteBoadrdDiscovery instance.
 * @param uuid Unique id of the new SIB.
 * @param name Friendly name of the new SIB.
 */
typedef void (*WhiteBoardDiscoverySIBCB) (WhiteBoardDiscovery *context,
				     gchar *uuid,
				     gchar *name,
				     gpointer userdata);

/**
 * Use this identifier with g_signal_connect() to receive sib signals.
 */
#define WHITEBOARD_DISCOVERY_SIGNAL_SIB "sib"

/**
 * Type definition for callback that is called when a SIB is inserted.
 *
 * @param context the signaling WhiteBoardDiscovery instance.
 * @param uuid Unique id of the new source.
 * @param name Friendly name of the new source.
 */
typedef void (*WhiteBoardDiscoverySIBInsertedCB) (WhiteBoardDiscovery *context,
					     gchar *uuid,
					     gchar *name,
					     gpointer userdata);

/**
 * Use this identifier with g_signal_connect() to receive sib inserted signals.
 */
#define WHITEBOARD_DISCOVERY_SIGNAL_SIB_INSERTED "sib_inserted"

/**
 * Type definition for callback that is called when a source is removed.
 *
 * @param context the signaling WhiteBoardDiscovery instance.
 * @param uuid Unique id of the source which is removed.
 * @param name Friendly name of the sib which is removed.
 */
typedef void (*WhiteBoardDiscoverySIBRemovedCB) (WhiteBoardDiscovery *context,
						 gchar *uuid,
						 gchar *name,
						 gpointer userdata);

/**
 * Use this identifier with g_signal_connect() to receive sib removed signals.
 */
#define WHITEBOARD_DISCOVERY_SIGNAL_SIB_REMOVED "sib_removed"

/**
 * Type definition for callback that is called after sib signal has been emitted for all known sibs.
 *
 * @param context the signaling WhiteBoardDiscovery instance.
 */
typedef void (*WhiteBoardDiscoveryAllFowNowCB) (WhiteBoardDiscovery *context,
						gpointer userdata);

/**
 * Use this identifier with g_signal_connect() to receive all for now signals.
 */
#define WHITEBOARD_DISCOVERY_SIGNAL_ALL_FOR_NOW "all_for_now"

/**
 * Create a new WhiteBoardDiscovery instance.
 * this function does NOT create a new GMainLoop if the
 * mainloop parameter is omitted.
 *
 * @param maincontext A pre-allocated GMainContext (usually obtained with
 *                    g_main_context_default(), unless you know better)
 * @return Pointer to a new WhiteBoardDiscovery instance
 */
GObject *whiteboard_discovery_new(GMainContext *maincontext);


/**
 * Get all available SIBs. This is asynchronous.
 *
 * @param self A WhiteBoardDiscovery instance
 * @return TRUE if the sending of the DBUS message was successful, otherwise FALSE
 */
gboolean whiteboard_discovery_get_sibs(WhiteBoardDiscovery *self);

/**
 * Get the GLib type of WhiteBoardDiscovery
 */
GType whiteboard_discovery_get_type();

#endif
