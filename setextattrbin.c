#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/extattr.h>

static void usage(void) {
  fprintf(stderr, "usage: setextattrbin <attrnamespace> <attrname> -x <attrvalue> <path>\n");
  fprintf(stderr, "       setextattrbin <attrnamespace> <attrname> -stdin <path>\n");
}

static int hex2dec(char c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  } else if ('A' <= c && c <= 'F') {
    return c - 'A' + 10;
  } else if ('a' <= c && c <= 'f') {
    return c - 'a' + 10;
  } else {
    return -1;
  }
}

int main(int argc, const char *argv[]) {
  const char *path;
  int attrnamespace;
  const char *attrname;
  void *attrvalue;
  size_t attrlen;
  if (argc < 5) {
    usage();
    return 1;
  }
  // parse attrnamespace
  if (strcmp(argv[1], "user") == 0) {
    attrnamespace = EXTATTR_NAMESPACE_USER;
  } else if (strcmp(argv[1], "system") == 0) {
    attrnamespace = EXTATTR_NAMESPACE_SYSTEM;
  } else {
    fprintf(stderr, "Unknown namespace '%s'\n", argv[2]);
    return 1;
  }
  // paese attrname
  attrname = argv[2];
  // parse attrvalue
  if (strcmp(argv[3], "-x") == 0) {
    attrlen = strlen(argv[4]);
    off_t i;
    if (argc != 6) {
      usage();
      return 1;
    }
    if (attrlen % 2 != 0) {
      fprintf(stderr, "Invalid hex value (odd number of nibbles)\n");
      return 1;
    }
    attrlen = attrlen / 2;
    attrvalue = malloc(attrlen);
    for (i = 0; i < attrlen; i++) {
      int hi = hex2dec(argv[4][i * 2 + 0]);
      int lo = hex2dec(argv[4][i * 2 + 1]);
      if (hi == -1 || lo == -1) {
        fprintf(stderr, "Invalid hex value (bad character)\n");
        return 1;
      }
      ((unsigned char *) attrvalue)[i] = hi << 4 | lo;
    }
    path = argv[5];
  } else if (strcmp(argv[3], "-stdin") == 0) {
    size_t bufsize = 1024;
    int hi = -1, lo;
    attrvalue = malloc(bufsize);
    attrlen = 0;
    while ((lo = getchar()) != -1) {
      if (lo == '\n' || lo == '\r') {
        continue;
      }
      if (hi == -1) {
        hi = lo;
        continue;
      }
      hi = hex2dec(hi);
      lo = hex2dec(lo);
      if (hi == -1 || lo == -1) {
        fprintf(stderr, "Invalid hex value (bad character)\n");
        return 1;
      }
      if (attrlen == bufsize) {
        bufsize = bufsize * 2;
        attrvalue = reallocf(attrvalue, bufsize);
      }
      ((unsigned char *) attrvalue)[attrlen++] = hi << 4 | lo;
      hi = -1;
    }
    if (hi != -1) {
      fprintf(stderr, "Invalid hex value (odd number of nibbles)\n");
      return 1;
    }
    path = argv[4];
  } else {
    usage();
    return 1;
  }
  if (extattr_set_file(path, attrnamespace, attrname, attrvalue, attrlen) != attrlen) {
    perror("extattr_set_file");
    return 1;
  }
  return 0;
}
