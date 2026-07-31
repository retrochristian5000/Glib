/*
 * Copyright (c) 2024-2026 Florian "sp1rit" <sp1rit@disroot.org>
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

#include "gjavafilestream.h"
#include "gioerror.h"
#include <glib-java.h>

typedef struct
{
  struct
  {
    jclass klass;
    jmethodID get_channel;
  } j_file_istream;
  struct
  {
    jclass klass;
    jmethodID close;
    jmethodID read;
    jmethodID skip;
  } j_istream;
  struct
  {
    jclass klass;
    jmethodID get_channel;
  } j_file_ostream;
  struct
  {
    jclass klass;
    jmethodID close;
    jmethodID flush;
    jmethodID write;
  } j_ostream;
  struct
  {
    jclass klass;
    jmethodID get_position;
    jmethodID set_position;
    jmethodID get_size;
    jmethodID truncate;
  } j_file_channel;
  struct
  {
    jclass klass;
    jmethodID get_message;
  } j_throwable;
  struct
  {
    jclass io_exception;
    jclass eof_exception;
    jclass not_found_exception;
    jclass access_denied_exception;
    jclass not_empty_exception;
    jclass exists_exception;
    jclass loop_exception;
    jclass no_file_exception;
    jclass not_dir_exception;
    jclass malformed_uri_exception;
    jclass channel_closed_exception;
  } j_exceptions;
} GJavaFileStreamCache;

static GJavaFileStreamCache *
get_jcache (void)
{
  static GJavaFileStreamCache java_cache;
  static GJavaFileStreamCache *java_cache_initialized = NULL;
   if (g_once_init_enter_pointer (&java_cache_initialized))
    {
      JNIEnv *env = g_java_get_env ();

#define POPULATE_CLASS(cname, jclazz) {                     \
  jclass cls = (*env)->FindClass (env, (jclazz));           \
  java_cache.cname.klass = (*env)->NewGlobalRef (env, cls); \
  (*env)->DeleteLocalRef (env, cls);                        \
}
#define POPULATE_METHOD(cclass, cname, jname, jsignature) \
  java_cache.cclass.cname = (*env)->GetMethodID (env, java_cache.cclass.klass, (jname), (jsignature));
      POPULATE_CLASS (j_file_istream, "java/io/FileInputStream")
      POPULATE_METHOD (j_file_istream, get_channel, "getChannel", "()Ljava/nio/channels/FileChannel;")
      POPULATE_CLASS (j_istream, "java/io/InputStream")
      POPULATE_METHOD (j_istream, close, "close", "()V")
      POPULATE_METHOD (j_istream, read, "read", "([BII)I")
      POPULATE_METHOD (j_istream, skip, "skip", "(J)J")

      POPULATE_CLASS (j_file_ostream, "java/io/FileOutputStream")
      POPULATE_METHOD (j_file_ostream, get_channel, "getChannel", "()Ljava/nio/channels/FileChannel;")
      POPULATE_CLASS (j_ostream, "java/io/OutputStream")
      POPULATE_METHOD (j_ostream, close, "close", "()V")
      POPULATE_METHOD (j_ostream, flush, "flush", "()V")
      POPULATE_METHOD (j_ostream, write, "write", "([BII)V")

      POPULATE_CLASS (j_file_channel, "java/nio/channels/FileChannel")
      POPULATE_METHOD (j_file_channel, get_position, "position", "()J")
      POPULATE_METHOD (j_file_channel, set_position, "position", "(J)Ljava/nio/channels/FileChannel;")
      POPULATE_METHOD (j_file_channel, get_size, "size", "()J")
      POPULATE_METHOD (j_file_channel, truncate, "truncate", "(J)Ljava/nio/channels/FileChannel;")

      POPULATE_CLASS (j_throwable, "java/lang/Throwable")
      POPULATE_METHOD (j_throwable, get_message, "getMessage", "()Ljava/lang/String;")
#undef POPULATE_CLASS
#undef POPULATE_METHOD

#define POPULATE_EXCEPTION(cname, jclazz) {                        \
  jclass cls = (*env)->FindClass (env, (jclazz));                  \
  java_cache.j_exceptions.cname = (*env)->NewGlobalRef (env, cls); \
  (*env)->DeleteLocalRef (env, cls);                               \
}
      POPULATE_EXCEPTION (io_exception, "java/io/IOException")
      POPULATE_EXCEPTION (eof_exception, "java/io/EOFException")
      POPULATE_EXCEPTION (not_found_exception, "java/io/FileNotFoundException")
      POPULATE_EXCEPTION (access_denied_exception, "java/nio/file/AccessDeniedException")
      POPULATE_EXCEPTION (not_empty_exception, "java/nio/file/DirectoryNotEmptyException")
      POPULATE_EXCEPTION (exists_exception, "java/nio/file/FileAlreadyExistsException")
      POPULATE_EXCEPTION (loop_exception, "java/nio/file/FileSystemLoopException")
      POPULATE_EXCEPTION (no_file_exception, "java/nio/file/NoSuchFileException")
      POPULATE_EXCEPTION (not_dir_exception, "java/nio/file/NotDirectoryException")
      POPULATE_EXCEPTION (malformed_uri_exception, "java/net/MalformedURLException")
      POPULATE_EXCEPTION (channel_closed_exception, "java/nio/channels/ClosedChannelException")
#undef POPULATE_EXCEPTION

      g_once_init_leave_pointer (&java_cache_initialized, &java_cache);
    }
  return &java_cache;
}

/**
 * g_java_file_stream_have_io_exception:
 * @env: the java environment for the current thread
 * @error: return location for a [struct@GLib.Error], or %NULL
 *
 * This function behaves similarly to [func@GLib.java_check_exception],
 * with the difference being that the returned @error will be from
 * [error@Gio.IOErrorEnum].
 *
 * Returns: %TRUE if there was an exception, %FALSE otherwise
 * Since: 2.86
 */
