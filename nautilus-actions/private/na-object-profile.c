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

#include <string.h>

#include <libnautilus-extension/nautilus-file-info.h>

#include <api/na-object-api.h>

#include <runtime/na-utils.h>

#include "na-iduplicable.h"
#include "na-object-profile-priv.h"
#include "na-gnome-vfs-uri.h"

/* private class data
 */
struct NAObjectProfileClassPrivate {
	void *empty;						/* so that gcc -pedantic is happy */
};

/* profile properties
 */
enum {
	NAPROFILE_PROP_PATH_ID = 1,
	NAPROFILE_PROP_PARAMETERS_ID,
	NAPROFILE_PROP_BASENAMES_ID,
	NAPROFILE_PROP_MATCHCASE_ID,
	NAPROFILE_PROP_MIMETYPES_ID,
	NAPROFILE_PROP_ISFILE_ID,
	NAPROFILE_PROP_ISDIR_ID,
	NAPROFILE_PROP_ACCEPT_MULTIPLE_ID,
	NAPROFILE_PROP_SCHEMES_ID,
	NAPROFILE_PROP_FOLDERS_ID
};

#define NAPROFILE_PROP_PATH					"na-profile-path"
#define NAPROFILE_PROP_PARAMETERS			"na-profile-parameters"
#define NAPROFILE_PROP_BASENAMES			"na-profile-basenames"
#define NAPROFILE_PROP_MATCHCASE			"na-profile-matchcase"
#define NAPROFILE_PROP_MIMETYPES			"na-profile-mimetypes"
#define NAPROFILE_PROP_ISFILE				"na-profile-isfile"
#define NAPROFILE_PROP_ISDIR				"na-profile-isdir"
#define NAPROFILE_PROP_ACCEPT_MULTIPLE		"na-profile-accept-multiple"
#define NAPROFILE_PROP_SCHEMES				"na-profile-schemes"
#define NAPROFILE_PROP_FOLDERS				"na-profile-folders"

static NAObjectClass *st_parent_class = NULL;

static GType     register_type( void );
static void      class_init( NAObjectProfileClass *klass );
static void      instance_init( GTypeInstance *instance, gpointer klass );
static void      instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec );
static void      instance_set_property( GObject *object, guint property_id, const GValue *value, GParamSpec *spec );
static void      instance_dispose( GObject *object );
static void      instance_finalize( GObject *object );

static gboolean  is_target_background_candidate( const NAObjectProfile *profile, NautilusFileInfo *current_folder );
static gboolean  is_target_toolbar_candidate( const NAObjectProfile *profile, NautilusFileInfo *current_folder );
static gboolean  is_current_folder_inside( const NAObjectProfile *profile, NautilusFileInfo *current_folder );
static gboolean  is_target_selection_candidate( const NAObjectProfile *profile, GList *files, gboolean from_nautilus );
static gboolean  tracked_is_directory( void *iter, gboolean from_nautilus );
static gchar    *tracked_to_basename( void *iter, gboolean from_nautilus );
static GFile    *tracked_to_location( void *iter, gboolean from_nautilus );
static gchar    *tracked_to_scheme( void *iter, gboolean from_nautilus );
static gchar    *tracked_to_mimetype( void *iter, gboolean from_nautilus );
static gchar    *tracked_to_uri( void *iter, gboolean from_nautilus );
static int       validate_schemes( GSList *schemes2test, void *iter, gboolean from_nautilus );
static gchar    *parse_parameters( const NAObjectProfile *profile, gint target, GList* files, gboolean from_nautilus );

static void      object_dump( const NAObject *profile );
static void      object_dump_list( const gchar *thisfn, const gchar *label, GSList *list );
static NAObject *object_new( const NAObject *profile );
static void      object_copy( NAObject *target, const NAObject *source );
static gboolean  object_are_equal( const NAObject *a, const NAObject *b );
static gboolean  object_is_valid( const NAObject *profile );
static gboolean  is_valid_path_parameters( const NAObjectProfile *profile );
static gboolean  is_valid_filenames( const NAObjectProfile *profile );
static gboolean  is_valid_mimetypes( const NAObjectProfile *profile );
static gboolean  is_valid_isfiledir( const NAObjectProfile *profile );
static gboolean  is_valid_schemes( const NAObjectProfile *profile );
static gboolean  is_valid_folders( const NAObjectProfile *profile );

static gchar    *object_id_new_id( const NAObjectId *object, const NAObjectId *new_parent );

GType
na_object_profile_get_type( void )
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
	static const gchar *thisfn = "na_object_profile_register_type";

	static GTypeInfo info = {
		sizeof( NAObjectProfileClass ),
		( GBaseInitFunc ) NULL,
		( GBaseFinalizeFunc ) NULL,
		( GClassInitFunc ) class_init,
		NULL,
		NULL,
		sizeof( NAObjectProfile ),
		0,
		( GInstanceInitFunc ) instance_init
	};

	g_debug( "%s", thisfn );

	return( g_type_register_static( NA_OBJECT_ID_TYPE, "NAObjectProfile", &info, 0 ));
}

static void
class_init( NAObjectProfileClass *klass )
{
	static const gchar *thisfn = "na_object_profile_class_init";
	GObjectClass *object_class;
	GParamSpec *spec;
	NAObjectClass *naobject_class;

	g_debug( "%s: klass=%p", thisfn, ( void * ) klass );

	st_parent_class = g_type_class_peek_parent( klass );

	object_class = G_OBJECT_CLASS( klass );
	object_class->dispose = instance_dispose;
	object_class->finalize = instance_finalize;
	object_class->set_property = instance_set_property;
	object_class->get_property = instance_get_property;

	spec = g_param_spec_string(
			NAPROFILE_PROP_PATH,
			"Command path",
			"Command path", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAPROFILE_PROP_PATH_ID, spec );

	spec = g_param_spec_string(
			NAPROFILE_PROP_PARAMETERS,
			"Command parameters",
			"Command parameters", "",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAPROFILE_PROP_PARAMETERS_ID, spec );

	spec = g_param_spec_pointer(
			NAPROFILE_PROP_BASENAMES,
			"Filenames mask",
			"Filenames mask",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAPROFILE_PROP_BASENAMES_ID, spec );

	spec = g_param_spec_boolean(
			NAPROFILE_PROP_MATCHCASE,
			"Match case",
			"Whether the filenames are case sensitive", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAPROFILE_PROP_MATCHCASE_ID, spec );

	spec = g_param_spec_pointer(
			NAPROFILE_PROP_MIMETYPES,
			"Mimetypes",
			"List of selectable mimetypes",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAPROFILE_PROP_MIMETYPES_ID, spec );

	spec = g_param_spec_boolean(
			NAPROFILE_PROP_ISFILE,
			"Only files",
			"Whether apply when only files are selected", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAPROFILE_PROP_ISFILE_ID, spec );

	spec = g_param_spec_boolean(
			NAPROFILE_PROP_ISDIR,
			"Only dirs",
			"Whether apply when only dirs are selected", FALSE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAPROFILE_PROP_ISDIR_ID, spec );

	spec = g_param_spec_boolean(
			NAPROFILE_PROP_ACCEPT_MULTIPLE,
			"Accept multiple selection",
			"Whether apply when multiple files or folders are selected", TRUE,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAPROFILE_PROP_ACCEPT_MULTIPLE_ID, spec );

	spec = g_param_spec_pointer(
			NAPROFILE_PROP_SCHEMES,
			"Schemes",
			"List of selectable schemes",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAPROFILE_PROP_SCHEMES_ID, spec );

	spec = g_param_spec_pointer(
			NAPROFILE_PROP_FOLDERS,
			"Folders",
			"List of folders to which the item applies",
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE );
	g_object_class_install_property( object_class, NAPROFILE_PROP_FOLDERS_ID, spec );

	klass->private = g_new0( NAObjectProfileClassPrivate, 1 );

	naobject_class = NA_OBJECT_CLASS( klass );
	naobject_class->dump = object_dump;
	naobject_class->new = object_new;
	naobject_class->copy = object_copy;
	naobject_class->are_equal = object_are_equal;
	naobject_class->is_valid = object_is_valid;
	naobject_class->get_childs = NULL;
	naobject_class->ref = NULL;
	naobject_class->unref = NULL;

	NA_OBJECT_ID_CLASS( klass )->new_id = object_id_new_id;
}

