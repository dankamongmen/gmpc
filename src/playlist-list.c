#include <stdio.h>
#include <string.h>
#include <libmpd/debug_printf.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpd-internal.h>
#include "playlist-list.h"
#include "main.h"

#include <glade/glade.h>
extern GladeXML *pl3_xml;

/* boring declarations of local functions */

static void playlist_list_init(CustomList * pkg_tree);

static void playlist_list_class_init(CustomListClass * klass);

static void playlist_list_tree_model_init(GtkTreeModelIface * iface);

static void playlist_list_finalize(GObject * object);

static GtkTreeModelFlags playlist_list_get_flags(GtkTreeModel * tree_model);

static gint playlist_list_get_n_columns(GtkTreeModel * tree_model);

static GType playlist_list_get_column_type(GtkTreeModel * tree_model,
		gint index);

static gboolean playlist_list_get_iter(GtkTreeModel * tree_model,
		GtkTreeIter * iter, GtkTreePath * path);

static GtkTreePath *playlist_list_get_path(GtkTreeModel * tree_model,
		GtkTreeIter * iter);

static void playlist_list_get_value(GtkTreeModel * tree_model,
		GtkTreeIter * iter,
		gint column, GValue * value);

static gboolean playlist_list_iter_next(GtkTreeModel * tree_model,
		GtkTreeIter * iter);

static gboolean playlist_list_iter_children(GtkTreeModel * tree_model,
		GtkTreeIter * iter,
		GtkTreeIter * parent);

static gboolean playlist_list_iter_has_child(GtkTreeModel * tree_model,
		GtkTreeIter * iter);

static gint playlist_list_iter_n_children(GtkTreeModel * tree_model,
		GtkTreeIter * iter);

static gboolean playlist_list_iter_nth_child(GtkTreeModel * tree_model,
		GtkTreeIter * iter,
		GtkTreeIter * parent, gint n);

static gboolean playlist_list_iter_parent(GtkTreeModel * tree_model,
		GtkTreeIter * iter,
		GtkTreeIter * child);

/* GObject stuff - nothing to worry about */
static GObjectClass *parent_class = NULL;

void playlist_list_set_current_song_pos(CustomList * cl, int new_pos)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	int song = cl->current_song_pos;
	cl->current_song_pos = new_pos;
	/* old song pos (song) is valid, make sure the row is updated so it shows unhighligted */
	if (song >= 0 && song < cl->num_rows) {
		path = gtk_tree_path_new();
		gtk_tree_path_append_index(path, song);
		/* iter */
		if (playlist_list_get_iter(GTK_TREE_MODEL(cl), &iter, path)) {
			/* changed */
			gtk_tree_model_row_changed(GTK_TREE_MODEL(cl), path,
					&iter);
		}
		gtk_tree_path_free(path);
	}

	/* if the current position is valid: update that row */
	if (cl->current_song_pos >= 0 && cl->current_song_pos < cl->num_rows) {
		path = gtk_tree_path_new();
		gtk_tree_path_append_index(path, cl->current_song_pos);
		/* iter */
		if (playlist_list_get_iter(GTK_TREE_MODEL(cl), &iter, path)) {
			/* changed */
			gtk_tree_model_row_changed(GTK_TREE_MODEL(cl), path,
					&iter);
		}
		gtk_tree_path_free(path);
	}
}

/* get the markup string */
gchar *playlist_list_get_markup(CustomList * cl)
{
	return cl->markup;
}

/* set the markup string */
void playlist_list_set_markup(CustomList * cl, gchar * markup)
{
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	MpdData *temp = cl->mpdata;
	if (!markup)
		return;
	if (cl->markup)
		g_free(cl->markup);
	cl->markup = g_strdup(markup);

	/* start sending signals */
	/* all rows are dirty now */
	for (temp = mpd_data_get_first(temp); temp;
			temp = mpd_data_get_next_real(temp, FALSE)) {

		path = gtk_tree_path_new();
		gtk_tree_path_append_index(path, temp->song->pos);
		/* create an iter */
		iter.stamp = cl->stamp;
		iter.user_data = temp;
		iter.user_data2 = GINT_TO_POINTER(temp->song->pos);
		iter.user_data3 = NULL;
		/* propegate change */
		gtk_tree_model_row_changed(GTK_TREE_MODEL(cl), path, &iter);
		gtk_tree_path_free(path);
	}
}

guint playlist_list_get_playtime(CustomList * cl)
{
	if (cl) {
		return cl->playtime;
	}
	return 0;
}

