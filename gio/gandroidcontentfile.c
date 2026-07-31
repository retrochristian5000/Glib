/*
 * Copyright (c) 2024,26 Florian "sp1rit" <sp1rit@disroot.org>
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

#include "giomodule-priv.h"

#include "gandroidcontentfileprivate.h"

typedef struct
{
  struct
  {
    jclass klass;
    jmethodID get_application_context;
    jmethodID get_content_resolver;
  } a_context;
  struct
  {
    jint flag_grant_read_uri_permission;
    jint flag_grant_write_uri_permission;
  } a_intent;
  struct
  {
    jclass klass;
    jmethodID get_type;
    jmethodID open_asset_fd;
    jmethodID open_typed_asset_fd;
    jmethodID query;
    jmethodID take_persistable_uri_permission;
    jstring scheme_content;
  } a_content_resolver;
  struct
  {
    jclass klass;
    jmethodID create_istream;
    jmethodID create_ostream;
    jstring mode_append;
    jstring mode_read;
    jstring mode_overwrite;
  } a_asset_fd;
  struct
  {
    jclass klass;
    jmethodID get_document_id;
    jmethodID get_tree_document_id;
    jmethodID build_children_from_tree;
    jmethodID build_document_from_tree;
    jmethodID copy_document;
    jmethodID create_document;
    jmethodID delete_document;
    jmethodID is_child_document;
    jmethodID is_document;
    jmethodID is_tree;
    jmethodID rename_document;
  } a_documents_contract;
  struct
  {
    jclass klass;
    jstring column_document_id;
    jstring column_display_name;
    jstring column_flags;
    jstring column_icon;
    jstring column_last_modified;
    jstring column_mime_type;
    jstring column_size;
    jstring column_summary;
    jint flag_dir_supports_create;
    jint flag_supports_copy;
    jint flag_supports_delete;
    jint flag_supports_move;
    jint flag_supports_rename;
    jint flag_supports_write;
    jint flag_virtual_document;
    jstring mime_directory;
  } a_documents_contract_document;
  struct
  {
    jclass klass;
    jmethodID get_int;
    jmethodID get_long;
    jmethodID get_string;
    jmethodID is_null;
    jmethodID move_to_next;
    jmethodID close;
  } a_cursor;
  struct
  {
    jclass klass;
    jmethodID get_path;
    jmethodID get_scheme;
    jmethodID normalize;
    jmethodID parse;
  } a_uri;
  struct
  {
    jclass klass;
    jmethodID guess_content_type_for_name;
    jstring mime_binary_data;
  } j_urlconnection;
  struct
  {
    jclass klass;
  } j_string;
  struct
  {
    jclass klass;
    jmethodID equals;
    jmethodID hash_code;
    jmethodID to_string;
  } j_object;
} GAndroidContentFileCache;

static GAndroidContentFileCache *
g_android_content_file_get_jcache (void)
{
  static GAndroidContentFileCache java_cache;
  static GAndroidContentFileCache *java_cache_initialized = NULL;
   if (g_once_init_enter_pointer (&java_cache_initialized))
    {
      JNIEnv *env = g_java_get_env ();
      (*env)->PushLocalFrame (env, 4);

#define POPULATE_CLASS(cname, jclazz) {                     \
  jclass cls = (*env)->FindClass (env, (jclazz));           \
  java_cache.cname.klass = (*env)->NewGlobalRef (env, cls); \
  (*env)->DeleteLocalRef (env, cls);                        \
}
#define POPULATE_METHOD(cclass, cname, jname, jsignature) \
  java_cache.cclass.cname = (*env)->GetMethodID (env, java_cache.cclass.klass, (jname), (jsignature));
#define POPULATE_STATIC_METHOD(cclass, cname, jname, jsignature) \
  java_cache.cclass.cname = (*env)->GetStaticMethodID (env, java_cache.cclass.klass, (jname), (jsignature));
#define POPULATE_FIELD(cclass, cname, jname) {                                                 \
    jfieldID field = (*env)->GetStaticFieldID (env, java_cache.cclass.klass, jname, "I");      \
    java_cache.cclass.cname = (*env)->GetStaticIntField (env, java_cache.cclass.klass, field); \
  }
#define POPULATE_STRING(cclass, cname, jname) {                                                            \
    jfieldID field = (*env)->GetStaticFieldID (env, java_cache.cclass.klass, jname, "Ljava/lang/String;"); \
    jstring string = (*env)->GetStaticObjectField (env, java_cache.cclass.klass, field);                   \
    java_cache.cclass.cname = (*env)->NewGlobalRef (env, string);                                          \
    (*env)->DeleteLocalRef (env, string);                                                                  \
  }

      POPULATE_CLASS (a_context, "android/content/Context")
      POPULATE_METHOD (a_context, get_application_context, "getApplicationContext", "()Landroid/content/Context;")
      POPULATE_METHOD (a_context, get_content_resolver, "getContentResolver", "()Landroid/content/ContentResolver;")

      POPULATE_CLASS (a_content_resolver, "android/content/ContentResolver")
      POPULATE_METHOD (a_content_resolver, get_type, "getType", "(Landroid/net/Uri;)Ljava/lang/String;")
      POPULATE_METHOD (a_content_resolver, open_asset_fd, "openAssetFileDescriptor", "(Landroid/net/Uri;Ljava/lang/String;Landroid/os/CancellationSignal;)Landroid/content/res/AssetFileDescriptor;")
      POPULATE_METHOD (a_content_resolver, open_typed_asset_fd, "openTypedAssetFileDescriptor", "(Landroid/net/Uri;Ljava/lang/String;Landroid/os/Bundle;Landroid/os/CancellationSignal;)Landroid/content/res/AssetFileDescriptor;")
      POPULATE_METHOD (a_content_resolver, query, "query", "(Landroid/net/Uri;[Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)Landroid/database/Cursor;")
      POPULATE_METHOD (a_content_resolver, take_persistable_uri_permission, "takePersistableUriPermission", "(Landroid/net/Uri;I)V")
      POPULATE_STRING (a_content_resolver, scheme_content, "SCHEME_CONTENT")

      POPULATE_CLASS (a_asset_fd, "android/content/res/AssetFileDescriptor")
      POPULATE_METHOD (a_asset_fd, create_istream, "createInputStream", "()Ljava/io/FileInputStream;")
      POPULATE_METHOD (a_asset_fd, create_ostream, "createOutputStream", "()Ljava/io/FileOutputStream;")
      java_cache.a_asset_fd.mode_append = (*env)->NewGlobalRef (env, (*env)->NewStringUTF (env, "wa"));
      java_cache.a_asset_fd.mode_read = (*env)->NewGlobalRef (env, (*env)->NewStringUTF (env, "r"));
      java_cache.a_asset_fd.mode_overwrite = (*env)->NewGlobalRef (env, (*env)->NewStringUTF (env, "wt"));

      POPULATE_CLASS (a_documents_contract, "android/provider/DocumentsContract")
      POPULATE_STATIC_METHOD (a_documents_contract, get_document_id, "getDocumentId", "(Landroid/net/Uri;)Ljava/lang/String;");
      POPULATE_STATIC_METHOD (a_documents_contract, get_tree_document_id, "getTreeDocumentId", "(Landroid/net/Uri;)Ljava/lang/String;");
      POPULATE_STATIC_METHOD (a_documents_contract, build_children_from_tree, "buildChildDocumentsUriUsingTree", "(Landroid/net/Uri;Ljava/lang/String;)Landroid/net/Uri;");
      POPULATE_STATIC_METHOD (a_documents_contract, build_document_from_tree, "buildDocumentUriUsingTree", "(Landroid/net/Uri;Ljava/lang/String;)Landroid/net/Uri;");
      POPULATE_STATIC_METHOD (a_documents_contract, copy_document, "copyDocument", "(Landroid/content/ContentResolver;Landroid/net/Uri;Landroid/net/Uri;)Landroid/net/Uri;");
      POPULATE_STATIC_METHOD (a_documents_contract, create_document, "createDocument", "(Landroid/content/ContentResolver;Landroid/net/Uri;Ljava/lang/String;Ljava/lang/String;)Landroid/net/Uri;");
      POPULATE_STATIC_METHOD (a_documents_contract, delete_document, "deleteDocument", "(Landroid/content/ContentResolver;Landroid/net/Uri;)Z");
      POPULATE_STATIC_METHOD (a_documents_contract, is_child_document, "isChildDocument", "(Landroid/content/ContentResolver;Landroid/net/Uri;Landroid/net/Uri;)Z");
      POPULATE_STATIC_METHOD (a_documents_contract, is_document, "isDocumentUri", "(Landroid/content/Context;Landroid/net/Uri;)Z");
      POPULATE_STATIC_METHOD (a_documents_contract, is_tree, "isTreeUri", "(Landroid/net/Uri;)Z");
      POPULATE_STATIC_METHOD (a_documents_contract, rename_document, "renameDocument", "(Landroid/content/ContentResolver;Landroid/net/Uri;Ljava/lang/String;)Landroid/net/Uri;");

      POPULATE_CLASS (a_documents_contract_document, "android/provider/DocumentsContract$Document")
      POPULATE_STRING (a_documents_contract_document, column_document_id, "COLUMN_DOCUMENT_ID")
      POPULATE_STRING (a_documents_contract_document, column_display_name, "COLUMN_DISPLAY_NAME")
      POPULATE_STRING (a_documents_contract_document, column_flags, "COLUMN_FLAGS")
      POPULATE_STRING (a_documents_contract_document, column_icon, "COLUMN_ICON")
      POPULATE_STRING (a_documents_contract_document, column_last_modified, "COLUMN_LAST_MODIFIED")
      POPULATE_STRING (a_documents_contract_document, column_mime_type, "COLUMN_MIME_TYPE")
      POPULATE_STRING (a_documents_contract_document, column_size, "COLUMN_SIZE")
      POPULATE_STRING (a_documents_contract_document, column_summary, "COLUMN_SUMMARY")
      POPULATE_FIELD (a_documents_contract_document, flag_dir_supports_create, "FLAG_DIR_SUPPORTS_CREATE")
      POPULATE_FIELD (a_documents_contract_document, flag_supports_copy, "FLAG_SUPPORTS_COPY")
      POPULATE_FIELD (a_documents_contract_document, flag_supports_delete, "FLAG_SUPPORTS_DELETE")
      POPULATE_FIELD (a_documents_contract_document, flag_supports_move, "FLAG_SUPPORTS_MOVE")
      POPULATE_FIELD (a_documents_contract_document, flag_supports_rename, "FLAG_SUPPORTS_RENAME")
      POPULATE_FIELD (a_documents_contract_document, flag_supports_write, "FLAG_SUPPORTS_WRITE")
      POPULATE_FIELD (a_documents_contract_document, flag_virtual_document, "FLAG_VIRTUAL_DOCUMENT")
      POPULATE_STRING (a_documents_contract_document, mime_directory, "MIME_TYPE_DIR")

      POPULATE_CLASS (a_cursor, "android/database/Cursor")
      POPULATE_METHOD (a_cursor, get_int, "getInt", "(I)I")
      POPULATE_METHOD (a_cursor, get_long, "getLong", "(I)J")
      POPULATE_METHOD (a_cursor, get_string, "getString", "(I)Ljava/lang/String;")
      POPULATE_METHOD (a_cursor, is_null, "isNull", "(I)Z")
      POPULATE_METHOD (a_cursor, move_to_next, "moveToNext", "()Z")
      POPULATE_METHOD (a_cursor, close, "close", "()V")

      POPULATE_CLASS (a_uri, "android/net/Uri")
      POPULATE_METHOD (a_uri, get_path, "getPath", "()Ljava/lang/String;")
      POPULATE_METHOD (a_uri, get_scheme, "getScheme", "()Ljava/lang/String;")
      POPULATE_METHOD (a_uri, normalize, "normalizeScheme", "()Landroid/net/Uri;")
      POPULATE_STATIC_METHOD (a_uri, parse, "parse", "(Ljava/lang/String;)Landroid/net/Uri;")

      POPULATE_CLASS (j_urlconnection, "java/net/URLConnection")
      POPULATE_STATIC_METHOD (j_urlconnection, guess_content_type_for_name, "guessContentTypeFromName", "(Ljava/lang/String;)Ljava/lang/String;")
      java_cache.j_urlconnection.mime_binary_data = (*env)->NewGlobalRef (env, (*env)->NewStringUTF (env, "application/octet-stream"));

      POPULATE_CLASS (j_string, "java/lang/String")

      POPULATE_CLASS (j_object, "java/lang/Object")
      POPULATE_METHOD (j_object, equals, "equals", "(Ljava/lang/Object;)Z")
      POPULATE_METHOD (j_object, hash_code, "hashCode", "()I")
      POPULATE_METHOD (j_object, to_string, "toString", "()Ljava/lang/String;")

#undef POPULATE_CLASS
#undef POPULATE_METHOD
#undef POPULATE_STATIC_METHOD
#undef POPULATE_FIELD
#undef POPULATE_STRING

#define POPULATE_FIELD_L(lclass, cclass, cname, jname) {                                \
    jfieldID field = (*env)->GetStaticFieldID (env, lclass, jname, "I");      \
    java_cache.cclass.cname = (*env)->GetStaticIntField (env, lclass, field); \
  }
      jclass a_intent = (*env)->FindClass (env, "android/content/Intent");
      POPULATE_FIELD_L (a_intent, a_intent, flag_grant_read_uri_permission, "FLAG_GRANT_READ_URI_PERMISSION")
      POPULATE_FIELD_L (a_intent, a_intent, flag_grant_write_uri_permission, "FLAG_GRANT_WRITE_URI_PERMISSION")
      (*env)->DeleteLocalRef (env, a_intent);
#undef POPULATE_FIELD_L

      (*env)->PopLocalFrame (env, NULL);
      g_once_init_leave_pointer (&java_cache_initialized, &java_cache);
    }

  return &java_cache;
}

enum
{
  G_ANDROID_CONTENT_PROJECTION_DOCUMENT_ID = 0,
  G_ANDROID_CONTENT_PROJECTION_DISPLAY_NAME,
  G_ANDROID_CONTENT_PROJECTION_FLAGS,
  G_ANDROID_CONTENT_PROJECTION_ICON,
  G_ANDROID_CONTENT_PROJECTION_LAST_MODIFIED,
  G_ANDROID_CONTENT_PROJECTION_MIME_TYPE,
  G_ANDROID_CONTENT_PROJECTION_SIZE,
  G_ANDROID_CONTENT_PROJECTION_SUMMARY
};

#define G_ANDROID_FLAG_TO_INFO(attr, flag)                               \
  if (g_file_attribute_matcher_matches (matcher, (attr)))                  \
    g_file_info_set_attribute_boolean (info,                               \
                                       G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE, \
                                       (qflags & g_android_content_file_get_jcache ()->a_documents_contract_document.flag_##flag) != 0);

static GFileInfo *
g_android_content_file_fileinfo_from_cursor (JNIEnv *env,
                                             const gchar *attributes,
                                             jobject context,
                                             jobject cursor,
                                             jobject uri)
{
  GFileInfo *info = g_file_info_new ();
  GFileAttributeMatcher *matcher = g_file_attribute_matcher_new (attributes);
  (*env)->PushLocalFrame (env, 5);

  jobject filename = (*env)->CallObjectMethod (env, cursor,
                                               g_android_content_file_get_jcache ()->a_cursor.get_string,
                                               G_ANDROID_CONTENT_PROJECTION_DISPLAY_NAME);
  gchar *filename_str = g_java_jstring_to_str (filename, NULL);

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME))
    g_file_info_set_display_name (info, filename_str);
  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_NAME))
    g_file_info_set_name (info, filename_str);
  g_free (filename_str);

  gint qflags = (*env)->CallIntMethod (env, cursor,
                                       g_android_content_file_get_jcache ()->a_cursor.get_int,
                                       G_ANDROID_CONTENT_PROJECTION_FLAGS);

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE)
   && !(*env)->CallBooleanMethod (env, cursor,
                                  g_android_content_file_get_jcache ()->a_cursor.is_null,
                                  G_ANDROID_CONTENT_PROJECTION_MIME_TYPE))
    {
      jobject jmime = (*env)->CallObjectMethod (env, cursor,
                                                g_android_content_file_get_jcache ()->a_cursor.get_string,
                                                G_ANDROID_CONTENT_PROJECTION_MIME_TYPE);
      gchar *mime = g_java_jstring_to_str (jmime, NULL);
      gchar *content = g_content_type_from_mime_type (mime);
      g_file_info_set_content_type (info, content);
      g_free (content);
      g_free (mime);
    }

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_DESCRIPTION)
      && !(*env)->CallBooleanMethod (env, cursor,
                                     g_android_content_file_get_jcache ()->a_cursor.is_null,
                                     G_ANDROID_CONTENT_PROJECTION_SUMMARY))
    {
      jobject jdesc = (*env)->CallObjectMethod (env, cursor,
                                                g_android_content_file_get_jcache ()->a_cursor.get_string,
                                                G_ANDROID_CONTENT_PROJECTION_SUMMARY);
      gchar *desc = g_java_jstring_to_str (jdesc, NULL);
      g_file_info_set_attribute_string (info, G_FILE_ATTRIBUTE_STANDARD_DESCRIPTION, desc);
      g_free (desc);
    }

  /*if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_ICON))
    {
      // not implemented
    }*/

  // I'm going to make the assumption that I can always read the document
  g_file_info_set_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ, TRUE);
  G_ANDROID_FLAG_TO_INFO (G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE, supports_write)

  G_ANDROID_FLAG_TO_INFO (G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE, supports_delete)
  G_ANDROID_FLAG_TO_INFO (G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME, supports_rename)
  G_ANDROID_FLAG_TO_INFO (G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL, virtual_document)

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_SIZE)
      && !(*env)->CallBooleanMethod (env, cursor,
                                     g_android_content_file_get_jcache ()->a_cursor.is_null,
                                     G_ANDROID_CONTENT_PROJECTION_SIZE))
    {
      jlong size = (*env)->CallLongMethod (env, cursor,
                                           g_android_content_file_get_jcache ()->a_cursor.get_long,
                                           G_ANDROID_CONTENT_PROJECTION_SIZE);
      g_file_info_set_size (info, size);
    }

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_TIME_MODIFIED)
      && !(*env)->CallBooleanMethod (env, cursor,
                                     g_android_content_file_get_jcache ()->a_cursor.is_null,
                                     G_ANDROID_CONTENT_PROJECTION_LAST_MODIFIED))
    {
      jlong time = (*env)->CallLongMethod (env, cursor,
                                           g_android_content_file_get_jcache ()->a_cursor.get_long,
                                           G_ANDROID_CONTENT_PROJECTION_LAST_MODIFIED);
      GDateTime *date = g_date_time_new_from_unix_utc_usec (time * 1000);
      g_file_info_set_modification_date_time (info, date);
      g_date_time_unref (date);
    }

  if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_TYPE))
    {
      if ((*env)->CallStaticBooleanMethod (env,
                                           g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                           g_android_content_file_get_jcache ()->a_documents_contract.is_tree,
                                           uri))
        g_file_info_set_file_type (info, G_FILE_TYPE_DIRECTORY);
      else if ((*env)->CallStaticBooleanMethod (env,
                                                g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                g_android_content_file_get_jcache ()->a_documents_contract.is_document,
                                                context, uri))
        g_file_info_set_file_type (info, G_FILE_TYPE_REGULAR);
      else
        g_file_info_set_file_type (info, G_FILE_TYPE_UNKNOWN);
    }

  (*env)->PopLocalFrame (env, NULL);
  g_file_attribute_matcher_unref (matcher);

  return info;
}

