#include "config1.h"
#include <gtk/gtk.h>
#include <libmpd/libmpd.h>
#ifndef __GMPC_PLUGIN_H__
#define __GMPC_PLUGIN_H__

#define PLUGIN_ID_MARK 1024
extern MpdObj *connection;
extern config_obj *config;

/* Plugin Type */
enum {
	GMPC_PLUGIN_DUMMY,
	GMPC_PLUGIN_PL_BROWSER
};



/* usefull defines */
#define PL3_ENTRY_ALBUM 64
#define PL3_ENTRY_ARTIST 32
#define PL3_ENTRY_DIRECTORY 16
#define PL3_CUR_PLAYLIST 8
#define PL3_ENTRY_STREAM 4
#define PL3_ENTRY_PLAYLIST 2
#define PL3_ENTRY_SONG 1

/* the gtk_tree_store row's */
enum 
{
	PL3_CAT_TYPE,
	PL3_CAT_TITLE,
	PL3_CAT_INT_ID,
	PL3_CAT_ICON_ID,
	PL3_CAT_PROC, /* for the lazy tree, if the dir is allready processed */
	PL3_CAT_ICON_SIZE,
	PL3_CAT_BROWSE_FORMAT,
	PL3_CAT_NROWS
} pl3_cat_store;

typedef struct {
	void (*add)(GtkWidget *cat_tree);
	void (*selected)(GtkWidget *container);
	void (*unselected)(GtkWidget *container);
	void (*cat_selection_changed)(GtkWidget *tree, GtkTreeIter *iter);
} gmpcPlBrowserPlugin;

/* sturcture */
typedef struct {
	char 	*name;
	int	version[3];
	int 	plugin_type;
	/* unique plugin id */
	int 	id; /* do not fill in, is done by gmpc */
	/* depending on type of plugin */
	gmpcPlBrowserPlugin *browser;
} gmpcPlugin;




#endif 
