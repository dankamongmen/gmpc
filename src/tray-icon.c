#include <gtk/gtk.h>
#include <string.h>
#include <glade/glade.h>
#include "eggtrayicon.h"
#include "libmpdclient.h"
#include "playlist2.h"
#include "song-browser.h"
#include "main.h"
#include "misc.h"
#include "strfsong.h"

EggTrayIcon *tray_icon = NULL;
GladeXML *tray_xml = NULL;
GdkPixbuf *logo = NULL;
GtkTooltips *tps = NULL;

/* size main window (I know odd place)*/
GtkAllocation player_wsize = {0,0,0,0};
GtkWidget *tip = NULL;
PangoLayout *tray_layout_tooltip = NULL;

guint tray_timeout = -1;
/**/
gchar *tray_get_tooltip_text()
{
	GString *string = g_string_new("");
	gchar result[1024];
	gchar *retval;
	int id;
	if(info.connection != NULL && info.status != NULL && info.mpdSong != NULL && info.status->state != MPD_STATUS_STATE_STOP) 
	{
		strfsong(result, 1024,
				"[<b>Stream:</b>\t%name%\n&[<b>Artist:</b>\t%artist%\n]"
				"<b>Title:</b>\t%title%[\n<b>Album:</b>\t%album%]]"
				"|<b>Stream:</b>\t%name%|[<b>Artist:</b>\t%artist%\n]"
				"<b>Title:</b>\t%title%[\n<b>Album:</b>\t%album%]"
				"|<b>Filename:</b>\t%shortfile%"
				"|", info.mpdSong);
		g_string_append(string, result);
		/* add time */
		if(info.status->totalTime != 0)
		{
			g_string_append_printf(string, "\n<b>Time:</b>\t%02i:%02i/%02i:%02i",
					info.status->elapsedTime/60, info.status->elapsedTime %60,
					info.status->totalTime/60, info.status->totalTime %60);
		}
		else
		{
			g_string_append_printf(string, "\n<b>Time:</b>\t%02i:%02i",
					info.status->elapsedTime/60, info.status->elapsedTime %60);
		}
	}
	else
	{
		g_string_append(string,"Gnome Music Player Client");
	}

	/* escape all & signs... needed for pango */
	for(id=0;id < string->len; id++)
	{
		if(string->str[id] == '&')
		{
			g_string_insert(string, id+1, "amp;");
			id++;
		}
	}
	/* return a string (that needs to be free'd */
	retval = string->str;
	g_string_free(string, FALSE);
	return retval;
}

void tray_paint_tip(GtkWidget *widget, GdkEventExpose *event)
{
	int width, height;
	GtkStyle *style;
	char *tooltiptext = tray_get_tooltip_text();
	if(tooltiptext == NULL) tooltiptext = g_strdup("oeps");
	pango_layout_set_markup(tray_layout_tooltip, tooltiptext, strlen(tooltiptext));
	pango_layout_set_wrap(tray_layout_tooltip, PANGO_WRAP_WORD);
	pango_layout_set_width(tray_layout_tooltip, 500000);
	style = widget->style;

//	gtk_paint_flat_box (style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
//			NULL, widget, "tooltip", 0, 0, -1, -1);                     	
	gdk_draw_rectangle(widget->window, widget->style->fg_gc[GTK_STATE_NORMAL], TRUE,0,0,widget->allocation.width, widget->allocation.height);

	

	gdk_draw_arc(widget->window, widget->style->bg_gc[GTK_STATE_NORMAL], TRUE, 1,1,19,19,0,64*360);
	gdk_draw_arc(widget->window, widget->style->bg_gc[GTK_STATE_NORMAL], TRUE, widget->allocation.width-20,1,19,19,0,64*360);
	gdk_draw_arc(widget->window, widget->style->bg_gc[GTK_STATE_NORMAL], TRUE, 1,widget->allocation.height-20,19,19,0,64*360);
	gdk_draw_arc(widget->window, widget->style->bg_gc[GTK_STATE_NORMAL], TRUE, widget->allocation.width-20,widget->allocation.height-20,19,19,0,64*360);
	gdk_draw_rectangle(widget->window, widget->style->bg_gc[GTK_STATE_NORMAL], TRUE, 11,1,widget->allocation.width-22, widget->allocation.height-2);
	gdk_draw_rectangle(widget->window, widget->style->bg_gc[GTK_STATE_NORMAL], TRUE, 1,11,widget->allocation.width-2, widget->allocation.height-22);














	gtk_paint_layout (style, widget->window, GTK_STATE_NORMAL, TRUE,
			NULL, widget, "tooltip", 4, 4, tray_layout_tooltip);

	if(info.connection != NULL && info.status != NULL)
	{

		pango_layout_get_size(tray_layout_tooltip, &width, &height);
		width= PANGO_PIXELS(width);
		height= PANGO_PIXELS(height);


		if(info.status->totalTime != 0)
		{


			gdk_draw_rectangle(widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
					FALSE,4,height+5, width ,8);                              		
			width = (info.status->elapsedTime/(float)info.status->totalTime)*width;
			gdk_draw_rectangle(widget->window, 
					widget->style->mid_gc[GTK_STATE_NORMAL],
					TRUE,4,height+5, width ,8);
			gdk_draw_rectangle(widget->window, 
					widget->style->fg_gc[GTK_STATE_NORMAL],
					FALSE,4,height+5, width ,8);
		}
	}
	g_free(tooltiptext);


	return;
}



