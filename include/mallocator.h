/*

This Mallocator code was taken from:

CppCon2014 Presentations
STL Features And Implementation Techniques
Stephan T. Lavavej

https://github.com/CppCon/CppCon2014      

*/


#ifndef _MALLOCATOR_H
#define _MALLOCATOR_H

#include <stdlib.h>          // size_t, malloc, free
#include <new>               // bad_alloc, bad_array_new_length
#include "esp32-hal-psram.h" // ps_malloc

template <class T> struct Mallocator {
  typedef T value_type;
  Mallocator() noexcept {} // default ctor not required
  template <class U> Mallocator(const Mallocator<U> &) noexcept {}
  template <class U> bool operator==(const Mallocator<U> &) const noexcept {
    return true;
  }
  template <class U> bool operator!=(const Mallocator<U> &) const noexcept {
    return false;
  }

  T *allocate(const size_t n) const {
    if (n == 0) {
      return nullptr;
    }
    if (n > static_cast<size_t>(-1) / sizeof(T)) {
      throw std::bad_array_new_length();
    }
    
#ifndef BOARD_HAS_PSRAM
    void *const pv = malloc(n * sizeof(T));
#else
    void *const pv = ps_malloc(n * sizeof(T));
#endif

    if (!pv) {
      throw std::bad_alloc();
    }
    return static_cast<T *>(pv);
  }
  void deallocate(T *const p, size_t) const noexcept { free(p); }
};

#endif