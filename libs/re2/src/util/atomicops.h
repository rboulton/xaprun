// Copyright 2006-2008 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef RE2_UTIL_ATOMICOPS_H__
#define RE2_UTIL_ATOMICOPS_H__

#if defined(__i386__) 

inline void MemoryBarrier() {
  int x;
  __asm__ __volatile__("xchgl (%0),%0"  // The lock prefix is implicit for xchg.
                       :: "r" (&x));
}

#elif defined(__x86_64__) 

// 64-bit implementations of memory barrier can be simpler, because it
// "mfence" is guaranteed to exist.
inline void MemoryBarrier() {
  __asm__ __volatile__("mfence" : : : "memory");
}

#else

#error Need MemoryBarrier for architecture.

// Windows
inline void MemoryBarrier() {
  LONG x;
  ::InterlockedExchange(&x, 0);
}

#endif

#endif  // RE2_UTIL_ATOMICOPS_H__
