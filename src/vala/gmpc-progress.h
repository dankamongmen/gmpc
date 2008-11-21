
#ifndef __GMPC_PROGRESS_H__
#define __GMPC_PROGRESS_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GMPC_TYPE_PROGRESS (gmpc_progress_get_type ())
#define GMPC_PROGRESS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_PROGRESS, GmpcProgress))
#define GMPC_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_PROGRESS, GmpcProgressClass))
#define GMPC_IS_PROGRESS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_PROGRESS))
#define GMPC_IS_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_PROGRESS))
#define GMPC_PROGRESS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_PROGRESS, GmpcProgressClass))

typedef struct _GmpcProgress GmpcProgress;
typedef struct _GmpcProgressClass GmpcProgressClass;
typedef struct _GmpcProgressPrivate GmpcProgressPrivate;

struct _GmpcProgress {
	GtkEventBox parent_instance;
	GmpcProgressPrivate * priv;
	gboolean _hide_text;
};

struct _GmpcProgressClass {
	GtkEventBoxClass parent_class;
};


void gmpc_progress_set_time (GmpcProgress* self, guint total, guint current);
GmpcProgress* gmpc_progress_new (void);
gboolean gmpc_progress_get_hide_text (GmpcProgress* self);
void gmpc_progress_set_hide_text (GmpcProgress* self, gboolean value);
gboolean gmpc_progress_get_do_countdown (GmpcProgress* self);
void gmpc_progress_set_do_countdown (GmpcProgress* self, gboolean value);
GType gmpc_progress_get_type (void);


G_END_DECLS

#endif