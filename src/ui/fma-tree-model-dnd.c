/*
 * FileManager-Actions
 * A file-manager extension which offers configurable context menu actions.
 *
 * Copyright (C) 2005 The GNOME Foundation
 * Copyright (C) 2006-2008 Frederic Ruaudel and others (see AUTHORS)
 * Copyright (C) 2009-2015 Pierre Wieser and others (see AUTHORS)
 *
 * FileManager-Actions is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * FileManager-Actions is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FileManager-Actions; see the file COPYING. If not, see
 * <http://www.gnu.org/licenses/>.
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
#include <libintl.h>
#include <string.h>

#include <api/fma-core-utils.h>
#include <api/fma-object-api.h>

#include <core/fma-gnome-vfs-uri.h>
#include <core/fma-importer.h>

#include "fma-application.h"
#include "fma-clipboard.h"
#include "fma-status-bar.h"
#include "fma-main-window.h"
#include "fma-tree-model.h"
#include "fma-tree-model-priv.h"
#include "fma-tree-ieditable.h"

/*
 * call once egg_tree_multi_drag_add_drag_support( treeview ) at init time (before gtk_main)
 *
 * when we start with drag
 * 	 call once egg_tree_multi_dnd_on_button_press_event( treeview, event, drag_source )
 *   call many egg_tree_multi_dnd_on_motion_event( treeview, event, drag_source )
 *     until mouse quits the selected area
 *
 * as soon as mouse has quitted the selected area
 *   call once egg_tree_multi_dnd_stop_drag_check( treeview )
 *   call once fma_tree_model_dnd_imulti_drag_source_row_draggable: drag_source=0x92a0d70, path_list=0x9373c90
 *   call once fma_tree_model_dnd_on_drag_begin
 *     fma_clipboard_dnd_clear()
 *   call once fma_clipboard_on_drag_begin( treeview, context, main_window )
 *
 * when we drop (e.g. in Nautilus)
 *   call once egg_tree_multi_dnd_on_drag_data_get( treeview, context, selection_data, info=0, time )
 *   call once fma_tree_model_dnd_imulti_drag_source_drag_data_get( drag_source, context, selection_data, path_list, atom=XdndDirectSave0 )
 *     fma_clipboard_dnd_set()
 *   call once fma_tree_model_dnd_on_drag_end
 *     fma_clipboard_dnd_drag_end
 *       fma_clipboard_get_from_dnd_clipboard_callback
 *     fma_clipboard_dnd_clear
 *
 * when we drop in FileManager-Actions
 *   call once egg_tree_multi_dnd_on_drag_data_get( treeview, context, selection_data, info=0, time )
 *   call once fma_tree_model_imulti_drag_source_drag_data_get( drag_source, context, selection_data, path_list, atom=XdndFileManagerActions )
 *   call once fma_clipboard_get_data_for_intern_use
 *   call once fma_tree_model_idrag_dest_drag_data_received
 *   call once fma_clipboard_on_drag_end( treeview, context, main_window )
 */

#define MAX_XDS_ATOM_VAL_LEN			4096
#define TEXT_ATOM						gdk_atom_intern( "text/plain", FALSE )
#define XDS_ATOM						gdk_atom_intern( "XdndDirectSave0", FALSE )
#define XDS_FILENAME					"xds.txt"

/* as a dnd source, we provide
 * - a special XdndNautilusAction format for internal move/copy
 * - a XdndDirectSave, suitable for exporting to a file manager
 *   (note that Nautilus recognized the "XdndDirectSave0" format as XDS
 *   protocol)
 * - a text (xml) format, to go to clipboard or a text editor
 */
static GtkTargetEntry dnd_source_formats[] = {
	{ "XdndFileManagerActions", GTK_TARGET_SAME_WIDGET, FMA_XCHANGE_FORMAT_FMA },
	{ "XdndDirectSave0",        GTK_TARGET_OTHER_APP,   FMA_XCHANGE_FORMAT_XDS },
	{ "application/xml",        GTK_TARGET_OTHER_APP,   FMA_XCHANGE_FORMAT_APPLICATION_XML },
	{ "text/plain",             GTK_TARGET_OTHER_APP,   FMA_XCHANGE_FORMAT_TEXT_PLAIN },
};

/* as a dnd dest, we accept
 * - of course, the same special XdndNautilusAction format for internal move/copy
 * - a list of uris, to be imported
 * - a XML buffer, to be imported
 * - a plain text, which we are going to try to import as a XML description
 */
GtkTargetEntry tree_model_dnd_dest_formats[] = {
	{ "XdndFileManagerActions", 0, FMA_XCHANGE_FORMAT_FMA },
	{ "text/uri-list",          0, FMA_XCHANGE_FORMAT_URI_LIST },
	{ "application/xml",        0, FMA_XCHANGE_FORMAT_APPLICATION_XML },
	{ "text/plain",             0, FMA_XCHANGE_FORMAT_TEXT_PLAIN },
};

guint tree_model_dnd_dest_formats_count = G_N_ELEMENTS( tree_model_dnd_dest_formats );

static const gchar *st_refuse_drop_profile = N_( "Unable to drop a profile here" );
static const gchar *st_refuse_drop_item = N_( "Unable to drop an action or a menu here" );
static const gchar *st_parent_not_writable = N_( "Unable to drop here as parent is not writable" );
static const gchar *st_level_zero_not_writable = N_( "Unable to drop here as level zero is not writable" );

