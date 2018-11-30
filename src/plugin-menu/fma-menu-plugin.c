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

#include <string.h>
#include <glib/gi18n.h>

#include <api/fma-core-utils.h>
#include <api/fma-fm-defines.h>
#include <api/fma-object-api.h>
#include <api/fma-timeout.h>

#include <core/fma-pivot.h>
#include <core/fma-about.h>
#include <core/fma-selected-info.h>
#include <core/fma-tokens.h>

#include "fma-menu-plugin.h"

/* private class data
 */
struct _FMAMenuPluginClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMAMenuPluginPrivate {
	gboolean   dispose_has_run;
	FMAPivot  *pivot;
	gulong     items_changed_handler;
	gulong     settings_changed_handler;
	FMATimeout change_timeout;
};

static GObjectClass *st_parent_class  = NULL;
static GType         st_actions_type  = 0;
static gint          st_burst_timeout = 100;		/* burst timeout in msec */

static void                 class_init( FMAMenuPluginClass *klass );
static void                 instance_init( GTypeInstance *instance, gpointer klass );
static void                 instance_constructed( GObject *object );
static void                 instance_dispose( GObject *object );
static void                 instance_finalize( GObject *object );
static void                 menu_provider_iface_init( FileManagerMenuProviderIface *iface );
static GList               *menu_provider_get_background_items( FileManagerMenuProvider *provider, GtkWidget *window, FileManagerFileInfo *current_folder );
static GList               *menu_provider_get_file_items( FileManagerMenuProvider *provider, GtkWidget *window, GList *files );
#if defined( HAVE_NAUTILUS_MENU_PROVIDER_GET_TOOLBAR_ITEMS ) || \
	defined( HAVE_NEMO_MENU_PROVIDER_GET_TOOLBAR_ITEMS )
static GList               *menu_provider_get_toolbar_items( FileManagerMenuProvider *provider, GtkWidget *window, FileManagerFileInfo *current_folder );
#endif
static GList               *selected_info_get_list_from_item( FileManagerFileInfo *item );
static GList               *selected_info_get_list_from_list( GList *selection );
static FMASelectedInfo     *new_from_file_manager_file_info( FileManagerFileInfo *item );
static GList               *build_filemanager_menu( FMAMenuPlugin *plugin, guint target, GList *selection );
static GList               *build_filemanager_menu_rec( GList *tree, guint target, GList *selection, FMATokens *tokens );
static void                 attach_submenu_to_item( FileManagerMenuItem *item, GList *subitems );
static void                 weak_notify_profile( FMAObjectProfile *profile, FileManagerMenuItem *item );
static void                 execute_action( FileManagerMenuItem *item, FMAObjectProfile *profile );
static void                 execute_about( FileManagerMenuItem *item, FMAMenuPlugin *plugin );
static FileManagerMenuItem *create_item_from_profile( FMAObjectProfile *profile, guint target, GList *files, FMATokens *tokens );
static FileManagerMenuItem *create_item_from_menu( FMAObjectMenu *menu, GList *subitems, guint target );
static FileManagerMenuItem *create_menu_item( const FMAObjectItem *item, guint target );
static FMAObjectItem       *expand_tokens_item( const FMAObjectItem *item, FMATokens *tokens );
static void                 expand_tokens_context( FMAIContext *context, FMATokens *tokens );
static FMAObjectProfile    *get_candidate_profile( FMAObjectAction *action, guint target, GList *files );
static GList               *create_root_menu( FMAMenuPlugin *plugin, GList *filemanager_menu );
static void                 weak_notify_menu_item( void *user_data /* =NULL */, FileManagerMenuItem *item );
static GList               *add_about_item( FMAMenuPlugin *plugin, GList *filemanager_menu );
static void                 on_pivot_items_changed_handler( FMAPivot *pivot, FMAMenuPlugin *plugin );
static void                 on_settings_key_changed_handler( const gchar *group, const gchar *key, gconstpointer new_value, gboolean mandatory, FMAMenuPlugin *plugin );
static void                 on_change_event_timeout( FMAMenuPlugin *plugin );

GType
fma_menu_plugin_get_type( void )
{
	g_assert( st_actions_type );
	return( st_actions_type );
}