/*****************************************************************************
 *
 *  playlist_list_get_type: here we register our new type and its interfaces
 *                        with the type system. If you want to implement
 *                        additional interfaces like GtkTreeSortable, you
 *                        will need to do it here.
 *
 *****************************************************************************/




int playlist_list_state_handler(CustomList *cl)
{
	/* if we are called when the state is NONE
	 * we should remove ourself
	 */
	if(cl->pd.state == PD_STATE_NONE)
	{
		/* remove the idle handler */
		return FALSE;
	}	
	if(cl->pd.state == PD_STATE_RESTART)
	{
		if(cl->pd.data)mpd_data_free(cl->pd.data);
		if (cl->mpdata) {
			cl->mpdata = mpd_data_get_first(cl->mpdata);
		}                         


		cl->pd.data = mpd_playlist_get_changes(cl->pd.mi, cl->playlist_id);
		cl->pd.cell = NULL;
		cl->pd.iter = NULL;
		cl->pd.total_length =  mpd_playlist_get_playlist_length(cl->pd.mi);
		cl->pd.state = PD_STATE_UPDATE;
		printf("Restarting with high priority\n");
		g_idle_add_full(G_PRIORITY_LOW/*G_PRIORITY_HIGH_IDLE*/,(GSourceFunc)playlist_list_state_handler,cl,NULL);


		return FALSE;
	}
	else if (cl->pd.state == PD_STATE_STOP)
	{
		if(cl->pd.data)mpd_data_free(cl->pd.data);
		cl->pd.state = PD_STATE_CLEANUP;
	}
	else if (cl->pd.state == PD_STATE_INITIAL_FILL)
	{
		GtkTreePath *path = NULL;
		GtkTreeIter iter;
		MpdData *temp = NULL;
		if(cl->pd.data == NULL) {
			cl->pd.state = PD_STATE_CLEANUP;
			return TRUE;
		}
		/* start sending signals */
		path = gtk_tree_path_new();
		gtk_tree_path_append_index(path, cl->pd.data->song->pos);
		iter.stamp = cl->stamp;
		iter.user_data = cl->pd.data;
		iter.user_data2 = GINT_TO_POINTER(cl->pd.data->song->pos);
		iter.user_data3 = NULL;
		gtk_tree_model_row_inserted(GTK_TREE_MODEL(cl), path,
				&iter);
		gtk_tree_path_free(path);
		cl->playtime += cl->pd.data->song->time;
		cl->num_rows++;
		temp = cl->pd.data;
		cl->pd.data = mpd_data_get_next_real(cl->pd.data, FALSE);
		if(cl->pd.data){
			if((cl->pd.data->song->pos-temp->song->pos) != 1){
					printf("Inconsistency in my list\n");
			}
		}
	}
	else if (cl->pd.state == PD_STATE_UPDATE)
	{
		MpdData *temp = NULL;
		gint pos=0;
		GtkTreePath *path = NULL;
		GtkTreeIter iter;
		if(cl->pd.data == NULL) 
		{
			if(cl->pd.iter == NULL) cl->pd.iter = mpd_data_get_first(cl->mpdata);
			/* Delete songs from list that are removed */
			if (cl->num_rows > cl->pd.total_length && cl->mpdata) 
			{
				MpdData *temp = cl->pd.iter; 
				gint new_length = cl->pd.total_length;
				/* nasty trick to make it go backwards, otherwise gtk thinks everything
				 * is out of sync
				 */
				for (; !mpd_data_is_last(temp);	temp = mpd_data_get_next_real(temp, FALSE)) ;
				if (temp->song->pos >= new_length) 
				{
					int pos = temp->song->pos;
					cl->playtime -= temp->song->time;
					if((temp->song->file)[0] != 'x' && (temp->song->file)[1] != '\0')
					{
						cl->loaded--;
					}
					temp = mpd_data_delete_item(temp);
					cl->mpdata = temp;	//mpd_data_get_first(temp);
					cl->num_rows--;
					path = gtk_tree_path_new();
					gtk_tree_path_append_index(path, pos);
					gtk_tree_model_row_deleted(GTK_TREE_MODEL(cl),path);
					gtk_tree_path_free(path);
					cl->pd.iter = temp;
				} else {
					temp = NULL;
				}                                                                      		
				if(temp) return TRUE;
			}
			cl->pd.state = PD_STATE_CLEANUP;
			return TRUE;
		}
		if(cl->pd.iter == NULL) cl->pd.iter = mpd_data_get_first(cl->mpdata);
		temp = cl->pd.iter;
		pos = cl->pd.data->song->pos;
		if (cl->num_rows > 0 && pos < cl->num_rows) {
			int i = 0;
			for (;temp && temp->song->pos != pos;
					temp =
					mpd_data_get_next_real(temp, FALSE))i++ ;
			if(!temp) printf("gone wrong, no valid item found %i %i %i\n",pos, cl->num_rows,i);
			cl->loaded--;
			/* remove old song */
			cl->playtime -= temp->song->time;
			mpd_freeSong(temp->song);
			/* move new one in */
			temp->song = cl->pd.data->song;
			cl->pd.data->song = NULL;
			cl->playtime += temp->song->time;                                               		
			/* path */
			path = gtk_tree_path_new();
			gtk_tree_path_append_index(path,
					temp->song->pos);
			/* iter */
			iter.stamp = cl->stamp;
			iter.user_data = temp;
			iter.user_data2 = GINT_TO_POINTER(temp->song->pos);
			iter.user_data3 = NULL;
			/* changed */
			gtk_tree_model_row_changed(GTK_TREE_MODEL(cl),
					path, &iter);
			gtk_tree_path_free(path);

		} else {
			/* Get the last one */
			for (; temp && !mpd_data_is_last(temp);
					temp =
					mpd_data_get_next_real(temp, FALSE)) ;
			temp = mpd_new_data_struct_append(temp);
			/* set values */
			cl->mpdata = temp;	//mpd_data_get_first(temp);
			temp->type = MPD_DATA_TYPE_SONG;
			/* move song */
			temp->song = cl->pd.data->song;
			cl->pd.data->song = NULL;
			/* calculate time */
			cl->playtime += temp->song->time;
			/* create path */
			path = gtk_tree_path_new();
			gtk_tree_path_append_index(path,
					temp->song->pos);
			/* create iter */
			iter.stamp = cl->stamp;
			iter.user_data = temp;
			iter.user_data2 = GINT_TO_POINTER(temp->song->pos);
			iter.user_data3 = NULL;
			/* send signal */
			gtk_tree_model_row_inserted(GTK_TREE_MODEL(cl),
					path, &iter);
			gtk_tree_path_free(path);

			cl->num_rows++;
		}
		cl->pd.iter = temp;
		cl->pd.data = mpd_data_get_next(cl->pd.data);
		if(cl->pd.data == NULL) cl->pd.iter = NULL;

	}
	else if (cl->pd.state == PD_STATE_CLEANUP)
	{
		int length =  mpd_playlist_get_playlist_length(cl->pd.mi);
		printf("Cleanup\n");
		if (cl->num_rows != length) {
			debug_printf(DEBUG_ERROR, "sync went wrong local: %i remote: %i\n", cl->num_rows,length);
		}                                                                                                           	
		if (cl->mpdata) {
			cl->mpdata = mpd_data_get_first(cl->mpdata);
		}                                               	
		cl->playlist_id = mpd_playlist_get_playlist_id(cl->pd.mi);
		if(cl->pd.tree && length){
			GtkAdjustment *ad = NULL;
			gtk_tree_view_set_model(cl->pd.tree, GTK_TREE_MODEL(cl));
			/* quick hack to make the playlist scroll back to the old position.
			 * Its ugly, and needs fixing 
			 */
			if(cl->pd.cell){
				gtk_tree_view_scroll_to_cell(cl->pd.tree, cl->pd.cell, NULL, TRUE,0,0);
			}
		}
		if(cl->pd.cell) gtk_tree_path_free(cl->pd.cell);
		if(cl->mpdata && length) {
			cl->pd.data = NULL;
			cl->pd.iter = mpd_data_get_first(cl->mpdata);
			if(cfg_get_single_value_as_int_with_default(config, "playlist", "background-fill", TRUE))
			{
				cl->pd.state = PD_STATE_BACKGROUND_FILL;
				printf("Going todo background fill, on low priority\n");
				/* restart it with low priority */
				g_idle_add_full(G_PRIORITY_LOW, playlist_list_state_handler,cl,NULL);
			}
			else cl->pd.state = PD_STATE_NONE;
			return FALSE;
		}
		else{
			cl->pd.state = PD_STATE_NONE;
			return FALSE;
		}
	}
	else if(cl->pd.state == PD_STATE_BACKGROUND_FILL)
	{

		if(cl->pd.iter == NULL){
			cl->pd.state = PD_STATE_NONE;
			printf("Background fill completed\n");
			return TRUE;
		}
		if(cl->pd.iter->song->file[0] == 'x' && cl->pd.iter->song->file[1] == '\0') {
			//double perc = PLAYLIST_LIST(cl)->loaded/(double)(PLAYLIST_LIST(cl)->num_rows);
			mpd_Song *song = mpd_playlist_get_song_from_pos(cl->pd.mi,cl->pd.iter->song->pos);
			/*			if(song->pos != cl->pd.iter->song->pos){
						printf("ERROR at id\n");
						}
						*/			if(song) {
							mpd_freeSong(cl->pd.iter->song);
							cl->pd.iter->song = song;
						}
						PLAYLIST_LIST(cl)->loaded++;

						/*			if(pl3_xml && !(cl->loaded % 150) && perc <= 1 && perc >= 0)gtk_progress_bar_set_fraction(
									glade_xml_get_widget(pl3_xml, "progressbar_dbg"),perc );
									*/		}	                                                                                                                                         	
						cl->pd.iter = mpd_data_get_next_real(cl->pd.iter,FALSE);
	}
	return TRUE;
}

