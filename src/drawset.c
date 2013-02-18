/*
 * drawset.c
 * By Steven Smith
 */

#include <stdlib.h>
#include "drawset.h"
#include "tinycthread.h"

/* The draw buffers */
static sfgeRenderBuffers buffer1;
static sfgeRenderBuffers buffer2;

/* We store pointers to the last element of the last buffer for each buffer,
 * allowing us to see when we need to allocate memory. */

static int threadCount;
static mtx_t bufLock;
static cnd_t bufCond;
static int finishedCount = 0;
static int signalExit = 0;

#define SFGE_BUF_CHUNK_SIZE 100

/*
 * Initializes the draw buffers for the given number of threads.
 */
int sfgeInitDrawBuffers( int threads ) {
    threadCount = threads;
    if( mtx_init(&bufLock, mtx_plain) != thrd_success ) {
        return -1;
    }
    if( cnd_init(&bufCond) != thrd_success ) {
        return -1;
    }
    buffer1.lights = (sfgeLight*)malloc(sizeof(sfgeLight)
                     * SFGE_BUF_CHUNK_SIZE);
    buffer1.light_size = SFGE_BUF_CHUNK_SIZE;
    buffer1.light_count = 0;
    buffer1.drawables = (sfgeDrawable*)malloc(sizeof(sfgeDrawable)
                        * SFGE_BUF_CHUNK_SIZE);
    buffer1.draw_size = SFGE_BUF_CHUNK_SIZE;
    buffer1.draw_count = 0;
    buffer2.lights = (sfgeLight*)malloc(sizeof(sfgeLight)
                     * SFGE_BUF_CHUNK_SIZE);
    buffer2.light_size = SFGE_BUF_CHUNK_SIZE;
    buffer2.light_count = 0;
    buffer2.drawables = (sfgeDrawable*)malloc(sizeof(sfgeDrawable)
                        * SFGE_BUF_CHUNK_SIZE);
    buffer2.draw_size = SFGE_BUF_CHUNK_SIZE;
    buffer2.draw_count = 0;
    return 0;
}

void sfgeResetLights( sfgeRenderBuffers *bufs ) {
    bufs->light_count = 0;
}

void sfgeResetDrawables( sfgeRenderBuffers *bufs ) {
    bufs->draw_count = 0;
}

sfgeLight * sfgeNextLight( sfgeRenderBuffers *bufs ) {
    if( bufs->light_count == bufs->light_size ) {
        bufs->lights = realloc(bufs->lights, sizeof(sfgeLight) * 2);
        bufs->light_size *= 2;
        bufs->light_count++;
        return bufs->lights + bufs->light_count;
    }
    else {
        bufs->light_count++;
        return bufs->lights + bufs->light_count - 1;
    }
}

sfgeDrawable * sfgeNextDrawable( sfgeRenderBuffers *bufs ) {
    if( bufs->draw_count == bufs->draw_size ) {
        bufs->drawables = realloc(bufs->drawables, sizeof(sfgeDrawable) * 2);
        bufs->draw_size *= 2;
        bufs->draw_count++;
        return bufs->drawables + bufs->draw_count;
    }
    else {
        bufs->draw_count++;
        return bufs->drawables + bufs->draw_count - 1;
    }
}

/*
 * Get the buffer to be used by the producer thread. For obvious reasons, there
 * should only be one producer thread. This function must be called before any
 * buffer swaps, or its behavior is undefined.
 */
sfgeRenderBuffers * sfgeGetProducerBuffer() {
    return &buffer1;
}

/*
 * Get the buffer to be used by a consumer thread. There can be multiple
 * consumer threads running at once, with the caveat that none of them modify
 * the data in the draw buffer. This function must be called before any buffer
 * swaps, or its behavior is undefined.
 */
sfgeRenderBuffers * sfgeGetConsumerBuffer() {
    return &buffer2;
}

/*
 * Signal to the other threads that they should exit. sfgeSwapBuffers() must be
 * called before the signalling thread can exit to ensure that every thread is
 * notified.
 */
void sfgeSignalExit() {
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
sfgeRenderBuffers * sfgeSwapBuffers( sfgeRenderBuffers *buf ) {
    mtx_lock(&bufLock);
    /* Before waiting for other threads */
    finishedCount++;
    /* Waiting for other threads */
    if( finishedCount == threadCount ) {
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
        if( buf == &buffer1 ) {
            return &buffer2;
        }
        else {
            return &buffer1;
        }
    }
}
