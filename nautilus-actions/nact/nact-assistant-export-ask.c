/*
 * Nautilus Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009 Pierre Wieser and others (see AUTHORS)
 *
 * This Program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   Frederic Ruaudel <grumz@grumz.net>
 *   Rodrigo Moya <rodrigo@gnome-db.org>
 *   Pierre Wieser <pwieser@trychlos.org>
 *   ... and many others (see AUTHORS)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include <api/na-object-api.h>

#include <runtime/na-iprefs.h>
#include <runtime/na-pivot.h>

#include "nact-application.h"
#include "nact-assistant-export-ask.h"

/* private class data
 */
struct NactAssistantExportAskClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactAssistantExportAskPrivate {
	gboolean        dispose_has_run;
	BaseWindow     *parent;
	NAObjectAction *action;
	gint            format;
};

static BaseDialogClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactAssistantExportAskClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *dialog );
static void     instance_finalize( GObject *dialog );

static NactAssistantExportAsk *assistant_export_ask_new( BaseWindow *parent );

static gchar   *base_get_iprefs_window_id( BaseWindow *window );
static gchar   *base_get_dialog_name( BaseWindow *window );
static gchar   *base_get_ui_filename( BaseWindow *dialog );
static void     on_base_initial_load_dialog( NactAssistantExportAsk *editor, gpointer user_data );
static void     on_base_runtime_init_dialog( NactAssistantExportAsk *editor, gpointer user_data );
static void     on_base_all_widgets_showed( NactAssistantExportAsk *editor, gpointer user_data );
static void     on_cancel_clicked( GtkButton *button, NactAssistantExportAsk *editor );
static void     on_ok_clicked( GtkButton *button, NactAssistantExportAsk *editor );
static gint     get_format( NactAssistantExportAsk *editor );
static gboolean base_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window );

GType
nact_assistant_export_ask_get_type( void )
{
	static GType dialog_type = 0;

	if( !dialog_type ){
		dialog_type = register_type();
	}

	return( dialog_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_assistant_export_ask_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactAssistantExportAskClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactAssistantExportAsk ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_DIALOG_TYPE, "NactAssistantExportAsk", &info, 0 );

	return( type );
}

static void
class_init( NactAssistantExportAskClass *klass )
{
	static const gchar *thisfn = "nact_assistant_export_ask_class_init";
	GObjectClass *object_class;
	BaseWindowClass *base_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NactAssistantExportAskClassPrivate, 1 );

	base_class = BASE_WINDOW_CLASS( klass );
	base_class->dialog_response = base_dialog_response;
	base_class->get_toplevel_name = base_get_dialog_name;
	base_class->get_iprefs_window_id = base_get_iprefs_window_id;
	base_class->get_ui_filename = base_get_ui_filename;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_assistant_export_ask_instance_init";
	NactAssistantExportAsk *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT_ASK( instance ));
	self = NACT_ASSISTANT_EXPORT_ASK( instance );

	self->private = g_new0( NactAssistantExportAskPrivate, 1 );

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_INITIAL_LOAD,
			G_CALLBACK( on_base_initial_load_dialog ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_RUNTIME_INIT,
			G_CALLBACK( on_base_runtime_init_dialog ));

	base_window_signal_connect(
			BASE_WINDOW( instance ),
			G_OBJECT( instance ),
			BASE_WINDOW_SIGNAL_ALL_WIDGETS_SHOWED,
			G_CALLBACK( on_base_all_widgets_showed));

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "nact_assistant_export_ask_instance_dispose";
	NactAssistantExportAsk *self;

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT_ASK( dialog ));
	self = NACT_ASSISTANT_EXPORT_ASK( dialog );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( dialog );
		}
	}
}

static void
instance_finalize( GObject *dialog )
{
	static const gchar *thisfn = "nact_assistant_export_ask_instance_finalize";
	NactAssistantExportAsk *self;

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );
	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT_ASK( dialog ));
	self = NACT_ASSISTANT_EXPORT_ASK( dialog );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/*
 * Returns a newly allocated NactAssistantExportAsk object.
 */
