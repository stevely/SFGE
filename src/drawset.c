/*
 * drawset.c
 * By Steven Smith
 */

#include "drawset.h"
#include "tinycthread.h"

/* The draw buffers */
static sgfeEntity buffer1[10];
static int buffer1_size;
static sgfeEntity buffer2[10];
static int buffer2_size;

#define THREAD_COUNT 2

static mtx_t bufLock;
static cnd_t bufCond;
static int finishedCount = 0;
static int signalExit = 0;

int sgfeInitDrawBuffers() {
    if( mtx_init(&bufLock, mtx_plain) != thrd_success ) {
        return -1;
    }
    if( cnd_init(&bufCond) != thrd_success ) {
        return -1;
    }
    return 0;
}

sgfeEntity * sgfeGetBuffer( int producer ) {
    if( producer ) {
        return buffer1;
    }
    else {
        return buffer2;
    }
}

void sgfeSignalExit() {
    mtx_lock(&bufLock);
    signalExit = 1;
    mtx_unlock(&bufLock);
}

/*
 * Thread-safe function for swapping between the two entity buffers. The second
 * parameter is a pointer to an integer that should contain the size of the
 * buffer the calling thread is working on, and will be set to the size of the
 * new buffer.
 * Returns NULL when an exit has been signaled.
 */
sgfeEntity * sgfeSwapBuffers( sgfeEntity *buf, int *size ) {
    mtx_lock(&bufLock);
    /* Before waiting for other threads */
    /* Buffer size updating is done here so that every thread updates its buffer
     * before either sleeping or signaling. This guarantees that every buffer
     * has its size updated by the time all the threads begin working again. */
    if( buf == buffer1 ) {
        buffer1_size = *size;
    }
    else {
        buffer2_size = *size;
    }
    finishedCount++;
    /* Waiting for other threads */
    if( finishedCount == THREAD_COUNT ) {
        /* Last thread, signal for wakeup */
        finishedCount = 0;
        cnd_broadcast(&bufCond);
    }
    else {
        /* Not last, gotta sleep */
        cnd_wait(&bufCond, &bufLock);
    }
    /* After waiting for other threads */
    if( signalExit ) {
        mtx_unlock(&bufLock);
        return NULL;
    }
    else {
        mtx_unlock(&bufLock);
        if( buf == buffer1 ) {
            *size = buffer2_size;
            return buffer2;
        }
        else {
            *size = buffer1_size;
            return buffer1;
        }
    }
}
