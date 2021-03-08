/* Copyright (C) 2020  Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of either:

   * the GNU General Public License as published by
     the Free Software Foundation; version 2.

   * the GNU General Public License as published by
     the Free Software Foundation; version 3.

   or both in parallel, as here.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copies of the GNU General Public License,
   version 2 and 3 along with this program;
   if not, see <https://www.gnu.org/licenses/>.  */

#ifndef _DEFINES_H
#define _DEFINES_H 1

#if defined(__GNUC__) && __GNUC__ >= 2
# define __GCC_PREREQ(maj, min) \
  ((__GNUC__ << 16) + __GNUC_MINOR__ >= (maj << 16) + min)
#else
# define __GCC_PREREQ(maj, min) 0
#endif

#if !__GCC_PREREQ (2, 6)
# warning "No attributes exist"
# define __attribute__(attr) /* nothing :( */
#endif

#if !defined(_Noreturn) && __STDC_VERSION__ < 201112L
# if __GCC_PREREQ (2, 8)
#  define __noreturn __attribute__ ((__noreturn__))
# elif _MSC_VER >= 1200
#  define __noreturn __declspec (noreturn)
# endif
#else
# define __noreturn _Noreturn
#endif

#if __GCC_PREREQ (2, 96)
# define __malloc __attribute__ ((__malloc__))
#else
# define __malloc
#endif

#if __GCC_PREREQ (4, 3)
# define __alloc_size(params) __attribute__ ((__alloc_size__ params))
#else
# define __alloc_size(params)
#endif

#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2 || defined(__clang__) || defined(__TINYC__) || defined(__PCC__)
#  define inline __inline__
# else
#  define inline /* Nothing.  */
# endif
#endif

/* Warn about unused results of certain
   function calls which can lead to problems.  */
#if __GCC_PREREQ (3, 4)
# define __warn_unused_result __attribute__ ((__warn_unused_result__))
#else
# define __warn_unused_result
#endif

/* Forces a function to be always inlined.  */
#if __GCC_PREREQ (3, 2) && __STDC_VERSION__ >= 199901L
# define __force_inline inline __attribute__ ((__always_inline__))
#elif defined (_MSC_VER)
# defined __force_inline __forceinline
#else
# define __force_inline
#endif

/* The nonull function attribute allows to mark pointer parameters which
   must not be NULL.  */
#if __GCC_PREREQ (3, 3)
# define __nonnull(params) __attribute__ ((__nonnull__ params))
#else
# define __nonnull(params)
#endif

/*
 * GCC 2.95 provides `__restrict' as an extension to C90 to support the
 * C99-specific `restrict' type qualifier.  We happen to use `__restrict' as
 * a way to define the `restrict' type qualifier without disturbing older
 * software that is unaware of C99 keywords.
 */
#if __STDC_VERSION__ < 199901L
# if __GCC_PREREQ (2, 95)
/* __restrict is known in EGCS 1.2 and above.  */
#  define restrict __restrict
# else
#  define restrict /* Nothing.  */
# endif
#endif

#if __GCC_PREREQ (4, 4)
# define __format_printf(p1, p2) __attribute__ ((__format__ (__gnu_printf__, p1, p2)))
#elif __GCC_PREREQ(2, 5)
# define __format_printf(p1, p2) __attribute__ ((__format__ (__printf__, p1, p2)))
#else
# define __format_printf(p1, p2) /* Nothing.  */
#endif

/* Attribute `returns_nonnull' was valid as of gcc 4.9.  */
#if __GCC_PREREQ (4, 9)
# define __returns_nonnull __attribute__ ((__returns_nonnull__))
#else
# define __returns_nonnull
#endif

/* Used for vararg functions to indicate that last argument must be NULL.  */
#if __GCC_PREREQ (4, 0)
# define __sentinel __attribute__ ((__sentinel__))
#else
# define __sentinel /* Nothing.  */
#endif

/*
 * __builtin_expect(CONDITION, EXPECTED_VALUE) evaluates to CONDITION,
 * but notifies the compiler that the most likely value of
 * CONDITION is EXPECTED_VALUE.
 */
#if __GCC_PREREQ (2, 96)
# define   likely(condition) __builtin_expect ((condition), true)
# define unlikely(condition) __builtin_expect ((condition), false)
#else
# define   likely(condition) (condition)
# define unlikely(condition) (condition)
#endif

#include <limits.h>

