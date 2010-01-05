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
#include <string.h>

#include <nautilus-actions/api/na-iio-provider.h>
#include <nautilus-actions/api/na-gconf-monitor.h>

#include "nagp-gconf-provider.h"
#include "nagp-read.h"
#include "nagp-write.h"
#include "nagp-keys.h"

/* private class data
 */
struct NagpGConfProviderClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

static GType         st_module_type = 0;
static GObjectClass *st_parent_class = NULL;
static gint          st_timeout_msec = 100;
static gint          st_timeout_usec = 100000;

static void     class_init( NagpGConfProviderClass *klass );
static void     iio_provider_iface_init( NAIIOProviderInterface *iface );
static void     instance_init( GTypeInstance *instance, gpointer klass );
static void     instance_dispose( GObject *object );
static void     instance_finalize( GObject *object );

static gchar   *get_id( const NAIIOProvider *provider );
static gchar   *get_name( const NAIIOProvider *provider );
static guint    get_version( const NAIIOProvider *provider );

static GList   *install_monitors( NagpGConfProvider *provider );
static void     config_path_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, NagpGConfProvider *provider );
static void     config_path_changed_reset_timeout( NagpGConfProvider *provider );
static void     config_path_changed_set_timeout( NagpGConfProvider *provider, const gchar *uuid );
static gboolean config_path_changed_trigger_interface( NagpGConfProvider *provider );
static gulong   time_val_diff( const GTimeVal *recent, const GTimeVal *old );
static gchar   *entry2uuid( GConfEntry *entry );

GType
nagp_gconf_provider_get_type( void )
{
	return( st_module_type );
}

