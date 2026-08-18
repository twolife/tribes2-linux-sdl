#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>

static int (*real_pthread_join)(pthread_t, void **);

int SDL_GetGamma(float *red, float *green, float *blue)
{
    if (red)
        *red = 1.0f;

    if (green)
        *green = 1.0f;

    if (blue)
        *blue = 1.0f;

    return 0;
}

void _init(void) {
    *(void **) (&real_pthread_join) = dlsym(RTLD_NEXT, "pthread_join");
}

int pthread_join(pthread_t thread, void **retval) {
    int ret;
    if(thread == 0) {
        ret = ESRCH;
    } else {
        ret = real_pthread_join(thread, retval);
    }
    return ret;
}
