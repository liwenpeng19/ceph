// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */

#ifndef __MUTEX_H
#define __MUTEX_H

#include <pthread.h>
#include "include/assert.h"

#define LOCKDEP

class Mutex {
private:
  const char *name;
  pthread_mutex_t _m;
  int nlock;
  bool recursive;

  // don't allow copying.
  void operator=(Mutex &M) {}
  Mutex( const Mutex &M ) {}

public:
  Mutex(const char *n, bool r = true) : name(n), nlock(0), recursive(r) {
    if (recursive) {
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
      pthread_mutex_init(&_m,&attr);
      pthread_mutexattr_destroy(&attr);
    } else {
      pthread_mutex_init(&_m,NULL);
    }
  }
  ~Mutex() {
    assert(nlock == 0);
    pthread_mutex_destroy(&_m); 
  }

#ifdef LOCKDEP
  void _will_lock(); // about to lock
  void _locked();    // just locked
  void _unlocked();  // just unlocked
#else
  void _will_lock() {} // about to lock
  void _locked() {}    // just locked
  void _unlocked() {}  // just unlocked
#endif

  bool is_locked() {
    return (nlock > 0);
  }

  bool TryLock() {
    int r = pthread_mutex_trylock(&_m);
    if (r == 0) {
      _locked();
      nlock++;
      assert(nlock == 1 || recursive);
    }
    return r == 0;
  }

  void Lock() {
    _will_lock();
    int r = pthread_mutex_lock(&_m);
    _locked();
    assert(r == 0);
    nlock++;
    assert(nlock == 1 || recursive);
  }

  void Unlock() {
    assert(nlock > 0);
    --nlock;
    int r = pthread_mutex_unlock(&_m);
    assert(r == 0);
    _unlocked();
  }

  friend class Cond;


public:
  class Locker {
    Mutex &mutex;

  public:
    Locker(Mutex& m) : mutex(m) {
      mutex.Lock();
    }
    ~Locker() {
      mutex.Unlock();
    }
  };
};


#endif