gboolean
g_java_file_stream_have_io_exception (JNIEnv *env, GError **err)
{
  (*env)->PushLocalFrame (env, 2);

  jthrowable exception = (*env)->ExceptionOccurred (env);
  if (exception)
    {
      (*env)->ExceptionClear (env);

      if (err)
        {
          jstring msg = (*env)->CallObjectMethod (env, exception,
                                                  get_jcache ()->j_throwable.get_message);
          gchar *cmsg = g_java_jstring_to_str (msg, NULL);

#define CHECK_IO_EXCEPTION(eclass, domain, code)                             \
  if ((*env)->IsInstanceOf (env,                                             \
                            exception,                                       \
                            get_jcache ()->j_exceptions.eclass##_exception)) \
    {                                                                        \
      g_set_error (err, (domain), (code), "%s", cmsg);                       \
      goto fin;                                                              \
    }

          CHECK_IO_EXCEPTION (eof,
                              G_IO_ERROR,
                              G_IO_ERROR_BROKEN_PIPE)
          CHECK_IO_EXCEPTION (not_found,
                              G_IO_ERROR,
                              G_IO_ERROR_NOT_FOUND)
          CHECK_IO_EXCEPTION (access_denied,
                              G_IO_ERROR,
                              G_IO_ERROR_PERMISSION_DENIED)
          CHECK_IO_EXCEPTION (not_empty,
                              G_IO_ERROR,
                              G_IO_ERROR_NOT_EMPTY)
          CHECK_IO_EXCEPTION (exists,
                              G_IO_ERROR,
                              G_IO_ERROR_EXISTS)
          CHECK_IO_EXCEPTION (loop,
                              G_IO_ERROR,
                              G_IO_ERROR_PERMISSION_DENIED)
          CHECK_IO_EXCEPTION (no_file,
                              G_IO_ERROR,
                              G_IO_ERROR_WOULD_RECURSE)
          CHECK_IO_EXCEPTION (not_dir,
                              G_IO_ERROR,
                              G_IO_ERROR_NOT_DIRECTORY)
          CHECK_IO_EXCEPTION (malformed_uri,
                              G_IO_ERROR,
                              G_IO_ERROR_INVALID_FILENAME)
          CHECK_IO_EXCEPTION (channel_closed,
                              G_IO_ERROR,
                              G_IO_ERROR_CLOSED)

#undef CHECK_IO_EXCEPTION

          g_set_error_literal (err, G_IO_ERROR, G_IO_ERROR_UNKNOWN, cmsg);
fin:
          g_free (cmsg);
        }
      (*env)->PopLocalFrame (env, NULL);
      return TRUE;
    }
  (*env)->PopLocalFrame (env, NULL);
  return FALSE;
}


#define G_JAVA_STREAM_CACHE_BUFFER_SIZE 4096

struct _GJavaFileInputStreamClass
{
  GFileInputStreamClass parent_class;
};

struct _GJavaFileInputStream
{
  GFileInputStream parent_instance;

  jbyteArray cached_buffer;
  jobject stream;
};

G_DEFINE_TYPE (GJavaFileInputStream,
               g_java_file_input_stream,
               G_TYPE_FILE_INPUT_STREAM)

static void
g_java_file_input_stream_finalize (GObject *object)
{
  GJavaFileInputStream *self = (GJavaFileInputStream *)object;
  JNIEnv *env = g_java_get_env ();

  (*env)->DeleteGlobalRef (env, self->cached_buffer);
  (*env)->DeleteGlobalRef (env, self->stream);

  G_OBJECT_CLASS (g_java_file_input_stream_parent_class)->finalize (object);
}

static gboolean
g_java_file_input_stream_close (GInputStream *stream,
                                        GCancellable *cancellable,
                                        GError **error)
{
  GJavaFileInputStream *self = (GJavaFileInputStream *)stream;
  JNIEnv *env = g_java_get_env ();

  (*env)->CallVoidMethod (env, self->stream,
                          get_jcache ()->j_istream.close);

  return !g_java_file_stream_have_io_exception (env, error);
}

static gssize
g_java_file_input_stream_read (GInputStream *stream,
                                         void *buffer, gsize count,
                                         GCancellable *cancellable,
                                         GError **error)
{
  GJavaFileInputStream *self = (GJavaFileInputStream *)stream;
  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 1);

  gssize total = 0;
  while (total < (gssize)count)
    {
      jsize n_bytes = MIN (G_JAVA_STREAM_CACHE_BUFFER_SIZE, count - total); // !
      jint len = (*env)->CallIntMethod (env, self->stream,
                                        get_jcache ()->j_istream.read,
                                        self->cached_buffer,
                                        0, n_bytes);

      if (g_java_file_stream_have_io_exception (env, error))
        {
          (*env)->PopLocalFrame (env, NULL);
          return -1;
        }

      if (len == -1 || len == 0) // im not sure about len == 0
        goto exit;

      if (len > 0)
        (*env)->GetByteArrayRegion (env, self->cached_buffer, 0, len, &((jbyte *)buffer)[total]);
      total += len;
    }

exit:
  (*env)->PopLocalFrame (env, NULL);
  return total;
}

static gssize
g_java_file_input_stream_skip (GInputStream *stream,
                                         gsize count,
                                         GCancellable *cancellable,
                                         GError **error)
{
  GJavaFileInputStream *self = (GJavaFileInputStream *)stream;

  JNIEnv *env = g_java_get_env ();

  jlong len = (*env)->CallLongMethod (env, self->stream,
                                      get_jcache ()->j_istream.skip,
                                      count);

  return g_java_file_stream_have_io_exception (env, error) ? -1 : len;
}


static gboolean
g_java_file_input_stream_can_seek (GFileInputStream *stream)
{
  // is this always the case?
  return TRUE;
}

static gboolean
g_java_file_input_stream_seek (GFileInputStream *stream,
                                         goffset off, GSeekType type,
                                         GCancellable *cancellable,
                                         GError **error)
{
  GJavaFileInputStream *self = (GJavaFileInputStream *)stream;
  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 2);

  jobject channel = (*env)->CallObjectMethod (env, self->stream,
                                              get_jcache ()->j_file_istream.get_channel);
  jlong new_position = 0;
  switch (type)
    {
      case G_SEEK_CUR:
        {
          jlong current = (*env)->CallLongMethod (env, channel,
                                                  get_jcache ()->j_file_channel.get_position);
          new_position = current + off;
        }
        break;
      case G_SEEK_SET:
        new_position = off;
        break;
      case G_SEEK_END:
        {
          jlong size = (*env)->CallLongMethod (env, channel,
                                               get_jcache ()->j_file_channel.get_size);
          new_position = size + off;
        }
        break;
      default:
        g_critical ("Encountered unknown seek type: %d", type);
    }

  (*env)->CallObjectMethod (env, channel,
                            get_jcache ()->j_file_channel.set_position,
                            new_position);

  gboolean ret = !g_java_file_stream_have_io_exception (env, error);
  (*env)->PopLocalFrame (env, NULL);
  return ret;
}

