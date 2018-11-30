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

#include <api/fma-core-utils.h>
#include <api/fma-timeout.h>

#include "fma-io-provider.h"
#include "fma-module.h"
#include "fma-pivot.h"

/* private class data
 */
struct _FMAPivotClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMAPivotPrivate {
	gboolean    dispose_has_run;

	guint       loadable_set;

	/* dynamically loaded modules (extension plugins)
	 */
	GList      *modules;

	/* configuration tree of actions and menus
	 */
	GList      *tree;

	/* timeout to manage i/o providers 'item-changed' burst
	 */
	FMATimeout  change_timeout;
};

/* FMAPivot properties
 */
enum {
	PRIVOT_PROP_0,

	PIVOT_PROP_LOADABLE_ID,
	PIVOT_PROP_TREE_ID,

	/* count of properties */
	PIVOT_PROP_N
};

/* signals
 */
enum {
	ITEMS_CHANGED,
	LAST_SIGNAL
};

static GObjectClass  *st_parent_class           = NULL;
static gint           st_burst_timeout          = 100;		/* burst timeout in msec */
static gint           st_signals[ LAST_SIGNAL ] = { 0 };

static GType          register_type( void );
static void           class_init( FMAPivotClass *klass );
static void           instance_init( GTypeInstance *instance, gpointer klass );
static void           instance_constructed( GObject *object );
static void           instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void           instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void           instance_dispose( GObject *object );
static void           instance_finalize( GObject *object );

static FMAObjectItem *get_item_from_tree( const FMAPivot *pivot, GList *tree, const gchar *id );

/* FMAIIOProvider management */
static void           on_items_changed_timeout( FMAPivot *pivot );

