/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gtktransition.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <plugin.h>


#define GMPC_URL_FETCHING_TYPE_GUI (gmpc_url_fetching_gui_get_type ())
#define GMPC_URL_FETCHING_GUI(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_URL_FETCHING_TYPE_GUI, GmpcUrlFetchingGui))
#define GMPC_URL_FETCHING_GUI_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_URL_FETCHING_TYPE_GUI, GmpcUrlFetchingGuiClass))
#define GMPC_URL_FETCHING_IS_GUI(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_URL_FETCHING_TYPE_GUI))
#define GMPC_URL_FETCHING_IS_GUI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_URL_FETCHING_TYPE_GUI))
#define GMPC_URL_FETCHING_GUI_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_URL_FETCHING_TYPE_GUI, GmpcUrlFetchingGuiClass))

typedef struct _GmpcUrlFetchingGui GmpcUrlFetchingGui;
typedef struct _GmpcUrlFetchingGuiClass GmpcUrlFetchingGuiClass;
typedef struct _GmpcUrlFetchingGuiPrivate GmpcUrlFetchingGuiPrivate;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define _g_error_free0(var) ((var == NULL) ? NULL : (var = (g_error_free (var), NULL)))
#define _g_free0(var) (var = (g_free (var), NULL))

typedef enum  {
	GMPC_URL_FETCHING_PARSE_ERROR_INVALID_SCHEME,
	GMPC_URL_FETCHING_PARSE_ERROR_FAILED_TO_PARSE
} GmpcUrlFetchingParseError;
#define GMPC_URL_FETCHING_PARSE_ERROR gmpc_url_fetching_parse_error_quark ()
struct _GmpcUrlFetchingGui {
	GObject parent_instance;
	GmpcUrlFetchingGuiPrivate * priv;
};

struct _GmpcUrlFetchingGuiClass {
	GObjectClass parent_class;
};

typedef gboolean (*GmpcUrlFetchingGuiParseUrl) (GmpcUrlFetchingGui* gui, const char* url, void* user_data, GError** error);
typedef gboolean (*GmpcUrlFetchingGuiValidateUrl) (GmpcUrlFetchingGui* gui, const char* url, void* user_data);
struct _GmpcUrlFetchingGuiPrivate {
	GtkBuilder* builder;
	GmpcUrlFetchingGuiParseUrl parse_callback;
	gpointer parse_callback_target;
	GDestroyNotify parse_callback_target_destroy_notify;
	GmpcUrlFetchingGuiValidateUrl validate_callback;
	gpointer validate_callback_target;
	GDestroyNotify validate_callback_target_destroy_notify;
	GDestroyNotify destroy_cb;
};


static gpointer gmpc_url_fetching_gui_parent_class = NULL;

#define use_transition TRUE
GQuark gmpc_url_fetching_parse_error_quark (void);
GType gmpc_url_fetching_gui_get_type (void);
#define GMPC_URL_FETCHING_GUI_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_URL_FETCHING_TYPE_GUI, GmpcUrlFetchingGuiPrivate))
enum  {
	GMPC_URL_FETCHING_GUI_DUMMY_PROPERTY
};
static void gmpc_url_fetching_gui_add_url_dialog_response (GmpcUrlFetchingGui* self, gint response_id);
static void gmpc_url_fetching_gui_url_entry_changed (GmpcUrlFetchingGui* self, GtkEditable* editable);
static void _gmpc_url_fetching_gui_add_url_dialog_response_gtk_dialog_response (GtkDialog* _sender, gint response_id, gpointer self);
static void _gmpc_url_fetching_gui_url_entry_changed_gtk_editable_changed (GtkEditable* _sender, gpointer self);
GmpcUrlFetchingGui* gmpc_url_fetching_gui_new (GmpcUrlFetchingGuiParseUrl parse_callback, void* parse_callback_target, GmpcUrlFetchingGuiValidateUrl validate_callback, void* validate_callback_target, GDestroyNotify destroy_cb);
GmpcUrlFetchingGui* gmpc_url_fetching_gui_construct (GType object_type, GmpcUrlFetchingGuiParseUrl parse_callback, void* parse_callback_target, GmpcUrlFetchingGuiValidateUrl validate_callback, void* validate_callback_target, GDestroyNotify destroy_cb);
static void gmpc_url_fetching_gui_finalize (GObject* obj);



GQuark gmpc_url_fetching_parse_error_quark (void) {
	return g_quark_from_static_string ("gmpc_url_fetching_parse_error-quark");
}