static void
instance_init( GTypeInstance *instance, gpointer klass )
{
	/*static const gchar *thisfn = "na_object_profile_instance_init";*/
	NAObjectProfile *self;

	/*g_debug( "%s: instance=%p, klass=%p", thisfn, ( void * ) instance, ( void * ) klass );*/
	g_return_if_fail( NA_IS_OBJECT_PROFILE( instance ));
	self = NA_OBJECT_PROFILE( instance );

	self->private = g_new0( NAObjectProfilePrivate, 1 );

	self->private->dispose_has_run = FALSE;

	/* initialize suitable default values
	 */
	self->private->path = g_strdup( "" );
	self->private->parameters = g_strdup( "" );
	self->private->basenames = g_slist_append( NULL, g_strdup( "*" ));
	self->private->match_case = TRUE;
	self->private->mimetypes = g_slist_append( NULL, g_strdup( "*/*" ));
	self->private->is_file = TRUE;
	self->private->is_dir = FALSE;
	self->private->accept_multiple = FALSE;
	self->private->schemes = g_slist_append( NULL, g_strdup( "file" ));
	self->private->folders = g_slist_append( NULL, g_strdup( "*" ));
}

static void
instance_get_property( GObject *object, guint property_id, GValue *value, GParamSpec *spec )
{
	NAObjectProfile *self;
	GSList *list;

	g_return_if_fail( NA_IS_OBJECT_PROFILE( object ));
	self = NA_OBJECT_PROFILE( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case NAPROFILE_PROP_PATH_ID:
				g_value_set_string( value, self->private->path );
				break;

			case NAPROFILE_PROP_PARAMETERS_ID:
				g_value_set_string( value, self->private->parameters );
				break;

			case NAPROFILE_PROP_BASENAMES_ID:
				list = na_utils_duplicate_string_list( self->private->basenames );
				g_value_set_pointer( value, list );
				break;

			case NAPROFILE_PROP_MATCHCASE_ID:
				g_value_set_boolean( value, self->private->match_case );
				break;

			case NAPROFILE_PROP_MIMETYPES_ID:
				list = na_utils_duplicate_string_list( self->private->mimetypes );
				g_value_set_pointer( value, list );
				break;

			case NAPROFILE_PROP_ISFILE_ID:
				g_value_set_boolean( value, self->private->is_file );
				break;

			case NAPROFILE_PROP_ISDIR_ID:
				g_value_set_boolean( value, self->private->is_dir );
				break;

			case NAPROFILE_PROP_ACCEPT_MULTIPLE_ID:
				g_value_set_boolean( value, self->private->accept_multiple );
				break;

			case NAPROFILE_PROP_SCHEMES_ID:
				list = na_utils_duplicate_string_list( self->private->schemes );
				g_value_set_pointer( value, list );
				break;

			case NAPROFILE_PROP_FOLDERS_ID:
				list = na_utils_duplicate_string_list( self->private->folders );
				g_value_set_pointer( value, list );
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
	NAObjectProfile *self;

	g_return_if_fail( NA_IS_OBJECT_PROFILE( object ));
	self = NA_OBJECT_PROFILE( object );

	if( !self->private->dispose_has_run ){

		switch( property_id ){
			case NAPROFILE_PROP_PATH_ID:
				g_free( self->private->path );
				self->private->path = g_value_dup_string( value );
				break;

			case NAPROFILE_PROP_PARAMETERS_ID:
				g_free( self->private->parameters );
				self->private->parameters = g_value_dup_string( value );
				break;

			case NAPROFILE_PROP_BASENAMES_ID:
				na_utils_free_string_list( self->private->basenames );
				self->private->basenames = na_utils_duplicate_string_list( g_value_get_pointer( value ));
				break;

			case NAPROFILE_PROP_MATCHCASE_ID:
				self->private->match_case = g_value_get_boolean( value );
				break;

			case NAPROFILE_PROP_MIMETYPES_ID:
				na_utils_free_string_list( self->private->mimetypes );
				self->private->mimetypes = na_utils_duplicate_string_list( g_value_get_pointer( value ));
				break;

			case NAPROFILE_PROP_ISFILE_ID:
				self->private->is_file = g_value_get_boolean( value );
				break;

			case NAPROFILE_PROP_ISDIR_ID:
				self->private->is_dir = g_value_get_boolean( value );
				break;

			case NAPROFILE_PROP_ACCEPT_MULTIPLE_ID:
				self->private->accept_multiple = g_value_get_boolean( value );
				break;

			case NAPROFILE_PROP_SCHEMES_ID:
				na_utils_free_string_list( self->private->schemes );
				self->private->schemes = na_utils_duplicate_string_list( g_value_get_pointer( value ));
				break;

			case NAPROFILE_PROP_FOLDERS_ID:
				na_utils_free_string_list( self->private->folders );
				self->private->folders = na_utils_duplicate_string_list( g_value_get_pointer( value ));
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
	/*static const gchar *thisfn = "na_object_profile_instance_dispose";*/
	NAObjectProfile *self;

	/*g_debug( "%s: object=%p", thisfn, ( void * ) object );*/
	g_return_if_fail( NA_IS_OBJECT_PROFILE( object ));
	self = NA_OBJECT_PROFILE( object );

	if( !self->private->dispose_has_run ){

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
	/*static const gchar *thisfn = "na_object_profile_instance_finalize";*/
	NAObjectProfile *self;

	/*g_debug( "%s: object=%p", thisfn, (void * ) object );*/
	g_return_if_fail( NA_IS_OBJECT_PROFILE( object ));
	self = NA_OBJECT_PROFILE( object );

	g_free( self->private->path );
	g_free( self->private->parameters );
	na_utils_free_string_list( self->private->basenames );
	na_utils_free_string_list( self->private->mimetypes );
	na_utils_free_string_list( self->private->schemes );
	na_utils_free_string_list( self->private->folders );

	g_free( self->private );

	/* chain call to parent class */
	if( G_OBJECT_CLASS( st_parent_class )->finalize ){
		G_OBJECT_CLASS( st_parent_class )->finalize( object );
	}
}

/**
 * na_object_profile_new:
 *
 * Allocates a new profile of the given name.
 *
 * Returns: the newly allocated #NAObjectProfile profile.
 */
NAObjectProfile *
na_object_profile_new( void )
{
	NAObjectProfile *profile = g_object_new( NA_OBJECT_PROFILE_TYPE, NULL );

	na_object_set_id( profile, OBJECT_PROFILE_PREFIX "zero" );

	/* i18n: default label for a new profile */
	na_object_set_label( profile, NA_OBJECT_PROFILE_DEFAULT_LABEL );

	return( profile );
}

/**
 * na_object_profile_get_path:
 * @profile: the #NAObjectProfile to be requested.
 *
 * Returns the path of the command attached to the profile.
 *
 * Returns: the command path as a newly allocated string. The returned
 * string must be g_free() by the caller.
 */
gchar *
na_object_profile_get_path( const NAObjectProfile *profile )
{
	gchar *path = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), NULL );

	if( !profile->private->dispose_has_run ){
		g_object_get( G_OBJECT( profile ), NAPROFILE_PROP_PATH, &path, NULL );
	}

	return( path );
}

/**
 * na_object_profile_get_parameters:
 * @profile: the #NAObjectProfile to be requested.
 *
 * Returns the parameters of the command attached to the profile.
 *
 * Returns: the command parameters as a newly allocated string. The
 * returned string must be g_free() by the caller.
 */
gchar *
na_object_profile_get_parameters( const NAObjectProfile *profile )
{
	gchar *parameters = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), NULL );

	if( !profile->private->dispose_has_run ){
		g_object_get( G_OBJECT( profile ), NAPROFILE_PROP_PARAMETERS, &parameters, NULL );
	}

	return( parameters );
}

