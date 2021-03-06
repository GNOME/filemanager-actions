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

#ifndef __CORE_FMA_FACTORY_OBJECT_H__
#define __CORE_FMA_FACTORY_OBJECT_H__

/* @title: FMAIFactoryObject
 * @short_description: The #FMAIFactoryObject Internal Functions
 * @include: core/fma-factory-object.h
 *
 * Declare the function only accessed from core library, i.e. not
 * published as API.
 */

#include <api/fma-ifactory-provider.h>

G_BEGIN_DECLS

typedef gboolean ( *FMAFactoryObjectIterBoxedFn )( const FMAIFactoryObject *object, FMADataBoxed *boxed, void *data );

#define FMA_IFACTORY_OBJECT_PROP_DATA			"fma-ifactory-object-prop-data"

void          fma_factory_object_define_properties( GObjectClass *class, const FMADataGroup *groups );
FMADataDef   *fma_factory_object_get_data_def     ( const FMAIFactoryObject *object, const gchar *name );
FMADataGroup *fma_factory_object_get_data_groups  ( const FMAIFactoryObject *object );
void          fma_factory_object_iter_on_boxed    ( const FMAIFactoryObject *object, FMAFactoryObjectIterBoxedFn pfn, void *user_data );

gchar        *fma_factory_object_get_default      ( FMAIFactoryObject *object, const gchar *name );
void          fma_factory_object_set_defaults     ( FMAIFactoryObject *object );

void          fma_factory_object_move_boxed       ( FMAIFactoryObject *target, const FMAIFactoryObject *source, FMADataBoxed *boxed );

void          fma_factory_object_copy             ( FMAIFactoryObject *target, const FMAIFactoryObject *source );
gboolean      fma_factory_object_are_equal        ( const FMAIFactoryObject *a, const FMAIFactoryObject *b );
gboolean      fma_factory_object_is_valid         ( const FMAIFactoryObject *object );
void          fma_factory_object_dump             ( const FMAIFactoryObject *object );
void          fma_factory_object_finalize         ( FMAIFactoryObject *object );

void          fma_factory_object_read_item        ( FMAIFactoryObject *object, const FMAIFactoryProvider *reader, void *reader_data, GSList **messages );
guint         fma_factory_object_write_item       ( FMAIFactoryObject *object, const FMAIFactoryProvider *writer, void *writer_data, GSList **messages );

void         *fma_factory_object_get_as_void      ( const FMAIFactoryObject *object, const gchar *name );
void          fma_factory_object_get_as_value     ( const FMAIFactoryObject *object, const gchar *name, GValue *value );
gboolean      fma_factory_object_is_set           ( const FMAIFactoryObject *object, const gchar *name );

void          fma_factory_object_set_from_value   ( FMAIFactoryObject *object, const gchar *name, const GValue *value );
void          fma_factory_object_set_from_void    ( FMAIFactoryObject *object, const gchar *name, const void *data );

G_END_DECLS

#endif /* __CORE_FMA_FACTORY_OBJECT_H__ */