static gboolean       drop_inside( FMATreeModel *model, GtkTreePath *dest, GtkSelectionData  *selection_data );
static gboolean       is_drop_possible( FMATreeModel *model, GtkTreePath *dest, FMAObjectItem **parent );
static gboolean       is_drop_possible_before_iter( FMATreeModel *model, GtkTreeIter *iter, FMAMainWindow *window, FMAObjectItem **parent );
static gboolean       is_drop_possible_into_dest( FMATreeModel *model, GtkTreePath *dest, FMAMainWindow *window, FMAObjectItem **parent );
static void           drop_inside_move_dest( FMATreeModel *model, GList *rows, GtkTreePath **dest );
static gboolean       drop_uri_list( FMATreeModel *model, GtkTreePath *dest, GtkSelectionData  *selection_data );
static FMAObjectItem *is_dropped_already_exists( const FMAObjectItem *importing, const FMAMainWindow *window );
static char          *get_xds_atom_value( GdkDragContext *context );
static gboolean       is_parent_accept_new_children( FMAApplication *application, FMAMainWindow *window, FMAObjectItem *parent );
static guint          target_atom_to_id( GdkAtom atom );

/**
 * fma_tree_model_dnd_idrag_dest_drag_data_received:
 * @drag_dest:
 * @dest:
 * @selection_data:
 *
 * Called when a drop occurs in the treeview;
 * this may be an import action from the outside world, or a move/copy
 * inside of the tree.
 *
 * Returns: %TRUE if the specified rows were successfully inserted at
 * the given dest, %FALSE else.
 *
 * When importing:
 * - selection=XdndSelection
 * - target=text/uri-list
 * - type=text/uri-list
 *
 * When moving/copy from the treeview to the treeview:
 * - selection=XdndSelection
 * - target=XdndFileManagerActions
 * - type=XdndFileManagerActions
 */
gboolean
fma_tree_model_dnd_idrag_dest_drag_data_received( GtkTreeDragDest *drag_dest, GtkTreePath *dest, GtkSelectionData  *selection_data )
{
	static const gchar *thisfn = "fma_tree_model_dnd_idrag_dest_drag_data_received";
	gboolean result = FALSE;
	FMATreeModel *model;
	gchar *atom_name;
	guint info;
	gchar *path_str;
	GdkAtom selection_data_selection;
	GdkAtom selection_data_target;
	GdkAtom selection_data_type;
	gint selection_data_format;
	gint selection_data_length;

	g_return_val_if_fail( FMA_IS_TREE_MODEL( drag_dest ), result );

	model = FMA_TREE_MODEL( drag_dest );

	if( !model->private->dispose_has_run ){

		g_debug( "%s: drag_dest=%p (ref_count=%d), dest=%p, selection_data=%p",
				thisfn,
				( void * ) drag_dest, G_OBJECT( drag_dest )->ref_count,
				( void * ) dest,
				( void * ) selection_data );

		selection_data_selection = gtk_selection_data_get_selection( selection_data );
		atom_name = gdk_atom_name( selection_data_selection );
		g_debug( "%s: selection=%s", thisfn, atom_name );
		g_free( atom_name );

		selection_data_target = gtk_selection_data_get_target( selection_data );
		atom_name = gdk_atom_name( selection_data_target );
		g_debug( "%s: target=%s", thisfn, atom_name );
		g_free( atom_name );

		selection_data_type = gtk_selection_data_get_data_type( selection_data );
		atom_name = gdk_atom_name( selection_data_type );
		g_debug( "%s: type=%s", thisfn, atom_name );
		g_free( atom_name );

		selection_data_format = gtk_selection_data_get_format( selection_data );
		selection_data_length = gtk_selection_data_get_length( selection_data );
		g_debug( "%s: format=%d, length=%d", thisfn, selection_data_format, selection_data_length );

		info = target_atom_to_id( selection_data_type );
		g_debug( "%s: info=%u", thisfn, info );

		path_str = gtk_tree_path_to_string( dest );
		g_debug( "%s: dest_path=%s", thisfn, path_str );
		g_free( path_str );

		switch( info ){
			case FMA_XCHANGE_FORMAT_FMA:
				result = drop_inside( model, dest, selection_data );
				break;

			/* drop some actions from outside
			 * most probably from the file manager as a list of uris
			 */
			case FMA_XCHANGE_FORMAT_URI_LIST:
				result = drop_uri_list( model, dest, selection_data );
				break;

			default:
				break;
		}
	}

	return( result );
}

/**
 * fma_tree_model_dnd_idrag_dest_row_drop_possible:
 * @drag_dest:
 * @dest_path:
 * @selection_data:
 *
 * Seems to only be called when the drop in _on_ a row (a square is
 * displayed), but not when dropped between two rows (a line is displayed),
 * nor during the motion.
 */
gboolean
fma_tree_model_dnd_idrag_dest_row_drop_possible( GtkTreeDragDest *drag_dest, GtkTreePath *dest_path, GtkSelectionData *selection_data )
{
	static const gchar *thisfn = "fma_tree_model_dnd_idrag_dest_row_drop_possible";

	g_debug( "%s: drag_dest=%p, dest_path=%p, selection_data=%p", thisfn, ( void * ) drag_dest, ( void * ) dest_path, ( void * ) selection_data );

	return( TRUE );
}

