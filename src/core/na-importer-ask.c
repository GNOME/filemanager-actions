/*
 * Nautilus-Actions
 * A Nautilus extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006, 2007, 2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009, 2010, 2011 Pierre Wieser and others (see AUTHORS)
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

#include <api/na-iimporter.h>
#include <api/na-object-api.h>

#include "na-gtk-utils.h"
#include "na-iprefs.h"
#include "na-importer-ask.h"
#include "na-settings.h"

/* private class data
 */
struct _NAImporterAskClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NAImporterAskPrivate {
	gboolean                dispose_has_run;
	GtkWindow              *toplevel;
	NAObjectItem           *importing;
	NAObjectItem           *existing;
	NAImporterAskUserParms *parms;
	guint                   mode;
};

static GObjectClass  *st_parent_class = NULL;
static NAImporterAsk *st_dialog       = NULL;
static const gchar   *st_uixml        = PKGDATADIR "/na-importer-ask.ui";

static GType      register_type( void );
static void       class_init( NAImporterAskClass *klass );
static void       instance_init( GTypeInstance *instance, gpointer klass );
static void       instance_dispose( GObject *dialog );
static void       instance_finalize( GObject *dialog );

static NAImporterAsk *import_ask_new( GtkWindow *parent );

static void       init_dialog( NAImporterAsk *editor );
static void       get_selected_mode( NAImporterAsk *editor );
static gboolean   on_destroy_toplevel( GtkWindow *toplevel, GdkEvent *event, NAImporterAsk *dialog );
static gboolean   on_dialog_response( NAImporterAsk *editor, gint code );

GType
na_importer_ask_get_type( void )
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
	static const gchar *thisfn = "na_importer_ask_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAImporterAskClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAImporterAsk ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NAImporterAsk", &info, 0 );

	return( type );
}

