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

#ifndef __NAUTILUS_ACTIONS_NA_IIO_PROVIDER_H__
#define __NAUTILUS_ACTIONS_NA_IIO_PROVIDER_H__

/**
 * SECTION: na_iio_provider
 * @short_description: #NAIIOProvider interface definition.
 * @include: nautilus-actions/api/na-iio-provider.h
 *
 * This is the API all I/O Providers should implement in order to
 * provide I/O storage resources to Nautilus-Actions.
 *
 * Nautilus-Actions v 2.30 - API version:  1
 */

#include <nautilus-actions/private/na-object-item-class.h>

G_BEGIN_DECLS

#define NA_IIO_PROVIDER_TYPE						( na_iio_provider_get_type())
#define NA_IIO_PROVIDER( object )					( G_TYPE_CHECK_INSTANCE_CAST( object, NA_IIO_PROVIDER_TYPE, NAIIOProvider ))
#define NA_IS_IIO_PROVIDER( object )				( G_TYPE_CHECK_INSTANCE_TYPE( object, NA_IIO_PROVIDER_TYPE ))
#define NA_IIO_PROVIDER_GET_INTERFACE( instance )	( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), NA_IIO_PROVIDER_TYPE, NAIIOProviderInterface ))

typedef struct NAIIOProvider                 NAIIOProvider;

typedef struct NAIIOProviderInterfacePrivate NAIIOProviderInterfacePrivate;

typedef struct {
	GTypeInterface                 parent;
	NAIIOProviderInterfacePrivate *private;

	/*
	 * This is the API the I/O providers have to implement.
	 */

	/**
	 * get_id:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: the id of the IO provider as a newly allocated string
	 * which should be g_free() by the caller.
	 *
	 * To avoid any collision, the IO provider id is allocated by the
	 * Nautilus-Actions maintainer team. If you wish develop a new IO
	 * provider, and so need a new provider id, please contact the
	 * maintainers (see nautilus-actions.doap).
	 *
	 * The I/O provider must implement this function.
	 */
	gchar *  ( *get_id )             ( const NAIIOProvider *instance );

	/**
	 * get_name:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: the name to be displayed for this I/O provider as a
	 * newly allocated string which should be g_free() by the caller.
	 *
	 * Defaults to an empty string.
	 */
	gchar *  ( *get_name )           ( const NAIIOProvider *instance );

	/**
	 * get_version:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Returns: the version of this API supported by the IO provider.
	 *
	 * Defaults to 1.
	 */
	guint    ( *get_version )        ( const NAIIOProvider *instance );

	/**
	 * read_items:
	 * @instance: the #NAIIOProvider provider.
	 * @messages: a pointer to a #GSList which has been initialized to
	 * NULL before calling this function ; the provider may append error
	 * messages to this list, but shouldn't reinitialize it.
	 *
	 * Reads the whole items list from the specified I/O provider.
	 *
	 * Returns: a unordered flat #GList of NAObjectItem-derived objects
	 * (menus or actions) ; the actions embed their own profiles.
	 */
	GList *  ( *read_items )         ( const NAIIOProvider *instance, GSList **messages );

	/**
	 * is_willing_to_write:
	 * @instance: the #NAIIOProvider provider.
	 *
	 * Checks for global writability of the I/O provider.
	 *
	 * Returns: %TRUE if we are able to update/write/delete a #NAObjectItem
	 * (menu or action) into this I/O provider, %FALSE else.
	 *
	 * Note that the I/O provider may return a positive writability
	 * flag when considering the whole I/O storage subsystem, while not
	 * being able to update/write/delete a particular item
	 * (see is_writable function below).
	 *
	 * Note also that, even if the I/O provider is willing to write,
	 * a sysadmin may have locked down it, by putting a 'true' value
	 * in the key '/apps/nautilus-actions/mandatory/<provider_id>/locked'
	 * (see na_io_provider_is_willing_to_write).
	 */
	gboolean ( *is_willing_to_write )( const NAIIOProvider *instance );

	/**
	 * is_writable:
	 * @instance: the #NAIIOProvider provider.
	 * @item: a #NAObjectItem-derived menu or action.
	 *
	 * Checks for writability of this particular @item.
	 *
	 * Returns: %TRUE if we are able to update/write/delete the
	 * @item, %FALSE else.
	 */
	gboolean ( *is_writable )        ( const NAIIOProvider *instance, const NAObjectItem *item );

	/**
	 * write_item:
	 * @instance: the #NAIIOProvider provider.
	 * @item: a #NAObjectItem-derived menu or action.
	 * @messages: a pointer to a #GSList which has been initialized to
	 * NULL before calling this function ; the provider may append error
	 * messages to this list, but shouldn't reinitialize it.
	 *
	 * Writes a new @item.
	 *
	 * Returns: %NA_IIO_PROVIDER_WRITE_OK if the write operation
	 * was successfull, or another code depending of the detected error.
	 *
	 * Note: there is no update_item function ; it is the responsability
	 * of the provider to delete the previous version of an item before
	 * writing the new version.
	 */
	guint    ( *write_item )         ( const NAIIOProvider *instance, const NAObjectItem *item, GSList **messages );

	/**
	 * delete_item:
	 * @instance: the #NAIIOProvider provider.
	 * @item: a #NAObjectItem-derived menu or action.
	 * @messages: a pointer to a #GSList which has been initialized to
	 * NULL before calling this function ; the provider may append error
	 * messages to this list, but shouldn't reinitialize it.
	 *
	 * Deletes an existing @item from the I/O subsystem.
	 *
	 * Returns: %NA_IIO_PROVIDER_WRITE_OK if the delete operation was
	 * successfull, or another code depending of the detected error.
	 */
	guint    ( *delete_item )        ( const NAIIOProvider *instance, const NAObjectItem *item, GSList **messages );
}
	NAIIOProviderInterface;

GType na_iio_provider_get_type       ( void );

/* This function is to be called by the I/O provider when it detects
 * that the specified object has been modified in its underlying storage
 * subsystem.
 */
void  na_iio_provider_config_changed ( const NAIIOProvider *instance, const gchar *id );

/* return code of write/delete operations
 */
enum {
	NA_IIO_PROVIDER_WRITE_OK = 0,
	NA_IIO_PROVIDER_NOT_WRITABLE,
	NA_IIO_PROVIDER_NOT_WILLING_TO_WRITE,
	NA_IIO_PROVIDER_WRITE_ERROR,
	NA_IIO_PROVIDER_NO_PROVIDER,
	NA_IIO_PROVIDER_PROGRAM_ERROR
};

G_END_DECLS

#endif /* __NAUTILUS_ACTIONS_NA_IIO_PROVIDER_H__ */