#if !__GCC_PREREQ (5, 0)
/* https://stackoverflow.com/a/1815371 */
static inline __wur __force_inline __nonnull ((3)) bool
__builtin_mul_overflow (size_t a, size_t b, size_t *c)
{
# define SIZE_BIT (sizeof (size_t) * CHAR_BIT / 2)
# define HI(x) (x >> SIZE_BIT)
# define LO(x) ((((size_t) 1 << SIZE_BIT) - 1) & x)

  size_t s0, s1, s2, s3, x, result, carry;

  x = LO(a) * LO(b);
  s0 = LO(x);

  x = HI(a) * LO(b) + HI(x);
  s1 = LO(x);
  s2 = HI(x);

  x = s1 + LO(a) * HI(b);
  s1 = LO(x);

  x = s2 + HI(a) * HI(b) + HI(x);
  s2 = LO(x);
  s3 = HI(x);

  result = s1 << SIZE_BIT | s0;
  carry = s3 << SIZE_BIT | s2;

  *c = result;

  return carry != 0;
}
#endif
#define mul_overflow(n, m, s) __builtin_mul_overflow (n, m, s)

/* Bound on length of the string representing an unsigned integer
   value representable in B bits.  log10 (2.0) < 146 / 485.  The
   smallest value of B where this bound is not tight is 2621.  */
#define INT_BITS_STRLEN_BOUND(b) (((b) * 146 + 484) / 485)

/* True if the real type T is signed.  */
#define TYPE_SIGNED(t) (!((t) 0 < (t) -1))

/* Bound on length of the string representing an integer type.
   Subtract 1 for the sign bit if T is signed, and then add 1 more for
   a minus sign if needed.  */
#define INT_STRLEN_BOUND(t) \
	(INT_BITS_STRLEN_BOUND (sizeof (t) * CHAR_BIT) + TYPE_SIGNED (t))

#define streq(s1, s2)     (strcmp ((s1), (s2)) == 0)
#define strneq(s1, s2, n) (strncmp ((s1), (s2), n) == 0)

#define stringify(x) #x
#define __glue(s1, s2) s1 ## s2
#define glue(s1, s2) __glue (s1, s2)

#define pragma(x) \
	_Pragma (stringify (x)) \
	extern void __force_compiler_to_require_semicolon_after_pragma (void)
#if defined(__GNUC__) && !defined(__PCC__)
# define poison(x) pragma (GCC poison x)
#elif defined(__clang__)
# define poison(x) pragma (clang poison x)
#else
# define poison(x)
#endif

#if __GCC_PREREQ (4, 5)
# define HAS_BUILTIN_UNREACHABLE 1
#elif defined (__has_builtin)
# define HAS_BUILTIN_UNREACHABLE __has_builtin (__builtin_unreachable)
#else
# define HAS_BUILTIN_UNREACHABLE 0
#endif

#if __GCC_PREREQ (3, 4) \
  || (__GCC_PREREQ (3, 3) && __GNUC_PATCHLEVEL__ > 3)
# define HAS_BUILTIN_TRAP 1
#elif defined(__has_builtin)
# define HAS_BUILTIN_TRAP __has_builtin (__builtin_trap)
#else
# define HAS_BUILTIN_TRAP 0
#endif

/* Assume that R always holds.  Behavior is undefined if R is false,
   fails to evaluate, or has side effects.

   'assume (R)' is a directive from the programmer telling the
   compiler that R is true so the compiler needn't generate code to
   test R.  This is why 'assume' is in verify.h: it is related to
   static checking (in this case, static checking done by the
   programmer), not dynamic checking.

   'assume (R)' can affect compilation of all the code, not just code
   that happens to be executed after the assume (R) is "executed".
   For example, if the code mistakenly does 'assert (R); assume (R);'
   the compiler is entitled to optimize away the 'assert (R)'.

   Although assuming R can help a compiler generate better code or
   diagnostics, performance can suffer if R uses hard-to-optimize
   features such as function calls not inlined by the compiler.

   Avoid Clang's __builtin_assume, as it breaks GNU Emacs master
   as of 2020-08-23T21:09:49Z!eggert@cs.ucla.edu; see
   <https://bugs.gnu.org/43152#71>.  It's not known whether this breakage
   is a Clang bug or an Emacs bug; play it safe for now.  */
#if HAS_BUILTIN_UNREACHABLE
# define assume(R) ((R) ? (void) 0 : __builtin_unreachable ())
#elif _MSC_VER >= 1200
# define assume(R) __assume (R)
#elif (defined(GCC_LINT) || defined(lint)) && HAS_BUILTIN_TRAP
/* Doing it this way helps various packages when configured with
   --enable-gcc-warnings, which compiles with -Dlint.  It is nicer
   when 'assume' silences warnings even with older GCCs.  */
# define assume(R) ((R) ? (void) 0 : __builtin_trap ())
#else
/* Some tools grok NOTREACHED, e.g., Oracle Studio 12.6.  */
# define assume(R) ((R) ? (void) 0 : /* NOTREACHED */ (void) 0)
#endif

#endif /* _DEFINES_H */