void
fma_menu_plugin_register_type( GTypeModule *module )
{
	static const gchar *thisfn = "fma_menu_plugin_register_type";

	static const GTypeInfo info = {
		sizeof( FMAMenuPluginClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAMenuPlugin ),
		0,
		( GInstanceInitFunc ) instance_init,
	};

	static const GInterfaceInfo menu_provider_iface_info = {
		( GInterfaceInitFunc ) menu_provider_iface_init,
		NULL,
		NULL
	};

	g_assert( st_actions_type == 0 );

	g_debug( "%s: module=%p", thisfn, ( void * ) module );

	st_actions_type = g_type_module_register_type(
			module, G_TYPE_OBJECT, "FMAMenuPlugin", &info, 0 );

	g_type_module_add_interface(
			module, st_actions_type, FILE_MANAGER_TYPE_MENU_PROVIDER, &menu_provider_iface_info );
}

static void
class_init( FMAMenuPluginClass *klass )
{
	static const gchar *thisfn = "fma_menu_plugin_class_init";
	GObjectClass *gobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	gobject_class = G_OBJECT_CLASS( klass );
	gobject_class->constructed = instance_constructed;
	gobject_class->dispose = instance_dispose;
	gobject_class->finalize = instance_finalize;

	klass->private = g_new0( FMAMenuPluginClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_menu_plugin_instance_init";
	FMAMenuPlugin *self;

	g_return_if_fail( FMA_IS_MENU_PLUGIN( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = FMA_MENU_PLUGIN( instance );

	self->private = g_new0( FMAMenuPluginPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->change_timeout.timeout = st_burst_timeout;
	self->private->change_timeout.handler = ( FMATimeoutFunc ) on_change_event_timeout;
	self->private->change_timeout.user_data = self;
	self->private->change_timeout.source_id = 0;
}

/*
 * Runtime modification management:
 * We have to react to some runtime environment modifications:
 *
 * - whether the items list has changed (we have to reload a new pivot)
 *   > registering for notifications against FMAPivot
 *
 * - whether to add the 'About FileManager-Actions' item
 * - whether to create a 'FileManager-Actions actions' root menu
 *   > registering for notifications against FMASettings
 */
static void
instance_constructed( GObject *object )
{
	static const gchar *thisfn = "fma_menu_plugin_instance_constructed";
	FMAMenuPluginPrivate *priv;

	g_return_if_fail( FMA_IS_MENU_PLUGIN( object ));

	priv = FMA_MENU_PLUGIN( object )->private;

	if( !priv->dispose_has_run ){

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->constructed ){
			G_OBJECT_CLASS( st_parent_class )->constructed( object );
		}

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		priv->pivot = fma_pivot_new();

		/* setup FMAPivot properties before loading items
		 */
		fma_pivot_set_loadable( priv->pivot, !PIVOT_LOAD_DISABLED & !PIVOT_LOAD_INVALID );
		fma_pivot_load_items( priv->pivot );

		/* register against FMAPivot to be notified of items changes
		 */
		priv->items_changed_handler =
				g_signal_connect( priv->pivot,
						PIVOT_SIGNAL_ITEMS_CHANGED,
						G_CALLBACK( on_pivot_items_changed_handler ),
						object );

		/* register against FMASettings to be notified of changes on
		 *  our runtime preferences
		 * because we only monitor here a few runtime keys, we prefer the
		 * callback way that the signal one
		 */
		fma_settings_register_key_callback(
				IPREFS_IO_PROVIDERS_READ_STATUS,
				G_CALLBACK( on_settings_key_changed_handler ),
				object );

		fma_settings_register_key_callback(
				IPREFS_ITEMS_ADD_ABOUT_ITEM,
				G_CALLBACK( on_settings_key_changed_handler ),
				object );

		fma_settings_register_key_callback(
				IPREFS_ITEMS_CREATE_ROOT_MENU,
				G_CALLBACK( on_settings_key_changed_handler ),
				object );

		fma_settings_register_key_callback(
				IPREFS_ITEMS_LEVEL_ZERO_ORDER,
				G_CALLBACK( on_settings_key_changed_handler ),
				object );

		fma_settings_register_key_callback(
				IPREFS_ITEMS_LIST_ORDER_MODE,
				G_CALLBACK( on_settings_key_changed_handler ),
				object );
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "fma_menu_plugin_instance_dispose";
	FMAMenuPlugin *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( FMA_IS_MENU_PLUGIN( object ));
	self = FMA_MENU_PLUGIN( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		if( self->private->items_changed_handler ){
			g_signal_handler_disconnect( self->private->pivot, self->private->items_changed_handler );
		}
		g_object_unref( self->private->pivot );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "fma_menu_plugin_instance_finalize";
	FMAMenuPlugin *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( FMA_IS_MENU_PLUGIN( object ));
	self = FMA_MENU_PLUGIN( object );

	g_free( self->private );

	/* chain up to the parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
menu_provider_iface_init( FileManagerMenuProviderIface *iface )
{
	static const gchar *thisfn = "fma_menu_plugin_menu_provider_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_file_items = menu_provider_get_file_items;
	iface->get_background_items = menu_provider_get_background_items;

#if defined( HAVE_NAUTILUS_MENU_PROVIDER_GET_TOOLBAR_ITEMS ) || \
	defined( HAVE_NEMO_MENU_PROVIDER_GET_TOOLBAR_ITEMS )
	iface->get_toolbar_items = menu_provider_get_toolbar_items;
#endif
}

/*
 * this function is called when nautilus has to paint a folder background
 * one of the first calls is with current_folder = 'x-nautilus-desktop:///'
 * the menu items are available :
 * a) in File menu
 * b) in contextual menu of the folder if there is no current selection
 *
 * get_background_items is very similar, from the user point of view, to
 * get_file_items when:
 * - either there is zero item selected - current_folder should so be the
 *   folder currently displayed in the file manager view
 * - or when there is only one selected directory
 *
 * Note that 'x-nautilus-desktop:///' cannot be interpreted by
 * #FMASelectedInfo::query_file_attributes() function. It so never participate
 * to the display of actions.
 */
static GList *
menu_provider_get_background_items(
		FileManagerMenuProvider *provider, GtkWidget *window, FileManagerFileInfo *current_folder )
{
	static const gchar *thisfn = "fma_menu_plugin_menu_provider_get_background_items";
	GList *filemanager_menus_list = NULL;
	gchar *uri;
	GList *selected;

	g_return_val_if_fail( FMA_IS_MENU_PLUGIN( provider ), NULL );

	if( !FMA_MENU_PLUGIN( provider )->private->dispose_has_run ){

		selected = selected_info_get_list_from_item( current_folder );

		if( selected ){
			uri = file_manager_file_info_get_uri( current_folder );
			g_debug( "%s: provider=%p, window=%p, current_folder=%p (%s)",
					thisfn,
					( void * ) provider,
					( void * ) window,
					( void * ) current_folder, uri );
			g_free( uri );

			filemanager_menus_list = build_filemanager_menu(
					FMA_MENU_PLUGIN( provider ),
					ITEM_TARGET_LOCATION,
					selected );

			fma_selected_info_free_list( selected );
		}
	}

	return( filemanager_menus_list );
}

/*
 * this function is called each time the selection changed
 * menus items are available :
 * a) in Edit menu while the selection stays unchanged
 * b) in contextual menu while the selection stays unchanged
 */
static GList *
menu_provider_get_file_items( FileManagerMenuProvider *provider, GtkWidget *window, GList *files )
{
	static const gchar *thisfn = "fma_menu_plugin_menu_provider_get_file_items";
	GList *filemanager_menus_list = NULL;
	GList *selected;

	g_return_val_if_fail( FMA_IS_MENU_PLUGIN( provider ), NULL );

	if( !FMA_MENU_PLUGIN( provider )->private->dispose_has_run ){

		/* no need to go further if there is no files in the list */
		if( !g_list_length( files )){
			return(( GList * ) NULL );
		}

		selected = selected_info_get_list_from_list(( GList * ) files );

		if( selected ){
			g_debug( "%s: provider=%p, window=%p, files=%p, count=%d",
					thisfn,
					( void * ) provider,
					( void * ) window,
					( void * ) files, g_list_length( files ));

#ifdef FMA_MAINTAINER_MODE
			GList *im;
			for( im = files ; im ; im = im->next ){
				gchar *uri = file_manager_file_info_get_uri( FILE_MANAGER_FILE_INFO( im->data ));
				gchar *mimetype = file_manager_file_info_get_mime_type( FILE_MANAGER_FILE_INFO( im->data ));
				g_debug( "%s: uri='%s', mimetype='%s'", thisfn, uri, mimetype );
				g_free( mimetype );
				g_free( uri );
			}
#endif

			filemanager_menus_list = build_filemanager_menu(
					FMA_MENU_PLUGIN( provider ),
					ITEM_TARGET_SELECTION,
					selected );

			fma_selected_info_free_list( selected );
		}
	}

	return( filemanager_menus_list );
}

#if defined( HAVE_NAUTILUS_MENU_PROVIDER_GET_TOOLBAR_ITEMS ) || \
	defined( HAVE_NEMO_MENU_PROVIDER_GET_TOOLBAR_ITEMS )
/*
 * as of 2.26, this function is only called for folders, but for the
 * desktop (x-nautilus-desktop:///) which seems to be only called by
 * get_background_items ; also, only actions (not menus) are displayed
 *
 * the API is removed starting with Nautilus 2.91.90
 * the API is not present as of Nemo 2.6.7
 */
static GList *
menu_provider_get_toolbar_items( FileManagerMenuProvider *provider, GtkWidget *window, FileManagerFileInfo *current_folder )
{
	static const gchar *thisfn = "fma_menu_plugin_menu_provider_get_toolbar_items";
	GList *filemanager_menus_list = NULL;
	gchar *uri;
	GList *selected;

	g_return_val_if_fail( FMA_IS_MENU_PLUGIN( provider ), NULL );

	if( !FMA_MENU_PLUGIN( provider )->private->dispose_has_run ){

		selected = selected_info_get_list_from_item( current_folder );

		if( selected ){
			uri = file_manager_file_info_get_uri( current_folder );
			g_debug( "%s: provider=%p, window=%p, current_folder=%p (%s)",
					thisfn,
					( void * ) provider,
					( void * ) window,
					( void * ) current_folder, uri );
			g_free( uri );

			filemanager_menus_list = build_filemanager_menu(
					FMA_MENU_PLUGIN( provider ),
					ITEM_TARGET_TOOLBAR,
					selected );

			fma_selected_info_free_list( selected );
		}
	}

	return( filemanager_menus_list );
}
#endif

/*
 * selected_info_get_list_from_item:
 * @item: a #NautilusFileInfo item
 *
 * Returns: a #GList list which contains a #FMASelectedInfo item with the
 * same URI that the @item.
 */
static GList *
selected_info_get_list_from_item( FileManagerFileInfo *item )
{
	GList *selected;

	selected = NULL;
	FMASelectedInfo *info = new_from_file_manager_file_info( item );

	if( info ){
		selected = g_list_prepend( NULL, info );
	}

	return( selected );
}

/*
 * selected_info_get_list_from_list:
 * @nautilus_selection: a #GList list of #NautilusFileInfo items.
 *
 * Returns: a #GList list of #FMASelectedInfo items whose URI correspond
 * to those of @nautilus_selection.
 */
static GList *
selected_info_get_list_from_list( GList *selection )
{
	GList *selected;
	GList *it;

	selected = NULL;

	for( it = selection ; it ; it = it->next ){
		FMASelectedInfo *info = new_from_file_manager_file_info( FILE_MANAGER_FILE_INFO( it->data ));

		if( info ){
			selected = g_list_prepend( selected, info );
		}
	}

	return( selected ? g_list_reverse( selected ) : NULL );
}

static FMASelectedInfo *
new_from_file_manager_file_info( FileManagerFileInfo *item )
{
	gchar *uri = file_manager_file_info_get_uri( item );
	gchar *mimetype = file_manager_file_info_get_mime_type( item );
	FMASelectedInfo *info = fma_selected_info_create_for_uri( uri, mimetype, NULL );
	g_free( mimetype );
	g_free( uri );

	return( info );
}

/*
 * build_filemanager_menu:
 * @target: whether the menu targets a location (a folder) or a selection
 *  (the list of currently selected items in the file manager)
 * @selection: a list of FMASelectedInfo, with:
 *  - only one item if a location
 *  - one item by selected file manager item, if a selection.
 *  Note: a FMASelectedInfo is just a sort of NautilusFileInfo, with
 *  some added APIs.
 *
 * Build the Nautilus/Nemo menu as a list of Nautilus/NemoMenuItem items
 *
 * Returns: the Nautilus/Nemo menu list
 */
static GList *
build_filemanager_menu( FMAMenuPlugin *plugin, guint target, GList *selection )
{
	static const gchar *thisfn = "fma_menu_plugin_build_filemanager_menu";
	GList *filemanager_menu;
	FMATokens *tokens;
	GList *tree;
	gboolean items_add_about_item;
	gboolean items_create_root_menu;

	g_return_val_if_fail( FMA_IS_PIVOT( plugin->private->pivot ), NULL );

	tokens = fma_tokens_new_from_selection( selection );

	tree = fma_pivot_get_items( plugin->private->pivot );
	g_debug( "%s: tree=%p, count=%d", thisfn, ( void * ) tree, g_list_length( tree ));

	filemanager_menu = build_filemanager_menu_rec( tree, target, selection, tokens );

	/* the FMATokens object has been attached (and reffed) by each found
	 * candidate profile, so it will be actually finalized only on actual
	 * NautilusMenu finalization itself
	 */
	g_object_unref( tokens );

	if( target != ITEM_TARGET_TOOLBAR && filemanager_menu && g_list_length( filemanager_menu )){

		items_create_root_menu = fma_settings_get_boolean( IPREFS_ITEMS_CREATE_ROOT_MENU, NULL, NULL );
		if( items_create_root_menu ){
			filemanager_menu = create_root_menu( plugin, filemanager_menu );

			items_add_about_item = fma_settings_get_boolean( IPREFS_ITEMS_ADD_ABOUT_ITEM, NULL, NULL );
			if( items_add_about_item ){
				filemanager_menu = add_about_item( plugin, filemanager_menu );
			}
		}
	}

	return( filemanager_menu );
}

static GList *
build_filemanager_menu_rec( GList *tree, guint target, GList *selection, FMATokens *tokens )
{
	static const gchar *thisfn = "fma_menu_plugin_build_filemanager_menu_rec";
	GList *filemanager_menu;
	GList *it;
	GList *subitems;
	FMAObjectItem *item;
	GList *submenu;
	FMAObjectProfile *profile;
	FileManagerMenuItem *menu_item;
	gchar *label;

	filemanager_menu = NULL;

	for( it=tree ; it ; it=it->next ){

		g_return_val_if_fail( FMA_IS_OBJECT_ITEM( it->data ), NULL );
		label = fma_object_get_label( it->data );
		g_debug( "%s: examining %s", thisfn, label );

		if( !fma_icontext_is_candidate( FMA_ICONTEXT( it->data ), target, selection )){
			g_debug( "%s: is not candidate (FMAIContext): %s", thisfn, label );
			g_free( label );
			continue;
		}

		item = expand_tokens_item( FMA_OBJECT_ITEM( it->data ), tokens );

		/* but we have to re-check for validity as a label may become
		 * dynamically empty - thus the FMAObjectItem invalid :(
		 */
		if( !fma_object_is_valid( item )){
			g_debug( "%s: item %s becomes invalid after tokens expansion", thisfn, label );
			g_object_unref( item );
			g_free( label );
			continue;
		}

		/* recursively build sub-menus
		 * the 'submenu' menu of nautilusMenuItem's is attached to the returned
		 * 'item'
		 */
		if( FMA_IS_OBJECT_MENU( it->data )){

			subitems = fma_object_get_items( FMA_OBJECT( it->data ));
			g_debug( "%s: menu has %d items", thisfn, g_list_length( subitems ));

			submenu = build_filemanager_menu_rec( subitems, target, selection, tokens );
			g_debug( "%s: submenu has %d items", thisfn, g_list_length( submenu ));

			if( submenu ){
				if( target == ITEM_TARGET_TOOLBAR ){
					filemanager_menu = g_list_concat( filemanager_menu, submenu );

				} else {
					menu_item = create_item_from_menu( FMA_OBJECT_MENU( item ), submenu, target );
					filemanager_menu = g_list_append( filemanager_menu, menu_item );
				}
			}
			g_object_unref( item );
			g_free( label );
			continue;
		}

		g_return_val_if_fail( FMA_IS_OBJECT_ACTION( item ), NULL );

		/* if we have an action, searches for a candidate profile
		 */
		profile = get_candidate_profile( FMA_OBJECT_ACTION( item ), target, selection );
		if( profile ){
			menu_item = create_item_from_profile( profile, target, selection, tokens );
			filemanager_menu = g_list_append( filemanager_menu, menu_item );

		} else {
			g_debug( "%s: %s does not have any valid candidate profile", thisfn, label );
		}

		g_object_unref( item );
		g_free( label );
	}

	return( filemanager_menu );
}

/*
 * expand_tokens_item:
 * @item: a FMAObjectItem read from the FMAPivot.
 * @tokens: the FMATokens object which holds current selection data
 *  (uris, basenames, mimetypes, etc.)
 *
 * Updates the @item, replacing parameters with the corresponding token.
 *
 * This function is not recursive, but works for the plain item:
 * - the menu (itself)
 * - the action and its profiles
 *
 * Returns: a duplicated object which has to be g_object_unref() by the caller.
 */
static FMAObjectItem *
expand_tokens_item( const FMAObjectItem *src, FMATokens *tokens )
{
	gchar *old, *new;
	GSList *subitems_slist, *its, *new_slist;
	GList *subitems, *it;
	FMAObjectItem *item;

	item = FMA_OBJECT_ITEM( fma_object_duplicate( src, FMA_DUPLICATE_OBJECT ));

	/* label, tooltip and icon name
	 * plus the toolbar label if this is an action
	 */
	old = fma_object_get_label( item );
	new = fma_tokens_parse_for_display( tokens, old, TRUE );
	fma_object_set_label( item, new );
	g_free( old );
	g_free( new );

	old = fma_object_get_tooltip( item );
	new = fma_tokens_parse_for_display( tokens, old, TRUE );
	fma_object_set_tooltip( item, new );
	g_free( old );
	g_free( new );

	old = fma_object_get_icon( item );
	new = fma_tokens_parse_for_display( tokens, old, TRUE );
	fma_object_set_icon( item, new );
	g_free( old );
	g_free( new );

	if( FMA_IS_OBJECT_ACTION( item )){
		old = fma_object_get_toolbar_label( item );
		new = fma_tokens_parse_for_display( tokens, old, TRUE );
		fma_object_set_toolbar_label( item, new );
		g_free( old );
		g_free( new );
	}

	/* A FMAObjectItem, whether it is an action or a menu, is also a FMAIContext
	 */
	expand_tokens_context( FMA_ICONTEXT( item ), tokens );

	/* subitems lists, whether this is the profiles list of an action
	 * or the items list of a menu, may be dynamic and embed a command;
	 * this command itself may embed parameters
	 */
	subitems_slist = fma_object_get_items_slist( item );
	new_slist = NULL;
	for( its = subitems_slist ; its ; its = its->next ){
		old = ( gchar * ) its->data;
		if( old[0] == '[' && old[strlen(old)-1] == ']' ){
			new = fma_tokens_parse_for_display( tokens, old, FALSE );
		} else {
			new = g_strdup( old );
		}
		new_slist = g_slist_prepend( new_slist, new );
	}
	fma_object_set_items_slist( item, new_slist );
	fma_core_utils_slist_free( subitems_slist );
	fma_core_utils_slist_free( new_slist );

	/* last, deal with profiles of an action
	 */
	if( FMA_IS_OBJECT_ACTION( item )){

		subitems = fma_object_get_items( item );

		for( it = subitems ; it ; it = it->next ){

			/* desktop Exec key = GConf path+parameters
			 * do not touch them here
			 */
			old = fma_object_get_working_dir( it->data );
			new = fma_tokens_parse_for_display( tokens, old, FALSE );
			fma_object_set_working_dir( it->data, new );
			g_free( old );
			g_free( new );

			/* a FMAObjectProfile is also a FMAIContext
			 */
			expand_tokens_context( FMA_ICONTEXT( it->data ), tokens );
		}
	}

	return( item );
}

static void
expand_tokens_context( FMAIContext *context, FMATokens *tokens )
{
	gchar *old, *new;

	old = fma_object_get_try_exec( context );
	new = fma_tokens_parse_for_display( tokens, old, FALSE );
	fma_object_set_try_exec( context, new );
	g_free( old );
	g_free( new );

	old = fma_object_get_show_if_registered( context );
	new = fma_tokens_parse_for_display( tokens, old, FALSE );
	fma_object_set_show_if_registered( context, new );
	g_free( old );
	g_free( new );

	old = fma_object_get_show_if_true( context );
	new = fma_tokens_parse_for_display( tokens, old, FALSE );
	fma_object_set_show_if_true( context, new );
	g_free( old );
	g_free( new );

	old = fma_object_get_show_if_running( context );
	new = fma_tokens_parse_for_display( tokens, old, FALSE );
	fma_object_set_show_if_running( context, new );
	g_free( old );
	g_free( new );
}

/*
 * could also be a FMAObjectAction method - but this is not used elsewhere
 */
static FMAObjectProfile *
get_candidate_profile( FMAObjectAction *action, guint target, GList *files )
{
	static const gchar *thisfn = "fma_menu_plugin_get_candidate_profile";
	FMAObjectProfile *candidate = NULL;
	gchar *action_label;
	gchar *profile_label;
	GList *profiles, *ip;

	action_label = fma_object_get_label( action );
	profiles = fma_object_get_items( action );

	for( ip = profiles ; ip && !candidate ; ip = ip->next ){
		FMAObjectProfile *profile = FMA_OBJECT_PROFILE( ip->data );

		if( fma_icontext_is_candidate( FMA_ICONTEXT( profile ), target, files )){
			profile_label = fma_object_get_label( profile );
			g_debug( "%s: selecting %s (profile=%p '%s')", thisfn, action_label, ( void * ) profile, profile_label );
			g_free( profile_label );

			candidate = profile;
		}
	}

	g_free( action_label );

	return( candidate );
}

static FileManagerMenuItem *
create_item_from_profile( FMAObjectProfile *profile, guint target, GList *files, FMATokens *tokens )
{
	FileManagerMenuItem *item;
	FMAObjectAction *action;
	FMAObjectProfile *duplicate;

	action = FMA_OBJECT_ACTION( fma_object_get_parent( profile ));
	duplicate = FMA_OBJECT_PROFILE( fma_object_duplicate( profile, FMA_DUPLICATE_ONLY ));
	fma_object_set_parent( duplicate, NULL );

	item = create_menu_item( FMA_OBJECT_ITEM( action ), target );

	g_signal_connect( item,
				"activate",
				G_CALLBACK( execute_action ),
				duplicate );

	/* unref the duplicated profile on menu item finalization
	 */
	g_object_weak_ref( G_OBJECT( item ), ( GWeakNotify ) weak_notify_profile, duplicate );

	g_object_set_data_full( G_OBJECT( item ),
			"filemanager-actions-tokens",
			g_object_ref( tokens ),
			( GDestroyNotify ) g_object_unref );

	return( item );
}

/*
 * called _after_ the Nautilus/NemoMenuItem has been finalized
 */
static void
weak_notify_profile( FMAObjectProfile *profile, FileManagerMenuItem *item )
{
	g_debug( "fma_menu_plugin_weak_notify_profile: profile=%p (ref_count=%d)",
			( void * ) profile, G_OBJECT( profile )->ref_count );

	g_object_unref( profile );
}

/*
 * note that each appended NautilusMenuItem is ref-ed by the NautilusMenu
 * we can so safely release our own ref on subitems after having attached
 * the submenu
 */
static FileManagerMenuItem *
create_item_from_menu( FMAObjectMenu *menu, GList *subitems, guint target )
{
	/*static const gchar *thisfn = "fma_menu_plugin_create_item_from_menu";*/
	FileManagerMenuItem *item;

	item = create_menu_item( FMA_OBJECT_ITEM( menu ), target );

	attach_submenu_to_item( item, subitems );

	file_manager_menu_item_list_free( subitems );

	/*g_debug( "%s: returning item=%p", thisfn, ( void * ) item );*/
	return( item );
}

/*
 * Creates a Nautilus/NemoMenuItem
 *
 * We attach a weak notify function to the created item in order to be able
 * to check for instanciation/finalization cycles
 */
static FileManagerMenuItem *
create_menu_item( const FMAObjectItem *item, guint target )
{
	FileManagerMenuItem *menu_item;
	gchar *id, *name, *label, *tooltip, *icon;

	id = fma_object_get_id( item );
	name = g_strdup_printf( "%s-%s-%s-%d", PACKAGE, G_OBJECT_TYPE_NAME( item ), id, target );
	label = fma_object_get_label( item );
	tooltip = fma_object_get_tooltip( item );
	icon = fma_object_get_icon( item );

	menu_item = file_manager_menu_item_new( name, label, tooltip, icon );

	g_object_weak_ref( G_OBJECT( menu_item ), ( GWeakNotify ) weak_notify_menu_item, NULL );

	g_free( icon );
 	g_free( tooltip );
 	g_free( label );
 	g_free( name );
 	g_free( id );

	return( menu_item );
}

/*
 * called _after_ the Nautilus/NemoMenuItem has been finalized
 */
static void
weak_notify_menu_item( void *user_data /* =NULL */, FileManagerMenuItem *item )
{
	g_debug( "fma_menu_plugin_weak_notify_menu_item: item=%p", ( void * ) item );
}

static void
attach_submenu_to_item( FileManagerMenuItem *item, GList *subitems )
{
	FileManagerMenu *submenu;
	GList *it;

	submenu = file_manager_menu_new();
	file_manager_menu_item_set_submenu( item, submenu );

	for( it = subitems ; it ; it = it->next ){
		file_manager_menu_append_item( submenu, FILE_MANAGER_MENU_ITEM( it->data ));
	}
}

/*
 * callback triggered when an item is activated
 * path and parameters must yet been parsed against tokens
 *
 * note that if first parameter if of singular form, then we have to loop
 * againt the selected, each time replacing the singular parameters with
 * the current item of the selection
 */
static void
execute_action( FileManagerMenuItem *item, FMAObjectProfile *profile )
{
	static const gchar *thisfn = "fma_menu_plugin_execute_action";
	FMATokens *tokens;

	g_debug( "%s: item=%p, profile=%p", thisfn, ( void * ) item, ( void * ) profile );

	tokens = FMA_TOKENS( g_object_get_data( G_OBJECT( item ), "filemanager-actions-tokens" ));
	fma_tokens_execute_action( tokens, profile );
}

/*
 * create a root submenu
 */
static GList *
create_root_menu( FMAMenuPlugin *plugin, GList *menu )
{
	static const gchar *thisfn = "fma_menu_plugin_create_root_menu";
	GList *filemanager_menu;
	FileManagerMenuItem *root_item;

	g_debug( "%s: plugin=%p, menu=%p (%d items)",
			thisfn, ( void * ) plugin, ( void * ) menu, g_list_length( menu ));

	if( !menu || !g_list_length( menu )){
		return( NULL );
	}

	root_item = file_manager_menu_item_new(
			"FMAMenuPluginExtensions",
			/* i18n: label of an automagic root submenu */
			_( "FileManager-Actions actions" ),
			/* i18n: tooltip of an automagic root submenu */
			_( "A submenu which embeds the currently available FileManager-Actions actions and menus" ),
			fma_about_get_icon_name());
	attach_submenu_to_item( root_item, menu );
	filemanager_menu = g_list_append( NULL, root_item );

	return( filemanager_menu );
}

/*
 * if there is a root submenu,
 * then add the about item to the end of the first level of this menu
 */
static GList *
add_about_item( FMAMenuPlugin *plugin, GList *menu )
{
	static const gchar *thisfn = "fma_menu_plugin_add_about_item";
	GList *filemanager_menu;
	gboolean have_root_menu;
	FileManagerMenuItem *root_item;
	FileManagerMenuItem *about_item;
	FileManagerMenu *first;

	g_debug( "%s: plugin=%p, menu=%p (%d items)",
			thisfn, ( void * ) plugin, ( void * ) menu, g_list_length( menu ));

	if( !menu || !g_list_length( menu )){
		return( NULL );
	}

	have_root_menu = FALSE;
	filemanager_menu = menu;

	if( g_list_length( menu ) == 1 ){
		root_item = FILE_MANAGER_MENU_ITEM( menu->data );
		g_object_get( G_OBJECT( root_item ), "menu", &first, NULL );
		if( first ){
			g_return_val_if_fail( FILE_MANAGER_IS_MENU( first ), NULL );
			have_root_menu = TRUE;
		}
	}

	if( have_root_menu ){
		about_item = file_manager_menu_item_new(
				"AboutFMAMenuPlugin",
				_( "About FileManager-Actions" ),
				_( "Display some information about FileManager-Actions" ),
				fma_about_get_icon_name());

		g_signal_connect_data(
				about_item,
				"activate",
				G_CALLBACK( execute_about ),
				plugin,
				NULL,
				0 );

		file_manager_menu_append_item( first, about_item );
	}

	return( filemanager_menu );
}

static void
execute_about( FileManagerMenuItem *item, FMAMenuPlugin *plugin )
{
	g_return_if_fail( FMA_IS_MENU_PLUGIN( plugin ));

	fma_about_display( NULL );
}

/*
 * Not only the items list itself, but also several runtime preferences have
 * an effect on the display of items in file manager context menu.
 *
 * We of course monitor here all this information; only asking FMAPivot
 * for reloading its items when we detect the end of a burst of changes.
 *
 * Only when FMAPivot has finished with reloading its items list, then we
 * inform the file manager that its items list has changed.
 */

/* signal emitted by FMAPivot at the end of a burst of 'item-changed' signals
 * from i/o providers
 */
static void
on_pivot_items_changed_handler( FMAPivot *pivot, FMAMenuPlugin *plugin )
{
	g_return_if_fail( FMA_IS_PIVOT( pivot ));
	g_return_if_fail( FMA_IS_MENU_PLUGIN( plugin ));

	if( !plugin->private->dispose_has_run ){

		fma_timeout_event( &plugin->private->change_timeout );
	}
}

/* callback triggered by FMASettings at the end of a burst of 'changed' signals
 * on runtime preferences which may affect the way file manager displays
 * its context menus
 */
static void
on_settings_key_changed_handler( const gchar *group, const gchar *key, gconstpointer new_value, gboolean mandatory, FMAMenuPlugin *plugin )
{
	g_return_if_fail( FMA_IS_MENU_PLUGIN( plugin ));

	if( !plugin->private->dispose_has_run ){

		fma_timeout_event( &plugin->private->change_timeout );
	}
}

/*
 * automatically reloads the items, then signal the file manager.
 */
static void
on_change_event_timeout( FMAMenuPlugin *plugin )
{
	static const gchar *thisfn = "fma_menu_plugin_on_change_event_timeout";
	g_debug( "%s: timeout expired", thisfn );

	fma_pivot_load_items( plugin->private->pivot );

#if defined( HAVE_NAUTILUS_MENU_PROVIDER_EMIT_ITEMS_UPDATED_SIGNAL ) || \
	defined( HAVE_NEMO_MENU_PROVIDER_EMIT_ITEMS_UPDATED_SIGNAL )
	file_manager_menu_provider_emit_items_updated_signal( FILE_MANAGER_MENU_PROVIDER( plugin ));
#endif
}
