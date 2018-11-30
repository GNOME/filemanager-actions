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

#include <glib/gi18n.h>
#include <string.h>

#include <libnautilus-extension/nautilus-file-info.h>

#include <api/fma-core-utils.h>
#include <api/fma-iio-provider.h>
#include <api/fma-ifactory-object.h>
#include <api/fma-object-api.h>

#include "fma-factory-provider.h"
#include "fma-factory-object.h"
#include "fma-selected-info.h"
#include "fma-gnome-vfs-uri.h"

/* private class data
 */
struct _FMAObjectProfileClassPrivate {
	void *empty;							/* so that gcc -pedantic is happy */
};

/* private instance data
 */
struct _FMAObjectProfilePrivate {
	gboolean dispose_has_run;
};

#define PROFILE_NAME_PREFIX					"profile-"

#define fma_object_is_file( obj )			(( gboolean ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_ISFILE )))
#define fma_object_is_dir( obj )			(( gboolean ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_ISDIR )))
#define fma_object_is_multiple( obj )		(( gboolean ) GPOINTER_TO_UINT( fma_ifactory_object_get_as_void( FMA_IFACTORY_OBJECT( obj ), FMAFO_DATA_MULTIPLE )))

extern FMADataGroup profile_data_groups [];	/* defined in fma-object-profile-factory.c */

static FMAObjectIdClass *st_parent_class = NULL;

static GType         register_type( void );
static void          class_init( FMAObjectProfileClass *klass );
static void          instance_init( GTypeInstance *instance, gpointer klass );
static void          instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void          instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void          instance_dispose( GObject *object );
static void          instance_finalize( GObject *object );

static void          object_dump( const FMAObject *object );
static gboolean      object_is_valid( const FMAObject *object );

static void          ifactory_object_iface_init( FMAIFactoryObjectInterface *iface, void *user_data );
static guint         ifactory_object_get_version( const FMAIFactoryObject *instance );
static FMADataGroup *ifactory_object_get_groups( const FMAIFactoryObject *instance );
static void          ifactory_object_read_done( FMAIFactoryObject *instance, const FMAIFactoryProvider *reader, void *reader_data, GSList **messages );
static guint         ifactory_object_write_done( FMAIFactoryObject *instance, const FMAIFactoryProvider *writer, void *writer_data, GSList **messages );

static void          icontext_iface_init( FMAIContextInterface *iface, void *user_data );
static gboolean      icontext_is_candidate( FMAIContext *object, guint target, GList *selection );

static gboolean      convert_pre_v3_parameters( FMAObjectProfile *profile );
static gboolean      convert_pre_v3_parameters_str( gchar *str );
static gboolean      convert_pre_v3_multiple( FMAObjectProfile *profile );
static gboolean      convert_pre_v3_isfiledir( FMAObjectProfile *profile );
static void          read_done_ending( FMAObjectProfile *profile );
static void          split_path_parameters( FMAObjectProfile *profile );
static gboolean      is_valid_path_parameters( const FMAObjectProfile *profile );

static gchar        *object_id_new_id( const FMAObjectId *item, const FMAObjectId *new_parent );

