#include <gtk/gtk.h>
#include <string.h>
#include <glade/glade.h>
#include <stdlib.h>
#include "libmpdclient.h"
#include "main.h"
#include "misc.h"
#include "strfsong.h"
#include "playlist2.h"
#define TITLE_LENGTH 42
scrollname scroll = {NULL, NULL, NULL, 0,0, TRUE};
/* wrapper functions for the title entry box. */
PangoLayout *layout = NULL, *time_layout = NULL;
guint expose_display_id = 0;


void time_exposed(GtkWidget *window)
{
	gint height, width;
	pango_layout_get_size(time_layout, &width, &height);
	width  = width/PANGO_SCALE;
	height = height/PANGO_SCALE;
	gdk_draw_rectangle(GDK_DRAWABLE(window->window),
			window->style->base_gc[GTK_STATE_NORMAL],
			TRUE,
			0,0,
			50,23);
	gdk_draw_layout(GDK_DRAWABLE(window->window),
			window->style->text_gc[GTK_STATE_NORMAL],
			MAX(0,(50-width)/2),MAX(0, (23-height)/2) ,
			time_layout);

	gdk_draw_rectangle(GDK_DRAWABLE(window->window),
			window->style->dark_gc[GTK_STATE_NORMAL],
			FALSE,
			0,0,
			49,22);
}



void display_exposed(GtkWidget *window)
{
	int width, height;
	g_signal_handler_block(G_OBJECT(window), expose_display_id);
	pango_layout_get_size(layout, &width, &height);
	width  = width/PANGO_SCALE;
	height = height/PANGO_SCALE;

	gdk_draw_rectangle(GDK_DRAWABLE(window->window),
			window->style->base_gc[GTK_STATE_NORMAL],
			TRUE,
			0,0,
			260,23);                            
	if(width <= 255)
	{
		gdk_draw_layout(GDK_DRAWABLE(window->window), 
			window->style->text_gc[GTK_STATE_NORMAL], 
			3, MAX(0, (23-height)/2),
			layout);
	}
	else{
		if(width-scroll.pos > 260)
		{
		gdk_draw_layout(GDK_DRAWABLE(window->window), 
			window->style->text_gc[GTK_STATE_NORMAL], 
			-scroll.pos, MAX(0, (23-height)/2),
			layout);
		}
		else{
			gdk_draw_layout(GDK_DRAWABLE(window->window), 
					window->style->text_gc[GTK_STATE_NORMAL], 
					-scroll.pos, MAX(0, (23-height)/2),
					layout);



			gdk_draw_layout(GDK_DRAWABLE(window->window), 
					window->style->text_gc[GTK_STATE_NORMAL], 
					(-scroll.pos+width),MAX(0, (23-height)/2),
					layout);
		}

		if((width-scroll.pos) < 0)
		{
			scroll.pos = 0;

		}



	}

	gdk_draw_rectangle(GDK_DRAWABLE(window->window),
			window->style->dark_gc[GTK_STATE_NORMAL],
			FALSE,
			0,0,
			259,22);                              
	g_signal_handler_unblock(G_OBJECT(window), expose_display_id);
}	
gboolean update_msg()
{
	int width;
	/* scroll will be -1 when there is getting stuff updated. hopefully this fixes the nasty segfault in pango*/
	if(scroll.exposed)
	{
		if(scroll.msg != NULL) g_free(scroll.msg);
		/* set the correct message in the msg box. and set posistion on 0 */
		if(scroll.popup_msg != NULL)
		{
			scroll.msg = g_strdup(scroll.popup_msg);
		}	
		else if(scroll.base_msg != NULL)
		{
			scroll.msg = g_strdup(scroll.base_msg);
		}
		else scroll.msg = g_strdup("Gnome Music Player Client");
		scroll.pos = 0;
		scroll.up = 0;
		pango_layout_set_text(layout, scroll.msg, -1);
		pango_layout_get_size(layout, &width, NULL);
		width = width/PANGO_SCALE;
		if(width > 255)
		{
			char *temp= scroll.msg;
			scroll.msg = g_strdup_printf("%s  ***  ", scroll.msg);
			g_free(temp);
			pango_layout_set_text(layout, scroll.msg, -1);
		}

	}
	/* scroll the song text */
	{
		GtkWidget *window = glade_xml_get_widget(xml_main_window, "entry_image");
		pango_layout_get_size(layout, &width, NULL);

		width = width/PANGO_SCALE;

		if(width > 255)
		{
			scroll.pos+=4;

		}
		gtk_widget_queue_draw(window);

		scroll.exposed = FALSE;
	}
	/* return true .. so that the it keeps going */
	return TRUE;
}

