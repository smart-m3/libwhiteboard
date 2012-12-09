
#ifndef __marshal_MARSHAL_H__
#define __marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:VOID (whiteboard_marshal.list:20) */
#define marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID

/* VOID:POINTER (whiteboard_marshal.list:26) */
#define marshal_VOID__POINTER	g_cclosure_marshal_VOID__POINTER

/* VOID:POINTER,INT,STRING,STRING,INT (whiteboard_marshal.list:33) */
extern void marshal_VOID__POINTER_INT_STRING_STRING_INT (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

/* VOID:INT (whiteboard_marshal.list:36) */
#define marshal_VOID__INT	g_cclosure_marshal_VOID__INT

/* VOID:INT,INT (whiteboard_marshal.list:39) */
extern void marshal_VOID__INT_INT (GClosure     *closure,
                                   GValue       *return_value,
                                   guint         n_param_values,
                                   const GValue *param_values,
                                   gpointer      invocation_hint,
                                   gpointer      marshal_data);

/* VOID:POINTER,STRING,STRING,INT (whiteboard_marshal.list:42) */
extern void marshal_VOID__POINTER_STRING_STRING_INT (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data);

/* VOID:POINTER,STRING,STRING,INT,INT,STRING (whiteboard_marshal.list:48) */
extern void marshal_VOID__POINTER_STRING_STRING_INT_INT_STRING (GClosure     *closure,
                                                                GValue       *return_value,
                                                                guint         n_param_values,
                                                                const GValue *param_values,
                                                                gpointer      invocation_hint,
                                                                gpointer      marshal_data);

/* VOID:POINTER,STRING,STRING,INT,INT,STRING,STRING (whiteboard_marshal.list:51) */
extern void marshal_VOID__POINTER_STRING_STRING_INT_INT_STRING_STRING (GClosure     *closure,
                                                                       GValue       *return_value,
                                                                       guint         n_param_values,
                                                                       const GValue *param_values,
                                                                       gpointer      invocation_hint,
                                                                       gpointer      marshal_data);

/* VOID:POINTER,INT,STRING,STRING,INT,INT,STRING (whiteboard_marshal.list:54) */
extern void marshal_VOID__POINTER_INT_STRING_STRING_INT_INT_STRING (GClosure     *closure,
                                                                    GValue       *return_value,
                                                                    guint         n_param_values,
                                                                    const GValue *param_values,
                                                                    gpointer      invocation_hint,
                                                                    gpointer      marshal_data);

/* VOID:POINTER,INT,STRING,STRING,INT,STRING (whiteboard_marshal.list:57) */
extern void marshal_VOID__POINTER_INT_STRING_STRING_INT_STRING (GClosure     *closure,
                                                                GValue       *return_value,
                                                                guint         n_param_values,
                                                                const GValue *param_values,
                                                                gpointer      invocation_hint,
                                                                gpointer      marshal_data);

/* VOID:STRING,STRING (whiteboard_marshal.list:67) */
extern void marshal_VOID__STRING_STRING (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);

/* VOID:INT,INT,STRING (whiteboard_marshal.list:70) */
extern void marshal_VOID__INT_INT_STRING (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

G_END_DECLS

#endif /* __marshal_MARSHAL_H__ */