/**
 * fma_tree_model_dnd_imulti_drag_source_drag_data_get:
 * @context: contains
 *  - the suggested action, as chosen by the drop site,
 *    between those we have provided in imulti_drag_source_get_drag_actions()
 *  - the target folder (XDS protocol)
 * @selection_data:
 * @rows: list of row references which are to be dropped
 * @info: the suggested format, as chosen by the drop site, between those
 *  we have provided in imulti_drag_source_get_target_list()
 *
 * This function is called when we release the selected items onto the
 * destination
 *
 * FMA_XCHANGE_FORMAT_FMA:
 * internal format for drag and drop inside the treeview:
 * - copy in the clipboard the list of row references
 * - selection data is empty
 *
 * FMA_XCHANGE_FORMAT_XDS:
 * exchange format to drop to outside:
 * - copy in the clipboard the list of row references
 * - set the destination folder
 * - selection data is 'success' or 'failure'
 *
 * FMA_XCHANGE_FORMAT_APPLICATION_XML:
 * FMA_XCHANGE_FORMAT_TEXT_PLAIN:
 * exchange format to export to outside:
 * - do not use dnd clipboard
 * - selection data receives the export in text format
 *
 * Returns: %TRUE if required data was actually provided by the source,
 * %FALSE else.
 */
gboolean
fma_tree_model_dnd_imulti_drag_source_drag_data_get( EggTreeMultiDragSource *drag_source,
				   GdkDragContext         *context,
				   GtkSelectionData       *selection_data,
				   GList                  *rows,
				   guint                   info )
{
	static const gchar *thisfn = "fma_tree_model_dnd_imulti_drag_source_drag_data_get";
	gchar *atom_name;
	FMATreeModel *model;
	gchar *data;
	gboolean ret = FALSE;
	gchar *dest_folder, *folder;
	gboolean is_writable;
	gboolean copy_data;
	GdkAtom selection_data_target;
	GdkDragAction context_suggested_action;
	GdkDragAction context_selected_action;

	selection_data_target = gtk_selection_data_get_target( selection_data );

	context_suggested_action = gdk_drag_context_get_suggested_action( context );
	context_selected_action = gdk_drag_context_get_selected_action( context );

	atom_name = gdk_atom_name( selection_data_target );
	g_debug( "%s: drag_source=%p (ref_count=%d), context=%p, suggested action=%d, selection_data=%p, rows=%p (count=%d), atom=%s",
			thisfn,
			( void * ) drag_source, G_OBJECT( drag_source )->ref_count,
			( void * ) context,
			( int ) context_suggested_action,
			( void * ) selection_data,
			( void * ) rows, g_list_length( rows ),
			atom_name );
	g_free( atom_name );

	model = FMA_TREE_MODEL( drag_source );
	g_return_val_if_fail( model->private->window, FALSE );

	if( !model->private->dispose_has_run ){

		if( !rows || !g_list_length( rows )){
			return( FALSE );
		}

		switch( info ){
			case FMA_XCHANGE_FORMAT_FMA:
				copy_data = ( context_selected_action == GDK_ACTION_COPY );
				gtk_selection_data_set( selection_data,
						selection_data_target, 8, ( guchar * ) "", 0 );
				fma_clipboard_dnd_set( model->private->clipboard, info, rows, NULL, copy_data );
				ret = TRUE;
				break;

			case FMA_XCHANGE_FORMAT_XDS:
				/* get the dest default filename as an uri
				 * e.g. file:///home/pierre/data/eclipse/filemanager-actions/trash/xds.txt
				 */
				folder = get_xds_atom_value( context );
				dest_folder = g_path_get_dirname( folder );

				/* check that target folder is writable
				 */
				is_writable = fma_core_utils_dir_is_writable_uri( dest_folder );
				g_debug( "%s: dest_folder=%s, is_writable=%s", thisfn, dest_folder, is_writable ? "True":"False" );
				gtk_selection_data_set( selection_data,
						selection_data_target, 8, ( guchar * )( is_writable ? "S" : "F" ), 1 );

				if( is_writable ){
					fma_clipboard_dnd_set( model->private->clipboard, info, rows, dest_folder, TRUE );
				}

				g_free( dest_folder );
				g_free( folder );
				ret = is_writable;
				break;

			case FMA_XCHANGE_FORMAT_APPLICATION_XML:
			case FMA_XCHANGE_FORMAT_TEXT_PLAIN:
				data = fma_clipboard_dnd_get_text( model->private->clipboard, rows );
				gtk_selection_data_set( selection_data,
						selection_data_target, 8, ( guchar * ) data, strlen( data ));
				g_free( data );
				ret = TRUE;
				break;

			default:
				break;
		}
	}

	return( ret );
}

/**
 * fma_tree_model_dnd_imulti_drag_source_drag_data_delete:
 * @drag_source:
 * @rows:
 */
gboolean
fma_tree_model_dnd_imulti_drag_source_drag_data_delete( EggTreeMultiDragSource *drag_source, GList *rows )
{
	static const gchar *thisfn = "fma_tree_model_dnd_imulti_drag_source_drag_data_delete";

	g_debug( "%s: drag_source=%p, path_list=%p", thisfn, ( void * ) drag_source, ( void * ) rows );

	return( TRUE );
}

/**
 * fma_tree_model_dnd_imulti_drag_source_get_drag_actions:
 * @drag_source:
 */
GdkDragAction
fma_tree_model_dnd_imulti_drag_source_get_drag_actions( EggTreeMultiDragSource *drag_source )
{
	return( GDK_ACTION_COPY | GDK_ACTION_MOVE );
}

GtkTargetList *
fma_tree_model_dnd_imulti_drag_source_get_format_list( EggTreeMultiDragSource *drag_source )
{
	GtkTargetList *target_list;

	target_list = gtk_target_list_new( dnd_source_formats, G_N_ELEMENTS( dnd_source_formats ));

	return( target_list );
}