static goffset
g_java_file_input_stream_tell (GFileInputStream *stream)
{
  GJavaFileInputStream *self = (GJavaFileInputStream *)stream;
  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 1);

  jobject channel = (*env)->CallObjectMethod (env, self->stream,
                                              get_jcache ()->j_file_istream.get_channel);
  jlong position = (*env)->CallLongMethod (env, channel,
                                           get_jcache ()->j_file_channel.get_position);

  goffset ret = g_java_file_stream_have_io_exception (env, NULL) ? -1 : position;
  (*env)->PopLocalFrame (env, NULL);
  return ret;
}

static void
g_java_file_input_stream_class_init (GJavaFileInputStreamClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GInputStreamClass *stream_class = (GInputStreamClass *)klass;
  GFileInputStreamClass *file_stream_class = (GFileInputStreamClass *)klass;

  object_class->finalize = g_java_file_input_stream_finalize;
  stream_class->close_fn = g_java_file_input_stream_close;
  stream_class->read_fn = g_java_file_input_stream_read;
  stream_class->skip = g_java_file_input_stream_skip;
  file_stream_class->can_seek = g_java_file_input_stream_can_seek;
  file_stream_class->seek = g_java_file_input_stream_seek;
  file_stream_class->tell = g_java_file_input_stream_tell;
}

