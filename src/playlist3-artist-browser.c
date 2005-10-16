/*
 *Copyright (C) 2004-2005 Qball Cow <Qball@qballcow.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade.h>

#include "main.h"
#include "strfsong.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-current-playlist-browser.h"
#include "open-location.h"
#include "vfs_download.h"
#include "config1.h"


void pl3_artist_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp);











extern config_obj *config;
extern GladeXML *pl3_xml;



enum{
	PL3_AB_ARTIST,
	PL3_AB_TYPE,
	PL3_AB_TITLE,
	PL3_AB_ICON,
	PL3_AB_ROWS
};


/* internal */
GtkWidget *pl3_ab_tree = NULL;
GtkListStore *pl3_ab_store = NULL;
GtkWidget *pl3_ab_sw = NULL;

int pl3_artist_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))	
	{
		return FALSE;                                                                                           	
	}
	return TRUE;
}

void pl3_artist_browser_init()
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column = NULL;
	GValue value;
	pl3_ab_store = gtk_list_store_new (PL3_AB_ROWS, 
			GTK_TYPE_STRING, /* path to file */
			GTK_TYPE_INT,	/* type, FILE/PLAYLIST/FOLDER  */
			GTK_TYPE_STRING,	/* title to display */
			GTK_TYPE_STRING); /* icon type */



	renderer = gtk_cell_renderer_pixbuf_new ();

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,renderer,"stock-id", PL3_AB_ICON,NULL);
	memset(&value, 0, sizeof(value));
	/* set value for ALL */
	g_value_init(&value, G_TYPE_FLOAT);
	g_value_set_float(&value, 0.0);
	g_object_set_property(G_OBJECT(renderer), "yalign", &value); 

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,renderer,"text", PL3_AB_TITLE, NULL);


	/* set up the tree */
	pl3_ab_tree= gtk_tree_view_new_with_model(GTK_TREE_MODEL(pl3_ab_store));
	/* insert the column in the tree */
	gtk_tree_view_append_column (GTK_TREE_VIEW (pl3_ab_tree), column);                                         	
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pl3_ab_tree), FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(pl3_ab_tree), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_ab_tree)), GTK_SELECTION_MULTIPLE);

	/* setup signals */
	g_signal_connect(G_OBJECT(pl3_ab_tree), "row-activated",G_CALLBACK(pl3_artist_browser_row_activated), NULL); 
//	g_signal_connect(G_OBJECT(pl3_ab_tree), "button-press-event", G_CALLBACK(pl3_artist_browser_button_press_event), NULL);
//	g_signal_connect(G_OBJECT(pl3_ab_tree), "button-release-event", G_CALLBACK(pl3_artist_browser_button_release_event), NULL);
//	g_signal_connect(G_OBJECT(pl3_ab_tree), "key-press-event", G_CALLBACK(pl3_browser_file_playlist_key_press), NULL);

	/* set up the scrolled window */
	pl3_ab_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_ab_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_ab_sw), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(pl3_ab_sw), pl3_ab_tree);

	/* set initial state */
	printf("initialized current playlist treeview\n");
	g_object_ref(G_OBJECT(pl3_ab_sw));
}

















































/****************************************************************************************
 *  ARTIST BROWSER									*
 *  When mpd 0.12 is stable this function is deprecated, but needed for pre-0.12 mpd's  *
 ****************************************************************************************/