static void gmpc_url_fetching_gui_add_url_dialog_response (GmpcUrlFetchingGui* self, gint response_id) {
	GError * _inner_error_;
	g_return_if_fail (self != NULL);
	_inner_error_ = NULL;
	if (response_id == 1) {
		GtkEntry* entry;
		char* url;
		entry = GTK_ENTRY (gtk_builder_get_object (self->priv->builder, "url_entry"));
		url = g_strdup (gtk_entry_get_text (entry));
		{
			self->priv->parse_callback (self, url, self->priv->parse_callback_target, &_inner_error_);
			if (_inner_error_ != NULL) {
				goto __catch0_g_error;
				goto __finally0;
			}
		}
		goto __finally0;
		__catch0_g_error:
		{
			GError * e;
			e = _inner_error_;
			_inner_error_ = NULL;
			{
				fprintf (stdout, "Error callback: %s\n", e->message);
				_g_error_free0 (e);
				_g_free0 (url);
				return;
			}
		}
		__finally0:
		if (_inner_error_ != NULL) {
			_g_free0 (url);
			g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, _inner_error_->message);
			g_clear_error (&_inner_error_);
			return;
		}
		_g_free0 (url);
	}
	fprintf (stdout, "destroy callback\n");
	self->priv->destroy_cb (self);
}


static gpointer _g_object_ref0 (gpointer self) {
	return self ? g_object_ref (self) : NULL;
}


static void gmpc_url_fetching_gui_url_entry_changed (GmpcUrlFetchingGui* self, GtkEditable* editable) {
	GtkButton* add_button;
	char* text;
	gboolean _tmp0_;
	gboolean _tmp1_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (editable != NULL);
	add_button = _g_object_ref0 (GTK_BUTTON (gtk_builder_get_object (self->priv->builder, "add_button")));
	text = g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));
	_tmp0_ = FALSE;
	_tmp1_ = FALSE;
	if (text != NULL) {
		_tmp1_ = self->priv->validate_callback != NULL;
	} else {
		_tmp1_ = FALSE;
	}
	if (_tmp1_) {
		_tmp0_ = self->priv->validate_callback (self, text, self->priv->validate_callback_target);
	} else {
		_tmp0_ = FALSE;
	}
	if (_tmp0_) {
		g_object_set ((GtkWidget*) add_button, "sensitive", TRUE, NULL);
	} else {
		g_object_set ((GtkWidget*) add_button, "sensitive", FALSE, NULL);
	}
	_g_object_unref0 (add_button);
	_g_free0 (text);
}


static void _gmpc_url_fetching_gui_add_url_dialog_response_gtk_dialog_response (GtkDialog* _sender, gint response_id, gpointer self) {
	gmpc_url_fetching_gui_add_url_dialog_response (self, response_id);
}


static void _gmpc_url_fetching_gui_url_entry_changed_gtk_editable_changed (GtkEditable* _sender, gpointer self) {
	gmpc_url_fetching_gui_url_entry_changed (self, _sender);
}


GmpcUrlFetchingGui* gmpc_url_fetching_gui_construct (GType object_type, GmpcUrlFetchingGuiParseUrl parse_callback, void* parse_callback_target, GmpcUrlFetchingGuiValidateUrl validate_callback, void* validate_callback_target, GDestroyNotify destroy_cb) {
	GError * _inner_error_;
	GmpcUrlFetchingGui * self;
	GmpcUrlFetchingGuiParseUrl _tmp0_;
	GmpcUrlFetchingGuiValidateUrl _tmp1_;
	GtkDialog* dialog;
	GtkEntry* entry;
	_inner_error_ = NULL;
	self = (GmpcUrlFetchingGui*) g_object_new (object_type, NULL);
	self->priv->parse_callback = (_tmp0_ = parse_callback, ((self->priv->parse_callback_target_destroy_notify == NULL) ? NULL : self->priv->parse_callback_target_destroy_notify (self->priv->parse_callback_target), self->priv->parse_callback = NULL, self->priv->parse_callback_target = NULL, self->priv->parse_callback_target_destroy_notify = NULL), self->priv->parse_callback_target = parse_callback_target, self->priv->parse_callback_target_destroy_notify = NULL, _tmp0_);
	self->priv->validate_callback = (_tmp1_ = validate_callback, ((self->priv->validate_callback_target_destroy_notify == NULL) ? NULL : self->priv->validate_callback_target_destroy_notify (self->priv->validate_callback_target), self->priv->validate_callback = NULL, self->priv->validate_callback_target = NULL, self->priv->validate_callback_target_destroy_notify = NULL), self->priv->validate_callback_target = validate_callback_target, self->priv->validate_callback_target_destroy_notify = NULL, _tmp1_);
	self->priv->destroy_cb = destroy_cb;
	{
		char* _tmp2_;
		gtk_builder_add_from_file (self->priv->builder, _tmp2_ = gmpc_get_full_glade_path ("gmpc-add-url.ui"), &_inner_error_);
		if (_inner_error_ != NULL) {
			goto __catch1_g_error;
			goto __finally1;
		}
		_g_free0 (_tmp2_);
	}
	goto __finally1;
	__catch1_g_error:
	{
		GError * e;
		e = _inner_error_;
		_inner_error_ = NULL;
		{
			g_error ("gmpc-url-fetching-gui.vala:101: Failed to load GtkBuilder file: %s", e->message);
			_g_error_free0 (e);
		}
	}
	__finally1:
	if (_inner_error_ != NULL) {
		g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, _inner_error_->message);
		g_clear_error (&_inner_error_);
		return NULL;
	}
	dialog = _g_object_ref0 (GTK_DIALOG (gtk_builder_get_object (self->priv->builder, "add_url_dialog")));
	gtk_window_set_transient_for ((GtkWindow*) dialog, (GtkWindow *)playlist3_get_window ());
	gtk_widget_show_all ((GtkWidget*) dialog);
	entry = _g_object_ref0 (GTK_ENTRY (gtk_builder_get_object (self->priv->builder, "url_entry")));
	g_signal_connect_object (dialog, "response", (GCallback) _gmpc_url_fetching_gui_add_url_dialog_response_gtk_dialog_response, self, 0);
	g_signal_connect_object ((GtkEditable*) entry, "changed", (GCallback) _gmpc_url_fetching_gui_url_entry_changed_gtk_editable_changed, self, 0);
	_g_object_unref0 (dialog);
	_g_object_unref0 (entry);
	return self;
}


