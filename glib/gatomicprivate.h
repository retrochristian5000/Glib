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

#ifndef __G_ATOMICPRIVATE_H__
#define __G_ATOMICPRIVATE_H__

#include <glib/gtypes.h>
#include <glib/glib-typeof.h>

G_BEGIN_DECLS

#if defined (__ATOMIC_SEQ_CST)

#define g_atomic_int_get_mem_order(atomic, mem_order) \
  (G_GNUC_EXTENSION ({                                                         \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (int));                        \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                    \
                                                                               \
    __atomic_load_n ((const int *)(atomic), (mem_order));                      \
  }))
#define g_atomic_int_set_mem_order(atomic, newval, mem_order) \
  (G_GNUC_EXTENSION ({                                                         \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (int));                        \
    (void) (0 ? *(atomic) ^ (newval) : 1);                                     \
                                                                               \
    __atomic_store_n ((int *)(atomic), (newval), (mem_order));                 \
  }))
#define g_atomic_pointer_get_mem_order(atomic, mem_order) \
  (G_GNUC_EXTENSION ({                                                         \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (void *));                     \
    (void) (0 ? (void **) (atomic) : NULL);                                    \
                                                                               \
    __atomic_load_n ((atomic), (mem_order));                                   \
  }))
#define g_atomic_pointer_set_mem_order(atomic, newval, mem_order) \
  (G_GNUC_EXTENSION ({                                                         \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (void *));                     \
    (void) (0 ? (void **) (atomic) : NULL);                                    \
                                                                               \
    __atomic_store_n ((atomic), (newval), (mem_order));                        \
  }))

#define g_atomic_int_get_relaxed(atomic)             g_atomic_int_get_mem_order(atomic, __ATOMIC_RELAXED)
#define g_atomic_int_set_relaxed(atomic, newval)     g_atomic_int_set_mem_order(atomic, newval, __ATOMIC_RELAXED)
#define g_atomic_int_get_acquire(atomic)             g_atomic_int_get_mem_order(atomic, __ATOMIC_ACQUIRE)
#define g_atomic_int_set_release(atomic, newval)     g_atomic_int_set_mem_order(atomic, newval, __ATOMIC_RELEASE)
#define g_atomic_pointer_get_relaxed(atomic)         g_atomic_pointer_get_mem_order(atomic, __ATOMIC_RELAXED)
#define g_atomic_pointer_set_relaxed(atomic, newval) g_atomic_pointer_set_mem_order(atomic, newval, __ATOMIC_RELAXED)
#define g_atomic_pointer_get_acquire(atomic)         g_atomic_pointer_get_mem_order(atomic, __ATOMIC_ACQUIRE)
#define g_atomic_pointer_set_release(atomic, newval) g_atomic_pointer_set_mem_order(atomic, newval, __ATOMIC_RELEASE)

#elif defined (__GNUC__) /*  GCC fallback */

#define g_atomic_int_get_relaxed(atomic) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (int));                      \
    (void) (0 ? *(atomic) ^ *(atomic) : 1);                                  \
                                                                             \
    *(volatile int *) (atomic);                                              \
  }))
#define g_atomic_int_set_relaxed(atomic, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (int));                      \
    (void) (0 ? *(atomic) ^ (newval) : 1);                                   \
                                                                             \
    *(volatile int *) (atomic) = (newval);                                   \
  }))
#define g_atomic_pointer_get_relaxed(atomic) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (void *));                   \
    (void) (0 ? (void **) (atomic) : NULL);                                  \
                                                                             \
    *(void * volatile *) (atomic);                                           \
  }))
#define g_atomic_pointer_set_relaxed(atomic, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    G_STATIC_ASSERT (sizeof *(atomic) == sizeof (void *));                   \
    (void) (0 ? (void **) (atomic) : NULL);                                  \
                                                                             \
    *(void * volatile *) (atomic) = (void *) (newval);                       \
  }))

#define g_atomic_int_get_acquire(atomic) \
  (G_GNUC_EXTENSION ({                                                       \
    int glib_priv_result__ = g_atomic_int_get_relaxed ( (atomic) );          \
                                                                             \
    __sync_synchronize ();                                                   \
    __asm__ __volatile__ ("" : : : "memory");                                \
                                                                             \
    glib_priv_result__;                                                      \
  }))
#define g_atomic_int_set_release(atomic, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    __sync_synchronize ();                                                   \
    __asm__ __volatile__ ("" : : : "memory");                                \
                                                                             \
    g_atomic_int_set_relaxed ( (atomic), (newval) );                         \
  }))