typedef struct _GAndroidContentFileEnumeratorClass
{
  GFileEnumeratorClass parent_class;
} GAndroidContentFileEnumeratorClass;

typedef struct _GAndroidContentFileEnumerator
{
  GFileEnumerator parent_instance;

  gchar *attributes;
  jobject context;
  jobject cursor;
  jobject parent_uri;
} GAndroidContentFileEnumerator;

#define G_TYPE_ANDROID_CONTENT_FILE_ENUMERATOR (g_android_content_file_enumerator_get_type ())
GType g_android_content_file_enumerator_get_type (void);

G_DEFINE_TYPE (GAndroidContentFileEnumerator,
               g_android_content_file_enumerator,
               G_TYPE_FILE_ENUMERATOR)

static void
g_android_content_file_enumerator_finalize (GObject *object)
{
  GAndroidContentFileEnumerator *self = (GAndroidContentFileEnumerator *)object;
  g_free (self->attributes);
  JNIEnv *env = g_java_get_env ();
  jobject cursor = self->cursor;
  (*env)->DeleteGlobalRef (env, self->parent_uri);
  (*env)->DeleteGlobalRef (env, self->context);
  G_OBJECT_CLASS (g_android_content_file_enumerator_parent_class)->finalize (object);
  // parent.finalize is calling close, so we have to wait before freeing cursor
  (*env)->DeleteGlobalRef (env, cursor);
}

