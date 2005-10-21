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
#include <config.h>

#include "plugin.h"
#include "main.h"
#include "strfsong.h"
#include "misc.h"
#include "open-location.h"
#include "config1.h"

#include "playlist3.h"
/* every part split out over multiple files */
#include "playlist3-find-browser.h"
#include "playlist3-file-browser.h"
#include "playlist3-artist-browser.h"
#include "playlist3-current-playlist-browser.h"
#include "playlist3-custom-stream-browser.h"
#include "playlist3-tag-browser.h"
#include "playlist3-osb-browser.h"


/*static GtkTargetEntry drag_types[] =
{
   { "pm_data", GTK_TARGET_SAME_APP, 100},
};
*/
int old_type = -1;

extern config_obj *config;
GladeXML *pl3_xml = NULL;
GtkTreeStore *pl3_tree = NULL;
GtkListStore *pl3_store = NULL;
GtkListStore *pl2_store= NULL;

/* size */
GtkAllocation pl3_wsize = { 0,0,0,0};
int pl3_hidden = TRUE;
void pl2_save_playlist ();
void pl3_detach_playlist();
void pl3_dp_scroll_current_song(int songid);


/****************************************************************/
/* We want to move this to mpdinteraction 			*/
/****************************************************************/
void pl3_clear_playlist()
{
   mpd_playlist_clear(connection);
}

void pl3_shuffle_playlist()
{
   mpd_playlist_shuffle(connection);
}

/* custom search and match function, this is a workaround for the problems with in gtk+-2.6 */
gboolean pl3_playlist_tree_search_func(GtkTreeModel *model, gint column, const char *key, GtkTreeIter *iter)
{
   char *value= NULL;
   char *lkey, *lvalue;
   int ret = TRUE;
   if(iter == NULL)
   {
      return TRUE;
   }
   gtk_tree_model_get(model, iter, column, &value, -1);
   if(value == NULL || key == NULL)
   {
      return TRUE;
   }
   lkey = g_utf8_casefold(key,-1);
   lvalue = g_utf8_casefold(value,-1);
   /* str str seems faster then the glib function todo this */
   if(strstr(lvalue,lkey) != NULL)
   {
      ret = FALSE;
   }
   g_free(lkey);
   g_free(lvalue);
   return ret;
}


/********************************************************************
 * Misc functions 
 */


/* Get the type of the selected row.. 
 * -1 means no row selected 
 */
int  pl3_cat_get_selected_browser()
{
   GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
   GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
   GtkTreeIter iter;

   if(gtk_tree_selection_get_selected(selec,&model, &iter))
   {
      gint type;
      gtk_tree_model_get(model, &iter, 0, &type, -1);
      if(type >= PL3_NTYPES || type < 0)
      {
	 return -1;
      }
      return type;
   }
   return -1;
}

/**************************************************
 * PLaylist Tree
 */
void pl3_browse_delete_playlist(GtkToggleButton *bt, char *string)
{
   /* create a warning message dialog */
   GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW
	    (glade_xml_get_widget
	     (pl3_xml, "pl3_win")),
	    GTK_DIALOG_MODAL,
	    GTK_MESSAGE_WARNING,
	    GTK_BUTTONS_NONE,
	    _("Are you sure you want to clear the selected playlist?"));
   gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_NO,
	 GTK_RESPONSE_CANCEL, GTK_STOCK_YES,
	 GTK_RESPONSE_OK, NULL);
   gtk_dialog_set_default_response (GTK_DIALOG (dialog),
	 GTK_RESPONSE_CANCEL);

   switch (gtk_dialog_run (GTK_DIALOG (dialog)))
   {
      case GTK_RESPONSE_OK:
	 mpd_playlist_delete(connection, string);
	 pl3_cat_sel_changed();

   }
   gtk_widget_destroy (GTK_WIDGET (dialog));
}

int pl3_playlist_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))	
	{
		return FALSE;                                                                                           	
	}
	return TRUE;

}
int pl3_playlist_button_release_event(GtkTreeView *tree, GdkEventButton *event)
{
	return TRUE;
}