void msg_set_base(gchar *msg)
{
	if(msg == NULL) return;
	/* don't update when its the same string :) */
	if(scroll.msg != NULL)
	{
		if(!strcmp(scroll.msg, msg)) return;
	}
	if(scroll.base_msg != NULL)
	{
		g_free(scroll.base_msg);
		scroll.base_msg = NULL;
	}
	if(!g_utf8_validate(msg, -1, NULL))
	{
		scroll.base_msg = g_strdup("No valid UTF-8. Please check youre locale");
	}
	else	scroll.base_msg = g_strdup(msg);

	scroll.exposed = TRUE;
}

void msg_push_popup(gchar *msg)
{
	if(msg == NULL) return;
	if(scroll.popup_msg != NULL)
	{
		g_free(scroll.popup_msg);
		scroll.popup_msg = NULL;
	}
	if(!g_utf8_validate(msg, -1, NULL))
	{
		scroll.popup_msg = g_strdup("No valid UTF-8. Please check youre locale");
	}
	else	scroll.popup_msg = g_strdup(msg);
	scroll.exposed = TRUE;
}

void msg_pop_popup()
{
	if(scroll.popup_msg != NULL)
	{
		g_free(scroll.popup_msg);
		scroll.popup_msg = NULL;
	}
	scroll.exposed = TRUE;
}

/* this updates the player.. this is called from the update function */
/* conlock isnt locked at this point.. so If I do decide to get anything lock it */

int update_player()
{
	/* update the volume slider */
	if(info.conlock) return TRUE;
	if(info.volume != info.status->volume)
	{
		GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "volume_slider");
		gtk_range_set_value(scale, (double) info.status->volume);
		info.volume = info.status->volume;
	}    

	/* things that only need to be updated during playing */
	if(info.status->state == MPD_STATUS_STATE_PLAY)
	{
		/* update the progress bar */
		{
			GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "progress_slider");
			gdouble  prog = ((double)info.status->elapsedTime/(double)info.status->totalTime)*100;
			gtk_range_set_value(scale, prog);
		}
		/* update the time box */
		{

			//			GtkWidget *entry = glade_xml_get_widget(xml_main_window, "time_entry");
			int e_min = (int)(info.status->elapsedTime/60);
			int e_sec = info.status->elapsedTime - 60*e_min;
			int r_min = (int)((info.status->totalTime- info.status->elapsedTime)/60);
			int r_sec = info.status->totalTime - info.status->elapsedTime - r_min*60;
			gchar *buf = NULL;
			if(info.time_format == TIME_FORMAT_ELAPSED)
			{
				buf = g_strdup_printf("%02i:%02i", abs(e_min), abs(e_sec));
			}
			else if (info.time_format == TIME_FORMAT_REMAINING) buf = g_strdup_printf("-%02i:%02i", abs(r_min), abs(r_sec));
			else{
				if(info.status->totalTime <= 0)
				{
					buf = g_strdup("n/a");
				}
				else
				{
			       	buf = g_strdup_printf("%3.1f %%", (double)((double)info.status->elapsedTime/(double)info.status->totalTime)*100);
				}
			}
			//		gtk_entry_set_text(GTK_ENTRY(entry), buf);
			pango_layout_set_text(time_layout, buf, -1);
			gtk_widget_queue_draw(glade_xml_get_widget(xml_main_window, "time_image"));

			g_free(buf);
		}
	}
	/* update the song title */

	if(info.song != info.status->song && info.status->state != MPD_STATUS_STATE_STOP)
	{
		GList *node = g_list_nth(info.playlist, info.status->song);
		mpd_Song *song;
		if(node != NULL){
			song = node->data;
			/* make a global song */
			if(info.status->state != MPD_STATUS_STATE_PLAY && info.status->state != MPD_STATUS_STATE_PAUSE)
			{
				msg_set_base("Gnome Music Player Client");
				gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), "Gnome Music Player Client");
			}
			else
			{
				g_print("update display\n");
				info.cursong = song;
				gchar buffer[1024];
				strfsong(buffer, 1024, preferences.markup_main_display, song);
				msg_set_base(buffer);
				
/*				info.cursong = song;
				if(song->artist != NULL && song->title != NULL)
				{
					gchar *buf = NULL;
					if(song->title != NULL && song->artist != NULL) buf  = g_strdup_printf("%s - %s", song->title, song->artist);
					else buf = g_strdup("GMPC - Invalid UTF-8. please check youre locale");
					msg_set_base(buf);
					gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), buf);
					g_free(buf);
				}
				else
				{
					gchar *buf  = remove_extention_and_basepath(song->file);
					gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), buf);
					msg_set_base(buf);
					g_free(buf);
				}
				*/
			}
		}
	}


	/* update if state changes */
	if(info.state != info.status->state)
	{
		GtkWidget *image = glade_xml_get_widget(xml_main_window, "play_button_image");
		if(info.status->state == MPD_STATUS_STATE_STOP || info.status->state == MPD_STATUS_STATE_UNKNOWN)
		{
			GtkWidget *entry;
			msg_set_base("GMPC - Stopped");
			gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), "Gnome Music Player Client");
			if(info.time_format == TIME_FORMAT_ELAPSED)
			{
				pango_layout_set_text(time_layout, "00:00", -1);
			}
			else if(info.time_format == TIME_FORMAT_REMAINING)
			{
				pango_layout_set_text(time_layout, "-00:00", -1);

			}
			else	{
				pango_layout_set_text(time_layout, "0.0 %", -1);
			}
			gtk_widget_queue_draw(glade_xml_get_widget(xml_main_window, "time_image"));
			entry =  glade_xml_get_widget(xml_main_window, "progress_slider");
			gtk_range_set_value(GTK_RANGE(entry), 0);

			info.song = -1;
		}
		if(info.status->state == MPD_STATUS_STATE_PLAY) gtk_image_set_from_file(GTK_IMAGE(image), PIXMAP_PATH"/media-pause.png");
		else gtk_image_set_from_file(GTK_IMAGE(image), PIXMAP_PATH"/media-play.png");

		info.state = info.status->state;
	}
	/* update random and repeat button */
	/* lock it to stop them from toggling and triggering another toggle*/
	if(info.status->repeat != gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "rep_button"))))
	{
		info.conlock = TRUE;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "rep_button")), info.status->repeat);
		info.conlock = FALSE;
	}
	if(info.status->random != gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "rand_button"))))
	{
		info.conlock = TRUE;
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "rand_button")), info.status->random);
		info.conlock = FALSE;
	}
	return FALSE;
}