void pl3_artist_browser_add()
{
   GtkTreeIter iter,child;
   gtk_tree_store_append(pl3_tree, &iter, NULL);
   gtk_tree_store_set(pl3_tree, &iter, 
	 PL3_CAT_TYPE, PL3_BROWSE_ARTIST,
	 PL3_CAT_TITLE, "Browse Artists",        	
	 PL3_CAT_INT_ID, "",
	 PL3_CAT_ICON_ID, "media-artist",
	 PL3_CAT_PROC, FALSE,
	 PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
   /* add fantom child for lazy tree */
   gtk_tree_store_append(pl3_tree, &child, &iter);
}


long unsigned pl3_artist_browser_view_folder(GtkTreeIter *iter_cat)
{
	char *artist, *string;
	GtkTreeIter iter;
	GtkTreePath *path = NULL;
	int depth = 0;
	long unsigned time =0;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter_cat, 2 , &artist, 1,&string, -1);
	if (check_connection_state ())
		return 0;


	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), iter_cat);
	if(path == NULL)
	{
		printf("Failed to get path\n");
		return 0;
	}
	depth = gtk_tree_path_get_depth(path) -1;                      	
	gtk_tree_path_free(path);
	if(artist == NULL || string == NULL)
	{
		return 0;
	}
	if(depth == 0)
	{
		/*lowest level, do nothing */
		/* fill artist list */
		MpdData *data = mpd_playlist_get_artists(connection);

		while(data != NULL)
		{	
			gtk_list_store_append (pl3_ab_store,&iter);
			gtk_list_store_set (pl3_ab_store,&iter,
					PL3_AB_ARTIST, data->value.artist,
					PL3_AB_TYPE, PL3_ENTRY_ARTIST, /* the field */
					PL3_AB_TITLE, data->value.artist, /* the artist name, if(1 and 2 together its an artist field) */
					PL3_AB_ICON, "media-artist",
					-1);

			data = mpd_data_get_next(data);
		}
		return 0;
	}
	if(depth == 1)
	{
		int albums = 0;
		MpdData *data = mpd_playlist_get_albums(connection,artist);
		while(data != NULL){
			gtk_list_store_append (pl3_ab_store, &iter);
			gtk_list_store_set (pl3_ab_store,&iter,
					PL3_AB_ARTIST, artist,
					PL3_AB_TYPE, PL3_ENTRY_ALBUM,
					PL3_AB_TITLE, data->value.album,
					PL3_AB_ICON, "media-album", 
					-1);
			data = mpd_data_get_next(data);
		}


		data = mpd_playlist_find(connection, MPD_TABLE_ARTIST, artist, TRUE);
		/* artist is selected */
		while(data != NULL)
		{
			if(data->type == MPD_DATA_TYPE_SONG)
			{
				if (data->value.song->album == NULL
						|| strlen (data->value.song->album) == 0)
				{
					gchar buffer[1024];
					char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
					strfsong (buffer, 1024,markdata,data->value.song);
					cfg_free_string(markdata);
					if(data->value.song->time != MPD_SONG_NO_TIME)
					{
						time += data->value.song->time;
					}
					if(data->value.song->file == NULL)
					{
						debug_printf(DEBUG_WARNING,"pl3_browser_view_folder: crap mpdSong has no file attribute.\n");
					}
					gtk_list_store_append (pl3_ab_store, &iter);
					gtk_list_store_set (pl3_ab_store, &iter,
							PL3_AB_TITLE, 	buffer,
							PL3_AB_ARTIST, 	data->value.song->file,
							PL3_AB_TYPE, 	PL3_ENTRY_SONG,
							PL3_AB_ICON,	"media-audiofile",
							-1);
				}
				else albums++;
			}
			data = mpd_data_get_next(data);
		}

		if(!albums)
		{
			if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &iter, iter_cat))
			{
				gtk_tree_store_remove(pl3_tree, &iter);      		
			}
		}                                                                                  		
	}
	else if(depth ==2)
	{
		/* artist and album is selected */
		MpdData *data = mpd_playlist_find(connection,MPD_TABLE_ALBUM, string, TRUE);
		while (data != NULL)
		{
			if(data->type == MPD_DATA_TYPE_SONG)
			{
				if (data->value.song->artist!= NULL
						&& !g_utf8_collate (data->value.song->artist, artist))
				{
					gchar buffer[1024];
					char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
					strfsong (buffer, 1024,markdata,data->value.song);
					cfg_free_string(markdata);
					if(data->value.song->time != MPD_SONG_NO_TIME)
					{
						time += data->value.song->time;
					}
					if(data->value.song->file == NULL)
					{
						debug_printf(DEBUG_WARNING,"pl3_browser_view_folder: crap mpdSong has no file attribute.\n");
					}
					gtk_list_store_append (pl3_ab_store, &iter);
					gtk_list_store_set (pl3_ab_store, &iter,
							PL3_AB_TITLE, buffer,
							PL3_AB_ARTIST, data->value.song->file,
							PL3_AB_TYPE, PL3_ENTRY_SONG,
							PL3_AB_ICON,"media-audiofile",
							-1);

				}
			}
			data = mpd_data_get_next(data);
		}

	}
	return time;
}


void pl3_artist_browser_fill_tree(GtkTreeIter *iter)
{
	char *artist, *alb_artist;
	int depth =0;
	GtkTreePath *path = NULL;
	GtkTreeIter child,child2;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree),iter, 1, &artist,2,&alb_artist, -1);
	gtk_tree_store_set(pl3_tree, iter, 4, TRUE, -1);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), iter);
	if(path == NULL)
	{
		printf("Failed to get path\n");
		return;
	}
	depth = gtk_tree_path_get_depth(path) -1;                      	
	gtk_tree_path_free(path);


	if (!mpd_check_connected(connection))
	{
		return;
	}
	if(depth == 0)
	{
		/* fill artist list */
		MpdData *data = mpd_playlist_get_artists(connection);

		while(data != NULL)
		{	
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, PL3_BROWSE_ARTIST,
					1, data->value.artist, /* the field */
					2, data->value.artist, /* the artist name, if(1 and 2 together its an artist field) */
					3, "media-artist",
					4, FALSE,
					PL3_CAT_ICON_SIZE,1,
					-1);
			gtk_tree_store_append(pl3_tree, &child2, &child);

			data = mpd_data_get_next(data);
		}
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gtk_tree_store_remove(pl3_tree, &child); 
		}
	}
	/* if where inside a artist */
	else if(depth == 1)
	{
		MpdData *data = mpd_playlist_get_albums(connection,artist);
		while(data != NULL){
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, PL3_BROWSE_ARTIST,
					1, data->value.album,
					2, artist,
					3, "media-album", 
					4, TRUE, 
					PL3_CAT_ICON_SIZE,1,
					-1);
			data = mpd_data_get_next(data);
		}

		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gtk_tree_store_remove(pl3_tree, &child); 
		}
	}
}