static NactAssistantExportAsk *
assistant_export_ask_new( BaseWindow *parent )
{
	return( g_object_new( NACT_ASSISTANT_EXPORT_ASK_TYPE, BASE_WINDOW_PROP_PARENT, parent, NULL ));
}

/**
 * nact_assistant_export_ask_run:
 * @parent: the NactAssistant parent of this dialog.
 *
 * Initializes and runs the dialog.
 *
 * Returns: the mode choosen by the user ; it defaults to 'none' (no export).
 *
 * When the user selects 'Keep same choice without asking me', this choice
 * becomes his preference export format.
 */
gint
nact_assistant_export_ask_user( BaseWindow *parent, NAObjectAction *action )
{
	static const gchar *thisfn = "nact_assistant_export_ask_run";
	NactApplication *application;
	NAPivot *pivot;
	NactAssistantExportAsk *editor;
	gint format;

	g_debug( "%s: parent=%p", thisfn, ( void * ) parent );
	g_return_val_if_fail( BASE_IS_WINDOW( parent ), IPREFS_EXPORT_NO_EXPORT );

	application = NACT_APPLICATION( base_window_get_application( parent ));
	g_return_val_if_fail( NACT_IS_APPLICATION( application ), IPREFS_EXPORT_NO_EXPORT );

	pivot = nact_application_get_pivot( application );
	g_return_val_if_fail( NA_IS_PIVOT( pivot ), IPREFS_EXPORT_NO_EXPORT );

	editor = assistant_export_ask_new( parent );
	/*g_object_set( G_OBJECT( editor ), BASE_WINDOW_PROP_HAS_OWN_BUILDER, TRUE, NULL );*/

	editor->private->parent = parent;
	editor->private->action = action;
	editor->private->format = na_iprefs_get_export_format( NA_IPREFS( pivot ), IPREFS_EXPORT_ASK_LAST_FORMAT );

	if( !base_window_run( BASE_WINDOW( editor ))){
		editor->private->format = IPREFS_EXPORT_NO_EXPORT;
	}

	na_iprefs_set_export_format( NA_IPREFS( pivot ), IPREFS_EXPORT_ASK_LAST_FORMAT, editor->private->format );
	format = editor->private->format;
	g_object_unref( editor );

	return( format );
}

static gchar *
base_get_iprefs_window_id( BaseWindow *window )
{
	return( g_strdup( "export-ask-user" ));
}

static gchar *
base_get_dialog_name( BaseWindow *window )
{
	return( g_strdup( "AssistantExportAsk" ));
}

static gchar *
base_get_ui_filename( BaseWindow *dialog )
{
	return( g_strdup( PKGDATADIR "/nact-assistant-export.ui" ));
}

static void
on_base_initial_load_dialog( NactAssistantExportAsk *editor, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_export_ask_on_initial_load_dialog";

	g_debug( "%s: editor=%p, user_data=%p", thisfn, ( void * ) editor, ( void * ) user_data );
	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT_ASK( editor ));
}