static void
class_init( NAImporterAskClass *klass )
{
	static const gchar *thisfn = "na_importer_ask_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NAImporterAskClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_importer_ask_instance_init";
	NAImporterAsk *self;

	g_return_if_fail( NA_IS_IMPORTER_ASK( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = NA_IMPORTER_ASK( instance );

	self->private = g_new0( NAImporterAskPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *dialog )
{
	static const gchar *thisfn = "na_importer_ask_instance_dispose";
	NAImporterAsk *self;

	g_return_if_fail( NA_IS_IMPORTER_ASK( dialog ));

	self = NA_IMPORTER_ASK( dialog );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: dialog=%p (%s)", thisfn, ( void * ) dialog, G_OBJECT_TYPE_NAME( dialog ));

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
	static const gchar *thisfn = "na_importer_ask_instance_finalize";
	NAImporterAsk *self;

	g_return_if_fail( NA_IS_IMPORTER_ASK( dialog ));

	g_debug( "%s: dialog=%p", thisfn, ( void * ) dialog );

	self = NA_IMPORTER_ASK( dialog );

	if( self->private->toplevel ){
		gtk_widget_destroy( GTK_WIDGET( self->private->toplevel ));
	}

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( dialog );
	}
}

/*
 * Returns a newly allocated NAImporterAsk object.
 */
static NAImporterAsk *
import_ask_new( GtkWindow *parent )
{
	NAImporterAsk *dialog;
	GtkBuilder *builder;
	GError *error;
	GtkWindow *toplevel;

	if( st_dialog ){
		dialog = st_dialog;

	} else {
		dialog = g_object_new( NA_IMPORTER_ASK_TYPE, NULL );

		builder = gtk_builder_new();
		error = NULL;
		gtk_builder_add_from_file( builder, st_uixml, &error );
		if( error ){
			gtk_message_dialog_new( parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", error->message );
			g_error_free( error );
			g_object_unref( dialog );
			dialog = NULL;

		} else {
			toplevel = GTK_WINDOW( gtk_builder_get_object( builder, "ImporterAskDialog" ));
			if( !toplevel ){
				/* l10n: 'ImporterAskDialog' is the dialog name: do not translate */
				gtk_message_dialog_new( parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _( "Unable to load 'ImporterAskDialog' from %s" ), st_uixml );
				g_object_unref( dialog );
				dialog = NULL;

			} else {
				dialog->private->toplevel = toplevel;

				if( parent ){
					gtk_window_set_transient_for( dialog->private->toplevel, parent );
					gtk_window_set_destroy_with_parent( dialog->private->toplevel, TRUE );
					g_signal_connect(
							G_OBJECT( dialog->private->toplevel ),
							"destroy", G_CALLBACK( on_destroy_toplevel ), dialog );
					st_dialog = dialog;
				}

#if !GTK_CHECK_VERSION( 2,22,0 )
				gtk_dialog_set_has_separator( GTK_DIALOG( toplevel ), FALSE );
#endif
			}
		}
		g_object_unref( builder );
	}

	return( dialog );
}

/*
 * na_importer_ask_user:
 * @importing: the #NAObjectItem-derived object being currently imported.
 * @existing: the #NAObjectItem-derived already existing object with the same ID.
 * @parms: a #NAIImporterUriParms structure.
 *
 * Ask the user for what to do when an imported item has the same ID
 * that an already existing one.
 *
 * If a parent is specified, then we allocate a new NAImporterAsk from
 * GtkBuilder, hiding and showing it for each invocation of the dialog
 * by the parent, as long as the parent exists.
 * When the parent is destroyed, this (maybe hidden) NAImporterAsk dialog
 * is also destroyed.
 *
 * If there is no specified parent, then we recreate a new NAImporterAsk
 * dialog at each invocation, destroying it after on_dialog_response()
 * returns.
 *
 * Returns: the definitive import mode.
 *
 * When the user selects 'Keep same choice without asking me', this choice
 * becomes his preference import mode.
 */
guint
na_importer_ask_user( const NAObjectItem *importing, const NAObjectItem *existing, NAImporterAskUserParms *parms )
{
	static const gchar *thisfn = "na_importer_ask_user";
	NAImporterAsk *dialog;
	guint mode;
	gint code;

	g_return_val_if_fail( NA_IS_OBJECT_ITEM( importing ), IMPORTER_MODE_NO_IMPORT );
	g_return_val_if_fail( NA_IS_OBJECT_ITEM( existing ), IMPORTER_MODE_NO_IMPORT );

	g_debug( "%s: importing=%p, existing=%p, parms=%p",
			thisfn, ( void * ) importing, ( void * ) existing, ( void * ) parms );

	mode = IMPORTER_MODE_NO_IMPORT;
	dialog = import_ask_new( parms->parent );

	if( dialog ){

		dialog->private->importing = ( NAObjectItem * ) importing;
		dialog->private->existing = ( NAObjectItem * ) existing;
		dialog->private->parms = parms;
		dialog->private->mode = na_iprefs_get_import_mode( NA_IPREFS_IMPORT_ASK_USER_LAST_MODE, NULL );

		init_dialog( dialog );

		do {
			code = gtk_dialog_run( GTK_DIALOG( dialog->private->toplevel ));
		} while ( !on_dialog_response( dialog, code ));

		mode = dialog->private->mode;
		na_gtk_utils_save_window_position( dialog->private->toplevel, NA_IPREFS_IMPORT_ASK_USER_WSP );

		if( parms->parent ){
			gtk_widget_hide( GTK_WIDGET( dialog->private->toplevel ));

		} else {
			g_object_unref( dialog );
		}
	}

	return( mode );
}

static void
init_dialog( NAImporterAsk *editor )
{
	static const gchar *thisfn = "na_importer_ask_init_dialog";
	gchar *imported_label, *existing_label;
	gchar *label;
	GtkWidget *widget;
	GtkWidget *button;

	g_return_if_fail( NA_IS_IMPORTER_ASK( editor ));

	g_debug( "%s: editor=%p", thisfn, ( void * ) editor );

	imported_label = na_object_get_label( editor->private->importing );
	existing_label = na_object_get_label( editor->private->existing );

	if( NA_IS_OBJECT_ACTION( editor->private->importing )){
		/* i18n: The action <action_label> imported from <file> has the same id than <existing_label> */
		label = g_strdup_printf(
				_( "The action \"%s\" imported from \"%s\" has the same identifiant than the already existing \"%s\"." ),
				imported_label, editor->private->parms->uri, existing_label );

	} else {
		/* i18n: The menu <menu_label> imported from <file> has the same id than <existing_label> */
		label = g_strdup_printf(
				_( "The menu \"%s\" imported from \"%s\" has the same identifiant than the already existing \"%s\"." ),
				imported_label, editor->private->parms->uri, existing_label );
	}

	widget = na_gtk_utils_search_for_child_widget( GTK_CONTAINER( editor->private->toplevel ), "ImporterAskLabel" );
	gtk_label_set_text( GTK_LABEL( widget ), label );
	g_free( label );

	switch( editor->private->mode ){
		case IMPORTER_MODE_RENUMBER:
			button = na_gtk_utils_search_for_child_widget( GTK_CONTAINER( editor->private->toplevel ), "AskRenumberButton" );
			break;

		case IMPORTER_MODE_OVERRIDE:
			button = na_gtk_utils_search_for_child_widget( GTK_CONTAINER( editor->private->toplevel ), "AskOverrideButton" );
			break;

		case IMPORTER_MODE_NO_IMPORT:
		default:
			button = na_gtk_utils_search_for_child_widget( GTK_CONTAINER( editor->private->toplevel ), "AskNoImportButton" );
			break;
	}
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), TRUE );

	button = na_gtk_utils_search_for_child_widget( GTK_CONTAINER( editor->private->toplevel ), "AskKeepChoiceButton" );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( button ), editor->private->parms->keep_choice );

	na_gtk_utils_restore_window_position( editor->private->toplevel, NA_IPREFS_IMPORT_ASK_USER_WSP );
	gtk_widget_show_all( GTK_WIDGET( editor->private->toplevel ));
}