static void
g_java_file_input_stream_init (GJavaFileInputStream *self)
{
  JNIEnv *env = g_java_get_env ();

  jbyteArray buffer = (*env)->NewByteArray (env, G_JAVA_STREAM_CACHE_BUFFER_SIZE);
  self->cached_buffer = (*env)->NewGlobalRef (env, buffer);
  (*env)->DeleteLocalRef (env, buffer);
}

/**
 * g_java_file_input_stream_new_for_java: (skip)
 * @env: the java environment for the current thread
 * @file_input_stream: the java stream object to wrap
 *
 * Create a new [class@Gio.JavaFileInputStream] instance from a Java
 * [FileInputStream](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/io/FileInputStream.html)
 * object.
 *
 * Returns: (transfer full): the newly created input stream
 * Since: 2.86
 */
GFileInputStream *
g_java_file_input_stream_new_for_java (JNIEnv *env, jobject file_input_stream)
{
  GJavaFileInputStream *self = g_object_new (G_TYPE_JAVA_FILE_INPUT_STREAM, NULL);
  self->stream = (*env)->NewGlobalRef (env, file_input_stream);
  return (GFileInputStream *)self;
}


struct _GJavaFileOutputStreamClass
{
  GFileOutputStreamClass parent_class;
};

struct _GJavaFileOutputStream
{
  GFileOutputStream parent_instance;