static gboolean
g_android_content_file_enumerator_close (GFileEnumerator *enumerator,
                                         GCancellable    *cancellable,
                                         GError         **error)
{
  GAndroidContentFileEnumerator *self = (GAndroidContentFileEnumerator *)enumerator;
  JNIEnv *env = g_java_get_env ();
  (*env)->CallVoidMethod (env, self->cursor,
                          g_android_content_file_get_jcache ()->a_cursor.close);
  return TRUE;
}


static GFileInfo *
g_android_content_file_enumerator_next_file (GFileEnumerator *enumerator,
                                             GCancellable    *cancellable,
                                             GError         **error)
{
  GAndroidContentFileEnumerator *self = (GAndroidContentFileEnumerator *)enumerator;
  JNIEnv *env = g_java_get_env ();
  if (!(*env)->CallBooleanMethod (env, self->cursor,
                                  g_android_content_file_get_jcache ()->a_cursor.move_to_next))
    return NULL;

  (*env)->PushLocalFrame (env, 2);

  jobject document_id = (*env)->CallObjectMethod (env, self->cursor,
                                                  g_android_content_file_get_jcache ()->a_cursor.get_string,
                                                  G_ANDROID_CONTENT_PROJECTION_DOCUMENT_ID);
  jobject uri = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                g_android_content_file_get_jcache ()->a_documents_contract.build_document_from_tree,
                                                self->parent_uri, document_id);

  GFileInfo *info = g_android_content_file_fileinfo_from_cursor (env,
                                                                 self->attributes,
                                                                 self->context,
                                                                 self->cursor,
                                                                 uri);

  (*env)->PopLocalFrame (env, NULL);
  return info;
}

static void
g_android_content_file_enumerator_class_init (GAndroidContentFileEnumeratorClass *klass)
{
  GObjectClass *object_class = (GObjectClass *)klass;
  GFileEnumeratorClass *enumerator_class = (GFileEnumeratorClass *)klass;

  object_class->finalize = g_android_content_file_enumerator_finalize;
  enumerator_class->close_fn = g_android_content_file_enumerator_close;
  enumerator_class->next_file = g_android_content_file_enumerator_next_file;
}

static void
g_android_content_file_enumerator_init (GAndroidContentFileEnumerator *self)
{}

static GFileEnumerator *
g_android_content_file_enumerator_create (const gchar *attributes,
                                          jobject      context,
                                          jobject      cursor,
                                          jobject      parent_uri)
{
  JNIEnv *env = g_java_get_env ();
  GAndroidContentFileEnumerator *self = g_object_new (G_TYPE_ANDROID_CONTENT_FILE_ENUMERATOR, NULL);
  self->attributes = g_strdup (attributes);
  self->context = (*env)->NewGlobalRef (env, context);
  self->cursor = (*env)->NewGlobalRef (env, cursor);
  self->parent_uri = (*env)->NewGlobalRef (env, parent_uri);
  return (GFileEnumerator *)self;
}

/// begin ContentFile

struct _GAndroidContentFileClass
{
  GObjectClass parent_class;
};

struct _GAndroidContentFile
{
  GObject parent_instance;
  jobjectArray query_projection;
  jobjectArray full_projection;