/**
 * fma_tree_model_dnd_imulti_drag_source_row_draggable:
 * @drag_source:
 * @rows:
 *
 * All selectable rows are draggable.
 * Nonetheless, it's a good place to store the dragged row references.
 * We only make use of them in on_drag_motion handler.
 */
gboolean
fma_tree_model_dnd_imulti_drag_source_row_draggable( EggTreeMultiDragSource *drag_source, GList *rows )
{
	static const gchar *thisfn = "fma_tree_model_dnd_imulti_drag_source_row_draggable";
	FMATreeModel *model;
	GtkTreeModel *store;
	GtkTreePath *path;
	GtkTreeIter iter;
	FMAObject *object;
	GList *it;

	g_debug( "%s: drag_source=%p (ref_count=%d), rows=%p (%d items)",
			thisfn,
			( void * ) drag_source, G_OBJECT( drag_source )->ref_count,
			( void * ) rows, g_list_length( rows ));

	g_return_val_if_fail( FMA_IS_TREE_MODEL( drag_source ), FALSE );
	model = FMA_TREE_MODEL( drag_source );

	if( !model->private->dispose_has_run ){

		model->private->drag_has_profiles = FALSE;
		store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));

		for( it = rows ; it && !model->private->drag_has_profiles ; it = it->next ){

			path = gtk_tree_row_reference_get_path(( GtkTreeRowReference * ) it->data );
			gtk_tree_model_get_iter( store, &iter, path );
			gtk_tree_model_get( store, &iter, TREE_COLUMN_NAOBJECT, &object, -1 );

			if( FMA_IS_OBJECT_PROFILE( object )){
				model->private->drag_has_profiles = TRUE;
			}

			g_object_unref( object );
			gtk_tree_path_free( path );
		}
	}

	return( TRUE );
}

/**
 * fma_tree_model_dnd_on_drag_begin:
 * @widget: the GtkTreeView from which item is to be dragged.
 * @context:
 * @window: the parent #FMAMainWindow instance.
 *
 * This function is called once, at the beginning of the drag operation,
 * when we are dragging from the IActionsList treeview.
 */
void
fma_tree_model_dnd_on_drag_begin( GtkWidget *widget, GdkDragContext *context, BaseWindow *window )
{
	static const gchar *thisfn = "fma_tree_model_dnd_on_drag_begin";
	FMATreeModel *model;
	GdkWindow *context_source_window;

	g_return_if_fail( GTK_IS_TREE_VIEW( widget ));
	model = ( FMATreeModel * ) gtk_tree_view_get_model( GTK_TREE_VIEW( widget ));
	g_return_if_fail( FMA_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		g_debug( "%s: widget=%p, context=%p, window=%p, model=%p (ref_count=%d)",
				thisfn,
				( void * ) widget,
				( void * ) context,
				( void * ) window,
				( void * ) model, G_OBJECT( model )->ref_count );

		model->private->drag_highlight = FALSE;
		model->private->drag_drop = FALSE;
		fma_clipboard_dnd_clear( model->private->clipboard );
		context_source_window = gdk_drag_context_get_source_window( context );

		gdk_property_change(
				context_source_window,
				XDS_ATOM, TEXT_ATOM, 8, GDK_PROP_MODE_REPLACE, ( guchar * ) XDS_FILENAME, strlen( XDS_FILENAME ));
	}
}

/**
 * fma_tree_model_dnd_on_drag_end:
 * @widget:
 * @context:
 * @window:
 */
void
fma_tree_model_dnd_on_drag_end( GtkWidget *widget, GdkDragContext *context, BaseWindow *window )
{
	static const gchar *thisfn = "fma_tree_model_dnd_on_drag_end";
	FMATreeModel *model;
	GdkWindow *context_source_window;

	g_return_if_fail( GTK_IS_TREE_VIEW( widget ));
	model = ( FMATreeModel * ) gtk_tree_view_get_model( GTK_TREE_VIEW( widget ));
	g_return_if_fail( FMA_IS_TREE_MODEL( model ));

	if( !model->private->dispose_has_run ){

		g_debug( "%s: widget=%p, context=%p, window=%p, model=%p (ref_count=%d)",
				thisfn,
				( void * ) widget,
				( void * ) context,
				( void * ) window,
				( void * ) model, G_OBJECT( model )->ref_count );

		fma_clipboard_dnd_drag_end( model->private->clipboard );
		fma_clipboard_dnd_clear( model->private->clipboard );
		context_source_window = gdk_drag_context_get_source_window( context );
		gdk_property_delete( context_source_window, XDS_ATOM );
	}
}

/*
 * called when a drop occurs in the treeview for a move/copy inside of
 * the tree
 *
 * Returns: %TRUE if the specified rows were successfully inserted at
 * the given dest, %FALSE else.
 */
