
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifndef GC_THREADS
# define GC_THREADS
#endif

#undef GC_NO_THREAD_REDIRECTS
#include "leak_detector.h"

#ifdef GC_PTHREADS
# include <pthread.h>
# include <string.h>
#else
# ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN 1
# endif
# define NOSERVICE
# include <windows.h>
#endif /* !GC_PTHREADS */

#include <stdio.h>

#ifdef GC_PTHREADS
  void * test(void * arg)
#else
  DWORD WINAPI test(LPVOID arg)
#endif
{
    int *p[10];
    int i;
    for (i = 0; i < 10; ++i) {
        p[i] = (int *)malloc(sizeof(int) + i);
    }
    CHECK_LEAKS();
    for (i = 1; i < 10; ++i) {
        free(p[i]);
    }
#   ifdef GC_PTHREADS
      return arg;
#   else
      return (DWORD)(GC_word)arg;
#   endif
}

#ifndef NTHREADS
# define NTHREADS 5
#endif

int main(void) {
# if NTHREADS > 0
    int i;
#   ifdef GC_PTHREADS
      pthread_t t[NTHREADS];
#   else
      HANDLE t[NTHREADS];
      DWORD thread_id;
#   endif
    int code;
# endif

    GC_set_find_leak(1); /* for new collect versions not compiled       */
                         /* with -DFIND_LEAK.                           */
    GC_INIT();

# if NTHREADS > 0
    for (i = 0; i < NTHREADS; ++i) {
#       ifdef GC_PTHREADS
          code = pthread_create(t + i, 0, test, 0);
          if (code != 0) {
            fprintf(stderr, "Thread #%d creation failed: %s\n",
                    i, strerror(code));
            exit(2);
          }
#       else
          t[i] = CreateThread(NULL, 0, test, 0, 0, &thread_id);
          if (NULL == t[i]) {
            fprintf(stderr, "Thread #%d creation failed, errcode= %d\n",
                    i, (int)GetLastError());
            exit(2);
          }
#       endif
    }

    for (i = 0; i < NTHREADS; ++i) {
#       ifdef GC_PTHREADS
          code = pthread_join(t[i], 0);
#       else
          code = WaitForSingleObject(t[i], INFINITE) == WAIT_OBJECT_0 ? 0 :
                                                        (int)GetLastError();
#       endif
        if (code != 0) {
            fprintf(stderr, "Thread #%d join failed, errcode= %d\n",
                    i, code);
            exit(2);
        }
    }
# endif

    CHECK_LEAKS();
    CHECK_LEAKS();
    CHECK_LEAKS();
    return 0;
}
