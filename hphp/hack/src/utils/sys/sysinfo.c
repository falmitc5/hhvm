/**
 * Copyright (c) 2015, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the "hack" directory of this source tree.
 *
 */

#define CAML_NAME_SPACE
#include <caml/alloc.h>
#include <caml/fail.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>

#include <assert.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#endif

#include <unistd.h>

// This function returns an option of a 9-int-member struct
value hh_sysinfo(void) {
  CAMLparam0();
#ifdef __linux__
  CAMLlocal2(result, some);
  result = caml_alloc_tuple(9);
  struct sysinfo info = {0}; // this initializes all members to 0
  if (sysinfo(&info) != 0) {
    caml_failwith("Failed to sysinfo()");
  }
  Store_field(result, 0, Val_long(info.uptime));
  Store_field(result, 1, Val_long(info.totalram * info.mem_unit));
  Store_field(result, 2, Val_long(info.freeram * info.mem_unit));
  Store_field(result, 3, Val_long(info.sharedram * info.mem_unit));
  Store_field(result, 4, Val_long(info.bufferram * info.mem_unit));
  Store_field(result, 5, Val_long(info.totalswap * info.mem_unit));
  Store_field(result, 6, Val_long(info.freeswap * info.mem_unit));
  Store_field(result, 7, Val_long(info.totalhigh * info.mem_unit));
  Store_field(result, 8, Val_long(info.freehigh * info.mem_unit));
  some = caml_alloc(1, 0);
  Store_field(some, 0, result);
  CAMLreturn(some);
#else
  CAMLreturn(Val_int(0)); // None
#endif
}

CAMLprim value hh_sysinfo_is_apple_os(void) {
  CAMLparam0();
  return Val_bool(0);
}

value hh_nproc(void) {
  CAMLparam0();
  CAMLlocal1(result);
  result = Val_long(sysconf(_SC_NPROCESSORS_ONLN));
  CAMLreturn(result);
}

/**
 * There are a bunch of functions that you expect to return a pid,
 * like Unix.getpid() and Unix.create_process(). However, on
 * Windows, instead of returning the process ID, they return a
 * process handle.
 *
 * Process handles seem act like pointers to a process. You can have
 * more than one handle that points to a single process (unlike
 * pids, where there is a single pid for a process).
 *
 * This isn't a problem normally, since functons like Unix.waitpid()
 * will take the process handle on Windows. But if you want to print
 * or log the pid, then you need to dereference the handle and get
 * the pid. And that's what this function does.
 */
value pid_of_handle(value handle) {
  CAMLparam1(handle);
  CAMLreturn(handle);
}

value handle_of_pid_for_termination(value pid) {
  CAMLparam1(pid);
  CAMLreturn(pid);
}