  jbyteArray cached_buffer;
  jobject stream;
};

G_DEFINE_TYPE (GJavaFileOutputStream,
               g_java_file_output_stream,
               G_TYPE_FILE_OUTPUT_STREAM)

static void
g_java_file_output_stream_finalize (GObject *object)
{
  GJavaFileInputStream *self = (GJavaFileInputStream *)object;
  JNIEnv *env = g_java_get_env ();

  (*env)->DeleteGlobalRef (env, self->cached_buffer);
  (*env)->DeleteGlobalRef (env, self->stream);

  G_OBJECT_CLASS (g_java_file_output_stream_parent_class)->finalize (object);
}

static gboolean
g_java_file_output_stream_close (GOutputStream *stream,
                                           GCancellable *cancellable,
                                           GError **error)
{
  GJavaFileOutputStream *self = (GJavaFileOutputStream *)stream;
  JNIEnv *env = g_java_get_env ();

  (*env)->CallVoidMethod (env, self->stream,
                          get_jcache ()->j_ostream.close);

  return !g_java_file_stream_have_io_exception (env, error);
}

static gboolean
g_java_file_output_stream_flush (GOutputStream *stream,
                                           GCancellable *cancellable,
                                           GError **error)
{
  GJavaFileOutputStream *self = (GJavaFileOutputStream *)stream;
  JNIEnv *env = g_java_get_env ();

  (*env)->CallVoidMethod (env, self->stream,
                          get_jcache ()->j_ostream.flush);

  return !g_java_file_stream_have_io_exception (env, error);
}

static gssize
g_java_file_output_stream_write (GOutputStream *stream,
                                           const void* buffer, gsize count,
                                           GCancellable *cancellable,
                                           GError **error)
{
  GJavaFileOutputStream *self = (GJavaFileOutputStream *)stream;
  JNIEnv *env = g_java_get_env ();

  gssize total = 0;
  while (total < (gssize)count)
    {
      jsize n_bytes = MIN (G_JAVA_STREAM_CACHE_BUFFER_SIZE, count - total); // !
      (*env)->SetByteArrayRegion (env, self->cached_buffer, 0, n_bytes, &((jbyte *)buffer)[total]);
      (*env)->CallVoidMethod (env, self->stream,
                              get_jcache ()->j_ostream.write,
                              self->cached_buffer,
                              0, n_bytes);

      if (g_java_file_stream_have_io_exception (env, error))
          return -1;

      total += n_bytes;
    }

  return total;
}

static gboolean
g_java_file_output_stream_can_seek (GFileOutputStream *stream)
{
  // is this always the case?
  return TRUE;
}

static gboolean
g_java_file_output_stream_can_truncate (GFileOutputStream *stream)
{
  // is this always the case?
  return TRUE;
}

static gboolean
g_java_file_output_stream_seek (GFileOutputStream *stream,
                                          goffset off, GSeekType type,
                                          GCancellable *cancellable,
                                          GError **error)
{
  GJavaFileOutputStream *self = (GJavaFileOutputStream *)stream;
  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 2);

  jobject channel = (*env)->CallObjectMethod (env, self->stream,
                                              get_jcache ()->j_file_ostream.get_channel);
  jlong new_position = 0;
  switch (type)
    {
      case G_SEEK_CUR:
        {
          jlong current = (*env)->CallLongMethod (env, channel,
                                                  get_jcache ()->j_file_channel.get_position);
          new_position = current + off;
        }
      break;
      case G_SEEK_SET:
        new_position = off;
      break;
      case G_SEEK_END:
        {
          jlong size = (*env)->CallLongMethod (env, channel,
                                               get_jcache ()->j_file_channel.get_size);
          new_position = size + off;
        }
      break;
      default:
        g_critical ("Encountered unknown seek type: %d", type);
    }

  (*env)->CallObjectMethod (env, channel,
                            get_jcache ()->j_file_channel.set_position,
                            new_position);

  gboolean ret = !g_java_file_stream_have_io_exception (env, error);
  (*env)->PopLocalFrame (env, NULL);
  return ret;
}