  jobject context;

  gboolean is_document;
  jobject uri;
  jstring child_name; // if this is set, uri refers to the parent of the file
};

/**
 * GAndroidContentFile:
 *
 * Adapted [iface@Gio.File] interface to interact with `content://` URIs
 * from the
 * [ContentProvider](https://developer.android.com/guide/topics/providers/content-provider-basics)
 * system of Android.
 *
 * As the
 * [SAF/DocumentProvider](https://developer.android.com/guide/topics/providers/document-provider)
 * interface is more restrictive than is commonly expected from a
 * "normal" filesystem, some methods do not work.
 *
 * Since: 2.86
 */
static void g_android_content_file_iface_init (GFileIface *iface);
G_DEFINE_TYPE_WITH_CODE (GAndroidContentFile, g_android_content_file, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_FILE, g_android_content_file_iface_init))

static void
g_android_content_file_finalize (GObject *object)
{
  GAndroidContentFile *self = (GAndroidContentFile *)object;
  JNIEnv *env = g_java_get_env ();
  if (self->child_name)
    (*env)->DeleteGlobalRef (env, self->child_name);
  if (self->uri)
    (*env)->DeleteGlobalRef (env, self->uri);
  (*env)->DeleteGlobalRef (env, self->context);
  (*env)->DeleteGlobalRef (env, self->query_projection);
  (*env)->DeleteGlobalRef (env, self->full_projection);
  G_OBJECT_CLASS (g_android_content_file_parent_class)->finalize (object);
}

static void
g_android_content_file_class_init (GAndroidContentFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = g_android_content_file_finalize;
}

static void
g_android_content_file_init (GAndroidContentFile *self)
{
  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 2);

  jobjectArray query_projection = (*env)->NewObjectArray (env, 2,
                                                          g_android_content_file_get_jcache ()->j_string.klass,
                                                          NULL);
  (*env)->SetObjectArrayElement (env, query_projection, G_ANDROID_CONTENT_PROJECTION_DOCUMENT_ID,
                                 g_android_content_file_get_jcache ()->a_documents_contract_document.column_document_id);
  (*env)->SetObjectArrayElement (env, query_projection, G_ANDROID_CONTENT_PROJECTION_DISPLAY_NAME,
                                 g_android_content_file_get_jcache ()->a_documents_contract_document.column_display_name);
  self->query_projection = (*env)->NewGlobalRef (env, query_projection);

  jobjectArray full_projection = (*env)->NewObjectArray (env, 8,
                                                         g_android_content_file_get_jcache ()->j_string.klass,
                                                         NULL);
  (*env)->SetObjectArrayElement (env, full_projection, G_ANDROID_CONTENT_PROJECTION_DOCUMENT_ID,
                                 g_android_content_file_get_jcache ()->a_documents_contract_document.column_document_id);
  (*env)->SetObjectArrayElement (env, full_projection, G_ANDROID_CONTENT_PROJECTION_DISPLAY_NAME,
                                 g_android_content_file_get_jcache ()->a_documents_contract_document.column_display_name);
  (*env)->SetObjectArrayElement (env, full_projection, G_ANDROID_CONTENT_PROJECTION_FLAGS,
                                 g_android_content_file_get_jcache ()->a_documents_contract_document.column_flags);
  (*env)->SetObjectArrayElement (env, full_projection, G_ANDROID_CONTENT_PROJECTION_ICON,
                                 g_android_content_file_get_jcache ()->a_documents_contract_document.column_icon);
  (*env)->SetObjectArrayElement (env, full_projection, G_ANDROID_CONTENT_PROJECTION_LAST_MODIFIED,
                                 g_android_content_file_get_jcache ()->a_documents_contract_document.column_last_modified);
  (*env)->SetObjectArrayElement (env, full_projection, G_ANDROID_CONTENT_PROJECTION_MIME_TYPE,
                                 g_android_content_file_get_jcache ()->a_documents_contract_document.column_mime_type);
  (*env)->SetObjectArrayElement (env, full_projection, G_ANDROID_CONTENT_PROJECTION_SIZE,
                                 g_android_content_file_get_jcache ()->a_documents_contract_document.column_size);
  (*env)->SetObjectArrayElement (env, full_projection, G_ANDROID_CONTENT_PROJECTION_SUMMARY,
                                 g_android_content_file_get_jcache ()->a_documents_contract_document.column_summary);
  self->full_projection = (*env)->NewGlobalRef (env, full_projection);

  (*env)->PopLocalFrame (env, NULL);
}

static gboolean
g_android_content_file_make_valid (GAndroidContentFile *self, GError **error)
{
  if (!self->is_document || self->child_name == NULL)
    return TRUE;

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 7);
  jobject resolver = (*env)->CallObjectMethod (env, self->context,
                                               g_android_content_file_get_jcache ()->a_context.get_content_resolver);

  jobject parent_document_id = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                               g_android_content_file_get_jcache ()->a_documents_contract.get_document_id,
                                                               self->uri);
  jobject children_uri = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                         g_android_content_file_get_jcache ()->a_documents_contract.build_children_from_tree,
                                                         self->uri, parent_document_id);

  jobject cursor = (*env)->CallObjectMethod (env, resolver,
                                             g_android_content_file_get_jcache ()->a_content_resolver.query,
                                             children_uri,
                                             self->query_projection, NULL, NULL, NULL);
  if (g_java_file_stream_have_io_exception (env, error))
    {
      (*env)->PopLocalFrame (env, NULL);
      return FALSE;
    }

  while ((*env)->CallBooleanMethod (env, cursor,
                                      g_android_content_file_get_jcache ()->a_cursor.move_to_next))
    {
      jobject filename = (*env)->CallObjectMethod (env, cursor,
                                                   g_android_content_file_get_jcache ()->a_cursor.get_string,
                                                   G_ANDROID_CONTENT_PROJECTION_DISPLAY_NAME);
      if ((*env)->CallBooleanMethod (env, self->child_name,
                                     g_android_content_file_get_jcache ()->j_object.equals,
                                     filename))
        {
          jobject document_id = (*env)->CallObjectMethod (env, cursor,
                                                          g_android_content_file_get_jcache ()->a_cursor.get_string,
                                                          G_ANDROID_CONTENT_PROJECTION_DOCUMENT_ID);
          jobject uri = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                        g_android_content_file_get_jcache ()->a_documents_contract.build_document_from_tree,
                                                        self->uri, document_id);

          (*env)->DeleteGlobalRef (env, self->uri);
          self->uri = (*env)->NewGlobalRef (env, uri);

          (*env)->DeleteGlobalRef (env, self->child_name);
          self->child_name = NULL;

          (*env)->CallVoidMethod (env, cursor,
                                  g_android_content_file_get_jcache ()->a_cursor.close);
          (*env)->PopLocalFrame (env, NULL);
          return TRUE;
        }

      (*env)->DeleteLocalRef (env, filename);
    }

  (*env)->CallVoidMethod (env, cursor,
                          g_android_content_file_get_jcache ()->a_cursor.close);
  (*env)->PopLocalFrame (env, NULL);

  if (error)
    {
      gchar *child_name = g_java_jstring_to_str (self->child_name, NULL);
      *error = g_error_new (G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "File \"%s\" was not found in directory", child_name);
      g_free (child_name);
    }
  return FALSE;
}

static jobject
g_android_content_file_open_descriptor (GAndroidContentFile *self,
                                        jstring mode,
                                        GCancellable *cancellable,
                                        GError **error)
{
  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 3);

  jobject resolver = (*env)->CallObjectMethod (env, self->context,
                                               g_android_content_file_get_jcache ()->a_context.get_content_resolver);
  jobject descriptor = (*env)->CallObjectMethod (env, resolver,
                                                 g_android_content_file_get_jcache ()->a_content_resolver.open_asset_fd,
                                                 self->uri, mode, NULL);

  if (g_java_file_stream_have_io_exception (env, error))
    {
      (*env)->PopLocalFrame (env, NULL);
      return NULL;
    }

  return (*env)->PopLocalFrame (env, descriptor);
}