/* start seeking in the song..  only allow this when youre playing or paused */
/* block it other wise. */
/* everything is blocked until the seek is done. */
/* show time to seek to in entry box */
int progress_seek_start()
{
	if(info.conlock) return TRUE;
	info.conlock = TRUE;
	if(info.status->state != MPD_STATUS_STATE_PLAY && info.status->state != MPD_STATUS_STATE_PAUSE)
	{
		info.conlock = FALSE;
		return TRUE;
	}
	return FALSE;
}

void change_progress_update()
{
	if(info.conlock)
	{
		if(info.connection != NULL)
		{
			GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "progress_slider");
			gchar *buf = NULL;
			gdouble value = gtk_range_get_value(scale);
			int newtime = (int)(info.status->totalTime*(double)(value/100));
			if(info.time_format == TIME_FORMAT_ELAPSED)
			{
				int min = (int)(newtime/60);
				int sec = newtime - 60*min;
				int t_min = (int)(info.status->totalTime/60);
				int t_sec = info.status->totalTime - 60*t_min;
				buf = g_strdup_printf("Seek to %02i:%02i/%02i:%02i", min, sec, t_min, t_sec);
			}
			else if (info.time_format == TIME_FORMAT_REMAINING)
			{
				int t_min = (int)(info.status->totalTime/60);
				int t_sec = info.status->totalTime - 60*t_min;
				int min = (int)((info.status->totalTime -newtime)/60);
				int sec = (info.status->totalTime -newtime) - 60*min;
				buf = g_strdup_printf("Seek to -%02i:%02i/%02i:%02i", min, sec, t_min, t_sec);
			}	
			else buf = g_strdup_printf("Seek to %3.1f%%", value);
			msg_push_popup(buf);
			g_free(buf);
		}
		/* do this so the title gets updated again, even if it doesnt need scrolling */
		scroll.pos = -1;
	}
}    