/**
 * na_object_profile_get_basenames:
 * @profile: the #NAObjectProfile to be requested.
 *
 * Returns the basenames of the files to which the profile applies.
 *
 * Returns: a GSList of newly allocated strings. The list must be
 * na_utils_free_string_list() by the caller.
 *
 * See na_object_profile_set_basenames() for some rationale about
 * basenames.
 */
GSList *
na_object_profile_get_basenames( const NAObjectProfile *profile )
{
	GSList *basenames = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), NULL );

	if( !profile->private->dispose_has_run ){
		g_object_get( G_OBJECT( profile ), NAPROFILE_PROP_BASENAMES, &basenames, NULL );
	}

	return( basenames );
}

/**
 * na_object_profile_get_matchcase:
 * @profile: the #NAObjectProfile to be requested.
 *
 * Are specified basenames case sensitive ?
 *
 * Returns: %TRUE if the provided filenames are case sensitive, %FALSE
 * else.
 *
 * See na_object_profile_set_matchcase() for some rationale about case
 * sensitivity.
 */
gboolean
na_object_profile_get_matchcase( const NAObjectProfile *profile )
{
	gboolean matchcase = FALSE;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), FALSE );

	if( !profile->private->dispose_has_run ){
		g_object_get( G_OBJECT( profile ), NAPROFILE_PROP_MATCHCASE, &matchcase, NULL );
	}

	return( matchcase );
}

/**
 * na_object_profile_get_mimetypes:
 * @profile: the #NAObjectProfile to be requested.
 *
 * Returns the list of mimetypes this profile applies to.
 *
 * Returns: a GSList of newly allocated strings. The list must be
 * na_utils_free_string_list() by the caller.
 *
 * See na_object_profile_set_mimetypes() for some rationale about
 * mimetypes.
 */
GSList *
na_object_profile_get_mimetypes( const NAObjectProfile *profile )
{
	GSList *mimetypes = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), NULL );

	if( !profile->private->dispose_has_run ){
		g_object_get( G_OBJECT( profile ), NAPROFILE_PROP_MIMETYPES, &mimetypes, NULL );
	}

	return( mimetypes );
}

/**
 * na_object_profile_get_is_file:
 * @profile: the #NAObjectProfile to be requested.
 *
 * Does this profile apply if the selection contains files ?
 *
 * Returns: %TRUE if it applies, %FALSE else.
 *
 * See na_object_profile_set_isfiledir() for some rationale about file
 * selection.
 */
gboolean
na_object_profile_get_is_file( const NAObjectProfile *profile )
{
	gboolean isfile = FALSE;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), FALSE );

	if( !profile->private->dispose_has_run ){
		g_object_get( G_OBJECT( profile ), NAPROFILE_PROP_ISFILE, &isfile, NULL );
	}

	return( isfile );
}

/**
 * na_object_profile_get_is_dir:
 * @profile: the #NAObjectProfile to be requested.
 *
 * Does this profile apply if the selection contains folders ?
 *
 * Returns: %TRUE if it applies, %FALSE else.
 *
 * See na_object_profile_set_isfiledir() for some rationale about file
 * selection.
 */
gboolean
na_object_profile_get_is_dir( const NAObjectProfile *profile )
{
	gboolean isdir = FALSE;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), FALSE );

	if( !profile->private->dispose_has_run ){
		g_object_get( G_OBJECT( profile ), NAPROFILE_PROP_ISDIR, &isdir, NULL );
	}

	return( isdir );
}

/**
 * na_object_profile_get_multiple:
 * @profile: the #NAObjectProfile to be requested.
 *
 * Does this profile apply if selection contains multiple files or
 * folders ?
 *
 * Returns: %TRUE if it applies, %FALSE else.
 *
 * See na_object_profile_set_multiple() for some rationale about
 * multiple selection.
 */
gboolean
na_object_profile_get_multiple( const NAObjectProfile *profile )
{
	gboolean multiple = FALSE;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), FALSE );

	if( !profile->private->dispose_has_run ){
		g_object_get( G_OBJECT( profile ), NAPROFILE_PROP_ACCEPT_MULTIPLE, &multiple, NULL );
	}

	return( multiple );
}

/**
 * na_object_profile_get_schemes:
 * @profile: the #NAObjectProfile to be requested.
 *
 * Returns the list of schemes this profile applies to.
 *
 * Returns: a GSList of newly allocated strings. The list must be
 * na_utils_free_string_list() by the caller.
 *
 * See na_object_profile_set_schemes() for some rationale about
 * schemes.
 */
GSList *
na_object_profile_get_schemes( const NAObjectProfile *profile )
{
	GSList *schemes = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), NULL );

	if( !profile->private->dispose_has_run ){
		g_object_get( G_OBJECT( profile ), NAPROFILE_PROP_SCHEMES, &schemes, NULL );
	}

	return( schemes );
}

/**
 * na_object_profile_get_folders:
 * @profile: the #NAObjectProfile to be requested.
 *
 * Returns the list of folders this item applies to.
 *
 * Returns: a GSList of newly allocated strings. The list must be
 * na_utils_free_string_list() by the caller.
 *
 * See na_object_profile_set_folders() for some rationale about
 * folders.
 */
GSList *
na_object_profile_get_folders( const NAObjectProfile *profile )
{
	GSList *folders = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), NULL );

	if( !profile->private->dispose_has_run ){

		g_object_get( G_OBJECT( profile ), NAPROFILE_PROP_FOLDERS, &folders, NULL );
	}

	return( folders );
}

/**
 * na_object_profile_set_path:
 * @profile: the #NAObjectProfile to be updated.
 * @path: the command path to be set.
 *
 * Sets the path of the command for this profile.
 *
 * #NAObjectProfile takes a copy of the provided path. This later may
 * so be g_free() by the caller after this function returns.
 */
void
na_object_profile_set_path( NAObjectProfile *profile, const gchar *path )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){
		g_object_set( G_OBJECT( profile ), NAPROFILE_PROP_PATH, path, NULL );
	}
}

