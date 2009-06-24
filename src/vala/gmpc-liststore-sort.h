
#ifndef __GMPC_LISTSTORE_SORT_H__
#define __GMPC_LISTSTORE_SORT_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GMPC_LISTSTORE_TYPE_SORT (gmpc_liststore_sort_get_type ())
#define GMPC_LISTSTORE_SORT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_LISTSTORE_TYPE_SORT, GmpcListstoreSort))
#define GMPC_LISTSTORE_SORT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_LISTSTORE_TYPE_SORT, GmpcListstoreSortClass))
#define GMPC_LISTSTORE_IS_SORT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_LISTSTORE_TYPE_SORT))
#define GMPC_LISTSTORE_IS_SORT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_LISTSTORE_TYPE_SORT))
#define GMPC_LISTSTORE_SORT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_LISTSTORE_TYPE_SORT, GmpcListstoreSortClass))

typedef struct _GmpcListstoreSort GmpcListstoreSort;
typedef struct _GmpcListstoreSortClass GmpcListstoreSortClass;
typedef struct _GmpcListstoreSortPrivate GmpcListstoreSortPrivate;

struct _GmpcListstoreSort {
	GtkListStore parent_instance;
	GmpcListstoreSortPrivate * priv;
};

struct _GmpcListstoreSortClass {
	GtkListStoreClass parent_class;
};


GType gmpc_liststore_sort_get_type (void);
GmpcListstoreSort* gmpc_liststore_sort_new (void);
GmpcListstoreSort* gmpc_liststore_sort_construct (GType object_type);


G_END_DECLS

#endif
