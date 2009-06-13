
#ifndef __GMPC_PANED_SIZE_GROUP_H__
#define __GMPC_PANED_SIZE_GROUP_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GMPC_TYPE_PANED_SIZE_GROUP (gmpc_paned_size_group_get_type ())
#define GMPC_PANED_SIZE_GROUP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_PANED_SIZE_GROUP, GmpcPanedSizeGroup))
#define GMPC_PANED_SIZE_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_PANED_SIZE_GROUP, GmpcPanedSizeGroupClass))
#define GMPC_IS_PANED_SIZE_GROUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_PANED_SIZE_GROUP))
#define GMPC_IS_PANED_SIZE_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_PANED_SIZE_GROUP))
#define GMPC_PANED_SIZE_GROUP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_PANED_SIZE_GROUP, GmpcPanedSizeGroupClass))

typedef struct _GmpcPanedSizeGroup GmpcPanedSizeGroup;
typedef struct _GmpcPanedSizeGroupClass GmpcPanedSizeGroupClass;
typedef struct _GmpcPanedSizeGroupPrivate GmpcPanedSizeGroupPrivate;

struct _GmpcPanedSizeGroup {
	GObject parent_instance;
	GmpcPanedSizeGroupPrivate * priv;
};

struct _GmpcPanedSizeGroupClass {
	GObjectClass parent_class;
};


GType gmpc_paned_size_group_get_type (void);
GmpcPanedSizeGroup* gmpc_paned_size_group_new (void);
GmpcPanedSizeGroup* gmpc_paned_size_group_construct (GType object_type);
void gmpc_paned_size_group_add_paned (GmpcPanedSizeGroup* self, GtkPaned* paned);


G_END_DECLS

#endif
