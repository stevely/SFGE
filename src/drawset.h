/*
 * drawset.h
 * By Steven Smith
 */

#ifndef DRAWSET_H_
#define DRAWSET_H_

#include "sst.h"

typedef struct {
    GLfloat pos[3];
    GLfloat vel[3];
    GLfloat rot[3];
    sstDrawableSet *set;
} sgfeEntity;

typedef struct sgfeEntityList {
    sgfeEntity entity;
    struct sgfeEntityList *next;
} sgfeEntityList;

/*
 * Initializes the draw buffers for the given number of threads.
 */
int sgfeInitDrawBuffers();

/*
 * Get the buffer to be used by the producer thread. For obvious reasons, there
 * should only be one producer thread. This function must be called before any
 * buffer swaps, or its behavior is undefined.
 */
sgfeEntity * sgfeGetProducerBuffer();

/*
 * Get the buffer to be used by a consumer thread. There can be multiple
 * consumer threads running at once, with the caveat that none of them modify
 * the data in the draw buffer.
 */
sgfeEntity * sgfeGetConsumerBuffer();

/*
 * Signal to the other threads that they should exit. sgfeSwapBuffers() must be
 * called before the signalling thread can exit to ensure that every thread is
 * notified.
 */
void sgfeSignalExit();

/*
 * Thread-safe function for swapping between the two entity buffers. The second
 * parameter is a pointer to an integer that should contain the size of the
 * buffer the calling thread is working on, and will be set to the size of the
 * new buffer.
 * Returns NULL when an exit has been signaled.
 */
sgfeEntity * sgfeSwapBuffers( sgfeEntity *buf, int *size );

#endif
