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

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "fma-import-mode.h"
#include "fma-ioption.h"

/* private class data
 */
struct _FMAImportModeClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMAImportModePrivate {
	gboolean   dispose_has_run;

	/* properties
	 */
	guint      id;
	gchar     *mode;
	gchar     *label;
	gchar     *description;
	GdkPixbuf *image;
};

/* properties
 */
enum {
	MAIN_PROP_0 = 0,

	FMA_IMPORT_PROP_MODE_ID,
	FMA_IMPORT_PROP_LABEL_ID,
	FMA_IMPORT_PROP_DESCRIPTION_ID,
	FMA_IMPORT_PROP_IMAGE_ID,

	FMA_IMPORT_PROP_N_PROPERTIES
};

static GObjectClass *st_parent_class = NULL;

static GType      register_type( void );
static void       class_init( FMAImportModeClass *klass );
static void       ioption_iface_init( FMAIOptionInterface *iface, void *user_data );
static gchar     *ioption_get_id( const FMAIOption *option );
static gchar     *ioption_get_label( const FMAIOption *option );
static gchar     *ioption_get_description( const FMAIOption *option );
static GdkPixbuf *ioption_get_pixbuf( const FMAIOption *option );
static void       instance_init( GTypeInstance *instance, gpointer klass );
static void       instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void       instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void       instance_dispose( GObject *object );
static void       instance_finalize( GObject *object );

GType
fma_import_mode_get_type( void )
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
	static const gchar *thisfn = "fma_import_mode_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMAImportModeClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAImportMode ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	static const GInterfaceInfo ioption_iface_info = {
		( GInterfaceInitFunc ) ioption_iface_init,
		NULL,
		NULL
	};

	type = g_type_register_static( G_TYPE_OBJECT, "FMAImportMode", &info, 0 );

	g_type_add_interface_static( type, FMA_TYPE_IOPTION, &ioption_iface_info );

	return( type );
}