GType
fma_object_profile_get_type( void )
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
	static const gchar *thisfn = "fma_object_profile_register_type";
	GType type;

	static GTypeInfo info = {
		sizeof( FMAObjectProfileClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( FMAObjectProfile ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	static const GInterfaceInfo icontext_iface_info = {
		( GInterfaceInitFunc ) icontext_iface_init,
		NULL,
		NULL
	};

	static const GInterfaceInfo ifactory_object_iface_info = {
		( GInterfaceInitFunc ) ifactory_object_iface_init,
		NULL,
		NULL
	};

	g_debug( "%s", thisfn );

	type = g_type_register_static( FMA_TYPE_OBJECT_ID, "FMAObjectProfile", &info, 0 );

	g_type_add_interface_static( type, FMA_TYPE_ICONTEXT, &icontext_iface_info );

	g_type_add_interface_static( type, FMA_TYPE_IFACTORY_OBJECT, &ifactory_object_iface_info );

	return( type );
}

static void
class_init( FMAObjectProfileClass *klass )
{
	static const gchar *thisfn = "fma_object_profile_class_init";
	GObjectClass *object_class;
	FMAObjectClass *naobject_class;
	FMAObjectIdClass *naobjectid_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;

	naobject_class = FMA_OBJECT_CLASS( klass );
	naobject_class->dump = object_dump;
	naobject_class->is_valid = object_is_valid;

	naobjectid_class = FMA_OBJECT_ID_CLASS( klass );
	naobjectid_class->new_id = object_id_new_id;

	klass->private = g_new0( FMAObjectProfileClassPrivate, 1 );

	fma_factory_object_define_properties( object_class, profile_data_groups );
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	static const gchar *thisfn = "fma_object_profile_instance_init";
	FMAObjectProfile *self;

	g_return_if_fail( FMA_IS_OBJECT_PROFILE( instance ));

	self = FMA_OBJECT_PROFILE( instance );

	g_debug( "%s: instance=%p (%s), klass=%p",
			thisfn, ( void * ) instance, G_OBJECT_TYPE_NAME( instance ), ( void * ) klass );

	self->private = g_new0( FMAObjectProfilePrivate, 1 );

	self->private->dispose_has_run = FALSE;
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	g_return_if_fail( FMA_IS_OBJECT_PROFILE( object ));
	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));

	if( !FMA_OBJECT_PROFILE( object )->private->dispose_has_run ){

		fma_factory_object_get_as_value( FMA_IFACTORY_OBJECT( object ), g_quark_to_string( property_id ), value );
	}
}

static void
instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec )
{
	g_return_if_fail( FMA_IS_OBJECT_PROFILE( object ));
	g_return_if_fail( FMA_IS_IFACTORY_OBJECT( object ));

	if( !FMA_OBJECT_PROFILE( object )->private->dispose_has_run ){

		fma_factory_object_set_from_value( FMA_IFACTORY_OBJECT( object ), g_quark_to_string( property_id ), value );
	}
}

