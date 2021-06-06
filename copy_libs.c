// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

// use dlopen() to look for C++/GCC libraries and copy them into
// the bundle directories

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COPY_LIBSTDCXX  0
#define COPY_LIBGCC     1
#define DIR_LIBSTDCXX   "checkrt.d/libstdc++"
#define DIR_LIBGCC      "checkrt.d/libgcc"

// silencing -Wunused-result
#define SYSTEM(x)   { int i=system(x); (void)i; }


void get_lib(int mode)
{
  char origin[4096];
  void *handle;
  char *cmd;
  const char *file, *dir, *format = "cp -vf %s/%s %s";

  if (mode == COPY_LIBSTDCXX) {
    file = "libstdc++.so.6";
    dir = DIR_LIBSTDCXX;
  } else {
    file = "libgcc_s.so.1";
    dir = DIR_LIBGCC;
  }

  if ((handle = dlopen(file, RTLD_LAZY)) == NULL) {
    return;
  }

  if (dlinfo(handle, RTLD_DI_ORIGIN, &origin) == -1) {
    dlclose(handle);
    return;
  }

  dlclose(handle);

  cmd = malloc(strlen(format) + strlen(origin) + strlen(file) + strlen(dir));
  sprintf(cmd, format, origin, file, dir);
  SYSTEM(cmd);
  free(cmd);

  return;
}

int main(void)
{
  SYSTEM("mkdir -vp " DIR_LIBSTDCXX);
  SYSTEM("mkdir -vp " DIR_LIBGCC);
  get_lib(COPY_LIBSTDCXX);
  get_lib(COPY_LIBGCC);
  return 0;
}
