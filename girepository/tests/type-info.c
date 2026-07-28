/*
 * Copyright 2025 Philip Chimento <philip.chimento@gmail.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "girepository.h"
#include "test-common.h"

static void
test_type_info_name (RepositoryFixture *fx,
                     const void *unused)
{
  GIInterfaceInfo *interface_info = NULL;
  GIVFuncInfo *vfunc;
  GITypeInfo *typeinfo;

  g_test_summary ("Test that gi_base_info_get_name() returns null for GITypeInfo");
  g_test_bug ("https://gitlab.gnome.org/GNOME/gobject-introspection/issues/96");

  interface_info = GI_INTERFACE_INFO (gi_repository_find_by_name (fx->repository, "Gio", "File"));
  g_assert_nonnull (interface_info);
  vfunc = gi_interface_info_find_vfunc (interface_info, "read_async");
  g_assert_nonnull (vfunc);

  typeinfo = gi_callable_info_get_return_type (GI_CALLABLE_INFO (vfunc));
  g_assert_nonnull (typeinfo);

  g_assert_null (gi_base_info_get_name (GI_BASE_INFO (typeinfo)));

  g_clear_pointer (&interface_info, gi_base_info_unref);
  g_clear_pointer (&vfunc, gi_base_info_unref);
  g_clear_pointer (&typeinfo, gi_base_info_unref);
}

static GITypeInfo *
obtain_type_info (RepositoryFixture *fx)
{
  GICallableInfo *func_info;
  GITypeInfo *retval;

  func_info = GI_CALLABLE_INFO (gi_repository_find_by_name (fx->repository, "Gio", "bus_get_sync"));
  g_assert_nonnull (func_info);

  retval = gi_callable_info_get_return_type (func_info);
  g_assert_nonnull (retval);

  g_clear_pointer (&func_info, gi_base_info_unref);
  return retval;
}

static void
test_type_info_serialize_heap (RepositoryFixture *fx,
                               const void *unused)
{
  GITypeInfo *type_info;
  GITypeInfo *heap;
  GIObjectInfo *bus_connection_info;
  GITypelib *typelib;
  GIBaseInfo *container;
  uint8_t buffer[GI_TYPE_INFO_SERIALIZE_BUFFER_LENGTH];

  type_info = obtain_type_info (fx);

  gi_type_info_serialize (type_info, buffer, GI_TYPE_INFO_SERIALIZE_BUFFER_LENGTH);
  typelib = gi_base_info_get_typelib (GI_BASE_INFO (type_info));
  container = gi_base_info_get_container (GI_BASE_INFO (type_info));

  heap = gi_repository_new_type_info_from_bytes (fx->repository, typelib, container, buffer);

  g_assert_true (gi_base_info_equal (GI_BASE_INFO (type_info), GI_BASE_INFO (heap)));

  g_clear_pointer (&type_info, gi_base_info_unref);

  /* Check that several GITypeInfo methods work even after the original
   * GITypeInfo is freed */
  g_assert_true (gi_type_info_is_pointer (heap));
  g_assert_cmpuint (gi_type_info_get_tag (heap), ==, GI_TYPE_TAG_INTERFACE);
  bus_connection_info = GI_OBJECT_INFO (gi_type_info_get_interface (heap));
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (bus_connection_info)), ==, "DBusConnection");
  g_assert_cmpuint (gi_type_info_get_storage_type (heap), ==, GI_TYPE_TAG_INTERFACE);

  g_clear_pointer (&bus_connection_info, gi_base_info_unref);
  g_clear_pointer (&heap, gi_base_info_unref);
}

static void
test_type_info_serialize_stack (RepositoryFixture *fx,
                                const void *unused)
{
  GITypeInfo *type_info;
  GITypeInfo stack;
  GIObjectInfo *bus_connection_info;
  GITypelib *typelib;
  GIBaseInfo *container;
  uint8_t buffer[GI_TYPE_INFO_SERIALIZE_BUFFER_LENGTH];

  type_info = obtain_type_info (fx);

  gi_type_info_serialize (type_info, buffer, GI_TYPE_INFO_SERIALIZE_BUFFER_LENGTH);
  typelib = gi_base_info_get_typelib (GI_BASE_INFO (type_info));
  container = gi_base_info_get_container (GI_BASE_INFO (type_info));

  gi_repository_load_type_info_from_bytes (fx->repository, typelib, container, buffer, &stack);

  g_assert_true (gi_base_info_equal (GI_BASE_INFO (type_info), GI_BASE_INFO (&stack)));

  g_clear_pointer (&type_info, gi_base_info_unref);

  /* Check that several GITypeInfo methods work even after the original
   * GITypeInfo is freed */
  g_assert_true (gi_type_info_is_pointer (&stack));
  g_assert_cmpuint (gi_type_info_get_tag (&stack), ==, GI_TYPE_TAG_INTERFACE);
  bus_connection_info = GI_OBJECT_INFO (gi_type_info_get_interface (&stack));
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (bus_connection_info)), ==, "DBusConnection");
  g_assert_cmpuint (gi_type_info_get_storage_type (&stack), ==, GI_TYPE_TAG_INTERFACE);

  g_clear_pointer (&bus_connection_info, gi_base_info_unref);
  gi_base_info_clear (&stack);
}

int
main (int argc, char **argv)
{
  repository_init (&argc, &argv);

  ADD_REPOSITORY_TEST ("/type-info/name", test_type_info_name, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/type-info/serialize/heap", test_type_info_serialize_heap, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/type-info/serialize/stack", test_type_info_serialize_stack, &typelib_load_spec_gio);

  return g_test_run ();
}