void pl3_playlist_row_activated(GtkTreeView *tree, GtkTreePath *tp, GtkTreeViewColumn *col)
{
	gint type = pl3_cat_get_selected_browser();
	if(type == -1 || check_connection_state())
	{
		return;
	}
	else if (type == PL3_BROWSE_CUSTOM_STREAM)
	{
		pl3_custom_stream_row_activated(tree,tp);
	}
	else if (type == PL3_BROWSE_XIPH)
	{
		pl3_osb_browser_row_activated(tree,tp);
	}
}

/**************************************************
 * Category Tree 
 */
void pl3_reinitialize_tree()
{
	int i;
	if(pl3_xml == NULL) return;
	GtkTreePath *path = gtk_tree_path_new_from_string("0");
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
	gtk_tree_store_clear(pl3_tree);
	/* add the current playlist */
	pl3_current_playlist_browser_add();
	pl3_file_browser_add();       	
	pl3_artist_browser_add();
	pl3_find_browser_add();
//	pl3_osb_browser_add();
//	pl3_custom_stream_add();
	pl3_custom_tag_browser_add();

	for(i=0; i< num_plugins;i++)
	{
		if(plugins[i]->plugin_type == GMPC_PLUGIN_PL_BROWSER)
		{
			printf("adding plugin id: %i %i\n", i,plugins[i]->id^PLUGIN_ID_MARK);
			plugins[i]->browser->add(glade_xml_get_widget(pl3_xml, "cat_tree"));
		}
	}

	
	gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));

	gtk_tree_selection_select_path(sel, path);               		
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
	gtk_tree_path_free(path);
}

void pl3_disconnect()
{
	if(pl3_xml != NULL)
	{
		pl3_reinitialize_tree();
	}
}

void pl3_cat_row_activated(GtkTreeView *tree, GtkTreePath *tp, GtkTreeViewColumn *col)
{
	gint type = pl3_cat_get_selected_browser();
	if(check_connection_state())
	{
		return;
	}
	/* nothing valid, so return */
	if(type == -1)
	{
		return;
	}

	else if(type == PL3_CURRENT_PLAYLIST)
	{

		pl3_current_playlist_browser_scroll_to_current_song();
	}
	else
	{
		if(gtk_tree_view_row_expanded(tree,tp))
		{
			gtk_tree_view_collapse_row(tree,tp);
		}
		else
		{
			gtk_tree_view_expand_row(tree,tp,FALSE);
		}
	}
}


void pl3_cat_row_expanded(GtkTreeView *tree, GtkTreeIter *iter, GtkTreePath *path)
{
	gint type,read;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter, 0, &type,4,&read, -1);
	/* check if the connection isnt down */
	if(check_connection_state())
	{
		/* if connection down, don't let the treeview open */
		gtk_tree_view_collapse_row(tree,path);
		return;
	}
	if(!read)
	{
		if(type == PL3_BROWSE_FILE)
		{
			pl3_file_browser_fill_tree(iter);
		}
		else if (type == PL3_BROWSE_ARTIST)
		{
			pl3_artist_browser_fill_tree(iter);
		}
		else if (type == PL3_BROWSE_CUSTOM_TAG)
		{
			pl3_custom_tag_browser_fill_tree(iter);
		}
	}
	/* avuton's Idea */
	if(cfg_get_single_value_as_int_with_default(config, "playlist", "open-to-position",0))
	{
		gtk_tree_view_scroll_to_cell(tree, path,gtk_tree_view_get_column(tree,0),TRUE,0.5,0);
	}
}