/**
 * na_object_profile_set_parameters:
 * @profile: the #NAObjectProfile to be updated.
 * @parameters : the command parameters to be set.
 *
 * Sets the parameters of the command for this profile.
 *
 * #NAObjectProfile takes a copy of the provided parameters. This later
 * may so be g_free() by the caller after this function returns.
 */
void
na_object_profile_set_parameters( NAObjectProfile *profile, const gchar *parameters )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){
		g_object_set( G_OBJECT( profile ), NAPROFILE_PROP_PARAMETERS, parameters, NULL );
	}
}

/**
 * na_object_profile_set_basenames:
 * @profile: the #NAObjectProfile to be updated.
 * @basenames : the basenames to be set.
 *
 * Sets the basenames of the elements on which this profile applies.
 *
 * #NAObjectProfile takes a copy of the provided basenames. This later
 * may so be na_utils_free_string_list() by the caller after this
 * function returns.
 *
 * The basenames list defaults to the single element "*", which means
 * that the profile will apply to all basenames.
 */
void
na_object_profile_set_basenames( NAObjectProfile *profile, GSList *basenames )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){
		g_object_set( G_OBJECT( profile ), NAPROFILE_PROP_BASENAMES, basenames, NULL );
	}
}

/**
 * na_object_profile_set_matchcase:
 * @profile: the #NAObjectProfile to be updated.
 * @matchcase : whether the basenames are case sensitive or not.
 *
 * Sets the 'match_case' flag, indicating if specified basename
 * patterns are, or not, case sensitive.
 *
 * This value defaults to %TRUE, which means that basename patterns
 * default to be case sensitive.
 */
void
na_object_profile_set_matchcase( NAObjectProfile *profile, gboolean matchcase )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){
		g_object_set( G_OBJECT( profile ), NAPROFILE_PROP_MATCHCASE, matchcase, NULL );
	}
}

/**
 * na_object_profile_set_mimetypes:
 * @profile: the #NAObjectProfile to be updated.
 * @mimetypes: list of mimetypes to be matched.
 *
 * Sets the mimetypes on which this profile applies.
 *
 * #NAObjectProfile takes a copy of the provided mimetypes. This later
 * may so be na_utils_free_string_list() by the caller after this
 * function returns.
 *
 * The mimetypes list defaults to the single element "* / *", which
 * means that the profile will apply to all types of files.
 */
void
na_object_profile_set_mimetypes( NAObjectProfile *profile, GSList *mimetypes )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){
		g_object_set( G_OBJECT( profile ), NAPROFILE_PROP_MIMETYPES, mimetypes, NULL );
	}
}

/**
 * na_object_profile_set_isfile:
 * @profile: the #NAObjectProfile to be updated.
 * @isfile: whether the profile applies only to files.
 *
 * Sets the 'isfile' flag on which this profile applies.
 */
void
na_object_profile_set_isfile( NAObjectProfile *profile, gboolean isfile )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){
		g_object_set( G_OBJECT( profile ), NAPROFILE_PROP_ISFILE, isfile, NULL );
	}
}

/**
 * na_object_profile_set_isdir:
 * @profile: the #NAObjectProfile to be updated.
 * @isdir: the profile applies only to folders.
 *
 * Sets the 'isdir' flag on which this profile applies.
 */
void
na_object_profile_set_isdir( NAObjectProfile *profile, gboolean isdir )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){
		g_object_set( G_OBJECT( profile ), NAPROFILE_PROP_ISDIR, isdir, NULL );
	}
}

/**
 * na_object_profile_set_isfiledir:
 * @profile: the #NAObjectProfile to be updated.
 * @isfile: whether the profile applies only to files.
 * @isdir: the profile applies only to folders.
 *
 * Sets the 'isfile' and 'isdir' flags on which this profile applies.
 *
 * File selection defaults to %TRUE.
 *
 * Folder selection defaults to %FALSE, which means that this profile will
 * not apply if the selection contains folders.
 */
void
na_object_profile_set_isfiledir( NAObjectProfile *profile, gboolean isfile, gboolean isdir )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){
		g_object_set( G_OBJECT( profile ), NAPROFILE_PROP_ISFILE, isfile, NAPROFILE_PROP_ISDIR, isdir, NULL );
	}
}

/**
 * na_object_profile_set_multiple:
 * @profile: the #NAObjectProfile to be updated.
 * @multiple: TRUE if it does.
 *
 * Sets if this profile accept multiple selection ?
 *
 * This value defaults to %FALSE, which means that this profile will
 * not apply if the selection contains more than one element.
 */
void
na_object_profile_set_multiple( NAObjectProfile *profile, gboolean multiple )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){
		g_object_set( G_OBJECT( profile ), NAPROFILE_PROP_ACCEPT_MULTIPLE, multiple, NULL );
	}
}

/**
 * na_object_profile_set_scheme:
 * @profile: the #NAObjectProfile to be updated.
 * @scheme: name of the scheme.
 * @selected: whether this scheme is candidate to this profile.
 *
 * Sets the status of a scheme relative to this profile.
 */
void
na_object_profile_set_scheme( NAObjectProfile *profile, const gchar *scheme, gboolean selected )
{
	/*static const gchar *thisfn = "na_object_profile_set_scheme";*/
	gboolean exist;

	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){

		exist = na_utils_find_in_list( profile->private->schemes, scheme );
		/*g_debug( "%s: scheme=%s exist=%s", thisfn, scheme, exist ? "True":"False" );*/

		if( selected && !exist ){
			profile->private->schemes = g_slist_prepend( profile->private->schemes, g_strdup( scheme ));
		}
		if( !selected && exist ){
			profile->private->schemes = na_utils_remove_ascii_from_string_list( profile->private->schemes, scheme );
		}
	}
}

/**
 * na_object_profile_set_schemes:
 * @profile: the #NAObjectProfile to be updated.
 * @schemes: list of schemes which apply.
 *
 * Sets the schemes on which this profile applies.
 *
 * #NAObjectProfile takes a copy of the provided schemes. This later
 * may so be na_utils_free_string_list() by the caller after this
 * function returns.
 *
 * The schemes list defaults to the single element "file", which means
 * that the profile will only apply to local files.
 */
void
na_object_profile_set_schemes( NAObjectProfile *profile, GSList *schemes )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){
		g_object_set( G_OBJECT( profile ), NAPROFILE_PROP_SCHEMES, schemes, NULL );
	}
}

/**
 * na_object_profile_set_folders:
 * @profile: the #NAObjectProfile to be updated.
 * @folders: list of folders which apply.
 *
 * Sets the folders on which this profile applies.
 *
 * #NAObjectProfile takes a copy of the provided folders. This later
 * may so be na_utils_free_string_list() by the caller after this
 * function returns.
 */
void
na_object_profile_set_folders( NAObjectProfile *profile, GSList *folders )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){

		g_object_set( G_OBJECT( profile ), NAPROFILE_PROP_FOLDERS, folders, NULL );
	}
}

/**
 * na_object_profile_replace_folder_uri:
 * @profile: the #NAObjectProfile to be updated.
 * @old: the old uri.
 * @new: the new uri.
 *
 * Replaces the @old URI by the @new one.
 */
void
na_object_profile_replace_folder_uri( NAObjectProfile *profile, const gchar *old, const gchar *new )
{
	g_return_if_fail( NA_IS_OBJECT_PROFILE( profile ));

	if( !profile->private->dispose_has_run ){

		profile->private->folders = na_utils_remove_from_string_list( profile->private->folders, old );
		profile->private->folders = g_slist_append( profile->private->folders, ( gpointer ) g_strdup( new ));
	}
}