static gboolean
drop_inside( FMATreeModel *model, GtkTreePath *dest, GtkSelectionData  *selection_data )
{
	static const gchar *thisfn = "fma_tree_model_dnd_inside_drag_and_drop";
	FMAApplication *application;
	FMAUpdater *updater;
	FMAMainWindow *main_window;
	FMAObjectItem *parent;
	gboolean copy_data;
	GList *rows;
	GtkTreePath *new_dest;
	GtkTreePath *path;
	FMAObject *current;
	FMAObject *inserted;
	GList *object_list, *it;
	GtkTreeIter iter;
	GList *deletable;
	gboolean relabel;
	FMATreeView *items_view;

	application = FMA_APPLICATION( gtk_window_get_application( GTK_WINDOW( model->private->window )));
	updater = fma_application_get_updater( application );

	g_return_val_if_fail( FMA_IS_MAIN_WINDOW( model->private->window ), FALSE );
	main_window = FMA_MAIN_WINDOW( model->private->window );
	items_view = fma_main_window_get_items_view( main_window );

	/*
	 * fma-config-tool format (may embed profiles, or not)
	 * 	with profiles: only valid dest is inside an action
	 *  without profile: only valid dest is outside (besides of) an action
	 */
	parent = NULL;
	rows = fma_clipboard_dnd_get_data( model->private->clipboard, &copy_data );

	if( !is_drop_possible( model, dest, &parent )){
		return( FALSE );
	}

	new_dest = gtk_tree_path_copy( dest );
	if( !copy_data ){
		drop_inside_move_dest( model, rows, &new_dest );
	}

	g_debug( "%s: rows has %d items, copy_data=%s", thisfn, g_list_length( rows ), copy_data ? "True":"False" );
	object_list = NULL;
	for( it = rows ; it ; it = it->next ){
		path = gtk_tree_row_reference_get_path(( GtkTreeRowReference * ) it->data );
		if( path ){
			if( gtk_tree_model_get_iter( GTK_TREE_MODEL( model ), &iter, path )){
				gtk_tree_model_get( GTK_TREE_MODEL( model ), &iter, TREE_COLUMN_NAOBJECT, &current, -1 );
				g_object_unref( current );

				if( copy_data ){
					inserted = ( FMAObject * ) fma_object_duplicate( current, FMA_DUPLICATE_REC );
					fma_object_set_origin( inserted, NULL );
					fma_object_check_status( inserted );
					relabel = fma_updater_should_pasted_be_relabeled( updater, inserted );

				} else {
					inserted = fma_object_ref( current );
					deletable = g_list_prepend( NULL, inserted );
					fma_tree_ieditable_delete( FMA_TREE_IEDITABLE( items_view ), deletable, TREE_OPE_MOVE );
					g_list_free( deletable );
					relabel = FALSE;
				}

				fma_object_prepare_for_paste( inserted, relabel, copy_data, parent );
				object_list = g_list_prepend( object_list, inserted );
				g_debug( "%s: dropped=%s", thisfn, fma_object_get_label( inserted ));
			}
			gtk_tree_path_free( path );
		}
	}
	object_list = g_list_reverse( object_list );

	fma_tree_ieditable_insert_at_path( FMA_TREE_IEDITABLE( items_view ), object_list, new_dest );

	fma_object_free_items( object_list );
	gtk_tree_path_free( new_dest );

	g_list_foreach( rows, ( GFunc ) gtk_tree_row_reference_free, NULL );
	g_list_free( rows );

	return( TRUE );
}

/*
 * is a drop possible at given dest ?
 *
 * the only case where we would be led to have to modify the dest if
 * we'd want be able to drop a profile into another profile, accepting
 * it, actually dropping the profile just before the target
 *
 * -> it appears both clearer for the user interface and easyer from a
 *    code point of view to just refuse to drop a profile into a profile
 *
 * so this function is just to check if a drop is possible at the given
 * dest
 */
static gboolean
is_drop_possible( FMATreeModel *model, GtkTreePath *dest, FMAObjectItem **parent )
{
	FMATreeModelPrivate *priv;
	gboolean drop_ok;
	GtkApplication *application;
	FMAMainWindow *main_window;
	GtkTreeIter iter;
	FMAObjectItem *parent_dest;
	FMAStatusBar *bar;

	priv = model->private;
	drop_ok = FALSE;
	parent_dest = NULL;
	application = gtk_window_get_application( GTK_WINDOW( priv->window ));
	g_return_val_if_fail( application && FMA_IS_APPLICATION( application ), FALSE );

	g_return_val_if_fail( priv->window && FMA_IS_MAIN_WINDOW( priv->window ), FALSE );
	main_window = FMA_MAIN_WINDOW( priv->window );

	/* if we can have an iter on given dest, then the dest already exists
	 * so dropped items should be of the same type that already existing
	 */
	if( gtk_tree_model_get_iter( GTK_TREE_MODEL( model ), &iter, dest )){
		drop_ok = is_drop_possible_before_iter( model, &iter, main_window, &parent_dest );

	/* inserting at the end of the list
	 * parent_dest is NULL
	 */
	} else if( gtk_tree_path_get_depth( dest ) == 1 ){

		if( priv->drag_has_profiles ){
			bar = fma_main_window_get_statusbar( main_window );
			fma_status_bar_display_with_timeout(
						bar, TREE_MODEL_STATUSBAR_CONTEXT, gettext( st_refuse_drop_profile ));

		} else {
			drop_ok = TRUE;
		}

	/* we cannot have an iter on the dest: this means that we try to
	 * insert items into not-opened dest (an empty menu or an action with
	 * zero or one profile) : check what is the parent
	 */
	} else {
		drop_ok = is_drop_possible_into_dest( model, dest, main_window, &parent_dest );
	}

	if( drop_ok ){
		drop_ok = is_parent_accept_new_children(
				FMA_APPLICATION( application ), main_window, parent_dest );
	}

	if( drop_ok && parent ){
		*parent = parent_dest;
	}

	return( drop_ok );
}