void playlist_list_data_update(CustomList * cl, MpdObj * mi,GtkTreeView *tree) {
	/* Check the state of the state machine */
	/* if the state machine is doing something, tell it to restart */
	if(cl->pd.state != PD_STATE_NONE) {
		cl->pd.state = PD_STATE_RESTART;
	}
	else
	{
		MpdData *data = mpd_playlist_get_changes(mi, cl->playlist_id);
		cl->pd.cl = cl;
		cl->pd.mi = mi;
		cl->pd.data = data;
		cl->pd.tree = tree;
		cl->pd.iter =NULL;
		cl->pd.cell = NULL;
		cl->pd.total_length =  mpd_playlist_get_playlist_length(mi);

		/* if the ammount of rows haven't changed, don't detach view,
		 * It makes it look a bit better.
		 */	
		if(abs(cl->pd.total_length - cl->num_rows) < 50)
		{
			cl->pd.tree= NULL;
		}

		/* Initial Import */
		if(cl->mpdata == NULL)
		{
			if(cl->pd.tree) {
				gtk_tree_view_set_model(tree, NULL);
			}  
			cl->mpdata = data;
			//	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,(GSourceFunc)playlist_list_data_fill_initial,cl,playlist_list_data_update_done);
			cl->pd.state = PD_STATE_INITIAL_FILL;
		}
		else{
			if(cl->pd.tree) {
				gtk_tree_view_get_visible_range(tree, &(cl->pd.cell), NULL);
				gtk_tree_view_set_model(tree, NULL);
			}
			cl->pd.state = PD_STATE_UPDATE;
		}
		g_idle_add_full(G_PRIORITY_LOW,(GSourceFunc)playlist_list_state_handler,cl,NULL);//playlist_list_data_update_done);
	}
}




