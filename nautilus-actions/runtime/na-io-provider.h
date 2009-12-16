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

#ifndef __NA_RUNTIME_IO_PROVIDER_H__
#define __NA_RUNTIME_IO_PROVIDER_H__

/**
 * SECTION: na_iio_provider
 * @short_description: #NAIIOProvider internal API.
 * @include: runtime/na-io-provider.h
 *
 * This is the API which is used by Nautilus-Actions to manage the actual
 * NAIIOProvider interface.
 */

#include "na-pivot.h"

G_BEGIN_DECLS

void           na_io_provider_register_callbacks( const NAPivot *pivot );

NAIIOProvider *na_io_provider_get_provider( const NAPivot *pivot, const gchar *id );
NAIIOProvider *na_io_provider_get_writable_provider( const NAPivot *pivot );

gchar         *na_io_provider_get_id( const NAPivot *pivot, const NAIIOProvider *provider );
gchar         *na_io_provider_get_name( const NAPivot *pivot, const NAIIOProvider *provider );
guint          na_io_provider_get_version( const NAPivot *pivot, const NAIIOProvider *provider );
gboolean       na_io_provider_is_willing_to_write( const NAPivot *pivot, const NAIIOProvider *provider );

GList         *na_io_provider_read_items( const NAPivot *pivot, GSList **messages );
guint          na_io_provider_write_item( const NAPivot *pivot, NAObjectItem *item, GSList **messages );
guint          na_io_provider_delete_item( const NAPivot *pivot, const NAObjectItem *item, GSList **messages );

G_END_DECLS

#endif /* __NA_RUNTIME_IO_PROVIDER_H__ */