static goffset
g_java_file_output_stream_tell (GFileOutputStream *stream)
{
  GJavaFileOutputStream *self = (GJavaFileOutputStream *)stream;
  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 1);

  jobject channel = (*env)->CallObjectMethod (env, self->stream,
                                              get_jcache ()->j_file_istream.get_channel);
  jlong position = (*env)->CallLongMethod (env, channel,
                                           get_jcache ()->j_file_channel.get_position);

  goffset ret = g_java_file_stream_have_io_exception (env, NULL) ? -1 : position;
  (*env)->PopLocalFrame (env, NULL);
  return ret;
}

static gboolean
g_java_file_output_stream_truncate (GFileOutputStream *stream,
                                              goffset size,
                                              GCancellable *cancellable,
                                              GError **error)
{
  GJavaFileOutputStream *self = (GJavaFileOutputStream *)stream;
  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 2);

  jobject channel = (*env)->CallObjectMethod (env, self->stream,
                                              get_jcache ()->j_file_ostream.get_channel);
  (*env)->CallObjectMethod (env, channel,
                            get_jcache ()->j_file_channel.truncate,
                            size);

  gboolean ret = !g_java_file_stream_have_io_exception (env, error);
  (*env)->PopLocalFrame (env, NULL);
  return ret;
}

static void
g_java_file_output_stream_class_init (GJavaFileOutputStreamClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GOutputStreamClass *stream_class = (GOutputStreamClass *)klass;
  GFileOutputStreamClass *file_stream_class = (GFileOutputStreamClass *)klass;

  object_class->finalize = g_java_file_output_stream_finalize;
  stream_class->close_fn = g_java_file_output_stream_close;
  stream_class->flush = g_java_file_output_stream_flush;
  stream_class->write_fn = g_java_file_output_stream_write;
  file_stream_class->can_seek = g_java_file_output_stream_can_seek;
  file_stream_class->can_truncate = g_java_file_output_stream_can_truncate;
  file_stream_class->seek = g_java_file_output_stream_seek;
  file_stream_class->tell = g_java_file_output_stream_tell;
  file_stream_class->truncate_fn = g_java_file_output_stream_truncate;
}

static void
g_java_file_output_stream_init (GJavaFileOutputStream *self)
{
  JNIEnv *env = g_java_get_env ();

  jbyteArray buffer = (*env)->NewByteArray (env, G_JAVA_STREAM_CACHE_BUFFER_SIZE);
  self->cached_buffer = (*env)->NewGlobalRef (env, buffer);
  (*env)->DeleteLocalRef (env, buffer);
}

/**
 * g_java_file_output_stream_new_for_java: (skip)
 * @env: the java environment for the current thread
 * @file_output_stream: the java stream object to wrap
 *
 * Create a new [class@Gio.JavaFileOutputStream] instance from a Java
 * [FileOutputStream](https://docs.oracle.com/en/java/javase/17/docs/api/java.base/java/io/FileOutputStream.html)
 * object.
 *
 * Returns: (transfer full): the newly created output stream
 * Since: 2.86
 */
GFileOutputStream *
g_java_file_output_stream_new_for_java (JNIEnv *env, jobject file_output_stream)
{
  GJavaFileInputStream *self = g_object_new (G_TYPE_JAVA_FILE_OUTPUT_STREAM, NULL);
  self->stream = (*env)->NewGlobalRef (env, file_output_stream);
  return (GFileOutputStream *)self;
}