/*
 *
 */


gboolean tray_motion_cb (GtkWidget *tv, GdkEventCrossing *event, gpointer n)
{
	int width,height;
	GdkRectangle msize;
	int x,y;
	int monitor = gdk_screen_get_monitor_at_window(
			gtk_widget_get_screen(tv), tv->window);
	if(tip != NULL) return FALSE;


	char *tooltiptext = NULL;

	tooltiptext = tray_get_tooltip_text();
	gdk_screen_get_monitor_geometry(
			gtk_widget_get_screen(tv), monitor, &msize);


	tip = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_set_app_paintable(tip, TRUE);




	gtk_window_set_resizable(GTK_WINDOW(tip), FALSE);
	gtk_widget_set_name(tip, "gtk-tooltips");
	g_signal_connect(G_OBJECT(tip), "expose_event",
			G_CALLBACK(tray_paint_tip), NULL);
	gtk_widget_ensure_style (tip);

	tray_layout_tooltip = gtk_widget_create_pango_layout (tip, NULL);
	pango_layout_set_wrap(tray_layout_tooltip, PANGO_WRAP_WORD);
	pango_layout_set_width(tray_layout_tooltip, 500000);
	pango_layout_set_markup(tray_layout_tooltip, tooltiptext, strlen(tooltiptext));


	pango_layout_get_size(tray_layout_tooltip, &width, &height);
	width= PANGO_PIXELS(width)+8;
	height= PANGO_PIXELS(height)+8+12;
	gtk_widget_set_usize(tip, width,height);


	/* calculate position */
	x = (int)event->x_root - event->x+tv->allocation.width/2 - width/2;
	y = (int)event->y_root+(tv->allocation.height - event->y) +5;	

	/* check borders left, right*/	
	if((x+width) > msize.width+msize.x)
	{	
		x = msize.x+msize.width-(width);
	}
	else if(x < 0)
	{
		x= 0;
	}
	/* check up down.. if can't place it below, place it above */
	if( y+height > msize.height+msize.y) 
	{
		y = event->y_root - event->y -5-(height);
	}
	/* place the window */
	gtk_window_move(GTK_WINDOW(tip), x, y);

	gtk_widget_show_all(tip);	


	/* testing */
	{
		GdkBitmap *gbm = (GdkBitmap *)gdk_pixmap_new(GDK_DRAWABLE(tip->window), width+1,height+1,1);
		GdkGCValues val;
		GdkGC *gc = gdk_gc_new(gbm);
		GdkGC *gc1 = gdk_gc_new(gbm);
		gdk_gc_get_values(gc, &val);
		gdk_gc_set_foreground(gc, &val.background);
		gdk_draw_rectangle(gbm, gc1, TRUE, 0,0,width, height);

		gdk_draw_rectangle(gbm, gc, TRUE, 10,0,width-20, height);
		gdk_draw_rectangle(gbm, gc, TRUE, 0,10,width, height-20);
		gdk_draw_arc(gbm, gc, TRUE, 0,0,20,20,0,64*360);
		gdk_draw_arc(gbm, gc, TRUE, width-20,0,20,20,0,64*360);
		gdk_draw_arc(gbm, gc, TRUE, 0,height-20,20,20,0,64*360);
		gdk_draw_arc(gbm, gc, TRUE, width-20,height-20,20,20,0,64*360);

		g_object_unref(gc);
		g_object_unref(gc1);




		gtk_widget_shape_combine_mask(tip, (GdkBitmap *) gbm, 0,0);

	}










		if(tray_timeout != -1) g_source_remove(tray_timeout);
		tray_timeout = g_timeout_add(800, (GSourceFunc)
		gtk_widget_queue_draw, tip);
		
	return TRUE;
}