static void
class_init( FMAImportModeClass *klass )
{
	static const gchar *thisfn = "fma_import_mode_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( FMAImportModeClassPrivate, 1 );

	g_object_class_install_property( object_class, FMA_IMPORT_PROP_MODE_ID,
			g_param_spec_string(
					FMA_IMPORT_PROP_MODE,
					"Import mode",
					"The string identifier of the import mode, stored in user's preferences",
					"",
					G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, FMA_IMPORT_PROP_LABEL_ID,
			g_param_spec_string(
					FMA_IMPORT_PROP_LABEL,
					"Import label",
					"The label associated to the import mode",
					"",
					G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, FMA_IMPORT_PROP_DESCRIPTION_ID,
			g_param_spec_string(
					FMA_IMPORT_PROP_DESCRIPTION,
					"Import mode description",
					"The description associated to the import mode",
					"",
					G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));

	g_object_class_install_property( object_class, FMA_IMPORT_PROP_IMAGE_ID,
			g_param_spec_pointer(
					FMA_IMPORT_PROP_IMAGE,
					"Import mode image",
					"The image associated to the import mode, as a GdkPixbuf",
					G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE ));
}

static void
ioption_iface_init( FMAIOptionInterface *iface, void *user_data )
{
	static const gchar *thisfn = "fma_import_mode_ioption_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );

	iface->get_id = ioption_get_id;
	iface->get_label = ioption_get_label;
	iface->get_description = ioption_get_description;
	iface->get_pixbuf = ioption_get_pixbuf;
}

/*
 * ioption_get_id:
 * @option: this #FMAIOption instance.
 *
 * Returns: the ASCII id of the @option, as a newly allocated string which
 * should be g_free() by the caller.
 */
static gchar *
ioption_get_id( const FMAIOption *option )
{
	gchar *id;
	FMAImportMode *mode;

	g_return_val_if_fail( FMA_IS_IMPORT_MODE( option ), NULL );
	mode = FMA_IMPORT_MODE( option );
	id = NULL;

	if( !mode->private->dispose_has_run ){

		id = g_strdup( mode->private->mode );
	}

	return( id );
}

/*
 * ioption_get_label:
 * @option: this #FMAIOption instance.
 *
 * Returns: the label associated to @option, as a newly allocated string
 * which should be g_free() by the caller.
 */
static gchar *
ioption_get_label( const FMAIOption *option )
{
	gchar *label;
	FMAImportMode *mode;

	g_return_val_if_fail( FMA_IS_IMPORT_MODE( option ), NULL );
	mode = FMA_IMPORT_MODE( option );
	label = NULL;

	if( !mode->private->dispose_has_run ){

		label = g_strdup( mode->private->label );
	}

	return( label );
}

/*
 * ioption_get_description:
 * @option: this #FMAIOption instance.
 *
 * Returns: the description associated to @option, as a newly allocated string
 * which should be g_free() by the caller.
 */
static gchar *
ioption_get_description( const FMAIOption *option )
{
	gchar *description;
	FMAImportMode *mode;

	g_return_val_if_fail( FMA_IS_IMPORT_MODE( option ), NULL );
	mode = FMA_IMPORT_MODE( option );
	description = NULL;

	if( !mode->private->dispose_has_run ){

		description = g_strdup( mode->private->description );
	}

	return( description );
}

/*
 * ioption_get_pixbuf:
 * @option: this #FMAIOption instance.
 *
 * Returns: a new reference to the pixbuf associated to @option;
 * which should later be g_object_unref() by the caller.
 */
static GdkPixbuf *
ioption_get_pixbuf( const FMAIOption *option )
{
	GdkPixbuf *pixbuf;
	FMAImportMode *mode;

	g_return_val_if_fail( FMA_IS_IMPORT_MODE( option ), NULL );
	mode = FMA_IMPORT_MODE( option );
	pixbuf = NULL;

	if( !mode->private->dispose_has_run ){

		pixbuf = mode->private->image ? g_object_ref( mode->private->image ) : NULL;
	}

	return( pixbuf );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_import_mode_instance_init";
	FMAImportMode *self;

	g_return_if_fail( FMA_IS_IMPORT_MODE( instance ));

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );
	self = FMA_IMPORT_MODE( instance );

	self->private = g_new0( FMAImportModePrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	FMAImportMode *self;

	g_return_if_fail( FMA_IS_IMPORT_MODE( object ));
	self = FMA_IMPORT_MODE( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case FMA_IMPORT_PROP_MODE_ID:
				g_value_set_string( value, self->private->mode );
				break;

			case FMA_IMPORT_PROP_LABEL_ID:
				g_value_set_string( value, self->private->label );
				break;

			case FMA_IMPORT_PROP_DESCRIPTION_ID:
				g_value_set_string( value, self->private->description );
				break;

			case FMA_IMPORT_PROP_IMAGE_ID:
				g_value_set_pointer( value, self->private->image );
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
	FMAImportMode *self;

	g_return_if_fail( FMA_IS_IMPORT_MODE( object ));
	self = FMA_IMPORT_MODE( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case FMA_IMPORT_PROP_MODE_ID:
				g_free( self->private->mode );
				self->private->mode = g_value_dup_string( value );
				break;

			case FMA_IMPORT_PROP_LABEL_ID:
				g_free( self->private->label );
				self->private->label = g_value_dup_string( value );
				break;

			case FMA_IMPORT_PROP_DESCRIPTION_ID:
				g_free( self->private->description );
				self->private->description = g_value_dup_string( value );
				break;

			case FMA_IMPORT_PROP_IMAGE_ID:
				self->private->image = g_value_get_pointer( value );
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
	static const gchar *thisfn = "fma_import_mode_instance_dispose";
	FMAImportMode *self;

	g_return_if_fail( FMA_IS_IMPORT_MODE( object ));

	self = FMA_IMPORT_MODE( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

		self->private->dispose_has_run = TRUE;

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	static const gchar *thisfn = "fma_import_mode_instance_finalize";
	FMAImportMode *self;

	g_return_if_fail( FMA_IS_IMPORT_MODE( object ));

	g_debug( "%s: object=%p", thisfn, ( void * ) object );

	self = FMA_IMPORT_MODE( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/*
 * fma_import_mode_new:
 * @mode_id: the internal identifier of the import mode.
 *
 * Returns: a newly allocated #FMAImportMode object.
 *
 * Since: 3.2
 */
FMAImportMode *
fma_import_mode_new( guint mode_id )
{
	FMAImportMode *mode;

	mode = g_object_new( FMA_TYPE_IMPORT_MODE, NULL );

	mode->private->id = mode_id;

	return( mode );
}

/*
 * fma_import_mode_get_id:
 * @mode: a #FMAImportMode object.
 *
 * Returns: the internal identifier of the import mode.
 *
 * Since: 3.2
 */
guint
fma_import_mode_get_id( const FMAImportMode *mode )
{
	guint id;

	g_return_val_if_fail( FMA_IS_IMPORT_MODE( mode ), 0 );

	id = 0;

	if( !mode->private->dispose_has_run ){

		id = mode->private->id;
	}

	return( id );
}