/* apply seek changes */
int progress_seek_stop()
{
	msg_pop_popup();
	if(info.connection == NULL)return TRUE;
	else if(info.status->state == MPD_STATUS_STATE_PLAY || info.status->state == MPD_STATUS_STATE_PAUSE)
	{
		GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "progress_slider");
		gdouble value = gtk_range_get_value(scale);
		int change = (int)(info.status->totalTime*(double)(value/100));
		mpd_sendSeekCommand(info.connection,info.status->song, change);
		mpd_finishCommand(info.connection);
		if(check_for_errors()) return FALSE;
		info.status->elapsedTime = change;	
		info.conlock = FALSE;
	}
	return FALSE;
}

/* if the volume slider is pressed (mouse button)  it holds the update so I Can display the volume in */
/* the entry box and it doesn't tries to move my volume slider while sliding */
/* also if volume isnt "slidable" block the user from changing it */
int volume_change_start()
{
	if(info.conlock) return TRUE;
	if(info.volume == -1) return TRUE;
	info.conlock = TRUE;
	return FALSE;
}

/* if the volume changes say it in the entry box.. this looks nice :) */    
void volume_change_update()
{
	if(info.connection != NULL && info.conlock)
	{
		GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "volume_slider");
		gdouble value = gtk_range_get_value(scale);
		gchar *buf = g_strdup_printf("Volume %i%%", (int)value);
		msg_push_popup(buf);
		g_free(buf);

		mpd_sendSetvolCommand(info.connection, (int)value);
		mpd_finishCommand(info.connection);
		if(check_for_errors()) return;

		/* do this so the title gets updated again, even if it doesnt need scrolling */
		/* it does look ugly .. need to find a better way */
		scroll.pos = -1;
	}
	else if(info.connection != NULL)
	{
		GtkRange *scale = (GtkRange *)glade_xml_get_widget(xml_main_window, "volume_slider");
		gdouble value = gtk_range_get_value(scale);
		if(value != info.volume)
		{
			mpd_sendSetvolCommand(info.connection, (int)value);
			mpd_finishCommand(info.connection);        
			if(check_for_errors()) return;
		}
	}
}
/* apply changes and give mpd free */
int volume_change_stop()
{
	msg_pop_popup();
	if(info.connection == NULL) return TRUE;
	else 
	{
		info.conlock = FALSE;    
	}
	return FALSE;
}

/* change the time format between elapsing and remaining and percentage */
void time_format_toggle()
{
	info.time_format++;
	if(info.time_format > 2) info.time_format = 0;
}


/* the id3 info screen */
void id3_info()
{
	if(info.connection == NULL) return;
	call_id3_window(info.status->song);
}

void style_changed(GtkWidget *window, GtkStyle *prev, PangoLayout *lay)
{
	pango_layout_context_changed(lay);

}
/* create the player and connect signals */
void create_player()
{
	xml_main_window = glade_xml_new(GLADE_PATH"gmpc.glade", "main_window", NULL);
	gtk_widget_set_app_paintable(glade_xml_get_widget(xml_main_window, "entry_image"),TRUE);
	gtk_widget_set_app_paintable(glade_xml_get_widget(xml_main_window, "time_image"),TRUE);
	layout = gtk_widget_create_pango_layout(glade_xml_get_widget(xml_main_window, "entry_image"), "");
	time_layout = gtk_widget_create_pango_layout(glade_xml_get_widget(xml_main_window, "time_image"), "");
	g_signal_connect(G_OBJECT(glade_xml_get_widget(xml_main_window, "entry_image")), 
			"style-set", G_CALLBACK(style_changed), layout);
	expose_display_id = g_signal_connect(G_OBJECT(glade_xml_get_widget(xml_main_window, "entry_image")), 
			"expose-event", G_CALLBACK(display_exposed), layout);

	g_signal_connect(G_OBJECT(glade_xml_get_widget(xml_main_window, "time_image")), 
			"style-set", G_CALLBACK(style_changed), time_layout);
	g_signal_connect(G_OBJECT(glade_xml_get_widget(xml_main_window, "time_image")), 
			"expose-event", G_CALLBACK(time_exposed), time_layout);

	pango_layout_set_text(time_layout, "00:00", -1);
	/* check for errors and axit when there is no gui file */
	if(xml_main_window == NULL)  g_error("Couldnt initialize GUI. Please check installation\n");
	glade_xml_signal_autoconnect(xml_main_window);
	gtk_timeout_add(300, (GSourceFunc)update_msg, NULL);
}