static GFileOutputStream *
g_android_content_file_append_to (GFile *file,
                                  GFileCreateFlags flags,
                                  GCancellable *cancellable,
                                  GError **error)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  if (!g_android_content_file_make_valid (self, error))
    return NULL;

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 2);

  jobject fd = g_android_content_file_open_descriptor (self,
                                                       g_android_content_file_get_jcache ()->a_asset_fd.mode_append,
                                                       NULL,
                                                       error);
  if (!fd)
    goto err;

  jobject ostream = (*env)->CallObjectMethod (env, fd,
                                              g_android_content_file_get_jcache ()->a_asset_fd.create_ostream);
  if (g_java_file_stream_have_io_exception (env, error))
    goto err;

  GFileOutputStream *stream = g_java_file_output_stream_new_for_java (env, ostream);
  (*env)->PopLocalFrame (env, NULL);
  return stream;
err:
  (*env)->PopLocalFrame (env, NULL);
  return NULL;
}

static gboolean
g_android_content_file_copy (GFile *file,
                             GFile *destination,
                             GFileCopyFlags flags,
                             GCancellable *cancellable,
                             GFileProgressCallback callback,
                             gpointer callback_data,
                             GError **error)
{
  g_return_val_if_fail (G_IS_ANDROID_CONTENT_FILE (file), FALSE);
  if (!G_IS_ANDROID_CONTENT_FILE (destination))
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Cannot copy content file into %s", G_OBJECT_TYPE_NAME (destination));
      return FALSE;
    }
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  GAndroidContentFile *dest = (GAndroidContentFile *)destination;
  if (!g_android_content_file_make_valid (self, error))
    return FALSE;

  if (self->child_name)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "Source does not exist");
      return FALSE;
    }

  JNIEnv *env = g_java_get_env ();
  if (dest->child_name)
    {
      if (self->is_document)
        {
          (*env)->PushLocalFrame (env, 3);

          jobject resolver = (*env)->CallObjectMethod (env, self->context,
                                                       g_android_content_file_get_jcache ()->a_context.get_content_resolver);
          jobject uri = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                        g_android_content_file_get_jcache ()->a_documents_contract.copy_document,
                                                        resolver, self->uri, dest->uri);
          if (g_java_file_stream_have_io_exception (env, error))
            {
              (*env)->PopLocalFrame (env, NULL);
              return FALSE;
            }
          uri = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                g_android_content_file_get_jcache ()->a_documents_contract.rename_document,
                                                uri, dest->child_name);
          if (g_java_file_stream_have_io_exception (env, error))
            {
              (*env)->PopLocalFrame (env, NULL);
              return FALSE;
            }

          (*env)->DeleteGlobalRef (env, dest->uri);
          dest->uri = (*env)->NewGlobalRef (env, uri);
          (*env)->DeleteGlobalRef (env, dest->child_name);
          dest->child_name = NULL;

          (*env)->PopLocalFrame (env, NULL);
          return TRUE;
        }
      else
        goto manual;
    }
  else if (flags & G_FILE_COPY_OVERWRITE)
manual:
    {
      GFileInputStream *istream = g_file_read (file, cancellable, error);
      if (!istream)
        return FALSE;
      GFileOutputStream *ostream = g_file_replace (file,
                                                   NULL,
                                                   FALSE,
                                                   G_FILE_CREATE_REPLACE_DESTINATION,
                                                   cancellable,
                                                   error);
      if (!ostream)
        {
          g_object_unref (istream);
          return FALSE;
        }

      if (g_output_stream_splice ((GOutputStream *)ostream,
                                  (GInputStream *)istream,
                                  G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
                                  cancellable,
                                  error) < 0)
        {
          g_object_unref (istream);
          g_object_unref (ostream);
          return FALSE;
        }

      g_object_unref (istream);
      g_object_unref (ostream);
      return TRUE;
    }
  else
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_EXISTS, "Copy destination already exist");
      return FALSE;
    }
}

static GFileOutputStream *
g_android_content_file_create (GFile *file,
                                 GFileCreateFlags flags,
                                 GCancellable *cancellable,
                                 GError **error)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  if (!self->is_document)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Operation not supported");
      return NULL;
    }

  if (!self->child_name)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_EXISTS, "File already exists");
      return NULL;
    }

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 3);

  jobject resolver = (*env)->CallObjectMethod (env, self->context,
                                               g_android_content_file_get_jcache ()->a_context.get_content_resolver);
  jstring mime = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->j_urlconnection.klass,
                                                 g_android_content_file_get_jcache ()->j_urlconnection.guess_content_type_for_name,
                                                 self->child_name);
  if (!mime)
    mime = g_android_content_file_get_jcache ()->j_urlconnection.mime_binary_data;
  jobject uri = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                g_android_content_file_get_jcache ()->a_documents_contract.create_document,
                                                resolver, self->uri, mime, self->child_name);

  if (g_java_file_stream_have_io_exception (env, error))
    {
      (*env)->PopLocalFrame (env, NULL);
      return NULL;
    }

  (*env)->DeleteGlobalRef (env, self->uri);
  self->uri = (*env)->NewGlobalRef (env, uri);
  (*env)->DeleteGlobalRef (env, self->child_name);
  self->child_name = NULL;

  (*env)->PopLocalFrame (env, NULL);

  return g_file_replace (file, NULL, FALSE, flags, cancellable, error);
}

static gboolean
g_android_content_file_delete_file (GFile *file,
                                    GCancellable *cancellable,
                                    GError **error)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  if (!g_android_content_file_make_valid (self, error))
    return FALSE;

  if (!self->is_document)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Operation not supported");
      return FALSE;
    }

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 1);
  jobject resolver = (*env)->CallObjectMethod (env, self->context,
                                               g_android_content_file_get_jcache ()->a_context.get_content_resolver);
  jboolean success = (*env)->CallStaticBooleanMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                      g_android_content_file_get_jcache ()->a_documents_contract.delete_document,
                                                      resolver, self->uri);
  if (g_java_file_stream_have_io_exception (env, error))
    {
      (*env)->PopLocalFrame (env, NULL);
      return FALSE;
    }

  (*env)->PopLocalFrame (env, NULL);
  return success;
}

static GFile *
g_android_content_file_dup (GFile *file)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  JNIEnv *env = g_java_get_env ();

  GAndroidContentFile *copy = g_object_new (G_TYPE_ANDROID_CONTENT_FILE, NULL);
  copy->context = (*env)->NewGlobalRef (env, self->context);
  copy->is_document = self->is_document;
  copy->uri = (*env)->NewGlobalRef (env, self->uri);
  copy->child_name = self->child_name ? (*env)->NewGlobalRef (env, self->child_name) : NULL;
  return (GFile *)copy;
}

static GFileEnumerator *
g_android_content_file_enumerate_children (GFile *file,
                                           const gchar *attributes,
                                           GFileQueryInfoFlags flags,
                                           GCancellable *cancellable,
                                           GError **error)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  if (!g_android_content_file_make_valid (self, error))
    return NULL;

  if (!self->is_document)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Operation not supported");
      return NULL;
    }

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 4);
  jobject resolver = (*env)->CallObjectMethod (env, self->context,
                                               g_android_content_file_get_jcache ()->a_context.get_content_resolver);

  jobject parent_document_id = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                               g_android_content_file_get_jcache ()->a_documents_contract.get_document_id,
                                                               self->uri);

  jobject children_uri = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                         g_android_content_file_get_jcache ()->a_documents_contract.build_children_from_tree,
                                                         self->uri, parent_document_id);

  jobject cursor = (*env)->CallObjectMethod (env, resolver,
                                             g_android_content_file_get_jcache ()->a_content_resolver.query,
                                             children_uri,
                                             self->full_projection, NULL, NULL, NULL);
  if (g_java_file_stream_have_io_exception (env, error))
    {
      (*env)->PopLocalFrame (env, NULL);
      return FALSE;
    }

  GFileEnumerator *enumerator = g_android_content_file_enumerator_create (attributes,
                                                                          self->context,
                                                                          cursor,
                                                                          self->uri);
  (*env)->PopLocalFrame (env, NULL);
  return enumerator;
}