static gboolean
is_drop_possible_before_iter( FMATreeModel *model, GtkTreeIter *iter, FMAMainWindow *window, FMAObjectItem **parent )
{
	static const gchar *thisfn = "fma_tree_model_dnd_is_drop_possible_before_iter";
	gboolean drop_ok;
	FMAObject *object;
	FMAStatusBar *bar;

	drop_ok = FALSE;
	*parent = NULL;
	bar = fma_main_window_get_statusbar( window );

	gtk_tree_model_get( GTK_TREE_MODEL( model ), iter, TREE_COLUMN_NAOBJECT, &object, -1 );
	g_object_unref( object );
	g_debug( "%s: current object at dest is %s", thisfn, G_OBJECT_TYPE_NAME( object ));

	if( model->private->drag_has_profiles ){

		if( FMA_IS_OBJECT_PROFILE( object )){
			drop_ok = TRUE;
			*parent = fma_object_get_parent( object );

		} else {
			/* unable to drop a profile here */
			fma_status_bar_display_with_timeout(
					bar, TREE_MODEL_STATUSBAR_CONTEXT, gettext( st_refuse_drop_profile ));
		}

	} else if( FMA_IS_OBJECT_ITEM( object )){
			drop_ok = TRUE;
			*parent = fma_object_get_parent( object );

	} else {
		/* unable to drop an action or a menu here */
		fma_status_bar_display_with_timeout(
				bar, TREE_MODEL_STATUSBAR_CONTEXT, gettext( st_refuse_drop_item ));
	}

	return( drop_ok );
}

static gboolean
is_drop_possible_into_dest( FMATreeModel *model, GtkTreePath *dest, FMAMainWindow *window, FMAObjectItem **parent )
{
	static const gchar *thisfn = "fma_tree_model_dnd_is_drop_possible_into_dest";
	gboolean drop_ok;
	GtkTreePath *path;
	GtkTreeIter iter;
	FMAObject *object;
	FMAStatusBar *bar;

	drop_ok = FALSE;
	*parent = NULL;
	bar = fma_main_window_get_statusbar( window );

	path = gtk_tree_path_copy( dest );

	if( gtk_tree_path_up( path )){
		if( gtk_tree_model_get_iter( GTK_TREE_MODEL( model ), &iter, path )){
			gtk_tree_model_get( GTK_TREE_MODEL( model ), &iter, TREE_COLUMN_NAOBJECT, &object, -1 );
			g_object_unref( object );
			g_debug( "%s: current object at parent dest is %s", thisfn, G_OBJECT_TYPE_NAME( object ));

			if( model->private->drag_has_profiles ){

				if( FMA_IS_OBJECT_ACTION( object )){
					drop_ok = TRUE;
					*parent = FMA_OBJECT_ITEM( object );

				} else {
					fma_status_bar_display_with_timeout(
							bar, TREE_MODEL_STATUSBAR_CONTEXT, gettext( st_refuse_drop_profile ));
				}

			} else if( FMA_IS_OBJECT_MENU( object )){
					drop_ok = TRUE;
					*parent = fma_object_get_parent( object );

			} else {
				fma_status_bar_display_with_timeout(
						bar, TREE_MODEL_STATUSBAR_CONTEXT, gettext( st_refuse_drop_item ));
			}
		}
	}

	gtk_tree_path_free( path );

	return( drop_ok );
}

static void
drop_inside_move_dest( FMATreeModel *model, GList *rows, GtkTreePath **dest )
{
	GList *it;
	GtkTreePath *path;
	gint before;
	gint i;
	gint *indices;

	g_return_if_fail( dest );

	before = 0;
	for( it = rows ; it ; it = it->next ){
		path = gtk_tree_row_reference_get_path(( GtkTreeRowReference * ) it->data );
		if( path ){
			if( gtk_tree_path_get_depth( path ) == 1 && gtk_tree_path_compare( path, *dest ) == -1 ){
				before += 1;
			}
			gtk_tree_path_free( path );
		}
	}

	g_debug( "drop_inside_move_dest: before=%d", before );
	g_debug( "drop_inside_move_dest: dest=%s", gtk_tree_path_to_string( *dest ));

	if( before ){
		indices = gtk_tree_path_get_indices( *dest );
		indices[0] -= before;
		path = gtk_tree_path_new_from_indices( indices[0], -1 );
		for( i = 1 ; i < gtk_tree_path_get_depth( *dest ); ++i ){
			gtk_tree_path_append_index( path, indices[i] );
		}
		gtk_tree_path_free( *dest );
		*dest = gtk_tree_path_copy( path );
		gtk_tree_path_free( path );
	}

	g_debug( "drop_inside_move_dest: dest=%s", gtk_tree_path_to_string( *dest ));
}

/*
 * called when a drop from the outside occurs in the treeview
 *
 * Returns: %TRUE if the specified rows were successfully inserted at
 * the given dest, %FALSE else.
 *
 * URI format only involves actions or menus
 *  so ony valid dest in outside (besides of) an action
 */