static void
get_selected_mode( NAImporterAsk *editor )
{
	guint import_mode;
	GtkWidget *button;
	gboolean keep;

	import_mode = IMPORTER_MODE_NO_IMPORT;

	button = na_gtk_utils_search_for_child_widget( GTK_CONTAINER( editor->private->toplevel ), "AskRenumberButton" );
	if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ))){
		import_mode = IMPORTER_MODE_RENUMBER;

	} else {
		button = na_gtk_utils_search_for_child_widget( GTK_CONTAINER( editor->private->toplevel ), "AskOverrideButton" );
		if( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ))){
			import_mode = IMPORTER_MODE_OVERRIDE;
		}
	}

	editor->private->mode = import_mode;
	na_iprefs_set_import_mode( NA_IPREFS_IMPORT_ASK_USER_LAST_MODE, editor->private->mode );

	button = na_gtk_utils_search_for_child_widget( GTK_CONTAINER( editor->private->toplevel ), "AskKeepChoiceButton" );
	keep = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( button ));
	na_settings_set_boolean( NA_IPREFS_IMPORT_ASK_USER_KEEP_LAST_CHOICE, keep );
}

/*
 * destroy signal is only connected if the NAImporterAsk has been created
 * with a parent window; it has been defined with 'destroy_with_parent'
 * and so we have yet to unref the NAImporterAsk object itself
 */
static gboolean
on_destroy_toplevel( GtkWindow *toplevel, GdkEvent *event, NAImporterAsk *dialog )
{
	static const gchar *thisfn = "na_importer_ask_on_destroy_toplevel";

	g_debug( "%s: toplevel=%p, event=%p, dialog=%p",
			thisfn, ( void * ) toplevel, ( void * ) event, ( void * ) dialog );

	g_return_val_if_fail( NA_IS_IMPORTER_ASK( dialog ), FALSE );

	if( !dialog->private->dispose_has_run ){
		if( toplevel == dialog->private->toplevel ){
			dialog->private->toplevel = NULL;
		}
		g_object_unref( dialog );
	}

	/* let the event be propagated */
	return( FALSE );
}

static gboolean
on_dialog_response( NAImporterAsk *editor, gint code )
{
	static const gchar *thisfn = "na_importer_ask_on_dialog_response";

	g_return_val_if_fail( NA_IS_IMPORTER_ASK( editor ), FALSE );

	g_debug( "%s: editor=%p, code=%d", thisfn, ( void * ) editor, code );

	switch( code ){
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CLOSE:
		case GTK_RESPONSE_CANCEL:
			editor->private->mode = IMPORTER_MODE_NO_IMPORT;
			return( TRUE );
			break;

		case GTK_RESPONSE_OK:
			get_selected_mode( editor );
			return( TRUE );
			break;
	}

	return( FALSE );
}