void tray_leave_cb (GtkWidget *w, GdkEventCrossing *e, gpointer n)
{

	if(tray_timeout != -1) g_source_remove(tray_timeout);
	tray_timeout = -1;
	if(tip != NULL)
	{
		gtk_widget_destroy(tip);
		g_object_unref(tray_layout_tooltip);
	}

	tip = NULL;
}


/* this draws the actual image to the window */
/* gtk will call this function when the image is exposed and the data is gone */
void exposed_signal(GtkWidget *event)
{
	gdk_draw_rectangle(event->window, event->style->bg_gc[GTK_STATE_NORMAL], TRUE, 0,0,20,20);

	gdk_draw_pixbuf(event->window,event->style->bg_gc[GTK_STATE_NORMAL],logo, 0,0,0,0,20,20, GDK_RGB_DITHER_MAX,0,0);		

	if(info.status == NULL)
	{
		return;
	}

	if(info.hidden == TRUE)
	{
		GdkPoint points[6] = {{19,13},{19,19}, {19,16},{16,16},{16,13},{16,19}};
		gdk_draw_lines(event->window, event->style->fg_gc[GTK_STATE_NORMAL], points, 6);
	}

	if(info.state == MPD_STATUS_STATE_STOP)
	{
		GdkPoint points[5] = {{4,1},{4,8}, {11,8},{11,1},{4,1}};
		gdk_draw_polygon(event->window, event->style->fg_gc[GTK_STATE_NORMAL],TRUE, points, 5);
	}
	else if(info.state == MPD_STATUS_STATE_PLAY)
	{
		GdkPoint points[4] = {{5,1},{5,11}, {10,6},{5,1}};
		gdk_draw_polygon(event->window, event->style->fg_gc[GTK_STATE_NORMAL],TRUE, points, 4);
	}
	else if(info.state == MPD_STATUS_STATE_PAUSE)
	{
		GdkPoint points[5] = {{4,1},{4,8}, {6,8},{6,1},{4,1}};
		GdkPoint points2[5] = {{8,1},{8,8}, {10,8},{10,1},{8,1}};	
		gdk_draw_polygon(event->window, event->style->fg_gc[GTK_STATE_NORMAL],TRUE, points, 5);
		gdk_draw_polygon(event->window, event->style->fg_gc[GTK_STATE_NORMAL],TRUE, points2, 5);
	}
	else return;

}

/* this function updates the trayicon on changes */
void update_tray_icon()
{
	if(!info.do_tray) return;
	if(info.status != NULL) 
	{
		if(info.song != info.status->song)
		{
			gchar *str = NULL;
			if(info.status->state == MPD_STATUS_STATE_PLAY || info.status->state == MPD_STATUS_STATE_PAUSE)
			{
				if(info.mpdSong != NULL)
				{
					gchar buffer[1024];
					strfsong(buffer, 1024, preferences.markup_main_display, info.mpdSong);
					str = g_strdup(buffer);
				}

			}
			else str = g_strdup(_("Gnome Music Player Client"));
			//			gtk_tooltips_set_tip(tps, GTK_WIDGET(tray_icon), str, "");
			g_free(str);
		}
		if(info.state != info.status->state)
		{
			gtk_widget_queue_draw(GTK_WIDGET(tray_icon));
		}
	}
}

/* if the item was destroyed and the user still wants an icon recreate it */
/* if the main window was hidden and the icon was destroyed, popup the main window because otherwise you wouldnt be able to 
 * get it back
 */

void tray_icon_destroyed()
{
	tray_icon = NULL;
	if(info.do_tray)
	{
		g_idle_add((GSourceFunc)create_tray_icon, NULL);
	}
	if(info.hidden)
	{
		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")));
		info.hidden = FALSE;
	}
}