static gboolean
g_android_content_file_equal (GFile *lhsf, GFile *rhsf)
{
  if (lhsf == rhsf)
    return TRUE;
  if (!G_IS_ANDROID_CONTENT_FILE (lhsf) || !G_IS_ANDROID_CONTENT_FILE (rhsf))
    return FALSE;

  GAndroidContentFile *lhs = (GAndroidContentFile *)lhsf;
  GAndroidContentFile *rhs = (GAndroidContentFile *)rhsf;
  g_android_content_file_make_valid (lhs, NULL);
  g_android_content_file_make_valid (rhs, NULL);

  JNIEnv *env = g_java_get_env ();

  if (lhs->is_document != rhs->is_document)
    return FALSE;

  if (!(*env)->CallBooleanMethod (env, lhs->uri,
                                  g_android_content_file_get_jcache ()->j_object.equals,
                                  rhs->uri))
    return FALSE;

  if (lhs->child_name == NULL && rhs->child_name == NULL)
    return TRUE;

  // as above, at least one is not NULL, so if one is NULL they can't be equal
  if (lhs->child_name == NULL || rhs->child_name == NULL)
    return FALSE;

  return (*env)->CallBooleanMethod (env, lhs->child_name,
                                    g_android_content_file_get_jcache ()->j_object.equals,
                                    rhs->child_name);
}

static gchar *
g_android_content_file_get_basename (GFile *file)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  if (!self->is_document)
    {
      gchar *path = g_file_get_path (file);
      gchar *basename = g_path_get_basename (path);
      g_free (path);
      return basename;
    }

  if (self->child_name)
    return g_java_jstring_to_str (self->child_name, NULL);

  GFileInfo *info = g_file_query_info (file,
                                       G_FILE_ATTRIBUTE_STANDARD_NAME,
                                       G_FILE_QUERY_INFO_NONE,
                                       NULL, NULL);
  g_return_val_if_fail (info, NULL);
  gchar *basename = g_strdup (g_file_info_get_name (info));
  g_object_unref (info);
  return basename;
}

static GFile *
g_android_content_file_get_child_for_displayname (GFile *file,
                                                  const gchar *display_name,
                                                  GError **error)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  if (!g_android_content_file_make_valid (self, error))
    return NULL;

  if (!self->is_document)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Operation not supported");
      return NULL;
    }

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 1);

  GAndroidContentFile *child = g_object_new (G_TYPE_ANDROID_CONTENT_FILE, NULL);
  child->context = (*env)->NewGlobalRef (env, self->context);
  child->is_document = TRUE;
  child->uri = (*env)->NewGlobalRef (env, self->uri);

  jobject child_name = g_java_str_to_jstring (display_name);
  child->child_name = (*env)->NewGlobalRef (env, child_name);

  // If the file already exists, normalize. (what about error condition?)
  g_android_content_file_make_valid (child, NULL);

  (*env)->PopLocalFrame (env, NULL);
  return (GFile *)child;
}

static GFile *
g_android_content_file_get_parent (GFile *file)
{
  // You do not have access to parent directories
  return NULL;
}

static gchar *
g_android_content_file_get_path (GFile *file)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  g_return_val_if_fail (g_android_content_file_make_valid (self, NULL), NULL);

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 1);
  jstring path_string = (*env)->CallObjectMethod (env, self->uri,
                                                  g_android_content_file_get_jcache ()->a_uri.get_path);
  gchar *path = g_java_jstring_to_str (path_string, NULL);
  (*env)->PopLocalFrame (env, NULL);
  return path;
}

static gchar *
g_android_content_file_get_uri (GFile *file)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  g_return_val_if_fail (g_android_content_file_make_valid (self, NULL), NULL);

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 1);
  jstring uri_string = (*env)->CallObjectMethod (env, self->uri,
                                                 g_android_content_file_get_jcache ()->j_object.to_string);
  gchar *uri = g_java_jstring_to_str (uri_string, NULL);
  (*env)->PopLocalFrame (env, NULL);
  return uri;
}

static gchar *
g_android_content_file_get_uri_scheme (GFile *file)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  g_return_val_if_fail (g_android_content_file_make_valid (self, NULL), NULL);

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 1);
  jstring scheme_string = (*env)->CallObjectMethod (env, self->uri,
                                                 g_android_content_file_get_jcache ()->a_uri.get_scheme);
  gchar *scheme = g_java_jstring_to_str (scheme_string, NULL);
  (*env)->PopLocalFrame (env, NULL);
  return scheme;
}

static gboolean
g_android_content_file_has_uri_scheme (GFile *file, const gchar *scheme)
{
  gchar* actual_scheme = g_file_get_uri_scheme (file);
  gboolean success = g_strcmp0 (scheme, actual_scheme) == 0;
  g_free (actual_scheme);
  return success;
}

static guint
g_android_content_file_hash (GFile *file)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  g_android_content_file_make_valid (self, NULL);

  JNIEnv *env = g_java_get_env ();
  guint hash = (guint)(*env)->CallIntMethod (env, self->uri,
                                             g_android_content_file_get_jcache ()->j_object.hash_code);
  if (self->child_name)
    hash ^= (guint)(*env)->CallIntMethod (env, self->child_name,
                                          g_android_content_file_get_jcache ()->j_object.hash_code);
  return hash;
}

static gboolean
g_android_content_file_is_native (GFile *file)
{
  // Depending on your definition of "native", this might be a lie; but given
  // the problems of dealing with SAF, it's probably better to tell users theat
  // these files are not "native".
  return FALSE;
}

static gboolean
g_android_content_file_make_directory (GFile *file,
                                       GCancellable *cancellable,
                                       GError **error)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  if (!self->is_document)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Operation not supported");
      return FALSE;
    }
  if (!self->child_name)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_EXISTS, "Directory already exists");
      return FALSE;
    }

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 2);

  jobject resolver = (*env)->CallObjectMethod (env, self->context,
                                               g_android_content_file_get_jcache ()->a_context.get_content_resolver);

  jobject uri = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                g_android_content_file_get_jcache ()->a_documents_contract.create_document,
                                                resolver,
                                                self->uri,
                                                g_android_content_file_get_jcache ()->a_documents_contract_document.mime_directory,
                                                self->child_name);

  if (g_java_file_stream_have_io_exception (env, error))
    {
      (*env)->PopLocalFrame (env, NULL);
      return FALSE;
    }

  (*env)->DeleteGlobalRef (env, self->uri);
  self->uri = (*env)->NewGlobalRef (env, uri);
  (*env)->DeleteGlobalRef (env, self->child_name);
  self->child_name = NULL;

  (*env)->PopLocalFrame (env, NULL);
  return TRUE;
}

static GFileMonitor *
g_android_content_file_monitor_file (GFile *file,
                                     GFileMonitorFlags flags,
                                     GCancellable *cancellable,
                                     GError **error)
{
  if (error)
    *error = g_error_new (G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Unimplemented");
  return NULL;
}

static gboolean
g_android_content_file_move (GFile *file,
                             GFile *destination,
                             GFileCopyFlags flags,
                             GCancellable *cancellable,
                             GFileProgressCallback callback,
                             gpointer callback_data,
                             GError **error)
{
  if (error)
    *error = g_error_new (G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Unimplemented");
  return FALSE;
}

static gboolean
g_android_content_file_prefix_matches (GFile *prefixf,
                                       GFile *filef)
{
  if (!G_IS_ANDROID_CONTENT_FILE(prefixf) || !G_IS_ANDROID_CONTENT_FILE(filef))
    return FALSE;
  GAndroidContentFile *prefix = (GAndroidContentFile *)prefixf;
  GAndroidContentFile *file = (GAndroidContentFile *)filef;
  g_android_content_file_make_valid (prefix, NULL);

  if (!prefix->is_document || !file->is_document)
      return FALSE;

  if (prefix->child_name)
    return FALSE; // if prefix does not exist, it cant be a prefix

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 1);
  jobject resolver = (*env)->CallObjectMethod (env, file->context,
                                               g_android_content_file_get_jcache ()->a_context.get_content_resolver);
  // it doesn't matter whether file has been created or not, given either it or its
  // paren will still have the same prefix.
  jboolean is_child = (*env)->CallStaticBooleanMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                       g_android_content_file_get_jcache ()->a_documents_contract.is_child_document,
                                                       resolver,
                                                       prefix->uri, file->uri);
  if (g_java_file_stream_have_io_exception (env, NULL))
    {
      (*env)->PopLocalFrame (env, NULL);
      return FALSE;
    }

  (*env)->PopLocalFrame (env, NULL);
  return is_child;
}

