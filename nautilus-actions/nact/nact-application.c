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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <private/na-object-action-fn.h>

#include <runtime/na-iabout.h>
#include <runtime/na-ipivot-consumer.h>
#include <runtime/na-utils.h>

#include "nact-application.h"
#include "nact-main-window.h"

/* private class data
 */
struct NactApplicationClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct NactApplicationPrivate {
	gboolean  dispose_has_run;
	NAPivot  *pivot;
};

/* private instance properties
 */
enum {
	NACT_APPLICATION_PROP_PIVOT_ID = 1
};

#define NACT_APPLICATION_PROP_PIVOT		"nact-application-pivot"

static gboolean              st_non_unique_opt = FALSE;
static gboolean              st_version_opt    = FALSE;

static GOptionEntry          st_entries[] = {
	{ "non-unique", 'n', 0, G_OPTION_ARG_NONE, &st_non_unique_opt,
			N_( "Set it to run multiple instances of the program [unique]" ), NULL },
	{ "version"   , 'v', 0, G_OPTION_ARG_NONE, &st_version_opt,
			N_( "Output the version number, and exit gracefully [no]" ), NULL },
	{ NULL }
};

static BaseApplicationClass *st_parent_class = NULL;

static GType    register_type( void );
static void     class_init( NactApplicationClass *klass );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void     instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void     instance_dispose( GObject *application );
static void     instance_finalize( GObject *application );

static gboolean appli_manage_options( BaseApplication *application );
static gboolean appli_initialize_unique_app( BaseApplication *application );
static gboolean appli_initialize_application( BaseApplication *application );
static gchar   *appli_get_application_name( BaseApplication *application );
static gchar   *appli_get_icon_name( BaseApplication *application );
static gchar   *appli_get_unique_app_name( BaseApplication *application );
static gchar   *appli_get_gtkbuilder_filename( BaseApplication *application );
static GObject *appli_get_main_window( BaseApplication *application );

GType
nact_application_get_type( void )
{
	static GType application_type = 0;

	if( !application_type ){
		application_type = register_type();
	}

	return( application_type );
}

static GType
register_type( void )
{
	static const gchar *thisfn = "nact_application_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( NactApplicationClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NactApplication ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( BASE_APPLICATION_TYPE, "NactApplication", &info, 0 );

	return( type );
}

static void
class_init( NactApplicationClass *klass )
{
	static const gchar *thisfn = "nact_application_class_init";
	GObjectClass *object_class;
	GParamSpec *spec;
	BaseApplicationClass *appli_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = BASE_APPLICATION_CLASS( g_type_class_peek_parent( klass ));

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->get_property = instance_get_property;
	object_class->set_property = instance_set_property;

	spec = g_param_spec_pointer(
			NACT_APPLICATION_PROP_PIVOT,
			NACT_APPLICATION_PROP_PIVOT,
			"NAPivot object pointer",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NACT_APPLICATION_PROP_PIVOT_ID, spec );

	klass->private = g_new0( NactApplicationClassPrivate, 1 );

	appli_class = BASE_APPLICATION_CLASS( klass );
	appli_class->manage_options = appli_manage_options;
	appli_class->initialize_unique_app = appli_initialize_unique_app;
	appli_class->initialize_application = appli_initialize_application;
	appli_class->get_application_name = appli_get_application_name;
	appli_class->get_icon_name = appli_get_icon_name;
	appli_class->get_unique_app_name = appli_get_unique_app_name;
	appli_class->get_ui_filename = appli_get_gtkbuilder_filename;
	appli_class->get_main_window = appli_get_main_window;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nact_application_instance_init";
	NactApplication *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_assert( NACT_IS_APPLICATION( instance ));
	self = NACT_APPLICATION( instance );

	self->private = g_new0( NactApplicationPrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NactApplication *self;

	g_assert( NACT_IS_APPLICATION( object ));
	self = NACT_APPLICATION( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case NACT_APPLICATION_PROP_PIVOT_ID:
				g_value_set_pointer( value, self->private->pivot );
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
	NactApplication *self;

	g_assert( NACT_IS_APPLICATION( object ));
	self = NACT_APPLICATION( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case NACT_APPLICATION_PROP_PIVOT_ID:
				self->private->pivot = g_value_get_pointer( value );
				break;

			default:
				G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, spec );
				break;
		}
	}
}

static void
instance_dispose( GObject *application )
{
	static const gchar *thisfn = "nact_application_instance_dispose";
	NactApplication *self;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );
	g_return_if_fail( NACT_IS_APPLICATION( application ));
	self = NACT_APPLICATION( application );

	if( !self->private->dispose_has_run ){

		if( self->private->pivot ){
			g_object_unref( self->private->pivot );
		}

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( application );
		}

		self->private->dispose_has_run = TRUE;
	}
}

static void
instance_finalize( GObject *application )
{
	static const gchar *thisfn = "nact_application_instance_finalize";
	NactApplication *self;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );
	g_return_if_fail( NACT_IS_APPLICATION( application ));
	self = NACT_APPLICATION( application );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( application );
	}
}

/**
 * Returns a newly allocated NactApplication object.
 *
 * @argc: count of command-line arguments.
 *
 * @argv: command-line arguments.
 */
