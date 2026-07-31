/* GLIB - Library of useful routines for C programming
 * Copyright (c) 2024  Florian "sp1rit" <sp1rit@disroot.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GLIB_ANDROID_H__
#define __GLIB_ANDROID_H__

#include <glib.h>
#ifndef G_PLATFORM_ANDROID
#error "This header may only be included if GLib was configured with Android platform support"
#endif

#include <glib/glib-java.h>

G_BEGIN_DECLS

GLIB_AVAILABLE_IN_2_86
jobject g_android_get_context  (void);

GLIB_AVAILABLE_IN_2_86
gboolean g_android_set_context (jobject context);

G_END_DECLS

#endif /* __GLIB_ANDROID_H__ */