GmpcUrlFetchingGui* gmpc_url_fetching_gui_new (GmpcUrlFetchingGuiParseUrl parse_callback, void* parse_callback_target, GmpcUrlFetchingGuiValidateUrl validate_callback, void* validate_callback_target, GDestroyNotify destroy_cb) {
	return gmpc_url_fetching_gui_construct (GMPC_URL_FETCHING_TYPE_GUI, parse_callback, parse_callback_target, validate_callback, validate_callback_target, destroy_cb);
}


static void gmpc_url_fetching_gui_class_init (GmpcUrlFetchingGuiClass * klass) {
	gmpc_url_fetching_gui_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcUrlFetchingGuiPrivate));
	G_OBJECT_CLASS (klass)->finalize = gmpc_url_fetching_gui_finalize;
}


static void gmpc_url_fetching_gui_instance_init (GmpcUrlFetchingGui * self) {
	self->priv = GMPC_URL_FETCHING_GUI_GET_PRIVATE (self);
	self->priv->builder = gtk_builder_new ();
	self->priv->parse_callback = NULL;
	self->priv->validate_callback = NULL;
}


static void gmpc_url_fetching_gui_finalize (GObject* obj) {
	GmpcUrlFetchingGui * self;
	self = GMPC_URL_FETCHING_GUI (obj);
	{
		fprintf (stdout, "~Gui\n");
		if (self->priv->builder != NULL) {
			GtkDialog* dialog;
			dialog = _g_object_ref0 (GTK_DIALOG (gtk_builder_get_object (self->priv->builder, "add_url_dialog")));
			if (dialog != NULL) {
				gtk_object_destroy ((GtkObject*) dialog);
			}
			_g_object_unref0 (dialog);
		}
	}
	_g_object_unref0 (self->priv->builder);
	(self->priv->parse_callback_target_destroy_notify == NULL) ? NULL : self->priv->parse_callback_target_destroy_notify (self->priv->parse_callback_target);
	self->priv->parse_callback = NULL;
	self->priv->parse_callback_target = NULL;
	self->priv->parse_callback_target_destroy_notify = NULL;
	(self->priv->validate_callback_target_destroy_notify == NULL) ? NULL : self->priv->validate_callback_target_destroy_notify (self->priv->validate_callback_target);
	self->priv->validate_callback = NULL;
	self->priv->validate_callback_target = NULL;
	self->priv->validate_callback_target_destroy_notify = NULL;
	G_OBJECT_CLASS (gmpc_url_fetching_gui_parent_class)->finalize (obj);
}


GType gmpc_url_fetching_gui_get_type (void) {
	static GType gmpc_url_fetching_gui_type_id = 0;
	if (gmpc_url_fetching_gui_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcUrlFetchingGuiClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_url_fetching_gui_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcUrlFetchingGui), 0, (GInstanceInitFunc) gmpc_url_fetching_gui_instance_init, NULL };
		gmpc_url_fetching_gui_type_id = g_type_register_static (G_TYPE_OBJECT, "GmpcUrlFetchingGui", &g_define_type_info, 0);
	}
	return gmpc_url_fetching_gui_type_id;
}