NactApplication *
nact_application_new_with_args( int argc, char **argv )
{
	return(
			g_object_new(
					NACT_APPLICATION_TYPE,
					BASE_APPLICATION_PROP_ARGC, argc,
					BASE_APPLICATION_PROP_ARGV, argv,
					BASE_APPLICATION_PROP_OPTIONS, st_entries,
					NULL )
	);
}

/**
 * Returns a pointer on the NAPivot object.
 *
 * @application: this NactApplication object.
 *
 * The returned pointer is owned by the NactApplication object.
 * It should not be freed not unref by the caller.
 */
NAPivot *
nact_application_get_pivot( NactApplication *application )
{
	NAPivot *pivot = NULL;

	g_return_val_if_fail( NACT_IS_APPLICATION( application ), NULL );

	if( !application->private->dispose_has_run ){
		pivot = application->private->pivot;
	}

	return( pivot );
}

/*
 * overriden to manage command-line options
 */
static gboolean
appli_manage_options( BaseApplication *application )
{
	gboolean ok;

	/* call parent class */
	ok = BASE_APPLICATION_CLASS( st_parent_class )->manage_options( application );

	if( ok ){
		if( st_version_opt ){
			na_utils_print_version();
			ok = FALSE;
		}
	}

	return( ok );
}

/*
 * overrided to provide a personalized error message
 */
static gboolean
appli_initialize_unique_app( BaseApplication *application )
{
	gboolean ok;
	gchar *msg1, *msg2;

	/* call parent class */
	ok = BASE_APPLICATION_CLASS( st_parent_class )->initialize_unique_app( application );

	if( !ok ){
		msg1 = g_strdup( _( "Another instance of Nautilus Actions Configuration Tool is already running." ));
		/* i18n: another instance is already running: second line of error message */
		msg2 = g_strdup( _( "Please switch back to it." ));

		g_object_set( G_OBJECT( application ),
				BASE_APPLICATION_PROP_EXIT_MESSAGE1, msg1,
				BASE_APPLICATION_PROP_EXIT_MESSAGE2, msg2,
				NULL );

		g_free( msg2 );
		g_free( msg1 );
	}

	return( ok );
}

/*
 * Overrided to complete the initialization of the application:
 * - allocate the #NApivot here, so that it will be available when the
 *   #NactMainWindow will require it
 * - do not register #NactApplication as a NAIPivotConsumer as this is
 *   essentially the NactMainwindow which will receive and deal with
 *   NAPivot notification messages
 *
 * At last, let the base class do its work, i.e. creating the main window.
 *
 * When the pivot will be empty, NAIDuplicable signals must yet be
 * recorded in the system. Done here because :
 * - I don't want do this in NAPivot which is also used by the plugin,
 * - this is the last place where I'm pretty sure NAObject has not yet
 *   been registered.
 * So we allocate a new NAObject-derived object to be sure the interface
 * is correctly initialized.
 */
static gboolean
appli_initialize_application( BaseApplication *application )
{
	static const gchar *thisfn = "nact_application_appli_initialize_application";
	gboolean ok;
	NAObjectAction *fake;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	fake = na_object_action_new();
	g_object_unref( fake );

	NACT_APPLICATION( application )->private->pivot = na_pivot_new();
	na_pivot_load_items( NACT_APPLICATION( application )->private->pivot );

	/* call parent class */
	ok = BASE_APPLICATION_CLASS( st_parent_class )->initialize_application( application );

	return( ok );
}

static gchar *
appli_get_application_name( BaseApplication *application )
{
	/*static const gchar *thisfn = "nact_application_appli_get_application_name";*/

	/*g_debug( "%s: application=%p", thisfn, ( void * ) application );*/

	/* i18n: this is the application name, used in window title */
	return( g_strdup( _( "Nautilus Actions Configuration Tool" )));
}

static gchar *
appli_get_icon_name( BaseApplication *application )
{
	static const gchar *thisfn = "nact_application_appli_get_icon_name";

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	return( na_iabout_get_icon_name());
}

static gchar *
appli_get_unique_app_name( BaseApplication *application )
{
	static const gchar *thisfn = "nact_application_appli_get_unique_app_name";

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	if( st_non_unique_opt ){
		return( g_strdup( "" ));
	}

	return( g_strdup( "org.nautilus-actions.ConfigurationTool" ));
}

static gchar *
appli_get_gtkbuilder_filename( BaseApplication *application )
{
	return( g_strdup( PKGDATADIR "/nautilus-actions-config-tool.ui" ));
}

/*
 * this should be called only once as base class is supposed to keep
 * the pointer to the main BaseWindow window as a property
 */
static GObject *
appli_get_main_window( BaseApplication *application )
{
	static const gchar *thisfn = "nact_application_appli_get_main_window";
	BaseWindow *window;

	g_debug( "%s: application=%p", thisfn, ( void * ) application );

	window = BASE_WINDOW( nact_main_window_new( application ));

	na_pivot_register_consumer(
			nact_application_get_pivot( NACT_APPLICATION( application )),
			NA_IPIVOT_CONSUMER( window ));

	return( G_OBJECT( window ));
}