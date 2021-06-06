// MIT License

// Copyright (c) 2021 djcj <djcj@gmx.de>

// Elf file parsing code was taken from https://github.com/finixbit/elf-parser
// Copyright (c) 2018 finixbit

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <elf.h>
#include <unistd.h>

#define DEBUG 1

#ifdef __LP64__  // http://gcc.gnu.org/onlinedocs/cpp/Common-Predefined-Macros.html
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Sym  Elf_Sym;
#define ELFCLASS ELFCLASS64
#else
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Sym  Elf_Sym;
#define ELFCLASS ELFCLASS32
#endif

typedef struct {
  int glibcxx[3];
  int cxxabi[3];
  int gcc[3];
} symbol_t;

typedef struct {
  size_t offset;
  char name[32];
  int type;
  int size;
} section_t;

#define CHECK_LIBSTDCXX  0
#define CHECK_LIBGCC     1

// returns 1 if A is higher than B
#define HIGHER_VERSION(A_MAJOR,A_MINOR,A_PATCH, B_MAJOR,B_MINOR,B_PATCH) \
  (A_MAJOR > B_MAJOR || \
    (A_MAJOR == B_MAJOR && \
      (A_MINOR > B_MINOR || \
        (A_MINOR == B_MINOR && A_PATCH > B_PATCH) \
      ) \
    ) \
  )

#ifdef DEBUG
# define PRINT_DEBUG(msg,val1,val2) \
    if (debug) { fprintf(stderr, msg, val1, val2); }
#else
# define PRINT_DEBUG(msg,val1,val2)
#endif


#ifdef DEBUG
int debug = 0;
#endif

int higher_version(int idx, symbol_t a, symbol_t b)
{
  if (idx == 0) {
    return HIGHER_VERSION(a.glibcxx[0], a.glibcxx[1], a.glibcxx[2], b.glibcxx[0], b.glibcxx[1], b.glibcxx[2]);
  } else if (idx == 1) {
    return HIGHER_VERSION(a.cxxabi[0], a.cxxabi[1], a.cxxabi[2], b.cxxabi[0], b.cxxabi[1], b.cxxabi[2]);
  }
  return HIGHER_VERSION(a.gcc[0], a.gcc[1], a.gcc[2], b.gcc[0], b.gcc[1], b.gcc[2]);
}

