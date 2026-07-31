/*
 * Copyright (c) 2024-2025 Florian "sp1rit" <sp1rit@disroot.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef __G_ANDROID_CONTENT_FILE_H__
#define __G_ANDROID_CONTENT_FILE_H__

#include <glib.h>
#ifndef G_PLATFORM_ANDROID
#error "This header may only be included if GLib was configured with Android platform support"
#endif

#include <gio/gio.h>
#include <glib/glib-android.h>
#include <jni.h>

G_BEGIN_DECLS

typedef struct _GAndroidContentFile      GAndroidContentFile;
typedef struct _GAndroidContentFileClass GAndroidContentFileClass;

#define G_TYPE_ANDROID_CONTENT_FILE       (g_android_content_file_get_type ())
#define G_ANDROID_CONTENT_FILE(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), G_TYPE_ANDROID_CONTENT_FILE, GAndroidContentFile))
#define G_IS_ANDROID_CONTENT_FILE(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), G_TYPE_ANDROID_CONTENT_FILE))

GIO_AVAILABLE_IN_2_86
GType g_android_content_file_get_type (void);

GIO_AVAILABLE_IN_2_86
GFile *g_android_content_file_from_uri (jobject uri);

GIO_AVAILABLE_IN_2_86
jobject g_android_content_file_get_uri_object (GAndroidContentFile *self);

GIO_AVAILABLE_IN_2_86
void g_android_content_file_persist (GAndroidContentFile *self, gboolean read, gboolean write);

G_END_DECLS

#endif // __G_ANDROID_CONTENT_FILE_H__