void pl3_cat_sel_changed()
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeView *tree = (GtkTreeView *) glade_xml_get_widget (pl3_xml, "cat_tree");

	GtkWidget *container = glade_xml_get_widget(pl3_xml, "browser_container");
	
	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		gint type;
		gtk_tree_model_get(model, &iter, 0, &type, -1);
		if(old_type != type )
		{
			if(old_type == PL3_CURRENT_PLAYLIST)
			{
                		pl3_current_playlist_browser_unselected();
                	}
			else if (old_type == PL3_BROWSE_FILE)
			{
				pl3_file_browser_unselected();
			}
			else if (old_type == PL3_BROWSE_ARTIST)
			{
				pl3_artist_browser_unselected(container);
			}
			else if (old_type == PL3_FIND)
			{
				pl3_find_browser_unselected(container);
			}
			else if (old_type == PL3_BROWSE_CUSTOM_TAG)
			{
				pl3_tag_browser_unselected(container);
			}
			else if (old_type|PLUGIN_ID_MARK && old_type > 0)
			{
				printf("old_type: %i\n", old_type);
				plugins[old_type^PLUGIN_ID_MARK]->browser->unselected(container);
			}
		}

		if(type == PL3_CURRENT_PLAYLIST)
		{
			if(old_type != type)
			{
				pl3_current_playlist_browser_selected();
			}
		}
		else if (type == PL3_BROWSE_FILE)
		{
			if(old_type != type) pl3_file_browser_selected();
			pl3_file_browser_cat_sel_changed(GTK_TREE_VIEW(tree),&iter);
		}
		else if (type == PL3_BROWSE_ARTIST)
		{
			if(old_type != type) pl3_artist_browser_selected(container);
			pl3_artist_browser_category_selection_changed(tree,&iter);
		}
		else if (type == PL3_BROWSE_CUSTOM_TAG)
		{
			if(old_type != type) pl3_tag_browser_selected(container);
			pl3_custom_tag_browser_category_selection_changed(tree,&iter);
		}
		else if (type == PL3_FIND)
		{
			if(old_type != type){
			       	pl3_find_browser_selected(container);
				pl3_find_browser_category_selection_changed(tree,&iter);
			}
		}
		else if(type == PL3_BROWSE_XIPH)
		{
			pl3_osb_browser_category_selection_changed(tree,&iter);
		}
		else if(type == PL3_BROWSE_CUSTOM_STREAM)
		{
			pl3_custom_stream_category_selection_changed(tree,&iter);
		}
		else if(type|PLUGIN_ID_MARK)
		{
			printf("plugins: %s\n", plugins[type^PLUGIN_ID_MARK]->name);
			if(old_type != type)plugins[type^PLUGIN_ID_MARK]->browser->selected(container);
			plugins[type^PLUGIN_ID_MARK]->browser->cat_selection_changed(GTK_WIDGET(tree),&iter);
		}
		old_type = type;
	}
}


/* handle right mouse clicks on the cat tree view */
/* gonna be a big function*/
int pl3_cat_tree_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))	
	{
		return FALSE;                                                                                           	
	}
	return TRUE;

}
int pl3_cat_tree_button_release_event(GtkTreeView *tree, GdkEventButton *event)
{
	gint type  = pl3_cat_get_selected_browser();
	if(type == -1 || check_connection_state())
	{
		/* no selections, or no usefull one.. so propagate the signal */
		return FALSE;
	}

	if(event->button != 3)
	{
		/* if its not the right mouse button propagate the signal */
		return FALSE;
	}
	/* if it's the current playlist */
	if (type == PL3_CURRENT_PLAYLIST)
	{
		pl3_current_playlist_browser_cat_menu_popup(tree,event);
	}
	else if (type == PL3_BROWSE_FILE)
	{
		pl3_file_browser_cat_popup(tree,event);
	}
	else if (type == PL3_BROWSE_ARTIST)
	{
		pl3_artist_browser_cat_popup(tree,event);
	}
	else if (type == PL3_BROWSE_XIPH)
	{
		/* here we have:  Add. Replace*/
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
		GtkTreeIter iter;
		char *id;

		/* add the add widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_osb_browser_add_source), NULL);		

		if(gtk_tree_selection_get_selected(selec,&model, &iter))
		{
			gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &id, -1);
			if(strlen(id) > 0)
			{
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE,NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_osb_browser_del_source), NULL);		

				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH,NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_osb_browser_refresh), NULL);		
			}
		}
		/* show everything and popup */
		gtk_widget_show_all(menu);                                                        		
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);

	}
	else if (type ==  PL3_BROWSE_CUSTOM_TAG)
	{
		pl3_custom_tag_browser_right_mouse_menu(event);
	}

	return TRUE;
}


/**********************************************************
 * MISC
 */