/* TODO: put this in an idle loop? or is this fast enough? */
/* it seems to be fast enough, for 65k items I see no slowdown */
void playlist_list_clear(CustomList * list,GtkTreeView *tree)
{
	MpdData_real *temp = (MpdData_real *) list->mpdata;
	if(temp == NULL) {
		return;
	}
	if(tree)gtk_tree_view_set_model(tree, NULL);
	/* get the last one */
	for (; !mpd_data_is_last((MpdData *) temp); temp = (MpdData_real *) mpd_data_get_next_real((MpdData *) temp, FALSE)) ;


	if (temp)
		do {
			GtkTreePath *path = gtk_tree_path_new();
			list->num_rows--;
			gtk_tree_path_append_index(path, temp->song->pos);
			gtk_tree_model_row_deleted(GTK_TREE_MODEL(list), path);
			gtk_tree_path_free(path);
			temp = temp->prev;
		} while (temp);
	if(list->mpdata) mpd_data_free(list->mpdata);
	list->mpdata = NULL;
	list->playlist_id = -1;
	list->num_rows = 0;
	list->loaded = 0;
	list->playtime = 0;
	list->current_song_pos = -1;
	if(tree)gtk_tree_view_set_model(tree, GTK_TREE_MODEL(list));

	/* kill update */
	if(list->pd.state != PD_STATE_NONE)
	{
		list->pd.state = PD_STATE_STOP;
	}
}

GType playlist_list_get_type(void)
{
	static GType playlist_list_type = 0;

	if (playlist_list_type) {
		return playlist_list_type;
	}

	/* Some boilerplate type registration stuff */
	if (1) {
		static const GTypeInfo playlist_list_info = {
			sizeof(CustomListClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) playlist_list_class_init,
			NULL,	/* class finalize */
			NULL,	/* class_data */
			sizeof(CustomList),
			0,	/* n_preallocs */
			(GInstanceInitFunc) playlist_list_init
		};

		playlist_list_type =
			g_type_register_static(G_TYPE_OBJECT, "CustomList",
					&playlist_list_info, (GTypeFlags) 0);
	}

	/* Here we register our GtkTreeModel interface with the type system */
	if (1) {
		static const GInterfaceInfo tree_model_info = {
			(GInterfaceInitFunc) playlist_list_tree_model_init,
			NULL,
			NULL
		};

		g_type_add_interface_static(playlist_list_type,
				GTK_TYPE_TREE_MODEL,
				&tree_model_info);
	}

	return playlist_list_type;
}

