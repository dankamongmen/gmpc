
#ifndef __GMPC_METADATA_PREFETCHER_H__
#define __GMPC_METADATA_PREFETCHER_H__

#include <glib.h>
#include <gmpc-plugin.h>

G_BEGIN_DECLS


#define GMPC_PLUGIN_TYPE_METADATA_PREFETCHER (gmpc_plugin_metadata_prefetcher_get_type ())
#define GMPC_PLUGIN_METADATA_PREFETCHER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN_TYPE_METADATA_PREFETCHER, GmpcPluginMetadataPrefetcher))
#define GMPC_PLUGIN_METADATA_PREFETCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_PLUGIN_TYPE_METADATA_PREFETCHER, GmpcPluginMetadataPrefetcherClass))
#define GMPC_PLUGIN_IS_METADATA_PREFETCHER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN_TYPE_METADATA_PREFETCHER))
#define GMPC_PLUGIN_IS_METADATA_PREFETCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_PLUGIN_TYPE_METADATA_PREFETCHER))
#define GMPC_PLUGIN_METADATA_PREFETCHER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_PLUGIN_TYPE_METADATA_PREFETCHER, GmpcPluginMetadataPrefetcherClass))

typedef struct _GmpcPluginMetadataPrefetcher GmpcPluginMetadataPrefetcher;
typedef struct _GmpcPluginMetadataPrefetcherClass GmpcPluginMetadataPrefetcherClass;
typedef struct _GmpcPluginMetadataPrefetcherPrivate GmpcPluginMetadataPrefetcherPrivate;

struct _GmpcPluginMetadataPrefetcher {
	GmpcPluginBase parent_instance;
	GmpcPluginMetadataPrefetcherPrivate * priv;
	gint* version;
	gint version_length1;
};

struct _GmpcPluginMetadataPrefetcherClass {
	GmpcPluginBaseClass parent_class;
};


GType gmpc_plugin_metadata_prefetcher_get_type (void);
GmpcPluginMetadataPrefetcher* gmpc_plugin_metadata_prefetcher_new (void);
GmpcPluginMetadataPrefetcher* gmpc_plugin_metadata_prefetcher_construct (GType object_type);


G_END_DECLS

#endif