static void
on_base_runtime_init_dialog( NactAssistantExportAsk *editor, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_export_ask_on_runtime_init_dialog";
	gchar *action_label;
	gchar *label;
	GtkWidget *widget;
	GtkWidget *button;

	g_debug( "%s: editor=%p, user_data=%p", thisfn, ( void * ) editor, ( void * ) user_data );
	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT_ASK( editor ));

	action_label = na_object_get_label( editor->private->action );

	/* i18n: The action <action_label> is about to be exported */
	label = g_strdup_printf( _( "The action \"%s\" is about to be exported." ), action_label );

	widget = base_window_get_widget( BASE_WINDOW( editor ), "ExportAskLabel1" );
	gtk_label_set_text( GTK_LABEL( widget ), label );
	g_free( label );

	switch( editor->private->format ){
		case IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V1:
			button = base_window_get_widget( BASE_WINDOW( editor ), "AskGConfSchemaV1Button" );
			break;

		case IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V2:
			button = base_window_get_widget( BASE_WINDOW( editor ), "AskGConfSchemaV2Button" );
			break;

		case IPREFS_EXPORT_FORMAT_GCONF_ENTRY:
		default:
			button = base_window_get_widget( BASE_WINDOW( editor ), "AskGConfEntryButton" );
			break;
	}
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), TRUE );

	button = base_window_get_widget( BASE_WINDOW( editor ), "AskKeepChoiceButton" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), FALSE );

	base_window_signal_connect_by_name(
			BASE_WINDOW( editor ),
			"CancelButton",
			"clicked",
			G_CALLBACK( on_cancel_clicked ));

	base_window_signal_connect_by_name(
			BASE_WINDOW( editor ),
			"OKButton",
			"clicked",
			G_CALLBACK( on_ok_clicked ));
}

static void
on_base_all_widgets_showed( NactAssistantExportAsk *editor, gpointer user_data )
{
	static const gchar *thisfn = "nact_assistant_export_ask_on_all_widgets_showed";

	g_debug( "%s: editor=%p, user_data=%p", thisfn, ( void * ) editor, ( void * ) user_data );
	g_return_if_fail( NACT_IS_ASSISTANT_EXPORT_ASK( editor ));
}

static void
on_cancel_clicked( GtkButton *button, NactAssistantExportAsk *editor )
{
	GtkWindow *toplevel = base_window_get_toplevel( BASE_WINDOW( editor ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_CLOSE );
}

static void
on_ok_clicked( GtkButton *button, NactAssistantExportAsk *editor )
{
	GtkWindow *toplevel = base_window_get_toplevel( BASE_WINDOW( editor ));
	gtk_dialog_response( GTK_DIALOG( toplevel ), GTK_RESPONSE_OK );
}

static gint
get_format( NactAssistantExportAsk *editor )
{
	gint export_format;
	NactApplication *application;
	NAPivot *pivot;
	GtkWidget *button;
	gboolean keep;

	export_format = IPREFS_EXPORT_FORMAT_GCONF_ENTRY;
	button = base_window_get_widget( BASE_WINDOW( editor ), "AskGConfSchemaV1Button" );
	if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ))){
		export_format = IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V1;
	} else {
		button = base_window_get_widget( BASE_WINDOW( editor ), "AskGConfSchemaV2Button" );
		if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ))){
			export_format = IPREFS_EXPORT_FORMAT_GCONF_SCHEMA_V2;
		}
	}

	button = base_window_get_widget( BASE_WINDOW( editor ), "AskKeepChoiceButton" );
	keep = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ));
	if( keep ){
		application = NACT_APPLICATION( base_window_get_application( BASE_WINDOW( editor )));
		pivot = nact_application_get_pivot( application );
		na_iprefs_set_export_format( NA_IPREFS( pivot ), IPREFS_EXPORT_FORMAT, export_format );
	}

	return( export_format );
}

static gboolean
base_dialog_response( GtkDialog *dialog, gint code, BaseWindow *window )
{
	static const gchar *thisfn = "nact_assistant_export_ask_on_dialog_response";
	NactAssistantExportAsk *editor;

	g_debug( "%s: dialog=%p, code=%d, window=%p", thisfn, ( void * ) dialog, code, ( void * ) window );
	g_assert( NACT_IS_ASSISTANT_EXPORT_ASK( window ));
	editor = NACT_ASSISTANT_EXPORT_ASK( window );

	switch( code ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_CANCEL:

			editor->private->format = IPREFS_EXPORT_NO_EXPORT;
			return( TRUE );
			break;

		case GTK_RESPONSE_OK:
			editor->private->format = get_format( editor );
			return( TRUE );
			break;
	}

	return( FALSE );
}