/**
 * na_object_profile_is_candidate:
 * @profile: the #NAObjectProfile to be checked.
 * @target: the current target.
 * @files: the currently selected items, as provided by Nautilus.
 *
 * Determines if the given profile is candidate to be displayed in the
 * Nautilus context menu, regarding the list of currently selected
 * items.
 *
 * Returns: %TRUE if this profile succeeds to all tests and is so a
 * valid candidate to be displayed in Nautilus context menu, %FALSE
 * else.
 *
 * This method could have been left outside of the #NAObjectProfile
 * class, as it is only called by the plugin. Nonetheless, it is much
 * more easier to code here (because we don't need all get methods, nor
 * free the parameters after).
 */
gboolean
na_object_profile_is_candidate( const NAObjectProfile *profile, gint target, GList *files )
{
	gboolean is_candidate;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), FALSE );

	if( !na_object_is_valid( profile )){
		return( FALSE );
	}

	switch( target ){
		case ITEM_TARGET_BACKGROUND:
			is_candidate = is_target_background_candidate( profile, ( NautilusFileInfo * ) files->data );
			break;

		case ITEM_TARGET_TOOLBAR:
			is_candidate = is_target_toolbar_candidate( profile, ( NautilusFileInfo * ) files->data );
			break;

		case ITEM_TARGET_SELECTION:
		default:
			is_candidate = is_target_selection_candidate( profile, files, TRUE );
	}

	return( is_candidate );
}

static gboolean
is_target_background_candidate( const NAObjectProfile *profile, NautilusFileInfo *current_folder )
{
	gboolean is_candidate;

	is_candidate = is_current_folder_inside( profile, current_folder );

	return( is_candidate );
}

static gboolean
is_target_toolbar_candidate( const NAObjectProfile *profile, NautilusFileInfo *current_folder )
{
	gboolean is_candidate;

	is_candidate = is_current_folder_inside( profile, current_folder );

	return( is_candidate );
}

static gboolean
is_current_folder_inside( const NAObjectProfile *profile, NautilusFileInfo *current_folder )
{
	gboolean is_inside;
	GSList *ifold;
	const gchar *path;
	gchar *current_folder_uri;

	is_inside = FALSE;
	current_folder_uri = nautilus_file_info_get_uri( current_folder );

	for( ifold = profile->private->folders ; ifold && !is_inside ; ifold = ifold->next ){
		path = ( const gchar * ) ifold->data;
		if( path && g_utf8_strlen( path, -1 )){
			if( !strcmp( path, "*" )){
				is_inside = TRUE;
			} else {
				is_inside = g_str_has_prefix( current_folder_uri, path );
				g_debug( "na_object_profile_is_current_folder_inside: current_folder_uri=%s, path=%s, is_inside=%s", current_folder_uri, path, is_inside ? "True":"False" );
			}
		}
	}

	g_free( current_folder_uri );

	return( is_inside );
}

/**
 * na_object_profile_is_candidate_for_tracked:
 * @profile: the #NAObjectProfile to be checked.
 * @files: the currently selected items, as a list of uris.
 *
 * Determines if the given profile is candidate to be displayed in the
 * Nautilus context menu, regarding the list of currently selected
 * items.
 *
 * Returns: %TRUE if this profile succeeds to all tests and is so a
 * valid candidate to be displayed in Nautilus context menu, %FALSE
 * else.
 *
 * The case where we only have URIs for target files is when we have
 * got this list through the org.nautilus_actions.DBus service (or
 * another equivalent) - typically for use in a command-line tool.
 */
gboolean
na_object_profile_is_candidate_for_tracked( const NAObjectProfile *profile, GList *tracked_items )
{
	gboolean is_candidate;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), FALSE );

	if( !na_object_is_valid( profile )){
		return( FALSE );
	}

	is_candidate = is_target_selection_candidate( profile, tracked_items, FALSE );

	return( is_candidate );
}

static gboolean
is_target_selection_candidate( const NAObjectProfile *profile, GList *files, gboolean from_nautilus )
{
	gboolean retv = FALSE;
	gboolean test_multiple_file = FALSE;
	gboolean test_file_type = FALSE;
	gboolean test_scheme = FALSE;
	gboolean test_basename = FALSE;
	gboolean test_mimetype = FALSE;
	GList* glob_patterns = NULL;
	GList* glob_mime_patterns = NULL;
	GSList* iter;
	GList* iter1;
	GList* iter2;
	guint dir_count = 0;
	guint file_count = 0;
	guint total_count = 0;
	guint scheme_ok_count = 0;
	guint glob_ok_count = 0;
	guint mime_glob_ok_count = 0;
	gboolean basename_match_ok = FALSE;
	gboolean mimetype_match_ok = FALSE;
	gchar *tmp_pattern, *tmp_filename, *tmp_filename2, *tmp_mimetype, *tmp_mimetype2;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), FALSE );

	if( profile->private->dispose_has_run ){
		return( FALSE );
	}

	if (profile->private->basenames && profile->private->basenames->next != NULL &&
			g_ascii_strcasecmp ((gchar*)(profile->private->basenames->data), "*") == 0)
	{
		/* if the only pattern is '*' then all files will match, so it
		 * is not necessary to make the test for each of them
		 */
		test_basename = TRUE;
	}
	else
	{
		for (iter = profile->private->basenames; iter; iter = iter->next)
		{
			tmp_pattern = (gchar*)iter->data;
			if (!profile->private->match_case)
			{
				/* --> if case-insensitive asked, lower all the string
				 * since the pattern matching function don't manage it
				 * itself.
				 */
				tmp_pattern = g_ascii_strdown ((gchar*)iter->data, strlen ((gchar*)iter->data));
			}

			glob_patterns = g_list_append (glob_patterns, g_pattern_spec_new (tmp_pattern));

			if (!profile->private->match_case)
			{
				g_free (tmp_pattern);
			}
		}
	}

	if (profile->private->mimetypes && profile->private->mimetypes->next != NULL &&
			(g_ascii_strcasecmp ((gchar*)(profile->private->mimetypes->data), "*") == 0 ||
			 g_ascii_strcasecmp ((gchar*)(profile->private->mimetypes->data), "*/*") == 0))
	{
		/* if the only pattern is '*' or * / * then all mimetypes will
		 * match, so it is not necessary to make the test for each of them
		 */
		test_mimetype = TRUE;
	}
	else
	{
		for (iter = profile->private->mimetypes; iter; iter = iter->next)
		{
			glob_mime_patterns = g_list_append (glob_mime_patterns, g_pattern_spec_new ((gchar*)iter->data));
		}
	}

	for( iter1 = files; iter1; iter1 = iter1->next ){

		tmp_filename = tracked_to_basename( iter1->data, from_nautilus );

		if (tmp_filename)
		{
			tmp_mimetype = tracked_to_mimetype( iter1->data, from_nautilus );

			if (!profile->private->match_case)
			{
				/* --> if case-insensitive asked, lower all the string
				 * since the pattern matching function don't manage it
				 * itself.
				 */
				tmp_filename2 = g_ascii_strdown (tmp_filename, strlen (tmp_filename));
				g_free (tmp_filename);
				tmp_filename = tmp_filename2;
			}

			/* --> for the moment we deal with all mimetypes case-insensitively */
			tmp_mimetype2 = g_ascii_strdown (tmp_mimetype, strlen (tmp_mimetype));
			g_free (tmp_mimetype);
			tmp_mimetype = tmp_mimetype2;

			if( tracked_is_directory( iter1->data, from_nautilus )){
				dir_count++;
			} else {
				file_count++;
			}

			scheme_ok_count += validate_schemes( profile->private->schemes, iter1->data, from_nautilus );

			if (!test_basename) /* if it is already ok, skip the test to improve performance */
			{
				basename_match_ok = FALSE;
				iter2 = glob_patterns;
				while (iter2 && !basename_match_ok)
				{
					if (g_pattern_match_string ((GPatternSpec*)iter2->data, tmp_filename))
					{
						basename_match_ok = TRUE;
					}
					iter2 = iter2->next;
				}

				if (basename_match_ok)
				{
					glob_ok_count++;
				}
			}

			if (!test_mimetype) /* if it is already ok, skip the test to improve performance */
			{
				mimetype_match_ok = FALSE;
				iter2 = glob_mime_patterns;
				while (iter2 && !mimetype_match_ok)
				{
					if (g_pattern_match_string ((GPatternSpec*)iter2->data, tmp_mimetype))
					{
						mimetype_match_ok = TRUE;
					}
					iter2 = iter2->next;
				}

				if (mimetype_match_ok)
				{
					mime_glob_ok_count++;
				}
			}

			g_free (tmp_mimetype);
			g_free (tmp_filename);
		}

		total_count++;
	}

	if ((files != NULL) && (files->next == NULL) && (!profile->private->accept_multiple))
	{
		test_multiple_file = TRUE;
	}
	else if (profile->private->accept_multiple)
	{
		test_multiple_file = TRUE;
	}

	if (profile->private->is_dir && profile->private->is_file)
	{
		if (dir_count > 0 || file_count > 0)
		{
			test_file_type = TRUE;
		}
	}
	else if (profile->private->is_dir && !profile->private->is_file)
	{
		if (file_count == 0)
		{
			test_file_type = TRUE;
		}
	}
	else if (!profile->private->is_dir && profile->private->is_file)
	{
		if (dir_count == 0)
		{
			test_file_type = TRUE;
		}
	}

	if (scheme_ok_count == total_count)
	{
		test_scheme = TRUE;
	}


	if (!test_basename) /* if not already tested */
	{
		if (glob_ok_count == total_count)
		{
			test_basename = TRUE;
		}
	}

	if (!test_mimetype) /* if not already tested */
	{
		if (mime_glob_ok_count == total_count)
		{
			test_mimetype = TRUE;
		}
	}

	if (test_basename && test_mimetype && test_file_type && test_scheme && test_multiple_file)
	{
		retv = TRUE;
	}

	g_list_foreach (glob_patterns, (GFunc) g_pattern_spec_free, NULL);
	g_list_free (glob_patterns);
	g_list_foreach (glob_mime_patterns, (GFunc) g_pattern_spec_free, NULL);
	g_list_free (glob_mime_patterns);

	return retv;
}