void
nagp_gconf_provider_register_type( GTypeModule *module )
{
	static const gchar *thisfn = "nagp_gconf_provider_register_type";

	static GTypeInfo info = {
		sizeof( NagpGConfProviderClass ),
		NULL,
		NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NagpGConfProvider ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo iio_provider_iface_info = {
		( GInterfaceInitFunc ) iio_provider_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	st_module_type = g_type_module_register_type( module, G_TYPE_OBJECT, "NagpGConfProvider", &info, 0 );

	g_type_module_add_interface( module, st_module_type, NA_IIO_PROVIDER_TYPE, &iio_provider_iface_info );
}

static void
class_init( NagpGConfProviderClass *klass )
{
	static const gchar *thisfn = "nagp_gconf_provider_class_init";
	GObjectClass *object_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	klass->private = g_new0( NagpGConfProviderClassPrivate, 1 );
}

static void
iio_provider_iface_init( NAIIOProviderInterface *iface )
{
	static const gchar *thisfn = "nagp_gconf_provider_iio_provider_iface_init";

	g_debug( "%s: iface=%p", thisfn, ( void * ) iface );

	iface->get_id = get_id;
	iface->get_name = get_name;
	iface->get_version = get_version;
	iface->read_items = nagp_iio_provider_read_items;
	iface->is_willing_to_write = nagp_iio_provider_is_willing_to_write;
	iface->is_writable = nagp_iio_provider_is_writable;
	iface->write_item = nagp_iio_provider_write_item;
	iface->delete_item = nagp_iio_provider_delete_item;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "nagp_gconf_provider_instance_init";
	NagpGConfProvider *self;

	g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );
	g_return_if_fail( NAGP_IS_GCONF_PROVIDER( instance ));
	self = NAGP_GCONF_PROVIDER( instance );

	self->private = g_new0( NagpGConfProviderPrivate, 1 );

	self->private->dispose_has_run = FALSE;
	self->private->gconf = gconf_client_get_default();
	self->private->monitors = install_monitors( self );
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "nagp_gconf_provider_instance_dispose";
	NagpGConfProvider *self;

	g_debug( "%s: object=%p", thisfn, ( void * ) object );
	g_return_if_fail( NAGP_IS_GCONF_PROVIDER( object ));
	self = NAGP_GCONF_PROVIDER( object );

	if( !self->private->dispose_has_run ){

		self->private->dispose_has_run = TRUE;

		/* release the GConf monitoring */
		na_gconf_monitor_release_monitors( self->private->monitors );

		/* release the GConf connexion */
		g_object_unref( self->private->gconf );

		/* chain up to the parent class */
		if( G_OBJECT_CLASS( st_parent_class )->dispose ){
			G_OBJECT_CLASS( st_parent_class )->dispose( object );
		}
	}
}

static void
instance_finalize( GObject *object )
{
	NagpGConfProvider *self;

	g_assert( NAGP_IS_GCONF_PROVIDER( object ));
	self = NAGP_GCONF_PROVIDER( object );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static gchar *
get_id( const NAIIOProvider *provider )
{
	return( g_strdup( "na-gconf" ));
}

static gchar *
get_name( const NAIIOProvider *provider )
{
	return( g_strdup( _( "Nautilus-Actions GConf I/O Provider" )));
}

static guint
get_version( const NAIIOProvider *provider )
{
	return( 1 );
}

static GList *
install_monitors( NagpGConfProvider *provider )
{
	GList *list = NULL;

	g_return_val_if_fail( NAGP_IS_GCONF_PROVIDER( provider ), NULL );
	g_return_val_if_fail( NA_IS_IIO_PROVIDER( provider ), NULL );
	g_return_val_if_fail( !provider->private->dispose_has_run, NULL );

	/* monitor the configurations/ directory which contains all menus,
	 * actions and profiles definitions
	 */
	list = g_list_prepend( list,
			na_gconf_monitor_new(
					NA_GCONF_CONFIG_PATH,
					( GConfClientNotifyFunc ) config_path_changed_cb,
					provider ));

	return( list );
}

/*
 * this callback is triggered each time a value is changed under our
 * configurations/ directory ; as each object has several entries which
 * describe it, this callback is triggered several times for each object
 * update
 *
 * up to and including 1.10.1, the user interface took care of writing
 * a special key in GConf at the end of each update operations ;
 * as GConf monitored only this special key, it triggered this callback
 * once for each global update operation
 *
 * this special key was of the form xxx:yyyyyyyy-yyyy-yyyy-..., where :
 *    xxx was a sequential number (inside of the ui session)
 *    yyyyyyyy-yyyy-yyyy-... was the uuid of the involved action
 *
 * this was a sort of hack which simplified a lot the notification
 * system, but didn't take into account any modification which might
 * come from outside of the ui
 *
 * if the modification is made elsewhere (an action is imported as a
 * xml file in gconf, or gconf is directly edited), we'd have to rely
 * only on the standard monitor (GConf watch) mechanism
 *
 * this is what we do below, in three phases:
 * - first, GConf underlying subsystem advertises us, through the watch
 *   mechanism, of each and every modification ; this leads us to be
 *   triggered for each new/modified/deleted _entry_
 * - as we would trigger the NAIIOProvider interface only once for each
 *   modified _object_, we install a timer in order to wait for all
 *   entries have been modified, or another object is concerned
 * - as soon as one of the two previous conditions is met, we trigger
 *   the NAIIOProvider interface with the corresponding object id
 */
static void
config_path_changed_cb( GConfClient *client, guint cnxn_id, GConfEntry *entry, NagpGConfProvider *provider )
{
	/*static const gchar *thisfn = "nagp_gconf_provider_config_path_changed_cb";*/
	gchar *uuid;

	/*g_debug( "%s: client=%p, cnxnid=%u, entry=%p, provider=%p",
			thisfn, ( void * ) client, cnxn_id, ( void * ) entry, ( void * ) provider );*/

	g_return_if_fail( NAGP_IS_GCONF_PROVIDER( provider ));
	g_return_if_fail( NA_IS_IIO_PROVIDER( provider ));

	if( !provider->private->dispose_has_run ){

		uuid = entry2uuid( entry );
		/*g_debug( "%s: uuid=%s", thisfn, uuid );*/

		if( provider->private->event_source_id ){
			if( g_ascii_strcasecmp( uuid, provider->private->last_triggered_id )){

				/* there already has a timeout, but on another object
				 * so trigger the interface for the previous object
				 * and set the timeout for the new object
				 */
				config_path_changed_trigger_interface( provider );
				config_path_changed_set_timeout( provider, uuid );
			}

			/* there already has a timeout for this same object
			 * do nothing
			 */

		} else {
			/* there was not yet any timeout: set it
			 */
			config_path_changed_set_timeout( provider, uuid );
		}

		g_free( uuid );
	}
}

static void
config_path_changed_reset_timeout( NagpGConfProvider *provider )
{
	g_free( provider->private->last_triggered_id );
	provider->private->last_triggered_id = NULL;
	/*provider->private->last_event = ( GTimeVal ) 0;*/
	provider->private->event_source_id = 0;
}

static void
config_path_changed_set_timeout( NagpGConfProvider *provider, const gchar *uuid )
{
	config_path_changed_reset_timeout( provider );
	provider->private->last_triggered_id = g_strdup( uuid );
	g_get_current_time( &provider->private->last_event );
	provider->private->event_source_id =
		g_timeout_add(
				st_timeout_msec,
				( GSourceFunc ) config_path_changed_trigger_interface,
				provider );
}

/*
 * this timer is set when we receive the first event of a serie
 * we continue to loop until last event is at least one half of a
 * second old
 * there is no race condition here as we are not multithreaded
 * or .. is there ?
 */
static gboolean
config_path_changed_trigger_interface( NagpGConfProvider *provider )
{
	/*static const gchar *thisfn = "nagp_gconf_provider_config_path_changed_trigger_interface";*/
	GTimeVal now;
	gulong diff;

	/*g_debug( "%s: provider=%p", thisfn, ( void * ) provider );*/

	g_get_current_time( &now );
	diff = time_val_diff( &now, &provider->private->last_event );
	if( diff < st_timeout_usec ){
		return( TRUE );
	}

	na_iio_provider_config_changed( NA_IIO_PROVIDER( provider ), provider->private->last_triggered_id );

	config_path_changed_reset_timeout( provider );
	return( FALSE );
}

/*
 * returns the difference in microseconds.
 */
static gulong
time_val_diff( const GTimeVal *recent, const GTimeVal *old )
{
	gulong microsec = 1000000 * ( recent->tv_sec - old->tv_sec );
	microsec += recent->tv_usec  - old->tv_usec;
	return( microsec );
}

/*
 * gets the uuid from an entry
 */
static gchar *
entry2uuid( GConfEntry *entry )
{
	const gchar *path;
	const gchar *subpath;
	gchar **split;
	gchar *uuid;

	g_return_val_if_fail( entry, NULL );

	path = gconf_entry_get_key( entry );
	subpath = path + strlen( NA_GCONF_CONFIG_PATH ) + 1;
	split = g_strsplit( subpath, "/", -1 );
	/*g_debug( "%s: [0]=%s, [1]=%s", thisfn, split[0], split[1] );*/
	uuid = g_strdup( split[0] );
	g_strfreev( split );

	return( uuid );
}