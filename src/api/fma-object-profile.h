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

#ifndef __FILEMANAGER_ACTIONS_API_OBJECT_PROFILE_H__
#define __FILEMANAGER_ACTIONS_API_OBJECT_PROFILE_H__

/**
 * SECTION: object-profile
 * @title: FMAObjectProfile
 * @short_description: The Action Profile Class Definition
 * @include: filemanager-actions/fma-object-item.h
 */

#include "fma-object-id.h"

G_BEGIN_DECLS

#define FMA_TYPE_OBJECT_PROFILE                ( fma_object_profile_get_type())
#define FMA_OBJECT_PROFILE( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_OBJECT_PROFILE, FMAObjectProfile ))
#define FMA_OBJECT_PROFILE_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_OBJECT_PROFILE, FMAObjectProfileClass ))
#define FMA_IS_OBJECT_PROFILE( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_OBJECT_PROFILE ))
#define FMA_IS_OBJECT_PROFILE_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_OBJECT_PROFILE ))
#define FMA_OBJECT_PROFILE_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_OBJECT_PROFILE, FMAObjectProfileClass ))

typedef struct _FMAObjectProfilePrivate        FMAObjectProfilePrivate;

typedef struct {
	/*< private >*/
	FMAObjectId              parent;
	FMAObjectProfilePrivate *private;
}
	FMAObjectProfile;

typedef struct _FMAObjectProfileClassPrivate   FMAObjectProfileClassPrivate;

typedef struct {
	/*< private >*/
	FMAObjectIdClass              parent;
	FMAObjectProfileClassPrivate *private;
}
	FMAObjectProfileClass;

enum {
	NA_EXECUTION_MODE_NORMAL = 1,
	NA_EXECUTION_MODE_TERMINAL,
	NA_EXECUTION_MODE_EMBEDDED,
	NA_EXECUTION_MODE_DISPLAY_OUTPUT
};

GType             fma_object_profile_get_type          ( void );

FMAObjectProfile *fma_object_profile_new               ( void );
FMAObjectProfile *fma_object_profile_new_with_defaults ( void );

void              fma_object_profile_convert_v2_to_last( FMAObjectProfile *profile );

G_END_DECLS

#endif /* __FILEMANAGER_ACTIONS_API_OBJECT_PROFILE_H__ */