static gboolean
tracked_is_directory( void *iter, gboolean from_nautilus )
{
	gboolean is_dir;
	GFile *file;
	GFileType type;

	if( from_nautilus ){
		is_dir = nautilus_file_info_is_directory(( NautilusFileInfo * ) iter );

	} else {
		file = g_file_new_for_uri((( NATrackedItem * ) iter )->uri );
		type = g_file_query_file_type( file, G_FILE_QUERY_INFO_NONE, NULL );
		is_dir = ( type == G_FILE_TYPE_DIRECTORY );
		g_object_unref( file );
	}

	return( is_dir );
}

static gchar *
tracked_to_basename( void *iter, gboolean from_nautilus )
{
	gchar *bname;
	GFile *file;

	if( from_nautilus ){
		bname = nautilus_file_info_get_name(( NautilusFileInfo * ) iter );
	} else {
		file = g_file_new_for_uri((( NATrackedItem * ) iter )->uri );
		bname = g_file_get_basename( file );
		g_object_unref( file );
	}

	return( bname );
}

static GFile *
tracked_to_location( void *iter, gboolean from_nautilus )
{
	GFile *file;

	if( from_nautilus ){
		file = nautilus_file_info_get_location(( NautilusFileInfo * ) iter );
	} else {
		file = g_file_new_for_uri((( NATrackedItem * ) iter )->uri );
	}

	return( file );
}

static gchar *
tracked_to_mimetype( void *iter, gboolean from_nautilus )
{
	gchar *type;
	NATrackedItem *tracked;
	GFile *file;
	GFileInfo *info;

	type = NULL;
	if( from_nautilus ){
		type = nautilus_file_info_get_mime_type(( NautilusFileInfo * ) iter );

	} else {
		tracked = ( NATrackedItem * ) iter;
		if( tracked->mimetype ){
			type = g_strdup( tracked->mimetype );

		} else {
			file = g_file_new_for_uri((( NATrackedItem * ) iter )->uri );
			info = g_file_query_info( file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL );
			if( info ){
				type = g_strdup( g_file_info_get_content_type( info ));
				g_object_unref( info );
			}
			g_object_unref( file );
		}
	}

	return( type );
}

static gchar *
tracked_to_scheme( void *iter, gboolean from_nautilus )
{
	gchar *scheme;
	NAGnomeVFSURI *vfs;

	if( from_nautilus ){
		scheme = nautilus_file_info_get_uri_scheme(( NautilusFileInfo * ) iter );

	} else {
		vfs = g_new0( NAGnomeVFSURI, 1 );
		na_gnome_vfs_uri_parse( vfs, (( NATrackedItem * ) iter )->uri );
		scheme = g_strdup( vfs->scheme );
		na_gnome_vfs_uri_free( vfs );
	}

	return( scheme );
}

static gchar *
tracked_to_uri( void *iter, gboolean from_nautilus )
{
	gchar *uri;

	if( from_nautilus ){
		uri = nautilus_file_info_get_uri(( NautilusFileInfo * ) iter );
	} else {
		uri = g_strdup((( NATrackedItem * ) iter )->uri );
	}

	return( uri );
}

static int
validate_schemes( GSList* schemes2test, void* tracked_iter, gboolean from_nautilus )
{
	int retv = 0;
	GSList* iter;
	gboolean found = FALSE;
	gchar *scheme;

	iter = schemes2test;
	while( iter && !found ){

		scheme = tracked_to_scheme( tracked_iter, from_nautilus );

		if( g_ascii_strncasecmp( scheme, ( gchar * ) iter->data, strlen(( gchar * ) iter->data )) == 0 ){
			found = TRUE;
			retv = 1;
		}

		g_free( scheme );
		iter = iter->next;
	}

	return retv;
}

/**
 * Expands the parameters path, in function of the found tokens.
 *
 * @profile: the selected profile.
 * @target: the current target.
 * @files: the list of currently selected items, as provided by Nautilus.
 *
 * Valid parameters are :
 *
 * %d : base dir of the (first) selected file(s)/folder(s)
 * %f : the name of the (first) selected file/folder
 * %h : hostname of the (first) URI
 * %m : list of the basename of the selected files/directories separated by space.
 * %M : list of the selected files/directories with their complete path separated by space.
 * %p : port number from the (first) URI
 * %R : space-separated list of URIs
 * %s : scheme of the (first) URI
 * %u : (first) URI
 * %U : username of the (first) URI
 * %% : a percent sign
 *
 * Adding a parameter requires updating of :
 * - nautilus-actions/private/na-action-profile.c:na_object_profile_parse_parameters()
 * - nautilus-actions/runtime/na-xml-names.h
 * - nautilus-actions/nact/nact-icommand-tab.c:parse_parameters()
 * - nautilus-actions/nact/nautilus-actions-config-tool.ui:LegendDialog
 */