int pl3_window_key_press_event(GtkWidget *mw, GdkEventKey *event)
{
	if(event->keyval == GDK_f && event->state != GDK_CONTROL_MASK)
	{
		/* disabled because of problems with gtk 2.6 */
		return FALSE;
	}
	else if (event->keyval == GDK_w && event->state == GDK_CONTROL_MASK)
	{
		pl3_close();
	}
	else if (event->keyval == GDK_Escape)
	{
		pl3_close();
	}

	if(check_connection_state())
	{
		return FALSE;
	}
	/* on F1 move to current playlist */
	if (event->keyval == GDK_F1)
	{
		GtkTreeIter iter;
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
//		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "playlist_tree"));

		/* select the current playlist */
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_tree), &iter))
		{
			gtk_tree_selection_select_iter(sel, &iter);               		
		}
	}
	else if (event->keyval == GDK_F2)
	{
		GtkTreePath *path = gtk_tree_path_new_from_string("1");
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));

		gtk_tree_selection_select_path(sel, path);               		
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
		gtk_tree_path_free(path);
	}
	else if (event->keyval == GDK_F3)
	{
		GtkTreePath *path = gtk_tree_path_new_from_string("2");
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));

		gtk_tree_selection_select_path(sel, path);               		
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
		gtk_tree_path_free(path);
	}
	else if (event->keyval == GDK_F4 || event->keyval == GDK_j)
	{
		if(event->keyval == GDK_j)
		{
			pl3_playlist_search();		
		}
		else
		{
			GtkTreePath *path = gtk_tree_path_new_from_string("3");
			GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
			gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));
			gtk_tree_selection_select_path(sel, path);               	                                             		
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
			gtk_tree_path_free(path);
		}
	}

	/* default gmpc/xmms/winamp key's*/
	else if (event->keyval == GDK_z && event->state == 0)
	{
		prev_song();	
	}
	else if ((event->keyval == GDK_x || event->keyval == GDK_c )&& event->state == 0)
	{
		play_song();	
	}
	else if (event->keyval == GDK_v && event->state == 0)
	{
		stop_song();
	}
	else if (event->keyval == GDK_b && event->state == 0)
	{
		next_song();
	}
	else
	{
		return FALSE;
	}

	/* don't propagate */
	return TRUE;
}



int pl3_cat_key_press_event(GtkWidget *mw, GdkEventKey *event)
{
	/* call default */
	gint type = pl3_cat_get_selected_browser();
	if(type == PL3_BROWSE_FILE)
	{
		pl3_file_browser_cat_key_press(event);
	}
	else if (type == PL3_BROWSE_ARTIST)
	{
		pl3_artist_browser_category_key_press(event);
	}
	else if (type == PL3_BROWSE_CUSTOM_TAG)
	{

	}

	return pl3_window_key_press_event(mw,event);
}

int pl3_playlist_key_press_event(GtkWidget *mw, GdkEventKey *event)
{
	gint type = pl3_cat_get_selected_browser();
	if (event->keyval == GDK_Delete && type == PL3_BROWSE_CUSTOM_STREAM)
	{
		pl3_custom_stream_remove();	
		return TRUE;
	}
	/* call default */
	return pl3_window_key_press_event(mw,event);
}

void pl3_playlist_search()
{
	if(!mpd_check_connected(connection))
	{
		return;
	}
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2"))))
	{
		create_playlist3();
	}
	else
	{
		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
	}
	if(pl3_xml != NULL)
	{
		GtkTreePath *path = gtk_tree_path_new_from_string("3");
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));
		gtk_tree_selection_select_path(sel, path);               	
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
		gtk_tree_path_free(path);
		pl3_find_browser_search_playlist();		
	}
}

int pl3_pop_statusbar_message(gpointer data)
{
	gint id = GPOINTER_TO_INT(data);
	gtk_statusbar_pop(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), id);
	return FALSE;
}


void pl3_push_statusbar_message(char *mesg)
{
	gint id = gtk_statusbar_get_context_id(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), mesg);
	/* message auto_remove after 5 sec */
	g_timeout_add(5000,(GSourceFunc)pl3_pop_statusbar_message, GINT_TO_POINTER(id));
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), id,mesg);
}

void pl3_push_rsb_message(gchar *string)
{
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
}

