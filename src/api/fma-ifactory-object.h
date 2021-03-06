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

#ifndef __FILEMANAGER_ACTIONS_API_IFACTORY_OBJECT_H__
#define __FILEMANAGER_ACTIONS_API_IFACTORY_OBJECT_H__

/**
 * SECTION: ifactory-object
 * @title: FMAIFactoryObject
 * @short_description: The #FMAObjectItem Interface v 1
 * @include: filemanager-actions/fma-ifactory-object.h
 *
 * This interface is implemented by #FMAObjectItem derived objects so that they
 * can take advantage of our data factory management system.
 *
 * A #FMAObjectItem derived object which would implement this #FMAIFactoryObject
 * interface must meet following conditions:
 * <itemizedlist>
 *   <listitem>
 *     <para>
 *       accept an empty constructor
 *     </para>
 *   </listitem>
 * </itemizedlist>
 *
 * <refsect2>
 *  <title>Versions historic</title>
 *  <table>
 *    <title>Historic of the versions of the #FMAIFactoryObject interface</title>
 *    <tgroup rowsep="1" colsep="1" align="center" cols="3">
 *      <colspec colname="fma-version" />
 *      <colspec colname="api-version" />
 *      <colspec colname="current" />
 *      <thead>
 *        <row>
 *          <entry>&prodname; version</entry>
 *          <entry>#FMAIFactoryObject interface version</entry>
 *          <entry></entry>
 *        </row>
 *      </thead>
 *      <tbody>
 *        <row>
 *          <entry>since 2.30</entry>
 *          <entry>1</entry>
 *          <entry>current version</entry>
 *        </row>
 *      </tbody>
 *    </tgroup>
 *  </table>
 * </refsect2>
 */

#include "fma-data-def.h"
#include "fma-data-boxed.h"
#include "fma-ifactory-provider-provider.h"

G_BEGIN_DECLS

#define FMA_TYPE_IFACTORY_OBJECT                      ( fma_ifactory_object_get_type())
#define FMA_IFACTORY_OBJECT( instance )               ( G_TYPE_CHECK_INSTANCE_CAST( instance, FMA_TYPE_IFACTORY_OBJECT, FMAIFactoryObject ))
#define FMA_IS_IFACTORY_OBJECT( instance )            ( G_TYPE_CHECK_INSTANCE_TYPE( instance, FMA_TYPE_IFACTORY_OBJECT ))
#define FMA_IFACTORY_OBJECT_GET_INTERFACE( instance ) ( G_TYPE_INSTANCE_GET_INTERFACE(( instance ), FMA_TYPE_IFACTORY_OBJECT, FMAIFactoryObjectInterface ))

typedef struct _FMAIFactoryObject                     FMAIFactoryObject;
typedef struct _FMAIFactoryObjectInterfacePrivate     FMAIFactoryObjectInterfacePrivate;

/**
 * FMAIFactoryObjectInterface:
 * @get_version: returns the version of this interface the FMAObjectItem implements.
 * @get_groups:  returns a pointer to the FMADataGroup which defines this object.
 * @copy:        post copy callback.
 * @are_equal:   tests if two FMAObjectItem are equals.
 * @is_valid:    tests if one FMAObjectItem is valid.
 * @read_start:  triggered before serializing a FMAObjectItem.
 * @read_done:   triggered after a FMAObjectItem has been serialized.
 * @write_start: triggered before unserializing a FMAObjectItem.
 * @write_done:  triggered after a FMAObjectItem has been unserialized.
 *
 * In order to take full advantage of our data managament system,
 * FMAObjectItem-derived objects all implement this #FMAIFactoryObject
 * interface.
 */
