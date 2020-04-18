/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <stdarg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "native_client/src/include/build_config.h"

/*
 * TODO(sehr): enable thread local storage.
 */
#define THREAD

char **environ;

char    bss_buf[1024];  /* just to see ELF info */
int     bss_int;

THREAD int  tls_bss_int;
THREAD char tls_bss_buf[8192];

THREAD int  tls_data_int = 10;
THREAD char tls_data_buf[128] = "Hello world\n";


void  test_tls(void)
{
  printf("%s\n", tls_data_buf);
}


void  test_fmt(char *fmt, int val)
{
  fputs(fmt, stdout); putc(':', stdout);
  printf(fmt, val);
  putc('\n', stdout);
}

void  test_fp_fmt(char  *fmt, double val)
{
  fputs(fmt, stdout); putc(':', stdout);
  printf(fmt, val);
  putc('\n', stdout);
}

void  test_fp_val(double  val)
{
  printf("\n123456789012345678901234567890"
         "123456789012345678901234567890\n");
  test_fp_fmt("%e", val);
  test_fp_fmt("%30.15e.", val);
  test_fp_fmt("%-30.15e.", val);
  test_fp_fmt("%f.", val);
  test_fp_fmt("%30.15f.", val);
  test_fp_fmt("%-30.15f.", val);
}

void  test_fp(void)
{
  double d;

  printf("one half is %e\n", 0.5);
  printf("one half is %f\n", 0.5);
  printf("one third is %e\n", 1.0/3.0);
  printf("one third is %f\n", 1.0/3.0);

  test_fp_val(0.0);
  test_fp_fmt("%.500e", 1.0);
  d = 1.234567e+10;
  test_fp_val(d);
  test_fp_val(-d);

  d = 1.234567e308;
  test_fp_val(d);
  test_fp_val(-d);

  d = 1.234567e-307;
  test_fp_val(d);
  test_fp_val(-d);

  d = 1.234567e-310;
  test_fp_val(d);
  test_fp_val(-d);
#if 0
  /* TODO: the code below doesn't compile on Windows
     NOTE(robertm): disabled this code for all platforms
     to be able to do with one golden file */
  d = 1.234567e+310;  /* inf */
  test_fp_val(d);
  test_fp_val(-d);
  d = 1.234567e+310;  /* inf */
  test_fp_val(d);
  test_fp_val(-d);

  d = 0.0/0.0;  /* nan */
  test_fp_val(d);
#endif  /*NACL_WINDOWS*/
}

void  test_malloc(void)
{
  char    *p;
  printf("test_malloc: entered\n");
  p = malloc(10);
  fprintf(stderr, "# got 0x%08x\n", (unsigned int) p);
  printf("test_malloc: leaving\n");
}

void test_time(void)
{
  time_t tval;
  printf("test_time: entered\n");
  tval = time(NULL);
  fprintf(stderr, "# current time is %d \n", (int) tval);
  printf("test_time: leaving\n");
}

void  test_fopen(const char* fn)
{
  FILE  *fp;
  int   c;
  char  line[1024];
  char  *rv;

  printf("test_fopen: entered\n");
  fp = fopen(fn, "rb");
  if (NULL == fp) {
    printf("could not open file %s!\n", fn);
    exit(-1);
  } else {
    printf("reading file\n");
    while (EOF != (c = fgetc(fp))) {
      if ((int)c != 13)
        /*
          TODO(gregoryd): make sure the input files are identical
          on all platforms, so we won't need the if.
        */
        putc(c, stdout);
    }
    fclose(fp);
  }

  while (0 != (rv = (fputs("input: ", stdout),
                     fflush(stdout),
                     fgets(line, sizeof line, stdin)))) {
    printf("Got line: <<%s>>, %d\n", rv, strlen(rv));
  }
  printf("test_fopen: done\n");
}


int NaClMain(const char* fn)
{
  char const  *hello = "Hello world\n";
  char const  *bye = "Goodbye cruel world";
  int         r;

  r = write(1, hello, strlen(hello));

  printf("write returned %d.\n", r);
  test_fmt("<%04d>", r);
  test_fmt("<%-d>", r);
  test_fmt("<%-04d>", r);
  r = -r;
  test_fmt("<%d>", r);
  test_fmt("<%04d>", r);
  test_fmt("<%-d>", r);
  test_fmt("<%-04d>", r);

  printf("The message was \"%s\".\n", hello);

  printf("Bye message: \"%25s\".\n", bye);
  printf("Bye message: \"%-25s\".\n", bye);

  test_fp();
  test_malloc();
  test_time();
  test_fopen(fn);

  fflush(0);
  fprintf(stderr, "# Hello is at address 0x%08lx\n", (long) hello);
  fprintf(stderr, "# Hello is at address %p\n", (void *) hello);
  {
    short s = -1;
    unsigned short us = (unsigned short) -1;

    printf("s = %hd\n", s);
    printf("us = %hu\n", us);
  }

  printf("Goodbye cruel world.  Really.\n");
  fprintf(stderr, "My pid is %d\n", getpid());
  fflush(0);
  return 0;
}

int SimpleMain(void)
{
  static char const  hello[] = "Hello from SimpleMain\n";

  write(1, hello, -1 + sizeof(hello));
  return 0;
}

int main(int argc, char *argv[], char **ep)
{
  if (argc < 2) {
    printf("you must specify a file to read\n");
    exit(-1);
  }
#if 1
  return NaClMain(argv[1]);
#else
  return SimpleMain();
#endif
}

void test_start(char *arg, ...)
{
  int   argc;
  char  **argv;
  char  **envp;

  argv = &arg;
  argc = *(int *)(argv-1);
  envp = argv + argc + 1;

  environ = envp;

  exit(main(argc, argv, envp));
}