int pl3_close()
{
	if(pl3_xml != NULL)
	{
		gtk_window_get_position(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.x, &pl3_wsize.y);
		gtk_window_get_size(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.width, &pl3_wsize.height);

		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2"))))
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2")), FALSE);
			return TRUE;
		}
		gtk_widget_hide(glade_xml_get_widget(pl3_xml, "pl3_win"));	
		pl3_hidden = TRUE;
		return TRUE;
	}
	return TRUE;
}
void updating_changed(MpdObj *mi, int updating)
{
	char *mesg = "MPD database is updating";
	gint id = gtk_statusbar_get_context_id(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), mesg);
	/* message auto_remove after 5 sec */


	if(pl3_xml == NULL) return;
	if(updating >0)
	{
		gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), id,mesg);
		gtk_widget_show(glade_xml_get_widget(pl3_xml, "image_updating"));
	}
	else
	{
		gtk_statusbar_pop(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), id);
		gtk_widget_hide(glade_xml_get_widget(pl3_xml, "image_updating"));
	}
}

/* create the playlist view 
 * This is done only once, for the rest its hidden, but still there
 */

void create_playlist3 ()
{
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	GtkTreeSelection *sel;
	GtkTreeViewColumn *column = NULL;
	GtkTreeIter iter;
	pl3_hidden = FALSE;
	if(pl3_xml != NULL)
	{
		gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));
		if(!pl3_wsize.x || !pl3_wsize.y) gtk_window_move(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), pl3_wsize.x, pl3_wsize.y);
		if(!pl3_wsize.height || !pl3_wsize.width) gtk_window_resize(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),pl3_wsize.width, pl3_wsize.height);
		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
		return;
	}
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2"))))
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2")),TRUE);
		return;
	}

	/* load gui desciption */
	pl3_xml = glade_xml_new (GLADE_PATH "playlist3.glade", "pl3_win", NULL);
	if(pl3_xml == NULL)
	{
		debug_printf(DEBUG_ERROR, "create_playlist3: Failed to open playlist3.glade.\n");
		return;
	}
	if (pl3_tree == NULL)
	{
		/* song id, song title */
		pl3_tree = gtk_tree_store_new (PL3_CAT_NROWS, 
				GTK_TYPE_INT,	/* row type, see free_type struct */
				GTK_TYPE_STRING, /* display name */
				GTK_TYPE_STRING,/* full path and stuff for backend */
				GTK_TYPE_STRING, /* icon id */
				GTK_TYPE_BOOL,  /* cat proc */
				GTK_TYPE_UINT,  /* icon size */
				GTK_TYPE_STRING /* browser markup */
				);
	}

	tree = glade_xml_get_widget (pl3_xml, "cat_tree");
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (pl3_tree));

	/* draw the column with the songs */
	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer, "stock-id",3,"stock-size",5,NULL);

	renderer = gtk_cell_renderer_text_new ();
	/* insert the column in the tree */
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer, "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 1);
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(pl3_cat_sel_changed), NULL);

	pl3_reinitialize_tree();

	gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));

	/* connect signals that are defined in the gui description */
	glade_xml_signal_autoconnect (pl3_xml);



	mpd_signal_set_updating_changed(connection, (void *)updating_changed, NULL);
	if(mpd_status_db_is_updating(connection))
	{
		updating_changed(connection, 1);
	}



	
	/* select the current playlist */
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_tree), &iter))
	{
		gtk_tree_selection_select_iter(sel, &iter);
	}
}

/* toggles the playlist on or off */
/* this should be placed on the player's toggle button */
gboolean toggle_playlist3(GtkToggleButton *tb)
{
	if(gtk_toggle_button_get_active(tb))
	{
		create_playlist3();
	}
	else
	{
		if(pl3_xml == NULL) return FALSE;
		pl3_close();
	}
	return TRUE;
}

