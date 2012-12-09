
#ifndef __marshal_MARSHAL_H__
#define __marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:VOID (sib_marshal.list:19) */
#define marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID

/* VOID:VOID (sib_marshal.list:22) */

/* VOID:POINTER (sib_marshal.list:25) */
#define marshal_VOID__POINTER	g_cclosure_marshal_VOID__POINTER

/* VOID:INT,INT,STRING (sib_marshal.list:28) */
extern void marshal_VOID__INT_INT_STRING (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

/* VOID:POINTER,STRING (sib_marshal.list:32) */
extern void marshal_VOID__POINTER_STRING (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

/* VOID:STRING,STRING,INT,INT (sib_marshal.list:35) */
extern void marshal_VOID__STRING_STRING_INT_INT (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);

/* VOID:STRING,STRING,INT,INT,STRING (sib_marshal.list:38) */
extern void marshal_VOID__STRING_STRING_INT_INT_STRING (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);

/* VOID:STRING,STRING,INT,INT,STRING,STRING (sib_marshal.list:41) */
extern void marshal_VOID__STRING_STRING_INT_INT_STRING_STRING (GClosure     *closure,
                                                               GValue       *return_value,
                                                               guint         n_param_values,
                                                               const GValue *param_values,
                                                               gpointer      invocation_hint,
                                                               gpointer      marshal_data);

/* VOID:STRING,STRING,INT,INT,STRING,STRING,STRING (sib_marshal.list:44) */
extern void marshal_VOID__STRING_STRING_INT_INT_STRING_STRING_STRING (GClosure     *closure,
                                                                      GValue       *return_value,
                                                                      guint         n_param_values,
                                                                      const GValue *param_values,
                                                                      gpointer      invocation_hint,
                                                                      gpointer      marshal_data);

G_END_DECLS

#endif /* __marshal_MARSHAL_H__ */

