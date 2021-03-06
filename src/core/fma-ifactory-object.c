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

#include <api/fma-ifactory-object.h>

#include "fma-factory-object.h"

/* private interface data
 */
struct _FMAIFactoryObjectInterfacePrivate {
	void *empty;					/* so that gcc -pedantic is happy */
};

static guint st_initializations = 0;	/* interface initialization count */

static GType register_type( void );
static void  interface_base_init( FMAIFactoryObjectInterface *klass );
static void  interface_base_finalize( FMAIFactoryObjectInterface *klass );

static guint ifactory_object_get_version( const FMAIFactoryObject *instance );

/*
 * Registers the GType of this interface.
 */
GType
fma_ifactory_object_get_type( void )
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
	static const gchar *thisfn = "fma_ifactory_object_register_type";
	GType type;

	static const GTypeInfo info = {
		sizeof( FMAIFactoryObjectInterface ),
		( GBaseInitFunc ) interface_base_init,
		( GBaseFinalizeFunc ) interface_base_finalize,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_INTERFACE, "FMAIFactoryObject", &info, 0 );

	g_type_interface_add_prerequisite( type, G_TYPE_OBJECT );

	return( type );
}

static void
interface_base_init( FMAIFactoryObjectInterface *klass )
{
	static const gchar *thisfn = "fma_ifactory_object_interface_base_init";

	if( !st_initializations ){

		g_debug( "%s: klass=%p (%s)", thisfn, ( void * ) klass, G_OBJECT_CLASS_NAME( klass ));

		klass->private = g_new0( FMAIFactoryObjectInterfacePrivate, 1 );

		klass->get_version = ifactory_object_get_version;
		klass->get_groups = NULL;
		klass->copy = NULL;
		klass->are_equal = NULL;
		klass->is_valid = NULL;
		klass->read_start = NULL;
		klass->read_done = NULL;
		klass->write_start = NULL;
		klass->write_done = NULL;
	}

	st_initializations += 1;
}

static void
interface_base_finalize( FMAIFactoryObjectInterface *klass )
{
	static const gchar *thisfn = "fma_ifactory_object_interface_base_finalize";

	st_initializations -= 1;

	if( !st_initializations ){

		g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

		g_free( klass->private );
	}
}

static guint
ifactory_object_get_version( const FMAIFactoryObject *instance )
{
	return( 1 );
}

/**
 * fma_ifactory_object_get_data_boxed:
 * @object: a #FMAIFactoryObject object.
 * @name: the name of the elementary data we are searching for.
 *
 * The returned #FMADataBoxed is owned by #FMAIFactoryObject @object, and
 * should not be released by the caller.
 *
 * Returns: The #FMADataBoxed object which contains the specified data,
 * or %NULL.
 *
 * Since: 2.30
 */
FMADataBoxed *
fma_ifactory_object_get_data_boxed( const FMAIFactoryObject *object, const gchar *name )
{
	GList *list, *ip;

	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), NULL );

	list = g_object_get_data( G_OBJECT( object ), FMA_IFACTORY_OBJECT_PROP_DATA );
	/*g_debug( "list=%p (count=%u)", ( void * ) list, g_list_length( list ));*/

	for( ip = list ; ip ; ip = ip->next ){
		FMADataBoxed *boxed = FMA_DATA_BOXED( ip->data );
		const FMADataDef *def = fma_data_boxed_get_data_def( boxed );

		if( !strcmp( def->name, name )){
			return( boxed );
		}
	}

	return( NULL );
}

/**
 * fma_ifactory_object_get_data_groups:
 * @object: a #FMAIFactoryObject object.
 *
 * The returned #FMADataGroup is owned by the #FMAIFactoryObject @object,
 * and should not be released by the caller.
 *
 * Returns: The #FMADataGroup groups definition, or %NULL.
 *
 * Since: 2.30
 */
FMADataGroup *
fma_ifactory_object_get_data_groups( const FMAIFactoryObject *object )
{
	FMADataGroup *groups;

	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), NULL );

	groups = NULL;

	if( FMA_IFACTORY_OBJECT_GET_INTERFACE( object )->get_groups ){
		groups = FMA_IFACTORY_OBJECT_GET_INTERFACE( object )->get_groups( object );
	}

	return( groups );
}

/**
 * fma_ifactory_object_get_as_void:
 * @object: this #FMAIFactoryObject instance.
 * @name: the elementary data whose value is to be got.
 *
 * If the type of the value is %FMA_DATA_TYPE_STRING, %FMA_DATA_TYPE_LOCALE_STRING,
 * or %FMA_DATA_TYPE_STRING_LIST, then the returned value is a newly allocated
 * one and should be g_free() (resp. fma_core_utils_slist_free()) by the
 * caller.
 *
 * Returns: the searched value.
 *
 * Since: 2.30
 */
void *
fma_ifactory_object_get_as_void( const FMAIFactoryObject *object, const gchar *name )
{
	g_return_val_if_fail( FMA_IS_IFACTORY_OBJECT( object ), NULL );

	return( fma_factory_object_get_as_void( object, name ));
}

/**
 * fma_ifactory_object_set_from_void:
 * @object: this #FMAIFactoryObject instance.
 * @name: the name of the elementary data whose value is to be set.
 * @data: the value to set.
 *
 * Set the elementary data with the given value.
 *
 * Since: 2.30
 */
void
fma_ifactory_object_set_from_void( FMAIFactoryObject *object, const gchar *name, const void *data )
{
	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));

	fma_factory_object_set_from_void( object, name, data );
}
