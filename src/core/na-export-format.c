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

#include "na-export-format.h"

/* private class data
 */
struct _NAExportFormatClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _NAExportFormatPrivate {
	gboolean     dispose_has_run;
	GQuark       id;
	gchar       *format;
	gchar       *label;
	gchar       *description;
	GdkPixbuf   *pixbuf;
	NAIExporter *provider;
};

static GObjectClass *st_parent_class = NULL;

static GType  register_type( void );
static void   class_init( NAExportFormatClass *klass );
static void   instance_init( GTypeInstance *instance, gpointer klass );
static void   instance_dispose( GObject *object );
static void   instance_finalize( GObject *object );

GType
na_export_format_get_type( void )
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
	static const gchar *thisfn = "na_export_format_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NAExportFormatClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAExportFormat ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( G_TYPE_OBJECT, "NAExportFormat", &info, 0 );

	return( type );
}

static void
class_init( NAExportFormatClass *klass )
{
	static const gchar *thisfn = "na_export_format_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NAExportFormatClassPrivate, 1 );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "na_export_format_instance_init";
	NAExportFormat *self;

	g_return_if_fail( NA_IS_EXPORT_FORMAT( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );
	self = NA_EXPORT_FORMAT( instance );

	self->private = g_new0( NAExportFormatPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "na_export_format_instance_dispose";
	NAExportFormat *self;

	g_return_if_fail( NA_IS_EXPORT_FORMAT( object ));

	self = NA_EXPORT_FORMAT( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		if( self->private->pixbuf ){
			g_object_unref( self->private->pixbuf );
			self->private->pixbuf = NULL;
		}

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "na_export_format_instance_finalize";
	NAExportFormat *self;

	g_return_if_fail( NA_IS_EXPORT_FORMAT( object ));

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	self = NA_EXPORT_FORMAT( object );

	g_free( self->private->format );
	g_free( self->private->label );
	g_free( self->private->description );
	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * na_export_format_new:
 * @exporter_format: a #NAIExporterFormatExt which describes an export format.
 *
 * Returns: a newly allocated #NAExportFormat object.
 */
NAExportFormat *
na_export_format_new( const NAIExporterFormatExt *exporter_format )
{
	NAExportFormat *format;

	format = g_object_new( NA_EXPORT_FORMAT_TYPE, NULL );

	format->private->id = g_quark_from_string( exporter_format->format );
	format->private->format = g_strdup( exporter_format->format );
	format->private->label = g_strdup( exporter_format->label );
	format->private->description = g_strdup( exporter_format->description );
	format->private->pixbuf = exporter_format->pixbuf ? g_object_ref( exporter_format->pixbuf ) : NULL;
	format->private->provider = exporter_format->provider;

	return( format );
}

/*
 * na_export_format_get_quark:
 * @format: this #NAExportFormat object.
 *
 * Returns: the #GQuark associated with this format.
 */
GQuark
na_export_format_get_quark( const NAExportFormat *format )
{
	GQuark id;

	g_return_val_if_fail( NA_IS_EXPORT_FORMAT( format ), 0 );

	id = 0;

	if( !format->private->dispose_has_run ){

		id = format->private->id;
	}

	return( id );
}

/*
 * na_export_format_get_id:
 * @format: this #NAExportFormat object.
 *
 * Returns: the ASCII id of the format, as a newly allocated string which
 * should be g_free() by the caller.
 */
gchar *
na_export_format_get_id( const NAExportFormat *format )
{
	gchar *id;

	g_return_val_if_fail( NA_IS_EXPORT_FORMAT( format ), NULL );

	id = NULL;

	if( !format->private->dispose_has_run ){

		id = g_strdup( format->private->format );
	}

	return( id );
}

/*
 * na_export_format_get_label:
 * @format: this #NAExportFormat object.
 *
 * Returns: the UTF-8 localizable label of the format, as a newly
 * allocated string which should be g_free() by the caller.
 */
gchar *
na_export_format_get_label( const NAExportFormat *format )
{
	gchar *label;

	g_return_val_if_fail( NA_IS_EXPORT_FORMAT( format ), NULL );

	label = NULL;

	if( !format->private->dispose_has_run ){

		label = g_strdup( format->private->label );
	}

	return( label );
}

/*
 * na_export_format_get_description:
 * @format: this #NAExportFormat object.
 *
 * Returns: the UTF-8 localizable description of the format, as a newly
 * allocated string which should be g_free() by the caller.
 */
gchar *
na_export_format_get_description( const NAExportFormat *format )
{
	gchar *description;

	g_return_val_if_fail( NA_IS_EXPORT_FORMAT( format ), NULL );

	description = NULL;

	if( !format->private->dispose_has_run ){

		description = g_strdup( format->private->description );
	}

	return( description );
}

/*
 * na_export_format_get_provider:
 * @format: this #NAExportFormat object.
 *
 * Returns: a pointer to the #NAIExporter which provides this format.
 *
 * The pointer is owned by NAEportFormat class, and should not be released
 * by the caller.
 */
NAIExporter *
na_export_format_get_provider( const NAExportFormat *format )
{
	NAIExporter *exporter;

	g_return_val_if_fail( NA_IS_EXPORT_FORMAT( format ), NULL );

	exporter = NULL;

	if( !format->private->dispose_has_run ){

		exporter = format->private->provider;
	}

	return( exporter );
}

/*
 * na_export_format_get_pixbuf:
 * @format: this #NAExportFormat object.
 *
 * Returns: a new reference to the #GdkPixbuf image associated with this format,
 * or %NULL.
 */
GdkPixbuf *
na_export_format_get_pixbuf( const NAExportFormat *format )
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail( NA_IS_EXPORT_FORMAT( format ), NULL );

	pixbuf = NULL;

	if( !format->private->dispose_has_run ){

		if( format->private->pixbuf ){
			pixbuf = g_object_ref( format->private->pixbuf );
		}
	}

	return( pixbuf );
}