#define g_atomic_pointer_get_acquire(atomic) \
  (G_GNUC_EXTENSION ({                                                       \
    void *glib_priv_result__ = g_atomic_pointer_get_relaxed ( (atomic) );    \
                                                                             \
    __sync_synchronize ();                                                   \
    __asm__ __volatile__ ("" : : : "memory");                                \
                                                                             \
    glib_priv_result__;                                                      \
  }))
#define g_atomic_pointer_set_release(atomic, newval) \
  (G_GNUC_EXTENSION ({                                                       \
    __sync_synchronize ();                                                   \
    __asm__ __volatile__ ("" : : : "memory");                                \
                                                                             \
    g_atomic_pointer_set_relaxed ( atomic, (newval) );                       \
  }))

#elif defined (_MSC_VER)

#include <windows.h>
#include <intrin.h>

#if (!defined (_M_IX86) && !defined (_M_AMD64)) || _MSC_VER >= 1920  /* VS2019 */
#define HAVE_ISO_VOLATILE_INTRINSICS
#endif

#if defined (HAVE_ISO_VOLATILE_INTRINSICS)

G_STATIC_ASSERT (sizeof (int) == sizeof (__int32));

static inline int
g_atomic_int_get_relaxed (const int *atomic)
{
  return (int) __iso_volatile_load32 (atomic);
}

static inline void
g_atomic_int_set_relaxed (int *atomic,
                          int  value)
{
  __iso_volatile_store32 (atomic, value);
}

static inline void *
g_atomic_pointer_get_relaxed (const void *atomic)
{
#if GLIB_SIZEOF_VOID_P == 8
  return (void*) (intptr_t) __iso_volatile_load64 (atomic);
#else
  return (void*) (intptr_t) __iso_volatile_load32 (atomic);
#endif
}

static inline void
g_atomic_pointer_set_relaxed (void *atomic,
                              void *value)
{
#if GLIB_SIZEOF_VOID_P == 8
  __iso_volatile_store64 ((__int64 *) atomic, (__int64) (intptr_t) value);
#else
  __iso_volatile_store32 ((__int32 *) atomic, (__int32) (intptr_t) value);
#endif
}

#else /* ! HAVE_ISO_VOLATILE_INTRINSICS */

static inline int
g_atomic_int_get_relaxed (const int *atomic)
{
  int result = *(volatile int *) atomic;

  return result;
}

static inline void
g_atomic_int_set_relaxed (int *atomic,
                          int  value)
{
  *(volatile int *) atomic = value;
}

static inline void *
g_atomic_pointer_get_relaxed (const void *atomic)
{
  void *result = *(void * volatile *) atomic;

  return result;
}

static inline void
g_atomic_pointer_set_relaxed (void *atomic,
                              void *value)
{
  *(void * volatile *) atomic = value;
}

#endif /* ! HAVE_ISO_VOLATILE_INTRINSICS */

static inline int
g_atomic_int_get_acquire (const int *atomic)
{
  int result = g_atomic_int_get_relaxed (atomic);

  MemoryBarrier ();
  _ReadWriteBarrier ();

  return result;
}

static inline void
g_atomic_int_set_release (int *atomic,
                          int  value)
{
  _ReadWriteBarrier ();
  MemoryBarrier ();

  g_atomic_int_set_relaxed (atomic, value);
}

static inline void *
g_atomic_pointer_get_acquire (const void *atomic)
{
  void *result = g_atomic_pointer_get_relaxed (atomic);

  MemoryBarrier ();
  _ReadWriteBarrier ();

  return result;
}

static inline void
g_atomic_pointer_set_release (void *atomic,
                              void *value)
{
  _ReadWriteBarrier ();
  MemoryBarrier ();

  g_atomic_pointer_set_relaxed (atomic, value);
}

#else /* ! __ATOMIC_SEQ_CST ! __GNUC__ ! _MSC_VER */

#message "Please, implement weakly-ordered atomics for this toolchain.  Using seq-cst fallbacks..."

#define g_atomic_int_get_relaxed(atomic)             g_atomic_int_get(atomic)
#define g_atomic_int_set_relaxed(atomic, newval)     g_atomic_int_set(atomic, newval)
#define g_atomic_int_get_acquire(atomic)             g_atomic_int_get(atomic)
#define g_atomic_int_set_release(atomic, newval)     g_atomic_int_set(atomic, newval)
#define g_atomic_pointer_get_relaxed(atomic)         g_atomic_pointer_get(atomic)
#define g_atomic_pointer_set_relaxed(atomic, newval) g_atomic_pointer_set(atomic, newval)
#define g_atomic_pointer_get_acquire(atomic)         g_atomic_pointer_get(atomic)
#define g_atomic_pointer_set_release(atomic, newval) g_atomic_pointer_set(atomic, newval)

#endif /* ! __ATOMIC_SEQ_CST ! __GNUC__ ! _MSC_VER */

#endif /* __G_ATOMICPRIVATE_H__ */