static void
instance_dispose( GObject *object )
{
	static const gchar *thisfn = "fma_object_profile_instance_dispose";
	FMAObjectProfile *self;

	g_return_if_fail( FMA_IS_OBJECT_PROFILE( object ));

	self = FMA_OBJECT_PROFILE( object );

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
	static const gchar *thisfn = "fma_object_profile_instance_finalize";
	FMAObjectProfile *self;

	g_return_if_fail( FMA_IS_OBJECT_PROFILE( object ));

	self = FMA_OBJECT_PROFILE( object );

	g_debug( "%s: object=%p (%s)", thisfn, ( void * ) object, G_OBJECT_TYPE_NAME( object ));

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

static void
object_dump( const FMAObject *object )
{
	static const char *thisfn = "fma_object_profile_object_dump";
	FMAObjectProfile *self;

	g_return_if_fail( FMA_IS_OBJECT_PROFILE( object ));

	self = FMA_OBJECT_PROFILE( object );

	if( !self->private->dispose_has_run ){
		g_debug( "%s: object=%p (%s, ref_count=%d)", thisfn,
				( void * ) object, G_OBJECT_TYPE_NAME( object ), G_OBJECT( object )->ref_count );

		/* chain up to the parent class */
		if( FMA_OBJECT_CLASS( st_parent_class )->dump ){
			FMA_OBJECT_CLASS( st_parent_class )->dump( object );
		}

		g_debug( "+- end of dump" );
	}
}

static gboolean
object_is_valid( const FMAObject *object )
{
	static const gchar *thisfn = "fma_object_profile_object_is_valid";
	gboolean is_valid;
	FMAObjectProfile *profile;

	g_return_val_if_fail( FMA_IS_OBJECT_PROFILE( object ), FALSE );

	is_valid = FALSE;
	profile = FMA_OBJECT_PROFILE( object );

	if( !profile->private->dispose_has_run ){
		g_debug( "%s: profile=%p (%s)", thisfn, ( void * ) profile, G_OBJECT_TYPE_NAME( profile ));

		is_valid = is_valid_path_parameters( profile );

		/* chain up to the parent class */
		if( FMA_OBJECT_CLASS( st_parent_class )->is_valid ){
			is_valid &= FMA_OBJECT_CLASS( st_parent_class )->is_valid( object );
		}
	}

	return( is_valid );
}

static void
ifactory_object_iface_init( FMAIFactoryObjectInterface *iface, void *user_data )
{
	static const gchar *thisfn = "fma_object_profile_ifactory_object_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );

	iface->get_version = ifactory_object_get_version;
	iface->get_groups = ifactory_object_get_groups;
	iface->read_done = ifactory_object_read_done;
	iface->write_done = ifactory_object_write_done;
}

static guint
ifactory_object_get_version( const FMAIFactoryObject *instance )
{
	return( 1 );
}

static FMADataGroup *
ifactory_object_get_groups( const FMAIFactoryObject *instance )
{
	return( profile_data_groups );
}

static void
ifactory_object_read_done( FMAIFactoryObject *instance, const FMAIFactoryProvider *reader, void *reader_data, GSList **messages )
{
	static const gchar *thisfn = "fma_object_profile_ifactory_object_read_done";
	FMAObjectAction *action;
	guint iversion;

	g_debug( "%s: instance=%p", thisfn, ( void * ) instance );

	/* converts pre-v3 data
	 */
	action = FMA_OBJECT_ACTION( fma_object_get_parent( instance ));
	iversion = fma_object_get_iversion( action );
	g_debug( "%s: iversion=%d", thisfn, iversion );

	if( iversion < 3 ){
		fma_object_profile_convert_v2_to_last( FMA_OBJECT_PROFILE( instance ));

	/* must be always called, but is called when converting profile, anyway
	 */
	} else {
		read_done_ending( FMA_OBJECT_PROFILE( instance ));
	}
}

static guint
ifactory_object_write_done( FMAIFactoryObject *instance, const FMAIFactoryProvider *writer, void *writer_data, GSList **messages )
{
	return( IIO_PROVIDER_CODE_OK );
}

static void
icontext_iface_init( FMAIContextInterface *iface, void *user_data )
{
	static const gchar *thisfn = "fma_object_profile_icontext_iface_init";

	g_debug( "%s: iface=%p, user_data=%p", thisfn, ( void * ) iface, ( void * ) user_data );

	iface->is_candidate = icontext_is_candidate;
}

static gboolean
icontext_is_candidate( FMAIContext *object, guint target, GList *selection )
{
	return( TRUE );
}

/*
 * starting wih v3, parameters are relabeled
 *   pre-v3 parameters					post-v3 parameters
 *   ----------------------------		-----------------------------------
 *   									%b: (first) basename	(was %f)
 *   									%B: list of basenames	(was %m)
 *   									%c: count				(new)
 * 	 %d: (first) base directory			...................		(unchanged)
 * 										%D: list of base dir	(new)
 *   %f: (first) filename	-> %b		%f: (first) pathname	(new)
 *   									%F: list of pathnames	(was %M)
 *   %h: (first) hostname				...................		(unchanged)
 *   %m: list of basenames	-> %B		%m: (first) mimetype	(new)
 *   %M: list of pathnames	-> %F		%M: list of mimetypes	(new)
 *   									%n: (first) username	(was %U)
 *   %p: (first) port number			...................		(unchanged)
 *   %R: list of URIs		-> %U		-						(removed)
 *   %s: (first) scheme					...................		(unchanged)
 *   %u: (first) URI					...................		(unchanged)
 *   %U: (first) username	-> %n		%U: list of URIs		(was %R)
 *   									%w: (first) basename w/o ext.	(new)
 *   									%W: list of basenames w/o ext.	(new)
 *   									%x: (first) extension	(new)
 *   									%X: list of extensions	(new)
 *   %%: %								...................		(unchanged)
 *
 * For pre-v3 items,
 * - substitute %f with %b, and as a special case %d/%f -> %f
 * - substitute %m with %B
 * - substitute %M with %F
 * - substitute %R with %U
 * - substitute %U with %n
 *
 * Note that pre-v3 items only have parameters in the command and path fields.
 * Are only located in 'profile' objects.
 * Are only found in GConf or XML providers, as .desktop files have been
 * simultaneously introduced.
 *
 * As a recall of the dynamics of the reading when loading an action:
 *  - fma_object_action_read_done: set action defaults
 *  - nagp_reader_read_done: read profiles
 *     > nagp_reader_read_start: attach profile to its parent
 *     >  fma_object_profile_read_done: convert old parameters
 *
 * So, when converting v2 to v3 parameters in a v2 profile,
 * action already has its default values (including iversion=3)
 */
static gboolean
convert_pre_v3_parameters( FMAObjectProfile *profile )
{
	static const gchar *thisfn = "fma_object_profile_convert_pre_v3_parameters";
	gboolean path_changed, parms_changed;
	gchar *before;

	gchar *path = fma_object_get_path( profile );
	before = g_strdup( path );
	path_changed = convert_pre_v3_parameters_str( path );
	if( path_changed ){
		fma_object_set_path( profile, path );
		g_debug( "%s: path=%s changed to %s", thisfn, before, path );
	}
	g_free( before );
	g_free( path );

	gchar *parms = fma_object_get_parameters( profile );
	before = g_strdup( parms );
	parms_changed = convert_pre_v3_parameters_str( parms );
	if( parms_changed ){
		fma_object_set_parameters( profile, parms );
		g_debug( "%s: parameters=%s changed to %s", thisfn, before, parms );
	}
	g_free( before );
	g_free( parms );

	return( path_changed || parms_changed );
}

static gboolean
convert_pre_v3_parameters_str( gchar *str )
{
	gboolean changed;
	gchar *iter = str;

	changed = FALSE;
	while( iter != NULL &&
			strlen( iter ) > 0 &&
			( iter = g_strstr_len( iter, strlen( iter ), "%" )) != NULL ){

		switch( iter[1] ){

			/* as a special optimization case, "%d/%f" parameters
			 * may be favourably converted to just "%f" instead of "%d/%b"
			 */
			case 'd':
				if( !strncmp( iter, "%d/%f", 5 )){
					strncpy( iter, iter+3, strlen( iter ));
					changed = TRUE;
				}
				break;

			/* %f (first filename) becomes %b
			 */
			case 'f':
				iter[1] = 'b';
				changed = TRUE;
				break;

			/* %m (list of basenames) becomes %B
			 */
			case 'm':
				iter[1] = 'B';
				changed = TRUE;
				break;

			/* %M (list of filenames) becomes %F
			 */
			case 'M':
				iter[1] = 'F';
				changed = TRUE;
				break;

			/* %R (list of URIs) becomes %U
			 */
			case 'R':
				iter[1] = 'U';
				changed = TRUE;
				break;

			/* %U ((first) username) becomes %n
			 */
			case 'U':
				iter[1] = 'n';
				changed = TRUE;
				break;
		}

		iter += 2;
	}

	return( changed );
}

/*
 * default changes from accept_multiple=false
 *                   to selection_count>0
 */
static gboolean
convert_pre_v3_multiple( FMAObjectProfile *profile )
{
	static const gchar *thisfn = "fma_object_profile_convert_pre_v3_multiple";
	gboolean accept_multiple;
	gchar *selection_count;

	accept_multiple = fma_object_is_multiple( profile );
	selection_count = g_strdup( accept_multiple ? ">0" : "=1" );
	fma_object_set_selection_count( profile, selection_count );
	g_debug( "%s: accept_multiple=%s changed to selection_count= %s",
			thisfn, accept_multiple ? "True":"False", selection_count );
	g_free( selection_count );

	return( TRUE );
}

/*
 * we may have file=true  and dir=false -> only files          -> all/allfiles
 *             file=false and dir=true  -> only dirs           -> inode/directory
 *             file=true  and dir=true  -> both files and dirs -> all/all
 *
 * we try to replace this with the corresponding mimetype, but only if
 * current mimetype is '*' (or * / * or all/all).
 *
 * note that 'inode/directory' is actually the mimetype provided by the
 * file manager; 'all/allfiles' mimetype has to be checked separately.
 */
static gboolean
convert_pre_v3_isfiledir( FMAObjectProfile *profile )
{
	static const gchar *thisfn = "fma_object_profile_convert_pre_v3_isfiledir";
	gboolean is_all_mimetypes;
	gboolean converted;
	gboolean isfile, isdir;
	GSList *mimetypes;
	GSList *before_list;
	gchar *before_str, *after_str;

	converted = FALSE;

	fma_object_check_mimetypes( profile );
	is_all_mimetypes = fma_object_get_all_mimetypes( profile );
	g_debug( "%s: is_all_mimetypes=%s", thisfn, is_all_mimetypes ? "True":"False" );

	if( is_all_mimetypes ){
		converted = TRUE;
		mimetypes = NULL;
		before_list = fma_object_get_mimetypes( profile );

		/* this is needed because fma_object_is_file() does not return the default
		 * value when the data is not set (see #651911)
		 */
		isfile = TRUE;
		if( fma_factory_object_is_set( FMA_IFACTORY_OBJECT( profile ), FMAFO_DATA_ISFILE )){
			isfile = fma_object_is_file( profile );
		}
		isdir = fma_object_is_dir( profile );

		if( isfile ){
			if( isdir ){
				/* both file and dir -> do not modify mimetypes
				 */
				converted = FALSE;
			} else {
				/* files only
				 */
				mimetypes = g_slist_prepend( NULL, g_strdup( "all/allfiles" ));
			}
		} else {
			if( isdir ){
				/* dir only
				 */
				mimetypes = g_slist_prepend( NULL, g_strdup( "inode/directory" ));
			} else {
				/* not files nor dir: this is an invalid case -> do not modify
				 * mimetypes
				 */
				g_warning( "%s: is_dir=False, is_file=False is invalid", thisfn );
				converted = FALSE;
			}
		}

		if( converted ){
			fma_object_set_mimetypes( profile, mimetypes );

			before_str = fma_core_utils_slist_join_at_end( before_list, ";" );
			after_str = fma_core_utils_slist_join_at_end( mimetypes, ";" );
			g_debug( "%s; mimetypes=[%s] changed to [%s]", thisfn, before_str, after_str );
			g_free( after_str );
			g_free( before_str );
		}

		fma_core_utils_slist_free( mimetypes );
		fma_core_utils_slist_free( before_list );
	}

	return( converted );
}

static void
read_done_ending( FMAObjectProfile *profile )
{
	/* split path+parameters
	 * not done in io-desktop because some actions may have all arguments in path
	 */
	split_path_parameters( profile );

	/* prepare the context after the reading
	 */
	fma_icontext_read_done( FMA_ICONTEXT( profile ));

	/* last, set profile defaults
	 */
	fma_factory_object_set_defaults( FMA_IFACTORY_OBJECT( profile ));
}

/*
 * GConf used to store command path and parameters as two separated fields;
 * Desktop store them as one field;
 * fma-config-tool user interface displays and edits them as two fields
 * (this let us have Browse and Legend buttons)
 * => so we definitively keep them as separated boxed in our internal objects
 */
static void
split_path_parameters( FMAObjectProfile *profile )
{
	gchar *path, *parameters;
	gchar *exec;

	path = fma_object_get_path( profile );
	parameters = fma_object_get_parameters( profile );
	exec = g_strstrip( g_strdup_printf( "%s %s", path ? path : "", parameters ? parameters : "" ));
	g_free( parameters );
	g_free( path );

	fma_core_utils_str_split_first_word( exec, &path, &parameters );
	g_free( exec );

	fma_object_set_path( profile, path );
	fma_object_set_parameters( profile, parameters );
	g_free( parameters );
	g_free( path );
}

/*
 * historical behavior was to not check path nor parameters at all
 * 2.29.x series, and up to 2.30.0, have tried to check an actual executable path
 * but most of already actions only used a command, relying on the PATH env variable
 * so, starting with 2.30.1, we only check for non empty path+parameters
 */
static gboolean
is_valid_path_parameters( const FMAObjectProfile *profile )
{
	gboolean valid;
	gchar *path, *parameters;
	gchar *command;

	path = fma_object_get_path( profile );
	parameters = fma_object_get_parameters( profile );

	command = g_strdup_printf( "%s %s", path, parameters );
	g_strstrip( command );

	valid = g_utf8_strlen( command, -1 ) > 0;

	g_free( command );
	g_free( parameters );
	g_free( path );

	if( !valid ){
		fma_object_debug_invalid( profile, "command" );
	}

	return( valid );
}

/*
 * new_parent is specifically set to be able to allocate a new id for
 * the current profile into the target parent
 */
static gchar *
object_id_new_id( const FMAObjectId *item, const FMAObjectId *new_parent )
{
	gchar *id = NULL;

	g_return_val_if_fail( FMA_IS_OBJECT_PROFILE( item ), NULL );
	g_return_val_if_fail( !new_parent || FMA_IS_OBJECT_ACTION( new_parent ), NULL );

	if( !FMA_OBJECT_PROFILE( item )->private->dispose_has_run ){

		if( new_parent ){
			id = fma_object_action_get_new_profile_name( FMA_OBJECT_ACTION( new_parent ));
		}
	}

	return( id );
}

/**
 * fma_object_profile_new:
 *
 * Allocates a new profile.
 *
 * Returns: the newly allocated #FMAObjectProfile profile.
 *
 * Since: 2.30
 */
FMAObjectProfile *
fma_object_profile_new( void )
{
	FMAObjectProfile *profile;

	profile = g_object_new( FMA_TYPE_OBJECT_PROFILE, NULL );

	return( profile );
}

/**
 * fma_object_profile_new_with_defaults:
 *
 * Allocates a new profile, and set default values.
 *
 * Returns: the newly allocated #FMAObjectProfile profile.
 *
 * Since: 2.30
 */
FMAObjectProfile *
fma_object_profile_new_with_defaults( void )
{
	FMAObjectProfile *profile = fma_object_profile_new();
	fma_object_set_id( profile, "profile-zero" );
	/* i18n: label for the default profile */
	fma_object_set_label( profile, _( "Default profile" ));
	fma_factory_object_set_defaults( FMA_IFACTORY_OBJECT( profile ));

	return( profile );
}

/**
 * fma_object_profile_convert_v2_to_last:
 * @profile: the #FMAObjectProfile profile to be converted.
 *
 * Converts a v2 profile to the last version, setting the defaults as needed.
 *
 * This is called after having converted a pre-v2 action on the newly created
 * profile, or just after having read a v2 profile.
 * In all situations, defaults are supposed to have been set.
 *
 * Since: 2.30
 */
void
fma_object_profile_convert_v2_to_last( FMAObjectProfile *profile )
{
	FMAObjectAction *action;
	guint iversion;
	gboolean parms_converted, multiple_converted, isfiledir_converted;

	g_return_if_fail( FMA_IS_OBJECT_PROFILE( profile ));

	action = FMA_OBJECT_ACTION( fma_object_get_parent( profile ));
	iversion = fma_object_get_iversion( action );
	g_return_if_fail( iversion < 3 );

	parms_converted = convert_pre_v3_parameters( profile );
	multiple_converted = convert_pre_v3_multiple( profile );
	isfiledir_converted = convert_pre_v3_isfiledir( profile );

	if( parms_converted || multiple_converted || isfiledir_converted ){
		fma_object_set_iversion( action, 3 );
	}

	read_done_ending( profile );
}