/* destroy the tray icon */
void destroy_tray_icon()
{
	gtk_widget_destroy(GTK_WIDGET(tray_icon));
}

/* wrong name: this handles clickes on the tray icon
 * button1: present/hide window
 * button2: play/pause
 * button3: menu
 */

int  tray_mouse_menu(GtkWidget *wid, GdkEventButton *event)
{
	if(event->button == 1)
	{
		if(info.hidden )
		{
			//	gtk_window_present(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")));
			gtk_window_move(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), player_wsize.x, player_wsize.y);
			gtk_window_resize(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")),player_wsize.width, player_wsize.height);
			gtk_widget_show(glade_xml_get_widget(xml_main_window, "main_window"));

			if(info.sb_hidden) song_browser_create();
			if(info.pl2_hidden) create_playlist2();

			info.hidden = FALSE;
			gtk_widget_queue_draw(GTK_WIDGET(tray_icon));
		}
		else
		{
			gtk_window_get_position(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), &player_wsize.x, &player_wsize.y);
			gtk_window_get_size(GTK_WINDOW(glade_xml_get_widget(xml_main_window, "main_window")), &player_wsize.width, &player_wsize.height);


			gtk_widget_hide(GTK_WIDGET(glade_xml_get_widget(xml_main_window, "main_window")));
			info.hidden = TRUE;
			gtk_widget_queue_draw(GTK_WIDGET(tray_icon));

			if(info.sb_hidden)
			{
				sb_close();
				/* make sure its showed again */
				info.sb_hidden = TRUE;
			}
			if(info.pl2_hidden)
			{
				hide_playlist2();      		
				info.pl2_hidden = TRUE;
			}
		}
	}
	else if (event->button == 2)
	{
		play_song();




	}
	else if(event->button == 3)
	{
		GtkMenu *menu = (GtkMenu *)glade_xml_get_widget(tray_xml, "tray_icon_menu");
		gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 3, event->time);
		gtk_widget_show_all(GTK_WIDGET(menu));
	}
	return FALSE;    
}

int create_tray_icon()
{
	GdkPixbuf  *temp;
	GtkWidget *event;
	GtkWidget *ali;
	if(tray_icon != NULL)
	{
		return FALSE;
	}
	/* set up tray icon */
	tray_icon = egg_tray_icon_new(_("Gnome Music Player Client"));
	//	tray_image = gtk_image_new();
	event = gtk_event_box_new();
	gtk_widget_set_usize(event, 20,20);
	gtk_widget_set_app_paintable(event, TRUE);
	ali = gtk_alignment_new(0.5,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), event);
	gtk_container_add(GTK_CONTAINER(tray_icon), ali);
	//	gtk_container_add(GTK_CONTAINER(event), tray_image);
	gtk_widget_show_all(GTK_WIDGET(tray_icon));
	/* set image */
	temp = gdk_pixbuf_new_from_file(PIXMAP_PATH"gmpc-tray.png",NULL);
	logo = gdk_pixbuf_scale_simple(temp, 20,20, GDK_INTERP_BILINEAR);
	g_object_unref(temp);

	g_signal_connect(G_OBJECT(event), "expose-event", G_CALLBACK(exposed_signal), NULL);
	g_signal_connect(G_OBJECT(event), "button-release-event", G_CALLBACK(tray_mouse_menu), NULL);
	g_signal_connect(G_OBJECT(tray_icon), "destroy", G_CALLBACK(tray_icon_destroyed), NULL);

	g_signal_connect(G_OBJECT(event), "enter-notify-event", 
			G_CALLBACK(tray_motion_cb), NULL);
	g_signal_connect(G_OBJECT(event), "leave-notify-event",
			G_CALLBACK(tray_leave_cb), NULL);
	/* show all */
	gtk_widget_show_all(GTK_WIDGET(tray_icon));
	if(tps == NULL)	tps = gtk_tooltips_new();

	/* we only need to load this one once */
	if(tray_xml == NULL)
	{
		tray_xml =   glade_xml_new(GLADE_PATH"gmpc.glade", "tray_icon_menu", NULL);
		glade_xml_signal_autoconnect(tray_xml);
	}
	//	gtk_tooltips_set_tip(tps, GTK_WIDGET(tray_icon), _("Gnome Music Player Client"), "");
	info.song = -1;
	return FALSE;
}