gchar *
na_object_profile_parse_parameters( const NAObjectProfile *profile, gint target, GList* files )
{
	return( parse_parameters( profile, target, files, TRUE ));
}

/**
 * na_object_profile_parse_parameters_for_tracked:
 * @profile: the selected profile.
 * @tracked_items: current selection.
 */
gchar *
na_object_profile_parse_parameters_for_tracked( const NAObjectProfile *profile, GList *tracked_items )
{
	return( parse_parameters( profile, ITEM_TARGET_SELECTION, tracked_items, FALSE ));
}

/*
 * Expands the parameters path, in function of the found tokens.
 */
static gchar *
parse_parameters( const NAObjectProfile *profile, gint target, GList* files, gboolean from_nautilus )
{
	gchar *parsed = NULL;
	GString *string;
	GList *ifi;
	gboolean first;
	gchar *iuri, *ipath, *ibname;
	GFile *iloc;
	gchar *uri = NULL;
	gchar *scheme = NULL;
	gchar *dirname = NULL;
	gchar *filename = NULL;
	gchar *hostname = NULL;
	gchar *username = NULL;
	gint port_number = 0;
	GString *basename_list, *pathname_list, *uris_list;
	gchar *tmp, *iter, *old_iter;
	NAGnomeVFSURI *vfs;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), NULL );

	if( profile->private->dispose_has_run ){
		return( NULL );
	}

	string = g_string_new( "" );
	basename_list = g_string_new( "" );
	pathname_list = g_string_new( "" );
	uris_list = g_string_new( "" );
	first = TRUE;

	for( ifi = files ; ifi ; ifi = ifi->next ){

		iuri = tracked_to_uri( ifi->data, from_nautilus );
		iloc = tracked_to_location( ifi->data, from_nautilus );
		ipath = g_file_get_path( iloc );
		ibname = g_file_get_basename( iloc );

		if( first ){

			vfs = g_new0( NAGnomeVFSURI, 1 );
			na_gnome_vfs_uri_parse( vfs, iuri );

			uri = g_strdup( iuri );
			dirname = g_path_get_dirname( ipath );
			scheme = g_strdup( vfs->scheme );
			filename = g_strdup( ibname );
			hostname = g_strdup( vfs->host_name );
			username = g_strdup( vfs->user_name );
			port_number = vfs->host_port;

			first = FALSE;
			na_gnome_vfs_uri_free( vfs );
		}

		tmp = g_shell_quote( ibname );
		g_string_append_printf( basename_list, " %s", tmp );
		g_free( tmp );

		tmp = g_shell_quote( ipath );
		g_string_append_printf( pathname_list, " %s", tmp );
		g_free( tmp );

		tmp = g_shell_quote( iuri );
		g_string_append_printf( uris_list, " %s", tmp );
		g_free( tmp );

		g_free( ibname );
		g_free( ipath );
		g_object_unref( iloc );
		g_free( iuri );
	}

	iter = g_strdup( profile->private->parameters );
	old_iter = iter;

	while(( iter = g_strstr_len( iter, strlen( iter ), "%" ))){

		string = g_string_append_len( string, old_iter, strlen( old_iter ) - strlen( iter ));
		switch( iter[1] ){

			/* base dir of the (first) selected item
			 */
			case 'd':
				tmp = g_shell_quote( dirname );
				string = g_string_append( string, tmp );
				g_free( tmp );
				break;

			/* basename of the (first) selected item
			 */
			case 'f':
				tmp = g_shell_quote( filename );
				string = g_string_append( string, tmp );
				g_free( tmp );
				break;

			/* hostname of the (first) URI
			 */
			case 'h':
				string = g_string_append( string, hostname );
				break;

			/* space-separated list of the basenames
			 */
			case 'm':
				string = g_string_append( string, basename_list->str );
				break;

			/* space-separated list of full pathnames
			 */
			case 'M':
				string = g_string_append( string, pathname_list->str );
				break;

			/* port number of the (first) URI
			 */
			case 'p':
				if( port_number > 0 ){
					g_string_append_printf( string, "%d", port_number );
				}
				break;

			/* list of URIs
			 */
			case 'R':
				string = g_string_append( string, uris_list->str );
				break;

			/* scheme of the (first) URI
			 */
			case 's':
				string = g_string_append( string, scheme );
				break;

			/* URI of the first item
			 */
			case 'u':
				string = g_string_append( string, uri );
				break;

			/* username of the (first) URI
			 */
			case 'U':
				string = g_string_append( string, username );
				break;

			/* a percent sign
			 */
			case '%':
				string = g_string_append_c( string, '%' );
				break;
		}

		iter += 2;			/* skip the % sign and the character after */
		old_iter = iter;	/* store the new start of the string */
	}

	string = g_string_append_len( string, old_iter, strlen( old_iter ));

	g_free( uri );
	g_free( dirname );
	g_free( scheme );
	g_free( hostname );
	g_free( username );
	g_free( iter );
	g_string_free( uris_list, TRUE );
	g_string_free( basename_list, TRUE );
	g_string_free( pathname_list, TRUE );

	parsed = g_string_free( string, FALSE );
	return( parsed );
}

static void
object_dump( const NAObject *object )
{
	static const gchar *thisfn = "na_object_profile_object_dump";
	NAObjectProfile *self;

	g_return_if_fail( NA_IS_OBJECT_PROFILE( object ));
	self = NA_OBJECT_PROFILE( object );

	if( !self->private->dispose_has_run ){

		g_debug( "%s:            path='%s'", thisfn, self->private->path );
		g_debug( "%s:      parameters='%s'", thisfn, self->private->parameters );
		g_debug( "%s: accept_multiple='%s'", thisfn, self->private->accept_multiple ? "True" : "False" );
		g_debug( "%s:          is_dir='%s'", thisfn, self->private->is_dir ? "True" : "False" );
		g_debug( "%s:         is_file='%s'", thisfn, self->private->is_file ? "True" : "False" );
		g_debug( "%s:      match_case='%s'", thisfn, self->private->match_case ? "True" : "False" );
		object_dump_list( thisfn, "basenames", self->private->basenames );
		object_dump_list( thisfn, "mimetypes", self->private->mimetypes );
		object_dump_list( thisfn, "  schemes", self->private->schemes );
		object_dump_list( thisfn, "  folders", self->private->folders );
	}
}

static void
object_dump_list( const gchar *thisfn, const gchar *label, GSList *list )
{
	gchar *string = na_utils_gslist_to_schema( list );
	g_debug( "%s:       %s=%s", thisfn, label, string );
	g_free( string );
}

static NAObject *
object_new( const NAObject *profile )
{
	return( NA_OBJECT( na_object_profile_new()));
}