static GFileInfo *
g_android_content_file_query_info (GFile *file,
                                   const gchar *attributes,
                                   GFileQueryInfoFlags flags,
                                   GCancellable *cancellable,
                                   GError **error)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  if (!g_android_content_file_make_valid (self, error))
    return NULL;

  if (!self->is_document)
    {
      GFileInfo *info = g_file_info_new ();

      GFileAttributeMatcher *matcher = g_file_attribute_matcher_new (attributes);
      if (g_file_attribute_matcher_matches (matcher, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE))
        {
          JNIEnv *env = g_java_get_env ();
          (*env)->PushLocalFrame (env, 2);
          jobject resolver = (*env)->CallObjectMethod (env, self->context,
                                                       g_android_content_file_get_jcache ()->a_context.get_content_resolver);
          jstring jcontent_type = (*env)->CallObjectMethod (env, resolver,
                                                            g_android_content_file_get_jcache ()->a_content_resolver.get_type,
                                                            self->uri);
          if (jcontent_type)
            {
              gchar *content_type = g_java_jstring_to_str (jcontent_type, NULL);
              g_file_info_set_content_type (info, content_type);
              g_free (content_type);
            }
          (*env)->PopLocalFrame (env, NULL);
        }
      g_file_attribute_matcher_unref (matcher);

      return info;
    }

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 2);
  jobject resolver = (*env)->CallObjectMethod (env, self->context,
                                               g_android_content_file_get_jcache ()->a_context.get_content_resolver);

  jobject cursor = (*env)->CallObjectMethod (env, resolver,
                                             g_android_content_file_get_jcache ()->a_content_resolver.query,
                                             self->uri,
                                             self->full_projection, NULL, NULL, NULL);
  if (g_java_file_stream_have_io_exception (env, error))
    {
      (*env)->PopLocalFrame (env, NULL);
      return FALSE;
    }

  GFileInfo *info = NULL;
  if ((*env)->CallBooleanMethod (env, cursor,
                                    g_android_content_file_get_jcache ()->a_cursor.move_to_next))
    info = g_android_content_file_fileinfo_from_cursor (env, attributes, self->context, cursor, self->uri);
    // all further entries (should they exist) are ignored
  else
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND, "File query did not return any results");

  (*env)->CallVoidMethod (env, cursor,
                          g_android_content_file_get_jcache ()->a_cursor.close);
  (*env)->PopLocalFrame (env, NULL);
  return info;
}

static GFileInputStream *
g_android_content_file_read (GFile *file, GCancellable *cancellable, GError **error)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  if (!g_android_content_file_make_valid (self, error))
    return NULL;

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 2);

  jobject fd = g_android_content_file_open_descriptor (self,
                                                       g_android_content_file_get_jcache ()->a_asset_fd.mode_read,
                                                       NULL,
                                                       error);
  if (!fd)
    goto err;

  jobject istream = (*env)->CallObjectMethod (env, fd,
                                              g_android_content_file_get_jcache ()->a_asset_fd.create_istream);
  if (g_java_file_stream_have_io_exception (env, error))
    goto err;

  GFileInputStream *stream = g_java_file_input_stream_new_for_java (env, istream);
  (*env)->PopLocalFrame (env, NULL);
  return stream;
err:
  (*env)->PopLocalFrame (env, NULL);
  return NULL;
}

static GFileOutputStream *
g_android_content_file_replace (GFile *file,
                                const gchar *etag,
                                gboolean make_backup,
                                GFileCreateFlags flags,
                                GCancellable *cancellable,
                                GError **error)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  if (self->child_name && (flags & G_FILE_CREATE_REPLACE_DESTINATION))
    return g_file_create (file, flags, cancellable, error);

  if (!g_android_content_file_make_valid (self, error))
    return NULL;

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 2);

  jobject fd = g_android_content_file_open_descriptor (self,
                                                       g_android_content_file_get_jcache ()->a_asset_fd.mode_overwrite,
                                                       NULL,
                                                       error);
  if (!fd)
    goto err;

  jobject ostream = (*env)->CallObjectMethod (env, fd,
                                              g_android_content_file_get_jcache ()->a_asset_fd.create_ostream);
  if (g_java_file_stream_have_io_exception (env, error))
    goto err;

  GFileOutputStream *stream = g_java_file_output_stream_new_for_java (env, ostream);
  (*env)->PopLocalFrame (env, NULL);
  return stream;
err:
  (*env)->PopLocalFrame (env, NULL);
  return NULL;
}

static GFile *
g_android_content_file_resolve_relative_path (GFile *file,
                                              const gchar *relative_path)
{
  while (g_str_has_prefix(relative_path, "./"))
    relative_path = &relative_path[2];
  if (!*relative_path)
    return g_object_ref (file);
  if (strstr(relative_path, G_DIR_SEPARATOR_S))
    return NULL;

  return g_file_get_child_for_display_name (file, relative_path, NULL);
}

static GFile *
g_android_content_file_set_display_name (GFile *file,
                                         const gchar *display_name,
                                         GCancellable *cancellable,
                                         GError **error)
{
  GAndroidContentFile *self = (GAndroidContentFile *)file;
  if (!self->is_document)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Operation not supported");
      return NULL;
    }

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 3);
  jobject new_name = g_java_str_to_jstring (display_name);

  if (self->child_name)
    {
      (*env)->DeleteGlobalRef (env, self->child_name);
      self->child_name = (*env)->NewGlobalRef (env, new_name);
    }
  else
    {
      jobject resolver = (*env)->CallObjectMethod (env, self->context,
                                                   g_android_content_file_get_jcache ()->a_context.get_content_resolver);

      jobject new_uri = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_documents_contract.klass,
                                                        g_android_content_file_get_jcache ()->a_documents_contract.rename_document,
                                                        resolver, self->uri, new_name);
      if (g_java_file_stream_have_io_exception (env, error))
        {
          (*env)->PopLocalFrame (env, NULL);
          return NULL;
        }

      (*env)->DeleteGlobalRef (env, self->uri);
      self->uri = (*env)->NewGlobalRef (env, new_uri);
    }

  (*env)->PopLocalFrame (env, NULL);
  return g_object_ref (file);
}

static void
g_android_content_file_iface_init (GFileIface *iface)
{
  iface->append_to = g_android_content_file_append_to;
  iface->copy = g_android_content_file_copy;
  iface->create = g_android_content_file_create;
  iface->delete_file = g_android_content_file_delete_file;
  iface->dup = g_android_content_file_dup;
  iface->enumerate_children = g_android_content_file_enumerate_children;
  iface->equal = g_android_content_file_equal;
  iface->get_basename = g_android_content_file_get_basename;
  iface->get_child_for_display_name = g_android_content_file_get_child_for_displayname;
  iface->get_parse_name = g_android_content_file_get_uri;
  iface->get_parent = g_android_content_file_get_parent;
  iface->get_path = g_android_content_file_get_path;
  iface->get_uri = g_android_content_file_get_uri;
  iface->get_uri_scheme = g_android_content_file_get_uri_scheme;
  iface->has_uri_scheme = g_android_content_file_has_uri_scheme;
  iface->hash = g_android_content_file_hash;
  iface->is_native = g_android_content_file_is_native;
  iface->make_directory = g_android_content_file_make_directory;
  iface->monitor_dir = g_android_content_file_monitor_file;
  iface->monitor_file = g_android_content_file_monitor_file;
  iface->move = g_android_content_file_move;
  iface->prefix_matches = g_android_content_file_prefix_matches;
  iface->query_info = g_android_content_file_query_info;
  iface->read_fn = g_android_content_file_read;
  iface->replace = g_android_content_file_replace;
  iface->resolve_relative_path = g_android_content_file_resolve_relative_path;
  iface->set_display_name = g_android_content_file_set_display_name;
}