void pl3_artist_browser_add_folder()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;

	if(!mpd_check_connected(connection))
	{
		return;
	}
	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		char *artist, *title;
		gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &artist, PL3_CAT_TITLE,&title, -1);
		if(!artist || !title)
		{
			return;
		}
		if(strlen(artist) && !g_utf8_collate(artist,title))
		{
			/* artist selected */
			gchar *message = g_strdup_printf("Added songs from artist '%s'",artist);
			MpdData * data = mpd_playlist_find(connection, MPD_TABLE_ARTIST, artist, TRUE);
			while (data != NULL)
			{                    
				if(data->type == MPD_DATA_TYPE_SONG)
				{				
					mpd_playlist_queue_add(connection, data->value.song->file);
				}
				data = mpd_data_get_next(data);
			}
			pl3_push_statusbar_message(message);
			g_free(message);

		}
		else
		{
			/* album selected */
			/* fetch all songs by this album and check if the artist is right. from mpd and add them to the add-list */
			gchar *message = g_strdup_printf("Added songs from album '%s' ",title);
			MpdData *data = mpd_playlist_find(connection, MPD_TABLE_ALBUM, title, TRUE);
			while (data != NULL)
			{
				if(data->type == MPD_DATA_TYPE_SONG)
				{
					if (!g_utf8_collate (data->value.song->artist, artist))
					{
						mpd_playlist_queue_add(connection,data->value.song->file);
					}
				}
				data = mpd_data_get_next(data);
			}
			pl3_push_statusbar_message(message);
			g_free(message);

		}

		/* if there are items in the add list add them to the playlist */
		mpd_playlist_queue_commit(connection);
	}

}

void pl3_artist_browser_replace_folder()
{
	pl3_clear_playlist();
	pl3_artist_browser_add_folder();
	mpd_player_play(connection);
}

void pl3_artist_browser_category_key_press(GdkEventKey *event)
{
	if(event->state == GDK_CONTROL_MASK && event->keyval == GDK_Insert )
	{
		pl3_artist_browser_replace_folder();
	}
	else if (event->keyval == GDK_Insert)
	{
		pl3_artist_browser_add_folder();
	}
}

void pl3_artist_browser_show_info(GtkTreeIter *iter)
{
	char *path;
	MpdData *data;
	gtk_tree_model_get (GTK_TREE_MODEL(pl3_ab_store), iter, PL3_SONG_ID, &path, -1);
	data = mpd_playlist_find_adv(connection,TRUE,MPD_TAG_ITEM_FILENAME,path,-1);
	while(data != NULL)                                                            	
	{
		if(data->type == MPD_DATA_TYPE_SONG)
		{
			call_id3_window_song(mpd_songDup(data->value.song));
		}
		data = mpd_data_get_next(data);                                        
	}
}

void pl3_artist_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
	GtkTreeIter iter;
	gchar *song_id;
	gint r_type;
	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp);
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_SONG_ID,&song_id, PL3_SONG_POS, &r_type, -1);
	if(song_id == NULL)
	{

		return;
	}
	if (r_type&(PL3_ENTRY_ARTIST|PL3_ENTRY_ALBUM))
	{
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
		GtkTreeIter iter;

		if(gtk_tree_selection_get_selected(selec,&model, &iter))
		{
			GtkTreeIter citer;
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_view_expand_row(GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "cat_tree")), path, FALSE);
			gtk_tree_path_free(path);
			if(gtk_tree_model_iter_children(model, &citer, &iter))
			{
				do{
					char *name = NULL;
					gtk_tree_model_get(model, &citer, 2, &name, -1);
					if(strcmp(name, song_id) == 0)
					{
						gtk_tree_selection_select_iter(selec,&citer);						
						path = gtk_tree_model_get_path(model, &citer);
						gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "cat_tree")), path,NULL,TRUE,0.5,0);
						gtk_tree_path_free(path);
						break;

					}
				}while(gtk_tree_model_iter_next(model, &citer));
			}	
		}
	}
	else
	{
		pl3_push_statusbar_message("Added a song");
		mpd_playlist_queue_add(connection, song_id);
	}
	mpd_playlist_queue_commit(connection);            
}

void pl3_artist_browser_category_selection_changed(GtkTreeView *tree,GtkTreeIter *iter)
{
	long unsigned time= 0;
	gchar *string;        			
	gtk_list_store_clear(pl3_ab_store);	
	time = pl3_artist_browser_view_folder(iter);
	gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl3_ab_store));
	string = format_time(time);
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
	g_free(string);
}

void pl3_artist_browser_selected()
{
	if(pl3_ab_tree == NULL)
	{
		pl3_artist_browser_init();
	}

	gtk_container_add(GTK_CONTAINER(glade_xml_get_widget(pl3_xml, "browser_container")), pl3_ab_sw);
	gtk_widget_show_all(pl3_ab_sw);
}
void pl3_artist_browser_unselected()
{
	gtk_container_remove(GTK_CONTAINER(glade_xml_get_widget(pl3_xml, "browser_container")), pl3_ab_sw);
}