GType
fma_pivot_get_type( void )
{
	static GType object_type = 0;

	if( !object_type ){
		object_type = register_type();
	}

	return( object_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "fma_pivot_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMAPivotClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAPivot ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "FMAPivot", &info, 0 );

	return( type );
}

static void
class_init( FMAPivotClass *klass )
{
	static const gchar *thisfn = "fma_pivot_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->constructed = instance_constructed;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	g_object_class_install_property( object_class, PIVOT_PROP_LOADABLE_ID,
			g_param_spec_uint(
					PIVOT_PROP_LOADABLE,
					"Loadable set",
					"The set of loadble items",
					0, 255, 0,
					G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, PIVOT_PROP_TREE_ID,
			g_param_spec_pointer(
					PIVOT_PROP_TREE,
					"Items tree",
					"Hierarchical tree of items",
					G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	klass->private = g_new0( FMAPivotClassPrivate, 1 );

	/*
	 * FMAPivot::pivot-items-changed:
	 *
	 * This signal is sent by FMAPivot at the end of a burst of modifications
	 * as signaled by i/o providers.
	 *
	 * The signal is registered without any default handler.
	 */
	st_signals[ ITEMS_CHANGED ] = g_signal_new(
				PIVOT_SIGNAL_ITEMS_CHANGED,
				FMA_TYPE_PIVOT,
				G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
				0,									/* class offset */
				NULL,								/* accumulator */
				NULL,								/* accumulator data */
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE,
				0 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_pivot_instance_init";
	FMAPivot *self;

	g_return_if_fail( FMA_IS_PIVOT( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self = FMA_PIVOT( instance );

	self->private = g_new0( FMAPivotPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->loadable_set = PIVOT_LOAD_NONE;
	self->private->modules = NULL;
	self->private->tree = NULL;

	/* initialize timeout parameters for 'item-changed' handler
	 */
	self->private->change_timeout.timeout = st_burst_timeout;
	self->private->change_timeout.handler = ( FMATimeoutFunc ) on_items_changed_timeout;
	self->private->change_timeout.user_data = self;
	self->private->change_timeout.source_id = 0;
}

static void
instance_constructed( GObject *object )
{
	static const gchar *thisfn = "fma_pivot_instance_constructed";
	FMAPivotPrivate *priv;

	g_return_if_fail( FMA_IS_PIVOT( object ));

	priv = FMA_PIVOT( object )->private;

	if( !priv->dispose_has_run ){

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->constructed ){
			G_OBJECT_CLASS( st_parent_class )->constructed( object );
		}

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		priv->modules = fma_module_load_modules();

		/* force class initialization and io-factory registration
		 */
		g_object_unref( fma_object_action_new_with_profile());
		g_object_unref( fma_object_menu_new());
	}
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	FMAPivot *self;

	g_return_if_fail( FMA_IS_PIVOT( object ));
	self = FMA_PIVOT( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case PIVOT_PROP_LOADABLE_ID:
				g_value_set_uint( value, self->private->loadable_set );
				break;

			case PIVOT_PROP_TREE_ID:
				g_value_set_pointer( value, self->private->tree );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	FMAPivot *self;

	g_return_if_fail( FMA_IS_PIVOT( object ));
	self = FMA_PIVOT( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case PIVOT_PROP_LOADABLE_ID:
				self->private->loadable_set = g_value_get_uint( value );
				break;

			case PIVOT_PROP_TREE_ID:
				self->private->tree = g_value_get_pointer( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "fma_pivot_instance_dispose";
	FMAPivot *self;

	g_return_if_fail( FMA_IS_PIVOT( object ));

	self = FMA_PIVOT( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		/* release modules */
		fma_module_release_modules( self->private->modules );
		self->private->modules = NULL;

		/* release item tree */
		g_debug( "%s: tree=%p (count=%u)", thisfn,
				( void * ) self->private->tree, g_list_length( self->private->tree ));
		fma_object_dump_tree( self->private->tree );
		self->private->tree = fma_object_free_items( self->private->tree );

		/* release the settings */
		fma_settings_free();

		/* release the I/O Provider object list */
		fma_io_provider_unref_io_providers_list();

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "fma_pivot_instance_finalize";
	FMAPivot *self;

	g_return_if_fail( FMA_IS_PIVOT( object ));

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	self = FMA_PIVOT( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * fma_pivot_new:
 *
 * This object takes care of all items/actions/menus/providers/settings
 * management which is required to correctly handle file manager context
 * menus.
 *
 * When this object is instantiated, it automatically takes care of:
 * - loading FileManager-Actions dynamic modules;
 * - initializing the preferences monitoring.
 *
 * Actual loading of items from i/o providers is delayed until a call
 * to call to fma_pivot_load_items() function, so that the caller is able
 * to set its own needed #FMAPivot properties (e.g. the loadable set of
 * items).
 *
 * Only one #FMAPivot object should be instantiated for a running application.
 *
 * Returns: a newly allocated #FMAPivot object which should be g_object_unref()
 * by the caller at the end of the application.
 */
FMAPivot *
fma_pivot_new( void )
{
	static const gchar *thisfn = "fma_pivot_new";
	FMAPivot *pivot;

	g_debug( "%s", thisfn );

	pivot = g_object_new( FMA_TYPE_PIVOT, NULL );

	return( pivot );
}

/*
 * fma_pivot_dump:
 * @pivot: the #FMAPivot object do be dumped.
 *
 * Dumps the content of a #FMAPivot object.
 */
void
fma_pivot_dump( const FMAPivot *pivot )
{
	static const gchar *thisfn = "fma_pivot_dump";
	GList *it;
	int i;

	if( !pivot->private->dispose_has_run ){

		g_debug( "%s: loadable_set=%d", thisfn, pivot->private->loadable_set );
		g_debug( "%s:      modules=%p (%d elts)", thisfn, ( void * ) pivot->private->modules, g_list_length( pivot->private->modules ));
		g_debug( "%s:         tree=%p (%d elts)", thisfn, ( void * ) pivot->private->tree, g_list_length( pivot->private->tree ));
		/*g_debug( "%s:     monitors=%p (%d elts)", thisfn, ( void * ) pivot->private->monitors, g_list_length( pivot->private->monitors ));*/

		for( it = pivot->private->tree, i = 0 ; it ; it = it->next ){
			g_debug( "%s:     [%d]: %p", thisfn, i++, it->data );
		}
	}
}

/*
 * fma_pivot_get_providers:
 * @pivot: this #FMAPivot instance.
 * @type: the type of searched interface.
 * For now, we only have FMA_TYPE_IIO_PROVIDER interfaces.
 *
 * Returns: a newly allocated list of providers of the required interface.
 *
 * This function is called by interfaces API in order to find the
 * list of providers registered for their own given interface.
 *
 * The returned list should be release by calling fma_pivot_free_providers().
 */
GList *
fma_pivot_get_providers( const FMAPivot *pivot, GType type )
{
	static const gchar *thisfn = "fma_pivot_get_providers";
	GList *list = NULL;

	g_return_val_if_fail( FMA_IS_PIVOT( pivot ), NULL );

	if( !pivot->private->dispose_has_run ){

		g_debug( "%s: pivot=%p, type=%lu (%s)", thisfn, ( void * ) pivot, ( unsigned long ) type, g_type_name( type ));

		list = fma_module_get_extensions_for_type( pivot->private->modules, type );
		g_debug( "%s: list=%p, count=%d", thisfn, ( void * ) list, list ? g_list_length( list ) : 0 );
	}

	return( list );
}

/*
 * fma_pivot_free_providers:
 * @providers: a list of providers.
 *
 * Frees a list of providers as returned from fma_pivot_get_providers().
 */
void
fma_pivot_free_providers( GList *providers )
{
	static const gchar *thisfn = "fma_pivot_free_providers";

	g_debug( "%s: providers=%p", thisfn, ( void * ) providers );

	fma_module_free_extensions_list( providers );
}

/*
 * fma_pivot_get_item:
 * @pivot: this #FMAPivot instance.
 * @id: the required item identifier.
 *
 * Returns the specified item, action or menu.
 *
 * Returns: the required #FMAObjectItem-derived object, or %NULL if not
 * found.
 *
 * The returned pointer is owned by #FMAPivot, and should not be
 * g_free() nor g_object_unref() by the caller.
 */
FMAObjectItem *
fma_pivot_get_item( const FMAPivot *pivot, const gchar *id )
{
	FMAObjectItem *object = NULL;

	g_return_val_if_fail( FMA_IS_PIVOT( pivot ), NULL );

	if( !pivot->private->dispose_has_run ){

		if( !id || !strlen( id )){
			return( NULL );
		}

		object = get_item_from_tree( pivot, pivot->private->tree, id );
	}

	return( object );
}

static FMAObjectItem *
get_item_from_tree( const FMAPivot *pivot, GList *tree, const gchar *id )
{
	GList *subitems, *ia;
	FMAObjectItem *found = NULL;

	for( ia = tree ; ia && !found ; ia = ia->next ){

		gchar *i_id = fma_object_get_id( FMA_OBJECT( ia->data ));

		if( !g_ascii_strcasecmp( id, i_id )){
			found = FMA_OBJECT_ITEM( ia->data );
		}

		if( !found && FMA_IS_OBJECT_ITEM( ia->data )){
			subitems = fma_object_get_items( ia->data );
			found = get_item_from_tree( pivot, subitems, id );
		}
	}

	return( found );
}

/*
 * fma_pivot_get_items:
 * @pivot: this #FMAPivot instance.
 *
 * Returns: the current configuration tree.
 *
 * The returned list is owned by this #FMAPivot object, and should not
 * be g_free(), nor g_object_unref() by the caller.
 */
GList *
fma_pivot_get_items( const FMAPivot *pivot )
{
	GList *tree;

	g_return_val_if_fail( FMA_IS_PIVOT( pivot ), NULL );

	tree = NULL;

	if( !pivot->private->dispose_has_run ){

		tree = pivot->private->tree;
	}

	return( tree );
}

/*
 * fma_pivot_load_items:
 * @pivot: this #FMAPivot instance.
 *
 * Loads the hierarchical list of items from I/O providers.
 */
void
fma_pivot_load_items( FMAPivot *pivot )
{
	static const gchar *thisfn = "fma_pivot_load_items";
	GSList *messages, *im;

	g_return_if_fail( FMA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		g_debug( "%s: pivot=%p", thisfn, ( void * ) pivot );

		messages = NULL;
		fma_object_free_items( pivot->private->tree );
		pivot->private->tree = fma_io_provider_load_items( pivot, pivot->private->loadable_set, &messages );

		for( im = messages ; im ; im = im->next ){
			g_warning( "%s: %s", thisfn, ( const gchar * ) im->data );
		}

		fma_core_utils_slist_free( messages );
	}
}

/*
 * fma_pivot_set_new_items:
 * @pivot: this #FMAPivot instance.
 * @tree: the new tree of items.
 *
 * Replace the current list with this one, acquiring the full ownership
 * of the provided @tree.
 */
void
fma_pivot_set_new_items( FMAPivot *pivot, GList *items )
{
	static const gchar *thisfn = "fma_pivot_set_new_items";

	g_return_if_fail( FMA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		g_debug( "%s: pivot=%p, items=%p (count=%d)",
				thisfn, ( void * ) pivot, ( void * ) items, items ? g_list_length( items ) : 0 );

		fma_object_free_items( pivot->private->tree );
		pivot->private->tree = items;
	}
}

/*
 * fma_pivot_on_item_changed_handler:
 * @provider: the #FMAIIOProvider which has emitted the signal.
 * @pivot: this #FMAPivot instance.
 *
 * This handler is trigerred by #FMAIIOProvider providers when an action
 * is changed in their underlying storage subsystems.
 *
 * The FMAIIOProvider is supposed to have itself already summarized
 * a minima its own burst of notifications.
 *
 * We don't care of updating our internal list with each and every
 * atomic modification; instead we wait for the end of notifications
 * series, and then signal our consumers.
 */
void
fma_pivot_on_item_changed_handler( FMAIIOProvider *provider, FMAPivot *pivot  )
{
	static const gchar *thisfn = "fma_pivot_on_item_changed_handler";

	g_return_if_fail( FMA_IS_IIO_PROVIDER( provider ));
	g_return_if_fail( FMA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){
		g_debug( "%s: provider=%p, pivot=%p", thisfn, ( void * ) provider, ( void * ) pivot );

		fma_timeout_event( &pivot->private->change_timeout );
	}
}

/*
 * this callback is triggered after having received a first 'item-changed' event,
 * and having received no more event during a 'st_burst_timeout' period; we can
 * so suppose that the burst if modification events is terminated
 * this is up to FMAPivot to send now its summarized signal
 */
static void
on_items_changed_timeout( FMAPivot *pivot )
{
	static const gchar *thisfn = "fma_pivot_on_items_changed_timeout";

	g_return_if_fail( FMA_IS_PIVOT( pivot ));

	g_debug( "%s: emitting %s signal", thisfn, PIVOT_SIGNAL_ITEMS_CHANGED );
	g_signal_emit_by_name(( gpointer ) pivot, PIVOT_SIGNAL_ITEMS_CHANGED );
}

/*
 * fma_pivot_set_loadable:
 * @pivot: this #FMAPivot instance.
 * @loadable: the population of items to be loaded.
 *
 * Sets the loadable set.
 */
void
fma_pivot_set_loadable( FMAPivot *pivot, guint loadable )
{
	g_return_if_fail( FMA_IS_PIVOT( pivot ));

	if( !pivot->private->dispose_has_run ){

		pivot->private->loadable_set = loadable;
	}
}