static gboolean
drop_uri_list( FMATreeModel *model, GtkTreePath *dest, GtkSelectionData  *selection_data )
{
	static const gchar *thisfn = "fma_tree_model_dnd_drop_uri_list";
	FMATreeModelPrivate *priv;
	gboolean drop_done;
	GtkApplication *application;
	FMAUpdater *updater;
	FMAMainWindow *main_window;
	FMAImporterParms parms;
	GList *import_results, *it;
	guint count;
	GSList *im;
	GList *imported, *overridden;
	const gchar *selection_data_data;
	FMATreeView *view;
	GSList *messages;
	gchar *dlg_message;
	GtkWidget *dialog;
	FMAStatusBar *bar;

	priv = model->private;
	gchar *dest_str = gtk_tree_path_to_string( dest );
	g_debug( "%s: model=%p, dest=%p (%s), selection_data=%p",
			thisfn, ( void * ) model, ( void * ) dest, dest_str, ( void * ) selection_data );
	g_free( dest_str );

	drop_done = FALSE;
	model->private->drag_has_profiles = FALSE;

	if( !is_drop_possible( model, dest, NULL )){
		return( FALSE );
	}

	application = gtk_window_get_application( GTK_WINDOW( priv->window ));
	g_return_val_if_fail( application && FMA_IS_APPLICATION( application ), FALSE );
	updater = fma_application_get_updater( FMA_APPLICATION( application ));

	g_return_val_if_fail( FMA_IS_MAIN_WINDOW( priv->window ), FALSE );
	main_window = FMA_MAIN_WINDOW( priv->window );

	selection_data_data = ( const gchar * ) gtk_selection_data_get_data( selection_data );
	g_debug( "%s", selection_data_data );

	memset( &parms, '\0', sizeof( FMAImporterParms ));
	parms.uris = g_slist_reverse( fma_core_utils_slist_from_split( selection_data_data, "\r\n" ));
	parms.check_fn = ( FMAImporterCheckFn ) is_dropped_already_exists;
	parms.check_fn_data = main_window;
	parms.preferred_mode = 0;
	parms.parent_toplevel = GTK_WINDOW( main_window );

	import_results = fma_importer_import_from_uris( FMA_PIVOT( updater ), &parms );

	/* analysing output results, simultaneously building a concatenation
	 * of all lines of messages, and the list of imported items
	 */
	imported = NULL;
	overridden = NULL;
	messages = NULL;

	for( it = import_results ; it ; it = it->next ){
		FMAImporterResult *result = ( FMAImporterResult * ) it->data;

		for( im = result->messages ; im ; im = im->next ){
			messages = g_slist_prepend( messages, im->data );
		}
		if( result->imported ){
			if( !result->exist || result->mode == IMPORTER_MODE_RENUMBER ){
				imported = g_list_prepend( imported, result->imported );
				fma_updater_check_item_writability_status( updater, result->imported );

			} else if( result->mode == IMPORTER_MODE_OVERRIDE ){
				overridden = g_list_prepend( overridden, result->imported );
			}
		}
	}

	/* if there is more than one message, display them in a dialog box
	 * else in the status bar
	 */
	count = g_slist_length( messages );
	g_debug( "%s: count=%d", thisfn, count );
	if( count == 1 ){
		bar = fma_main_window_get_statusbar( main_window );
		fma_status_bar_display_with_timeout(
				bar, TREE_MODEL_STATUSBAR_CONTEXT, messages->data );
	}
	if( count > 1 ){
		dlg_message = fma_core_utils_slist_join_at_end( messages, "\n" );
		g_debug( "%s: dlg_message='%s'", thisfn, dlg_message );
		dialog = gtk_message_dialog_new(
				parms.parent_toplevel,
				GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_CLOSE,
				"%s", _( "Some messages have occurred during drop operation." ));
		gtk_message_dialog_format_secondary_markup( GTK_MESSAGE_DIALOG( dialog ), "%s", dlg_message );
		gtk_dialog_run( GTK_DIALOG( dialog ));
		gtk_widget_destroy( dialog );
		g_free( dlg_message );
	}
	g_slist_free( messages );

	/* insert newly imported items in the list view
	 */
	if( imported ){
		fma_object_dump_tree( imported );
		view = fma_main_window_get_items_view( main_window );
		fma_tree_ieditable_insert_at_path( FMA_TREE_IEDITABLE( view ), imported, dest );
	}

	/* override items if needed
	 * they may safely be released after having updated the store
	 */
	if( overridden ){
		fma_object_dump_tree( overridden );
		view = fma_main_window_get_items_view( main_window );
		fma_tree_ieditable_set_items( FMA_TREE_IEDITABLE( view ), overridden );
		fma_object_free_items( overridden );
	}

	drop_done = TRUE;
	fma_object_free_items( imported );
	fma_object_free_items( overridden );
	fma_core_utils_slist_free( parms.uris );

	for( it = import_results ; it ; it = it->next ){
		fma_importer_free_result( it->data );
	}
	g_list_free( import_results );

	return( drop_done );
}

static FMAObjectItem *
is_dropped_already_exists( const FMAObjectItem *importing, const FMAMainWindow *window )
{
	FMATreeView *items_view;

	gchar *id = fma_object_get_id( importing );
	items_view = fma_main_window_get_items_view( window );
	FMAObjectItem *exists = fma_tree_view_get_item_by_id( items_view, id );
	g_free( id );

	return( exists );
}

/*
 * this function works well, but only called from on_drag_motion handler...
 */
/*static gboolean
is_row_drop_possible( FMATreeModel *model, GtkTreePath *path, GtkTreeViewDropPosition pos )
{
	gboolean ok = FALSE;
	GtkTreeModel *store;
	GtkTreeIter iter;
	FMAObject *object;

	store = gtk_tree_model_filter_get_model( GTK_TREE_MODEL_FILTER( model ));
	gtk_tree_model_get_iter( store, &iter, path );
	gtk_tree_model_get( store, &iter, IACTIONS_LIST_NAOBJECT_COLUMN, &object, -1 );

	if( model->private->drag_has_profiles ){
		if( FMA_IS_OBJECT_PROFILE( object )){
			ok = TRUE;
		} else if( FMA_IS_OBJECT_ACTION( object )){
			ok = ( pos == GTK_TREE_VIEW_DROP_INTO_OR_BEFORE || pos == GTK_TREE_VIEW_DROP_INTO_OR_AFTER );
		}
	} else {
		if( FMA_IS_OBJECT_ITEM( object )){
			ok = TRUE;
		}
	}

	g_object_unref( object );
	return( ok );
}*/