typedef struct {
	/*< private >*/
	GTypeInterface                     parent;
	FMAIFactoryObjectInterfacePrivate *private;

	/*< public >*/
	/**
	 * get_version:
	 * @instance: this #FMAIFactoryObject instance.
	 *
	 * Defaults to 1.
	 *
	 * Returns: the version of this interface supported by @instance implementation.
	 *
	 * Since: 2.30
	 */
	guint         ( *get_version )( const FMAIFactoryObject *instance );

	/**
	 * get_groups:
	 * @instance: this #FMAIFactoryObject instance.
	 *
	 * Returns: a pointer to the FMADataGroup which defines this object.
	 *
	 * Since: 2.30
	 */
	FMADataGroup * ( *get_groups ) ( const FMAIFactoryObject *instance );

	/**
	 * copy:
	 * @instance: the target #FMAIFactoryObject instance.
	 * @source: the source #FMAIFactoryObject instance.
	 *
	 * This function is triggered after having copied @source to
	 * @instance target. This later may take advantage of this call
	 * to do some particular copy tasks.
	 *
	 * Since: 2.30
	 */
	void          ( *copy )       ( FMAIFactoryObject *instance,
										const FMAIFactoryObject *source );

	/**
	 * are_equal:
	 * @a: the first #FMAIFactoryObject instance.
	 * @b: the second #FMAIFactoryObject instance.
	 *
	 * This function is triggered after all elementary data comparisons
	 * have been sucessfully made.
	 *
	 * Returns: %TRUE if @a is equal to @b.
	 *
	 * Since: 2.30
	 */
	gboolean      ( *are_equal )  ( const FMAIFactoryObject *a,
										const FMAIFactoryObject *b );

	/**
	 * is_valid:
	 * @object: the #FMAIFactoryObject instance whose validity is to be checked.
	 *
	 * This function is triggered after all elementary data comparisons
	 * have been sucessfully made.
	 *
	 * Returns: %TRUE if @object is valid.
	 *
	 * Since: 2.30
	 */
	gboolean      ( *is_valid )   ( const FMAIFactoryObject *object );

	/**
	 * read_start:
	 * @instance: this #FMAIFactoryObject instance.
	 * @reader: the instance which has provided read services.
	 * @reader_data: the data associated to @reader.
	 * @messages: a pointer to a #GSList list of strings; the instance
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Called just before the object is unserialized.
	 *
	 * Since: 2.30
	 */
	void          ( *read_start ) ( FMAIFactoryObject *instance,
										const FMAIFactoryProvider *reader,
										void *reader_data,
										GSList **messages );

	/**
	 * read_done:
	 * @instance: this #FMAIFactoryObject instance.
	 * @reader: the instance which has provided read services.
	 * @reader_data: the data associated to @reader.
	 * @messages: a pointer to a #GSList list of strings; the instance
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Called when the object has been unserialized.
	 *
	 * Since: 2.30
	 */
	void          ( *read_done )  ( FMAIFactoryObject *instance,
										const FMAIFactoryProvider *reader,
										void *reader_data,
										GSList **messages );

	/**
	 * write_start:
	 * @instance: this #FMAIFactoryObject instance.
	 * @writer: the instance which has provided writing services.
	 * @writer_data: the data associated to @writer.
	 * @messages: a pointer to a #GSList list of strings; the instance
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Called just before the object is serialized.
	 *
	 * Returns: a FMAIIOProvider operation return code.
	 *
	 * Since: 2.30
	 */
	guint         ( *write_start )( FMAIFactoryObject *instance,
										const FMAIFactoryProvider *writer,
										void *writer_data,
										GSList **messages );

	/**
	 * write_done:
	 * @instance: this #FMAIFactoryObject instance.
	 * @writer: the instance which has provided writing services.
	 * @writer_data: the data associated to @writer.
	 * @messages: a pointer to a #GSList list of strings; the instance
	 *  may append messages to this list, but shouldn't reinitialize it.
	 *
	 * Called when the object has been serialized.
	 *
	 * Returns: a FMAIIOProvider operation return code.
	 *
	 * Since: 2.30
	 */
	guint         ( *write_done ) ( FMAIFactoryObject *instance,
										const FMAIFactoryProvider *writer,
										void *writer_data,
										GSList **messages );
}
	FMAIFactoryObjectInterface;

GType         fma_ifactory_object_get_type       ( void );

FMADataBoxed *fma_ifactory_object_get_data_boxed ( const FMAIFactoryObject *object, const gchar *name );
FMADataGroup *fma_ifactory_object_get_data_groups( const FMAIFactoryObject *object );
void         *fma_ifactory_object_get_as_void    ( const FMAIFactoryObject *object, const gchar *name );
void          fma_ifactory_object_set_from_void  ( FMAIFactoryObject *object, const gchar *name, const void *data );

G_END_DECLS

#endif /* __FILEMANAGER_ACTIONS_API_IFACTORY_OBJECT_H__ */
