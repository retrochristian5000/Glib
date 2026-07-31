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

#ifndef __G_JAVA_FILE_STREAM_H__
#define __G_JAVA_FILE_STREAM_H__

#include <glib.h>
#ifndef G_HAVE_JAVA_SUPPORT
#error "This header may only be used if GLib was compiled with Java support"
#endif

#include <gio/gio.h>
#include <jni.h>

G_BEGIN_DECLS

typedef struct _GJavaFileInputStream      GJavaFileInputStream;
typedef struct _GJavaFileInputStreamClass GJavaFileInputStreamClass;

#define G_TYPE_JAVA_FILE_INPUT_STREAM       (g_java_file_input_stream_get_type ())
#define G_JAVA_FILE_INPUT_STREAM(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), G_TYPE_JAVA_FILE_INPUT_STREAM, GJavaFileInputStream))
#define G_IS_JAVA_FILE_INPUT_STREAM(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), G_TYPE_JAVA_FILE_INPUT_STREAM))

GIO_AVAILABLE_IN_2_86
GType g_java_file_input_stream_get_type (void);

GIO_AVAILABLE_IN_2_86
GFileInputStream *g_java_file_input_stream_new_for_java (JNIEnv *env, jobject file_input_stream);


typedef struct _GJavaFileOutputStream      GJavaFileOutputStream;
typedef struct _GJavaFileOutputStreamClass GJavaFileOutputStreamClass;

#define G_TYPE_JAVA_FILE_OUTPUT_STREAM       (g_java_file_output_stream_get_type ())
#define G_JAVA_FILE_OUTPUT_STREAM(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), G_TYPE_JAVA_FILE_OUTPUT_STREAM, GJavaFileOutputStream))
#define G_IS_JAVA_FILE_OUTPUT_STREAM(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_JAVA_FILE_OUTPUT_STREAM))

GIO_AVAILABLE_IN_2_86
GType g_java_file_output_stream_get_type (void);

GIO_AVAILABLE_IN_2_86
GFileOutputStream *g_java_file_output_stream_new_for_java (JNIEnv *env, jobject file_output_stream);

GIO_AVAILABLE_IN_2_86
gboolean g_java_file_stream_have_io_exception (JNIEnv *env, GError **err);

G_END_DECLS

#endif // __G_JAVA_FILE_STREAM_H__
