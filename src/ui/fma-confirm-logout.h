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

#ifndef __UI_FMA_CONFIRM_LOGOUT_H__
#define __UI_FMA_CONFIRM_LOGOUT_H__

/**
 * SECTION: fma_confirm_logout
 * @short_description: #FMAConfirmLogout class definition.
 * @include: ui/fma-confirm-logout.h
 *
 * This class is derived from BaseDialog.
 * It encapsulates the logout confirmation dialog.
 */

#include "base-dialog.h"
#include "nact-main-window-def.h"

G_BEGIN_DECLS

#define FMA_TYPE_CONFIRM_LOGOUT                ( fma_confirm_logout_get_type())
#define FMA_CONFIRM_LOGOUT( object )           ( G_TYPE_CHECK_INSTANCE_CAST( object, FMA_TYPE_CONFIRM_LOGOUT, FMAConfirmLogout ))
#define FMA_CONFIRM_LOGOUT_CLASS( klass )      ( G_TYPE_CHECK_CLASS_CAST( klass, FMA_TYPE_CONFIRM_LOGOUT, FMAConfirmLogoutClass ))
#define FMA_IS_CONFIRM_LOGOUT( object )        ( G_TYPE_CHECK_INSTANCE_TYPE( object, FMA_TYPE_CONFIRM_LOGOUT ))
#define FMA_IS_CONFIRM_LOGOUT_CLASS( klass )   ( G_TYPE_CHECK_CLASS_TYPE(( klass ), FMA_TYPE_CONFIRM_LOGOUT ))
#define FMA_CONFIRM_LOGOUT_GET_CLASS( object ) ( G_TYPE_INSTANCE_GET_CLASS(( object ), FMA_TYPE_CONFIRM_LOGOUT, FMAConfirmLogoutClass ))

typedef struct _FMAConfirmLogoutPrivate        FMAConfirmLogoutPrivate;

typedef struct {
	/*< private >*/
	BaseDialog               parent;
	FMAConfirmLogoutPrivate *private;
}
	FMAConfirmLogout;

typedef struct {
	/*< private >*/
	BaseDialogClass          parent;
}
	FMAConfirmLogoutClass;

GType    fma_confirm_logout_get_type( void );

gboolean fma_confirm_logout_run     ( NactMainWindow *parent );

G_END_DECLS

#endif /* __UI_FMA_CONFIRM_LOGOUT_H__ */
