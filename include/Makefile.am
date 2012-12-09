# Installation directory for header files
whiteboarddir = @includedir@/whiteboard

# Use alphabetical order with these so they are easy to find
whiteboard_HEADERS = \
	whiteboard_command.h \
	whiteboard_dbus_ifaces.h \
	whiteboard_discovery.h \
	whiteboard_log.h \
	whiteboard_node.h \
	whiteboard_sib_access.h \
	whiteboard_util.h \
	sibmsg.h \
	sibdefs.h \
	sib_object.h \
	sib_dbus_ifaces.h 

noinst_HEADERS = \
	whiteboard_marshal.h \
	ssap_sib_tokens.h \
	m3_sib_tokens.h \
	sib_marshal.h


EXTRA_DIST = \
	whiteboard_marshal.list \
	sib_marshal.list

whiteboard_marshal.h: stamp-whiteboard_marshal.h
	@true

stamp-whiteboard_marshal.h: whiteboard_marshal.list
	glib-genmarshal --prefix=marshal whiteboard_marshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh whiteboard_marshal.h 2>/dev/null || cp xgen-gmh whiteboard_marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~ \
	&& echo timestamp > $@

sib_marshal.h: stamp-sib_marshal.h
	@true

stamp-sib_marshal.h: sib_marshal.list
	glib-genmarshal --prefix=marshal sib_marshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh sib_marshal.h 2>/dev/null || cp xgen-gmh sib_marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~ \
	&& echo timestamp > $@