/*****************************************************************************
 *
 *  playlist_list_class_init: more boilerplate GObject/GType stuff.
 *                          Init callback for the type system,
 *                          called once when our new class is created.
 *
 *****************************************************************************/

static void playlist_list_class_init(CustomListClass * klass)
{
	GObjectClass *object_class;

	parent_class = (GObjectClass *) g_type_class_peek_parent(klass);
	object_class = (GObjectClass *) klass;

	object_class->finalize = playlist_list_finalize;
}

/*****************************************************************************
 *
 *  playlist_list_tree_model_init: init callback for the interface registration
 *                               in playlist_list_get_type. Here we override
 *                               the GtkTreeModel interface functions that
 *                               we implement.
 *
 *****************************************************************************/

static void playlist_list_tree_model_init(GtkTreeModelIface * iface)
{
	iface->get_flags = playlist_list_get_flags;
	iface->get_n_columns = playlist_list_get_n_columns;
	iface->get_column_type = playlist_list_get_column_type;
	iface->get_iter = playlist_list_get_iter;
	iface->get_path = playlist_list_get_path;
	iface->get_value = playlist_list_get_value;
	iface->iter_next = playlist_list_iter_next;
	iface->iter_children = playlist_list_iter_children;
	iface->iter_has_child = playlist_list_iter_has_child;
	iface->iter_n_children = playlist_list_iter_n_children;
	iface->iter_nth_child = playlist_list_iter_nth_child;
	iface->iter_parent = playlist_list_iter_parent;
}

/*****************************************************************************
 *
 *  playlist_list_init: this is called everytime a new custom list object
 *                    instance is created (we do that in playlist_list_new).
 *                    Initialise the list structure's fields here.
 *
 *****************************************************************************/

static void playlist_list_init(CustomList * playlist_list)
{
	playlist_list->n_columns = PLAYLIST_LIST_N_COLUMNS;

	playlist_list->column_types[PLAYLIST_LIST_COL_MPDSONG] = G_TYPE_POINTER;
	playlist_list->column_types[PLAYLIST_LIST_COL_MARKUP] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_PLAYING] = G_TYPE_BOOLEAN;
	playlist_list->column_types[PLAYLIST_LIST_COL_PLAYING_FONT_WEIGHT] = G_TYPE_INT;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_FILE] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_ARTIST] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_ALBUM] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_TITLE] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_TITLEFILE] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_TRACK] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_GENRE] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_NAME] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_COMPOSER] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_DATE] = G_TYPE_STRING;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_LENGTH] = G_TYPE_INT;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_POS] = G_TYPE_INT;
	playlist_list->column_types[PLAYLIST_LIST_COL_SONG_ID] = G_TYPE_INT;
	playlist_list->column_types[PLAYLIST_LIST_COL_ICON_ID] = G_TYPE_STRING;

	playlist_list->num_rows = 0;
	playlist_list->loaded = 0;
	playlist_list->mpdata = NULL;
	playlist_list->playlist_id = -1;
	playlist_list->current_song_pos = -1;

	playlist_list->playtime = 0;
	playlist_list->pd.state = PD_STATE_NONE; 
	/* default markup */
	playlist_list->markup = g_strdup("[%title%[ - %artist%]]|%shortfile%");

	playlist_list->stamp = g_random_int();	/* Random int to check whether an iter belongs to our model */

}

/*****************************************************************************
 *
 *  playlist_list_finalize: this is called just before a custom list is
 *                        destroyed. Free dynamically allocated memory here.
 *
 *****************************************************************************/

static void playlist_list_finalize(GObject * object)
{
	CustomList *cl = PLAYLIST_LIST(object);
	debug_printf(DEBUG_INFO, "Finalize playlist-backend\n");
	/* free all records and free all memory used by the list */
	if(cl->mpdata)mpd_data_free(cl->mpdata);
	if (cl->markup)
		g_free(cl->markup);

	/* must chain up - finalize parent */
	(*parent_class->finalize) (object);
}

/*****************************************************************************
 *
 *  playlist_list_get_flags: tells the rest of the world whether our tree model
 *                         has any special characteristics. In our case,
 *                         we have a list model (instead of a tree), and each
 *                         tree iter is valid as long as the row in question
 *                         exists, as it only contains a pointer to our struct.
 *
 *****************************************************************************/