/**
 * g_android_content_file_from_uri: (skip)
 * @uri: The `content://` URI
 *
 * Create a new [class@Gio.AndroidContentFile] instance from an Android
 * [URI](https://developer.android.com/reference/android/net/Uri)
 * object.
 *
 * Returns: (transfer full): the newly created file
 * Since: 2.86
 */
GFile *
g_android_content_file_from_uri (jobject uri)
{
  JNIEnv *env = g_java_get_env ();
  g_return_val_if_fail (env != NULL, NULL);
  g_return_val_if_fail ((*env)->IsInstanceOf (env, uri,
                                              g_android_content_file_get_jcache ()->a_uri.klass),
                        NULL);

  g_return_val_if_fail (g_android_get_context () != NULL, NULL);

  GAndroidContentFile *self = g_object_new (G_TYPE_ANDROID_CONTENT_FILE, NULL);
  jobject app_context = (*env)->CallObjectMethod (env, g_android_get_context (),
                                                  g_android_content_file_get_jcache ()->a_context.get_application_context);
  self->context = (*env)->NewGlobalRef (env, app_context);
  self->child_name = NULL;
  (*env)->DeleteLocalRef (env, app_context);

  if ((*env)->CallStaticBooleanMethod (env, g_android_content_file_get_jcache()->a_documents_contract.klass,
                                       g_android_content_file_get_jcache()->a_documents_contract.is_document,
                                       self->context, uri))
    {
      self->is_document = TRUE;
      self->uri = (*env)->NewGlobalRef (env, uri);
    }
  else if ((*env)->CallStaticBooleanMethod (env, g_android_content_file_get_jcache()->a_documents_contract.klass,
                                            g_android_content_file_get_jcache()->a_documents_contract.is_tree,
                                            uri))
    {
      self->is_document = TRUE;
      jstring document_id = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache()->a_documents_contract.klass,
                                                            g_android_content_file_get_jcache()->a_documents_contract.get_tree_document_id,
                                                            uri);
      jobject tree_uri = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache()->a_documents_contract.klass,
                                                         g_android_content_file_get_jcache()->a_documents_contract.build_document_from_tree,
                                                         uri, document_id);
      self->uri = (*env)->NewGlobalRef (env, tree_uri);
    }
  else
    {
      self->is_document = FALSE;
      self->uri = (*env)->NewGlobalRef (env, uri);
    }

  return (GFile *)self;
}

/**
 * g_android_content_file_get_uri_object: (skip)
 * @self: (transfer none): the content file
 *
 * Get the `content://` URI object that is backing @self.
 *
 * Returns: (nullable): the URI backing @self or %NULL if the file doesn't exist
 * Since. 2.86
 */
jobject
g_android_content_file_get_uri_object (GAndroidContentFile *self)
{
  g_return_val_if_fail (G_IS_ANDROID_CONTENT_FILE (self), NULL);
  if (!g_android_content_file_make_valid (self, NULL))
    return NULL;

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 1);
  jstring norm_uri = (*env)->CallObjectMethod (env, self->uri,
                                               g_android_content_file_get_jcache ()->a_uri.normalize);
  return (*env)->PopLocalFrame (env, norm_uri);
}

/**
 * g_android_content_file_persist:
 * @self: (transfer none): the content file
 * @read: persist read permission
 * @write: persist write permission
 *
 * Try to persist permission to use the file for longer than the
 * creating activity or process is alive.
 *
 * Since: 2.86
 */
void
g_android_content_file_persist (GAndroidContentFile *self, gboolean read, gboolean write)
{
  g_return_if_fail (G_IS_ANDROID_CONTENT_FILE (self));
  if (!g_android_content_file_make_valid (self, NULL))
    return;

  JNIEnv *env = g_java_get_env ();
  (*env)->PushLocalFrame (env, 1);
  jobject resolver = (*env)->CallObjectMethod (env, self->context,
                                               g_android_content_file_get_jcache ()->a_context.get_content_resolver);
  jint flags = 0;
  if (read)
    flags |= g_android_content_file_get_jcache ()->a_intent.flag_grant_read_uri_permission;
  if (write)
    flags |= g_android_content_file_get_jcache ()->a_intent.flag_grant_write_uri_permission;
  (*env)->CallVoidMethod (env, resolver,
                          g_android_content_file_get_jcache ()->a_content_resolver.take_persistable_uri_permission,
                          self->uri,
                          flags);
  GError *err = NULL;
  if (g_java_file_stream_have_io_exception (env, &err))
    {
      g_warning ("Failed to persist file: %s", err->message);
      g_error_free (err);
    }
  (*env)->PopLocalFrame (env, NULL);
}

// Android VFS

struct _GAndroidVfsClass
{
  GVfsClass parent_class;
};

struct _GAndroidVfs
{
  GVfs parent_instance;
};

G_DEFINE_TYPE_WITH_CODE (GAndroidVfs, g_android_vfs, G_TYPE_VFS,
                         _g_io_modules_ensure_extension_points_registered ();
			 g_io_extension_point_implement (G_VFS_EXTENSION_POINT_NAME,
							 g_define_type_id,
							 "android",
							 1))


static GFile *
g_android_vfs_get_file_for_path (GVfs       *vfs,
                                 const char *path)
{
  GVfs *local = g_vfs_get_local ();
  return g_vfs_get_file_for_path (local, path);
}

static GFile *
g_android_vfs_get_file_for_uri (GVfs       *vfs,
                                const char *uri)
{
  GJavaScope env = g_java_enter_scope (3);
  jobject juri = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_uri.klass,
                                                 g_android_content_file_get_jcache ()->a_uri.parse,
                                                 g_java_str_to_jstring (uri));
  jobject scheme = (*env)->CallObjectMethod (env, juri,
                                             g_android_content_file_get_jcache ()->a_uri.get_scheme);

  GFile *ret;
  if ((*env)->CallBooleanMethod (env, g_android_content_file_get_jcache ()->a_content_resolver.scheme_content,
                                 g_android_content_file_get_jcache ()->j_object.equals,
                                 scheme))
    {
      ret = g_android_content_file_from_uri (juri);
    }
  else
    {
      GVfs *local = g_vfs_get_local ();
      ret = g_vfs_get_file_for_uri (local, uri);
    }
  g_java_leave_scope (&env);
  return ret;
}

static const gchar * const *
g_android_vfs_get_supported_uri_schemes (GVfs *vfs)
{
  static const gchar * uri_schemes[] = { "content", "file", NULL };
  return uri_schemes;
}

static GFile *
g_android_vfs_parse_name (GVfs       *vfs,
                          const char *parse_name)
{
  GJavaScope env = g_java_enter_scope (3);
  jobject juri = (*env)->CallStaticObjectMethod (env, g_android_content_file_get_jcache ()->a_uri.klass,
                                                 g_android_content_file_get_jcache ()->a_uri.parse,
                                                 g_java_str_to_jstring (parse_name));
  jobject scheme = (*env)->CallObjectMethod (env, juri,
                                             g_android_content_file_get_jcache ()->a_uri.get_scheme);

  GFile *ret;
  if ((*env)->CallBooleanMethod (env, g_android_content_file_get_jcache ()->a_content_resolver.scheme_content,
                                 g_android_content_file_get_jcache ()->j_object.equals,
                                 scheme))
    {
      ret = g_android_content_file_from_uri (juri);
    }
  else
    {
      GVfs *local = g_vfs_get_local ();
      ret = g_vfs_parse_name (local, parse_name);
    }
  g_java_leave_scope (&env);
  return ret;
}

static gboolean
g_android_vfs_is_active (GVfs *vfs)
{
  return g_android_get_context () != NULL;
}

static void
g_android_vfs_class_init (GAndroidVfsClass *klass)
{
  GVfsClass *vfs_class = (GVfsClass *)klass;

  vfs_class->is_active = g_android_vfs_is_active;
  vfs_class->get_file_for_path = g_android_vfs_get_file_for_path;
  vfs_class->get_file_for_uri = g_android_vfs_get_file_for_uri;
  vfs_class->get_supported_uri_schemes = g_android_vfs_get_supported_uri_schemes;
  vfs_class->parse_name = g_android_vfs_parse_name;
}

static void
g_android_vfs_init (GAndroidVfs *self)
{}