static void
object_copy( NAObject *target, const NAObject *source )
{
	gchar *path, *parameters;
	gboolean matchcase, isfile, isdir, multiple;
	GSList *basenames, *mimetypes, *schemes;
	GSList *folders;

	g_return_if_fail( NA_IS_OBJECT_PROFILE( target ));
	g_return_if_fail( NA_IS_OBJECT_PROFILE( source ));

	if( !NA_OBJECT_PROFILE( target )->private->dispose_has_run &&
		!NA_OBJECT_PROFILE( source )->private->dispose_has_run ){

		g_object_get( G_OBJECT( source ),
				NAPROFILE_PROP_PATH, &path,
				NAPROFILE_PROP_PARAMETERS, &parameters,
				NAPROFILE_PROP_BASENAMES, &basenames,
				NAPROFILE_PROP_MATCHCASE, &matchcase,
				NAPROFILE_PROP_MIMETYPES, &mimetypes,
				NAPROFILE_PROP_ISFILE, &isfile,
				NAPROFILE_PROP_ISDIR, &isdir,
				NAPROFILE_PROP_ACCEPT_MULTIPLE, &multiple,
				NAPROFILE_PROP_SCHEMES, &schemes,
				NAPROFILE_PROP_FOLDERS, &folders,
				NULL );

		g_object_set( G_OBJECT( target ),
				NAPROFILE_PROP_PATH, path,
				NAPROFILE_PROP_PARAMETERS, parameters,
				NAPROFILE_PROP_BASENAMES, basenames,
				NAPROFILE_PROP_MATCHCASE, matchcase,
				NAPROFILE_PROP_MIMETYPES, mimetypes,
				NAPROFILE_PROP_ISFILE, isfile,
				NAPROFILE_PROP_ISDIR, isdir,
				NAPROFILE_PROP_ACCEPT_MULTIPLE, multiple,
				NAPROFILE_PROP_SCHEMES, schemes,
				NAPROFILE_PROP_FOLDERS, folders,
				NULL );

		g_free( path );
		g_free( parameters );
		na_utils_free_string_list( basenames );
		na_utils_free_string_list( mimetypes );
		na_utils_free_string_list( schemes );
	}
}

static gboolean
object_are_equal( const NAObject *a, const NAObject *b )
{
	NAObjectProfile *first = NA_OBJECT_PROFILE( a );
	NAObjectProfile *second = NA_OBJECT_PROFILE( b );
	gboolean equal = TRUE;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( a ), FALSE );
	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( b ), FALSE );

	if( !NA_OBJECT_PROFILE( a )->private->dispose_has_run &&
		!NA_OBJECT_PROFILE( b )->private->dispose_has_run ){

		if( equal ){
			equal =
				( g_utf8_collate( first->private->path, second->private->path ) == 0 ) &&
				( g_utf8_collate( first->private->parameters, second->private->parameters ) == 0 );
		}

		if( equal ){
			equal = (( first->private->accept_multiple && second->private->accept_multiple ) ||
					( !first->private->accept_multiple && !second->private->accept_multiple ));
		}

		if( equal ){
			equal = (( first->private->is_dir && second->private->is_dir ) ||
					( !first->private->is_dir && !second->private->is_dir ));
		}

		if( equal ){
			equal = (( first->private->is_file && second->private->is_file ) ||
					( !first->private->is_file && !second->private->is_file ));
		}

		if( equal ){
			equal = na_utils_string_lists_are_equal( first->private->basenames, second->private->basenames ) &&
					na_utils_string_lists_are_equal( first->private->mimetypes, second->private->mimetypes ) &&
					na_utils_string_lists_are_equal( first->private->schemes, second->private->schemes ) &&
					na_utils_string_lists_are_equal( first->private->folders, second->private->folders );
		}

#if NA_IDUPLICABLE_EDITION_STATUS_DEBUG
		g_debug( "na_object_profile_object_are_equal: a=%p (%s), b=%p (%s), are_equal=%s",
				( void * ) a, G_OBJECT_TYPE_NAME( a ),
				( void * ) b, G_OBJECT_TYPE_NAME( b ),
				equal ? "True":"False" );
#endif
	}

	return( equal );
}

/*
 * a valid NAObjectProfile requires :
 * - the first word of path+parameters should be an executable file
 * - filenames list of pseudo-regexp is not empty
 * - mimetypes list is not empty
 * - at least one of is_file, is_dir is set
 * - schemes list is not empty
 */
static gboolean
object_is_valid( const NAObject *profile )
{
	gboolean is_valid = FALSE;
	NAObjectItem *parent;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( profile ), FALSE );

	if( !NA_OBJECT_PROFILE( profile )->private->dispose_has_run ){

		is_valid = TRUE;
		parent = na_object_get_parent( profile );

		if( is_valid ){
			if( na_object_action_is_target_background( NA_OBJECT_ACTION( parent )) ||
				na_object_action_is_target_toolbar( NA_OBJECT_ACTION( parent ))){
				is_valid =
					is_valid_path_parameters( NA_OBJECT_PROFILE( profile )) &&
					is_valid_folders( NA_OBJECT_PROFILE( profile ));
			}
		}

		if( is_valid ){
			if( na_object_action_is_target_selection( NA_OBJECT_ACTION( parent ))){
				is_valid =
					is_valid_path_parameters( NA_OBJECT_PROFILE( profile )) &&
					is_valid_filenames( NA_OBJECT_PROFILE( profile )) &&
					is_valid_mimetypes( NA_OBJECT_PROFILE( profile )) &&
					is_valid_isfiledir( NA_OBJECT_PROFILE( profile )) &&
					is_valid_schemes( NA_OBJECT_PROFILE( profile ));
			}
		}
	}

	return( is_valid );
}

static gboolean
is_valid_path_parameters( const NAObjectProfile *profile )
{
	gboolean valid;
	gchar *command;
	gchar *exe;

	command = g_strdup_printf( "%s %s", profile->private->path, profile->private->parameters );
	exe = na_utils_get_first_word( command );

	valid =
		g_file_test( exe, G_FILE_TEST_EXISTS ) &&
		g_file_test( exe, G_FILE_TEST_IS_EXECUTABLE ) &&
		!g_file_test( exe, G_FILE_TEST_IS_DIR );

	g_free( exe );
	g_free( command );

	return( valid );
}

static gboolean
is_valid_filenames( const NAObjectProfile *profile )
{
	gboolean valid;

	valid = g_slist_length( profile->private->basenames ) > 0;

	return( valid );
}

static gboolean
is_valid_mimetypes( const NAObjectProfile *profile )
{
	gboolean valid;

	valid = g_slist_length( profile->private->mimetypes ) > 0;

	return( valid );
}

static gboolean
is_valid_isfiledir( const NAObjectProfile *profile )
{
	gboolean valid;

	valid = profile->private->is_file || profile->private->is_dir;

	return( valid );
}

static gboolean
is_valid_schemes( const NAObjectProfile *profile )
{
	gboolean valid;

	valid = g_slist_length( profile->private->schemes ) > 0;

	return( valid );
}

static gboolean
is_valid_folders( const NAObjectProfile *profile )
{
	gboolean valid;

	valid = g_slist_length( profile->private->folders ) > 0;

	return( valid );
}

/*
 * new_parent is specifically set to be able to allocate a new id for
 * the current profile into the target parent
 */
static gchar *
object_id_new_id( const NAObjectId *item, const NAObjectId *new_parent )
{
	gchar *id = NULL;

	g_return_val_if_fail( NA_IS_OBJECT_PROFILE( item ), NULL );
	g_return_val_if_fail( new_parent && NA_IS_OBJECT_ACTION( new_parent ), NULL );

	if( !NA_OBJECT_PROFILE( item )->private->dispose_has_run ){

		id = na_object_action_get_new_profile_name( NA_OBJECT_ACTION( new_parent ));
	}

	return( id );
}