static GtkTreeModelFlags playlist_list_get_flags(GtkTreeModel * tree_model)
{
	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), (GtkTreeModelFlags) 0);

	return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

/*****************************************************************************
 *
 *  playlist_list_get_n_columns: tells the rest of the world how many data
 *                             columns we export via the tree model interface
 *
 *****************************************************************************/

static gint playlist_list_get_n_columns(GtkTreeModel * tree_model)
{
	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), 0);

	return PLAYLIST_LIST(tree_model)->n_columns;
}

/*****************************************************************************
 *
 *  playlist_list_get_column_type: tells the rest of the world which type of
 *                               data an exported model column contains
 *
 *****************************************************************************/

	static GType
playlist_list_get_column_type(GtkTreeModel * tree_model, gint index)
{
	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), G_TYPE_INVALID);
	g_return_val_if_fail(index < PLAYLIST_LIST(tree_model)->n_columns
			&& index >= 0, G_TYPE_INVALID);

	return PLAYLIST_LIST(tree_model)->column_types[index];
}

/*****************************************************************************
 *
 *  playlist_list_get_iter: converts a tree path (physical position) into a
 *                        tree iter structure (the content of the iter
 *                        fields will only be used internally by our model).
 *                        We simply store a pointer to our CustomRecord
 *                        structure that represents that row in the tree iter.
 *
 *****************************************************************************/

static gboolean playlist_list_get_iter(GtkTreeModel * tree_model,
		GtkTreeIter * iter, GtkTreePath * path)
{
	CustomList *playlist_list = PLAYLIST_LIST(tree_model);
	MpdData *data;
	gint *indices, n, depth;

	g_assert(playlist_list != NULL);
	g_assert(path != NULL);

	indices = gtk_tree_path_get_indices(path);
	depth = gtk_tree_path_get_depth(path);

	/* we do not allow children */
	g_assert(depth == 1);	/* depth 1 = top level; a list only has top level nodes and no children */

	n = indices[0];		/* the n-th top level row */

	/* Check if valid */
	if (n >= playlist_list->num_rows || n < 0)
		return FALSE;

	data = playlist_list->mpdata;	/*mpd_data_get_first(playlist_list->mpdata); */
	g_assert(data != NULL);

	if (n < data->song->pos)
		data = mpd_data_get_first(data);
	for (; data != NULL && data->song->pos != n;
			data = mpd_data_get_next_real(data, FALSE)) ;

	g_assert(data->song->pos == n);

	playlist_list->mpdata = data;

	/* We simply store a pointer to our custom record in the iter */
	iter->stamp = playlist_list->stamp;
	iter->user_data = data;
	iter->user_data2 = GINT_TO_POINTER(data->song->pos);	/* unused */
	iter->user_data3 = NULL;	/* unused */

	return TRUE;
}

/*****************************************************************************
 *
 *  playlist_list_get_path: converts a tree iter into a tree path (ie. the
 *                        physical position of that row in the list).
 *
 *****************************************************************************/

static GtkTreePath *playlist_list_get_path(GtkTreeModel * tree_model,
		GtkTreeIter * iter)
{
	GtkTreePath *path;
	CustomList *playlist_list;
	MpdData *data;

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), NULL);
	g_return_val_if_fail(iter != NULL, NULL);
	g_return_val_if_fail(iter->user_data != NULL, NULL);

	playlist_list = PLAYLIST_LIST(tree_model);

	data = (MpdData *) iter->user_data;

	path = gtk_tree_path_new();
	gtk_tree_path_append_index(path, data->song->pos);

	return path;
}

/*****************************************************************************
 *
 *  playlist_list_get_value: Returns a row's exported data columns
 *                         (_get_value is what gtk_tree_model_get uses)
 *
 *****************************************************************************/

	static void
