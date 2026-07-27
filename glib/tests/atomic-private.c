/*
 * Copyright © 2026 Luca Bacci
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://gnu.org/licenses/>.
 */

#include <glib.h>

#include "gatomicprivate.h"

#include <stdint.h>

static void
test_types (void)
{
  const int *csp;
  const int * const *cspp;
  unsigned int u, u2;
  int s, s2;
  void *vp, *vp2;
  int *ip, *ip2;
  uintptr_t gu, gu2;

  csp = &s;
  cspp = &csp;

  /* relaxed */

  g_atomic_int_set_relaxed ((int *) &u, 5);
  u2 = (guint) g_atomic_int_get_relaxed ((int *) &u);
  g_assert_cmpuint (u2, ==, 5);

  g_atomic_int_set_relaxed (&s, 5);
  s2 = g_atomic_int_get_relaxed (&s);
  g_assert_cmpint (s2, ==, 5);

  g_atomic_pointer_set_relaxed (&vp, 0);
  vp2 = g_atomic_pointer_get_relaxed (&vp);
  g_assert_true (vp2 == 0);

  g_atomic_pointer_set_relaxed (&ip, 0);
  ip2 = g_atomic_pointer_get_relaxed (&ip);
  g_assert_true (ip2 == 0);

  g_atomic_pointer_set_relaxed (&gu, 0);
  vp2 = (gpointer) g_atomic_pointer_get_relaxed (&gu);
  gu2 = (guintptr) vp2;
  g_assert_cmpuint (gu2, ==, 0);

  g_assert_cmpint (g_atomic_int_get_relaxed (csp), ==, s);
  g_assert_true (g_atomic_pointer_get_relaxed ((const int **) cspp) == csp);

  /* acquire / release */

  g_atomic_int_set_release ((int *) &u, 5);
  u2 = (guint) g_atomic_int_get_acquire ((int *) &u);
  g_assert_cmpuint (u2, ==, 5);

  g_atomic_int_set_release (&s, 5);
  s2 = g_atomic_int_get_acquire (&s);
  g_assert_cmpint (s2, ==, 5);

  g_atomic_pointer_set_release (&vp, 0);
  vp2 = g_atomic_pointer_get_acquire (&vp);
  g_assert_true (vp2 == 0);

  g_atomic_pointer_set_release (&ip, 0);
  ip2 = g_atomic_pointer_get_acquire (&ip);
  g_assert_true (ip2 == 0);

  g_atomic_pointer_set_release (&gu, 0);
  vp2 = (gpointer) g_atomic_pointer_get_acquire (&gu);
  gu2 = (guintptr) vp2;
  g_assert_cmpuint (gu2, ==, 0);

  g_assert_cmpint (g_atomic_int_get_acquire (csp), ==, s);
  g_assert_true (g_atomic_pointer_get_acquire ((const int **) cspp) == csp);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/atomic-private/types", test_types);

  return g_test_run ();
}