/*
 * called when the user moves into the target widget
 * returns TRUE if a drop zone
 */
#if 0
static gboolean
on_drag_motion( GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, BaseWindow *window )
{
	FMATreeModel *model;
	gboolean ok = FALSE;
	GtkTreePath *path;
	GtkTreeViewDropPosition pos;

	model = FMA_TREE_MODEL( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));
	g_return_val_if_fail( FMA_IS_TREE_MODEL( model ), FALSE );

	if( !model->private->dispose_has_run ){

		if( !model->private->drag_highlight ){
			model->private->drag_highlight = TRUE;
			gtk_drag_highlight( widget );
		}

		/*target = gtk_drag_dest_find_target( widget, context, NULL );
		if( target == GDK_NONE ){
			gdk_drag_status( context, 0, time );
		} else {
			gtk_drag_get_data( widget, context, target, time );
		}*/

		if( gtk_tree_view_get_dest_row_at_pos( GTK_TREE_VIEW( widget ), x, y, &path, &pos )){
			ok = is_row_drop_possible( model, path, pos );
			if( ok ){
				gdk_drag_status( context, 0, time );
			}
		}

		gtk_tree_path_free( path );
		g_debug( "fma_tree_model_on_drag_motion: ok=%s, pos=%d", ok ? "True":"False", pos );
	}

	return( ok );
}
#endif

/*
 * called when the user drops the data
 * returns TRUE if a drop zone
 */
/*static gboolean
on_drag_drop( GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time, BaseWindow *window )
{
	FMATreeModel *model;

	model = FMA_TREE_MODEL( gtk_tree_view_get_model( GTK_TREE_VIEW( widget )));
	g_return_val_if_fail( FMA_IS_TREE_MODEL( model ), FALSE );

	if( !model->private->dispose_has_run ){

		model->private->drag_drop = TRUE;
	}

	return( TRUE );
}*/

/* The following function taken from bugzilla
 * (http://bugzilla.gnome.org/attachment.cgi?id=49362&action=view)
 * Author: Christian Neumair
 * Copyright: 2005 Free Software Foundation, Inc
 * License: GPL
 *
 * On a 32-bits system:
 * get_xds_atom_value: actual_length=63, actual_length=15
 * get_xds_atom_value: ret=file:///home/pierre/data/eclipse/filemanager-actions/trash/xds.txt0x8299
 * get_xds_atom_value: dup=file:///home/pierre/data/eclipse/filemanager-actions/trash/xds.txt
 * get_xds_atom_value: ret=file:///home/pi
 *
 * idem on a 64bits system.
 */
static char *
get_xds_atom_value( GdkDragContext *context )
{
	gchar *ret;
	gint actual_length;
	GdkWindow *context_source_window;

	g_return_val_if_fail( context != NULL, NULL );

	context_source_window = gdk_drag_context_get_source_window( context );

	g_return_val_if_fail( context_source_window != NULL, NULL );

	gdk_property_get( context_source_window,		/* a GdkWindow */
						XDS_ATOM, 					/* the property to retrieve */
						TEXT_ATOM,					/* the desired property type */
						0, 							/* offset (in 4 bytes chunks) */
						MAX_XDS_ATOM_VAL_LEN,		/* max length (in 4 bytes chunks) */
						FALSE, 						/* whether to delete the property */
						NULL, 						/* actual property type */
						NULL, 						/* actual format */
						&actual_length,				/* actual length (in 4 bytes chunks) */
						( guchar ** ) &ret );		/* data pointer */

	ret[actual_length] = '\0';

	return( ret );
}

/*
 * when dropping something somewhere, we must ensure that we will be able
 * to register the new child
 */
static gboolean
is_parent_accept_new_children( FMAApplication *application, FMAMainWindow *window, FMAObjectItem *parent )
{
	gboolean accept_ok;
	FMAUpdater *updater;
	FMAStatusBar *bar;

	accept_ok = FALSE;
	updater = fma_application_get_updater( application );
	bar = fma_main_window_get_statusbar( window );

	/* inserting as a level zero item
	 * ensure that level zero is writable
	 */
	if( parent == NULL ){
		if( fma_updater_is_level_zero_writable( updater )){
			accept_ok = TRUE;

		} else {
			fma_status_bar_display_with_timeout(
						bar, TREE_MODEL_STATUSBAR_CONTEXT, gettext( st_level_zero_not_writable ));
		}

	/* see if the parent is writable
	 */
	} else if( fma_object_is_finally_writable( parent, NULL )){
		accept_ok = TRUE;

	} else {
			fma_status_bar_display_with_timeout(
						bar, TREE_MODEL_STATUSBAR_CONTEXT, gettext( st_parent_not_writable ));
	}

	return( accept_ok );
}

static guint
target_atom_to_id( GdkAtom atom )
{
	gint i;
	guint info = 0;
	gchar *atom_name;

	atom_name = gdk_atom_name( atom );
	for( i = 0 ; i < tree_model_dnd_dest_formats_count ; ++i ){
		if( !g_ascii_strcasecmp( tree_model_dnd_dest_formats[i].target, atom_name )){
			info = tree_model_dnd_dest_formats[i].info;
			break;
		}
	}
	g_free( atom_name );
	return( info );
}