playlist_list_get_value(GtkTreeModel * tree_model,
		GtkTreeIter * iter, gint column, GValue * value)
{
	MpdData *data = NULL;

	g_return_if_fail(CUSTOM_IS_LIST(tree_model));
	g_return_if_fail(iter != NULL);
	g_return_if_fail(column < PLAYLIST_LIST(tree_model)->n_columns);

	g_value_init(value, PLAYLIST_LIST(tree_model)->column_types[column]);

	data = (MpdData *) iter->user_data;

	g_return_if_fail(data != NULL);

	if(data->song->file[0] == 'x' && data->song->file[1] == '\0') {
		mpd_Song *song = mpd_playlist_get_song(connection,data->song->id);
		mpd_freeSong(data->song);
		data->song = song;
		PLAYLIST_LIST(tree_model)->loaded++;
		/*		if(pl3_xml)gtk_progress_bar_set_fraction(
				glade_xml_get_widget(pl3_xml, "progressbar_dbg"), PLAYLIST_LIST(tree_model)->loaded/(float)(PLAYLIST_LIST(tree_model)->num_rows));
				*/	}	


		switch (column) {
			case PLAYLIST_LIST_COL_MPDSONG:
				g_value_set_pointer(value, data->song);
				break;
			case PLAYLIST_LIST_COL_PLAYING:
				if (data->song->pos ==
						PLAYLIST_LIST(tree_model)->current_song_pos) {
					g_value_set_boolean(value, TRUE);
				} else {
					g_value_set_boolean(value, FALSE);
				}
				break;
			case PLAYLIST_LIST_COL_PLAYING_FONT_WEIGHT:
				if (data->song->pos ==
						PLAYLIST_LIST(tree_model)->current_song_pos) {
					g_value_set_int(value, PANGO_WEIGHT_ULTRABOLD);
				} else {
					g_value_set_int(value, PANGO_WEIGHT_NORMAL);
				}
				break;
			case PLAYLIST_LIST_COL_MARKUP:
				{
					/* we want to go cache this stuff */
					gchar buffer[1024];
					mpd_song_markup(buffer, 1024,
							PLAYLIST_LIST(tree_model)->markup,
							data->song);
					g_value_set_string(value, buffer);
					break;
				}
			case PLAYLIST_LIST_COL_SONG_FILE:
				g_value_set_string(value, data->song->file);
				break;
			case PLAYLIST_LIST_COL_SONG_ARTIST:
				g_value_set_string(value, data->song->artist);
				break;
			case PLAYLIST_LIST_COL_SONG_ALBUM:
				g_value_set_string(value, data->song->album);
				break;
			case PLAYLIST_LIST_COL_SONG_TITLE:
				g_value_set_string(value, data->song->title);
				break;
			case PLAYLIST_LIST_COL_SONG_TITLEFILE:
				if(data->song->title == NULL) {
					gchar *path = g_path_get_basename(data->song->file);
					g_value_set_string(value, path);
					g_free(path);

				}else{
					g_value_set_string(value, data->song->title);
				}
				break;                                       			
			case PLAYLIST_LIST_COL_SONG_GENRE:
				g_value_set_string(value, data->song->genre);
				break;
			case PLAYLIST_LIST_COL_SONG_TRACK:
				g_value_set_string(value, data->song->track);
				break;
			case PLAYLIST_LIST_COL_SONG_NAME:
				g_value_set_string(value, data->song->name);
				break;
			case PLAYLIST_LIST_COL_SONG_COMPOSER:
				g_value_set_string(value, data->song->composer);
				break;
			case PLAYLIST_LIST_COL_SONG_DATE:
				g_value_set_string(value, data->song->date);
				break;
			case PLAYLIST_LIST_COL_SONG_LENGTH:
				g_value_set_int(value, data->song->time);
				break;
			case PLAYLIST_LIST_COL_SONG_POS:
				g_value_set_int(value, data->song->pos);
				break;
			case PLAYLIST_LIST_COL_SONG_ID:
				g_value_set_int(value, data->song->id);
				break;

			case PLAYLIST_LIST_COL_ICON_ID:
				if (data->song->pos ==
						PLAYLIST_LIST(tree_model)->current_song_pos) {
					g_value_set_string(value, "gtk-media-play");
				} else if (strstr(data->song->file, "://")) {
					g_value_set_string(value, "media-stream");
				} else {
					g_value_set_string(value, "media-audiofile");
				}
				break;

		}
}

/*****************************************************************************
 *
 *  playlist_list_iter_next: Takes an iter structure and sets it to point
 *                         to the next row.
 *
 *****************************************************************************/

	static gboolean
playlist_list_iter_next(GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	CustomList *playlist_list;
	MpdData *data = NULL;
	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), FALSE);

	if (iter == NULL || iter->user_data == NULL)
		return FALSE;

	playlist_list = PLAYLIST_LIST(tree_model);

	data = (MpdData *) iter->user_data;

	/* Is this the last record in the list? */
	if (mpd_data_is_last(data))
		return FALSE;
	data = mpd_data_get_next(data);

	iter->stamp = playlist_list->stamp;
	iter->user_data = data;
	iter->user_data2 = GINT_TO_POINTER(GPOINTER_TO_INT(iter->user_data2)+1);

	return TRUE;
}