int get_symbol_versions(const char *file, symbol_t *versions)
{
  int fd;
  struct stat st;
  void *handle;
  char origin[4096];
  char *path;
  const char *p;

  if ((handle = dlopen(file, RTLD_LAZY)) == NULL) {
    PRINT_DEBUG("%s%s: dlopen() failed\n", file, "");
    return 0;
  }

  if (dlinfo(handle, RTLD_DI_ORIGIN, &origin) == -1) {
    PRINT_DEBUG("%s%s: dlinfo() failed\n", file, "");
    dlclose(handle);
    return 0;
  }

  dlclose(handle);

  p = basename((char *)file);
  path = malloc(strlen(origin) + strlen(p) + 2);
  sprintf(path, "%s/%s", origin, p);
  fd = open(path, O_RDONLY);
  free(path);

  if (fd < 0) {
    PRINT_DEBUG("%s/%s: open() failed\n", origin, p);
    return 0;
  }

  if (fstat(fd, &st) < 0) {
    PRINT_DEBUG("%s/%s: fstat() failed\n", origin, p);
    close(fd);
    return 0;
  }

  // mmap
  uint8_t *m_mmap_program = (uint8_t *)(mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
  close(fd);

  if (m_mmap_program == MAP_FAILED) {
    PRINT_DEBUG("%s/%s: mmap() failed\n", origin, p);
    return 0;
  }

  Elf_Ehdr *ehdr = (Elf_Ehdr *)(m_mmap_program);

  if (ehdr->e_ident[EI_CLASS] != ELFCLASS) {
    PRINT_DEBUG("%s/%s: wrong elf class\n", origin, p);
    return 0;
  }

  Elf_Shdr *shdr = (Elf_Shdr *)(m_mmap_program + ehdr->e_shoff);
  int shnum = ehdr->e_shnum;

  Elf_Shdr *sh_strtab = &shdr[ehdr->e_shstrndx];
  const char *sh_strtab_p = (const char *)(m_mmap_program + sh_strtab->sh_offset);

  // count sections
  size_t section_count = 0;

  for (int i = 0; i < shnum; ++i) {
    switch (shdr[i].sh_type) {
      case SHT_STRTAB:
      case SHT_SYMTAB:
      case SHT_DYNSYM: {
          section_count++;
        }
        break;
      default:
        break;
    }
  }

  section_t section[section_count];
  size_t j = 0;

  // get sections
  for (int i = 0; i < shnum; ++i) {
    switch (shdr[i].sh_type) {
      case SHT_STRTAB:
      case SHT_SYMTAB:
      case SHT_DYNSYM: {
          strncpy(section[j].name, sh_strtab_p + shdr[i].sh_name, sizeof(section[section_count].name) - 1);
          section[j].type = shdr[i].sh_type;
          section[j].offset = shdr[i].sh_offset;
          section[j].size = shdr[i].sh_size;
          j++;
        }
        break;
      default:
        break;
    }
  }

  // get strtab
  sh_strtab_p = NULL;

  for (size_t i = 0; i < section_count; ++i) {
    if (section[i].type == SHT_STRTAB && strcmp(section[i].name, ".strtab") == 0){
      sh_strtab_p = (const char *)(m_mmap_program + section[i].offset);
      break;
    }
  }

  // get dynstr
  const char *sh_dynstr_p = NULL;

  for (size_t i = 0; i < section_count; ++i) {
    if (section[i].type == SHT_STRTAB && strcmp(section[i].name, ".dynstr") == 0){
      sh_dynstr_p = (const char *)(m_mmap_program + section[i].offset);
      break;
    }
  }

  // get symbols
  versions->glibcxx[0] = versions->glibcxx[1] = versions->glibcxx[2] = 0;
  versions->cxxabi[0] = versions->cxxabi[1] = versions->cxxabi[2] = 0;
  versions->gcc[0] = versions->gcc[1] = versions->gcc[2] = 0;

  for (size_t i = 0; i < section_count; ++i) {
    if (section[i].type != SHT_SYMTAB && section[i].type != SHT_DYNSYM) {
      continue;
    }

    Elf_Sym *syms_data = (Elf_Sym *)(m_mmap_program + section[i].offset);

    for (size_t j = 0; j < (section[i].size / sizeof(Elf_Sym)); ++j) {
      // pick only ABS index symbols
      if (syms_data[j].st_shndx != SHN_ABS) {
        continue;
      }

      const char *name;
      int ver[3] = { 0, 0, 0 };

      if (section[i].type == SHT_SYMTAB && sh_strtab_p) {
        name = sh_strtab_p + syms_data[j].st_name;
      } else if (section[i].type == SHT_DYNSYM && sh_dynstr_p) {
        name = sh_dynstr_p + syms_data[j].st_name;
      } else {
        name = section[i].name;
      }

      if (strncmp(name, "GLIBCXX_", 8) == 0) {
        sscanf(name + 8, "%d.%d.%d", &ver[0], &ver[1], &ver[2]);

        if (HIGHER_VERSION(ver[0], ver[1], ver[2], versions->glibcxx[0], versions->glibcxx[1], versions->glibcxx[2])) {
          versions->glibcxx[0] = ver[0];
          versions->glibcxx[1] = ver[1];
          versions->glibcxx[2] = ver[2];
        }
      } else if (strncmp(name, "CXXABI_", 7) == 0) {
        sscanf(name + 7, "%d.%d.%d", &ver[0], &ver[1], &ver[2]);

        if (HIGHER_VERSION(ver[0], ver[1], ver[2], versions->cxxabi[0], versions->cxxabi[1], versions->cxxabi[2])) {
          versions->cxxabi[0] = ver[0];
          versions->cxxabi[1] = ver[1];
          versions->cxxabi[2] = ver[2];
        }
      } else if (strncmp(name, "GCC_", 4) == 0) {
        sscanf(name + 4, "%d.%d.%d", &ver[0], &ver[1], &ver[2]);

        if (HIGHER_VERSION(ver[0], ver[1], ver[2], versions->gcc[0], versions->gcc[1], versions->gcc[2])) {
          versions->gcc[0] = ver[0];
          versions->gcc[1] = ver[1];
          versions->gcc[2] = ver[2];
        }
      }
    }
  }

  return 1;
}

// lib: 0 == libstdc++.so.6
//      non-zero == libgcc_s.so.1
// bundle: relative or absolute path to bundled library, without library name
//
// return values:
// -1: error
//  0: system lib is newer or equals bundled lib
//  1: bundled lib is newer
int compare_versions(int lib, const char *bundle)
{
  symbol_t ver_bundle, ver_system;
  const char *file;
  char *lib_bundle;

  if (lib == 0) {
    file = "libstdc++.so.6";
  } else {
    file = "libgcc_s.so.1";
  }

  lib_bundle = malloc(strlen(bundle) + strlen(file) + 2);
  sprintf(lib_bundle, "%s/%s", bundle, file);

  if (!get_symbol_versions(file, &ver_system) || !get_symbol_versions(lib_bundle, &ver_bundle)) {
    free(lib_bundle);
    return -1;
  }

  PRINT_DEBUG("%s%s\n", lib_bundle, "");
  free(lib_bundle);

  if (lib == 0) {
    // libstdc++
#ifdef DEBUG
    if (debug) {
      fprintf(stderr, "GLIBCXX system/bundle: %d.%d.%d / %d.%d.%d\n",
        ver_system.glibcxx[0], ver_system.glibcxx[1], ver_system.glibcxx[2],
        ver_bundle.glibcxx[0], ver_bundle.glibcxx[1], ver_bundle.glibcxx[2]);
      fprintf(stderr, "CXXABI system/bundle: %d.%d.%d / %d.%d.%d\n\n",
        ver_system.cxxabi[0], ver_system.cxxabi[1], ver_system.cxxabi[2],
        ver_bundle.cxxabi[0], ver_bundle.cxxabi[1], ver_bundle.cxxabi[2]);
    }
#endif

    if (higher_version(0, ver_bundle, ver_system) || higher_version(1, ver_bundle, ver_system)) {
      PRINT_DEBUG("%s%s: bundled library is newer\n\n", file, "");
      return 1;
    } else {
      PRINT_DEBUG("%s%s: system library is newer or identical\n\n", file, "");
      return 0;
    }
  } else {
    // libgcc
#ifdef DEBUG
    if (debug) {
      fprintf(stderr, "GCC system/bundle: %d.%d.%d / %d.%d.%d\n\n",
        ver_system.gcc[0], ver_system.gcc[1], ver_system.gcc[2],
        ver_bundle.gcc[0], ver_bundle.gcc[1], ver_bundle.gcc[2]);
    }
#endif

    if (higher_version(2, ver_bundle, ver_system)) {
      PRINT_DEBUG("%s%s: bundled library is newer\n\n", file, "");
      return 1;
    } else {
      PRINT_DEBUG("%s%s: system library is newer or identical\n\n", file, "");
      return 0;
    }
  }

  return -1;
}

// check for libgcc and libstdc++ and print a path that
// can be added to LD_LIBRARY_PATH 
int main(int argc, char *argv[])
{
  char *self, *dn, *bundle_cxx, *bundle_gcc;
  const char *append_cxx = "/libstdc++";
  const char *append_gcc = "/libgcc";
  size_t len;

#ifdef DEBUG
  if (argc > 1 && strcmp(argv[1], "-debug") == 0) {
    debug = 1;
  }
#else
  (void)argc;
  (void)argv;
#endif

  if ((self = realpath("/proc/self/exe", NULL)) == NULL) {
    PRINT_DEBUG("%s%s: realpath() failed to parse /proc/self/exe\n", argv[0], "");
    return 1;
  }

  if ((dn = dirname(self)) == NULL) {
    PRINT_DEBUG("%s: dirname() failed on path: %s\n", argv[0], self);
    free(self);
    return 1;
  }

  len = strlen(dn);
  bundle_cxx = malloc(len + strlen(append_cxx));
  bundle_gcc = malloc(len + strlen(append_gcc));

  sprintf(bundle_cxx, "%s%s", dn, append_cxx);
  sprintf(bundle_gcc, "%s%s", dn, append_gcc);

  if (compare_versions(CHECK_LIBSTDCXX, bundle_cxx) != 1) {
    bundle_cxx = NULL;
  }

  if (compare_versions(CHECK_LIBGCC, bundle_gcc) != 1) {
    bundle_gcc = NULL;
  }

  if (bundle_gcc && bundle_cxx) {
    printf("%s:%s\n", bundle_gcc, bundle_cxx);
  } else if (bundle_cxx) {
    printf("%s\n", bundle_cxx);
  } else if (bundle_gcc) {
    printf("%s\n", bundle_gcc);
  }

  free(bundle_cxx);
  free(bundle_gcc);

  return 0;
}