void pl3_highlight_song_change ()
{
	GtkTreeIter iter;
	gchar *temp;
	/* check if there is a connection */
	if (!mpd_check_connected (connection))
	{
		return;
	}

	/* unmark the old pos if it exists */
	if (info.old_pos != -1)
	{
		/* create a string so I can get the right iter */
		temp = g_strdup_printf ("%i", info.old_pos);
		if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL (pl2_store), &iter, temp))
		{
			gint song_id = 0, song_type = 0;
			/* check if we have the song we want */
			gtk_tree_model_get (GTK_TREE_MODEL (pl2_store), &iter, SONG_ID,&song_id,SONG_TYPE,&song_type, -1);
			/* if the old song is the new song (so tags updated) quit */
			if (song_id == mpd_player_get_current_song_id(connection))
			{
				g_free (temp);
				return;
			}
			/* unhighlight the song */
			gtk_list_store_set (pl2_store, &iter, WEIGHT_INT,PANGO_WEIGHT_NORMAL, 
					SONG_STOCK_ID, (!song_type)?"media-audiofile":"media-stream",    			
					-1);
		}
		g_free (temp);
		/* reset old pos */
		info.old_pos = -1;
	}
	/* check if we need to highlight a song */
	if (mpd_player_get_state(connection) > MPD_PLAYER_STOP && mpd_player_get_current_song_pos(connection) >= 0) 
	{
		temp = g_strdup_printf ("%i", mpd_player_get_current_song_pos(connection));
		if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (pl2_store), &iter, temp))
		{
			gint pos;
			gtk_tree_model_get (GTK_TREE_MODEL (pl2_store), &iter, SONG_POS,
					&pos, -1);
			/* check if we have the right song, if not, print an error */
			if (pos != mpd_player_get_current_song_pos(connection))
			{
				debug_printf(DEBUG_ERROR,"pl3_highlight_song_change: Error %i %i should be the same\n", pos,mpd_player_get_current_song_pos(connection));
			}
			gtk_list_store_set (pl2_store, &iter, WEIGHT_INT,PANGO_WEIGHT_ULTRABOLD,SONG_STOCK_ID,"gtk-media-play", -1);

			if(cfg_get_single_value_as_int_with_default(config, "playlist3", "st_cur_song", 0) && 
					pl3_xml != NULL && PL3_CURRENT_PLAYLIST == pl3_cat_get_selected_browser())
			{
				pl3_current_playlist_browser_scroll_to_current_song();
			}
		}
		g_free (temp);
		/* set highlighted position */
		info.old_pos = mpd_player_get_current_song_pos(connection);
	}
}

/* create a dialog that allows the user to save the current playlist */
void pl2_save_playlist ()
{
	gchar *str;
	GladeXML *xml = NULL;
	int run = TRUE;
	/* check if the connection is up */
	if (check_connection_state ())
		return;

	/* create the interface */
	xml = glade_xml_new (GLADE_PATH "playlist3.glade", "save_pl", NULL);

	/* run the interface */
	do
	{
		switch (gtk_dialog_run (GTK_DIALOG (glade_xml_get_widget (xml, "save_pl"))))
		{
			case GTK_RESPONSE_OK:
				run = FALSE;
				/* if the users agrees do the following: */
				/* get the song-name */
				str = (gchar *)	gtk_entry_get_text (GTK_ENTRY
						(glade_xml_get_widget (xml, "pl-entry")));
				/* check if the user entered a name, we can't do withouth */
				/* TODO: disable ok button when nothing is entered */
				/* also check if there is a connection */
				if (strlen (str) != 0 && !check_connection_state ())
				{
					if(mpd_playlist_save(connection, str) == MPD_O_PLAYLIST_EXIST )
					{
						gchar *errormsg = g_strdup_printf(_("<i>Playlist <b>\"%s\"</b> already exists\nOverwrite?</i>"), str);
						gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml, "label_error")), errormsg);
						gtk_widget_show(glade_xml_get_widget(xml, "hbox5"));
						/* ask to replace */
						gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "pl-entry")), FALSE);
						switch (gtk_dialog_run (GTK_DIALOG (glade_xml_get_widget (xml, "save_pl"))))
						{
							case GTK_RESPONSE_OK:
								run = FALSE;
								mpd_playlist_delete(connection, str);
								mpd_playlist_save(connection,str);
								break;
							default:
								run = TRUE;
						}
						/* return to stare */
						gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "pl-entry")), TRUE);
						gtk_widget_hide(glade_xml_get_widget(xml, "hbox5"));

						g_free(errormsg);
					}
				}
				break;
			default:
				run = FALSE;
		}
	}while(run);
	/* destroy the window */
	gtk_widget_destroy (glade_xml_get_widget (xml, "save_pl"));

	/* unref the gui description */
	g_object_unref (xml);
}

void pl3_playlist_changed()
{
	int type = 0;
	if(pl3_xml == NULL) return;
	type  = pl3_cat_get_selected_browser();
	if(type ==  PL3_CURRENT_PLAYLIST)
	{
		pl3_current_playlist_browser_playlist_changed();
	}
	if(type == PL3_FIND)
	{
		pl3_find_browser_playlist_changed();
	}
}