/*****************************************************************************
 *
 *  playlist_list_iter_children: Returns TRUE or FALSE depending on whether
 *                             the row specified by 'parent' has any children.
 *                             If it has children, then 'iter' is set to
 *                             point to the first child. Special case: if
 *                             'parent' is NULL, then the first top-level
 *                             row should be returned if it exists.
 *
 *****************************************************************************/

	static gboolean
playlist_list_iter_children(GtkTreeModel * tree_model,
		GtkTreeIter * iter, GtkTreeIter * parent)
{
	CustomList *playlist_list;

	g_return_val_if_fail(parent == NULL
			|| parent->user_data != NULL, FALSE);

	/* this is a list, nodes have no children */
	if (parent)
		return FALSE;

	/* parent == NULL is a special case; we need to return the first top-level row */

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), FALSE);

	playlist_list = PLAYLIST_LIST(tree_model);

	/* No rows => no first row */
	if (playlist_list->num_rows == 0)
		return FALSE;

	/* Set iter to first item in list */
	iter->stamp = playlist_list->stamp;
	iter->user_data = mpd_data_get_first((MpdData *) playlist_list->mpdata);

	return TRUE;
}

/*****************************************************************************
 *
 *  playlist_list_iter_has_child: Returns TRUE or FALSE depending on whether
 *                              the row specified by 'iter' has any children.
 *                              We only have a list and thus no children.
 *
 *****************************************************************************/

	static gboolean
playlist_list_iter_has_child(GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	return FALSE;
}

/*****************************************************************************
 *
 *  playlist_list_iter_n_children: Returns the number of children the row
 *                               specified by 'iter' has. This is usually 0,
 *                               as we only have a list and thus do not have
 *                               any children to any rows. A special case is
 *                               when 'iter' is NULL, in which case we need
 *                               to return the number of top-level nodes,
 *                               ie. the number of rows in our list.
 *
 *****************************************************************************/

	static gint
playlist_list_iter_n_children(GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	CustomList *playlist_list;

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), -1);
	g_return_val_if_fail(iter == NULL || iter->user_data != NULL, FALSE);

	playlist_list = PLAYLIST_LIST(tree_model);

	/* special case: if iter == NULL, return number of top-level rows */
	if (!iter)
		return playlist_list->num_rows;

	return 0;		/* otherwise, this is easy again for a list */
}

/*****************************************************************************
 *
 *  playlist_list_iter_nth_child: If the row specified by 'parent' has any
 *                              children, set 'iter' to the n-th child and
 *                              return TRUE if it exists, otherwise FALSE.
 *                              A special case is when 'parent' is NULL, in
 *                              which case we need to set 'iter' to the n-th
 *                              row if it exists.
 *
 *****************************************************************************/

	static gboolean
playlist_list_iter_nth_child(GtkTreeModel * tree_model,
		GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
	CustomList *playlist_list;
	MpdData *data;

	g_return_val_if_fail(CUSTOM_IS_LIST(tree_model), FALSE);

	playlist_list = PLAYLIST_LIST(tree_model);

	/* a list has only top-level rows */
	if (parent)
		return FALSE;

	/* special case: if parent == NULL, set iter to n-th top-level row */

	if (n >= playlist_list->num_rows)
		return FALSE;

	/*: if n > data->song->pos then up, else get the first item */
	data = (MpdData *) playlist_list->mpdata;
	if (data->song->pos > n)
		data = mpd_data_get_first(data);
	for (; data->song->pos != n &&
			!mpd_data_is_last(data); data = mpd_data_get_next(data)) ;

	g_assert(data != NULL);
	g_assert(data->song->pos == n);
	/* to improve lineair searches? */
	playlist_list->mpdata = data;
	iter->stamp = playlist_list->stamp;
	iter->user_data = data;

	return TRUE;
}

/*****************************************************************************
 *
 *  playlist_list_iter_parent: Point 'iter' to the parent node of 'child'. As
 *                           we have a list and thus no children and no
 *                           parents of children, we can just return FALSE.
 *
 *****************************************************************************/

	static gboolean
playlist_list_iter_parent(GtkTreeModel * tree_model,
		GtkTreeIter * iter, GtkTreeIter * child)
{
	return FALSE;
}

/*****************************************************************************
 *
 *  playlist_list_new:  This is what you use in your own code to create a
 *                    new custom list tree model for you to use.
 *
 *****************************************************************************/

CustomList *playlist_list_new(void)
{
	CustomList *newcustomlist;

	newcustomlist = (CustomList *) g_object_new(CUSTOM_TYPE_LIST, NULL);

	g_assert(newcustomlist != NULL);

	return newcustomlist;
}
