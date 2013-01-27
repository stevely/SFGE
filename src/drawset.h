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

typedef struct {
    GLfloat pos[3];
    GLfloat rot[3];
    sstDrawableSet *set;
} sgfeDrawable;

typedef struct {
    GLfloat dir[3];
    GLfloat color[3];
} sgfeDirectionalLight;

typedef struct {
    GLfloat pos[3];
    GLfloat color[3];
    GLfloat r_start; /* Distance to begin falloff */
    GLfloat r_end; /* Max light distance */
} sgfePointLight;

typedef struct {
    GLfloat pos[3];
    GLfloat dir[3];
    GLfloat color[3];
    GLfloat penumbra; /* Angle to begin falloff */
    GLfloat umbra; /* Max angle illuminated */
} sgfeSpotlight;

typedef struct {
    enum {
        directionalL,
        pointL,
        spotlightL,
        noneL
    } type;
    union {
        sgfeDirectionalLight directional;
        sgfePointLight point;
        sgfeSpotlight spotlight;
    } light;
} sgfeLight;

typedef struct {
    sgfeLight *lights;
    sgfeDrawable *drawables;
    int light_count; /* count = Number of filled slots */
    int draw_count;
    /* These fields should only be modified by code in drawset.c */
    int light_size; /* size = Total size of buffer */
    int draw_size;
} sgfeRenderBuffers;

typedef struct sgfeEntityList {
    sgfeEntity entity;
    struct sgfeEntityList *next;
} sgfeEntityList;

/*
 * Initializes the draw buffers for the given number of threads.
 */
int sgfeInitDrawBuffers();

void sgfeResetLights( sgfeRenderBuffers *bufs );

void sgfeResetDrawables( sgfeRenderBuffers *bufs );

sgfeLight * sgfeNextLight( sgfeRenderBuffers *bufs );

sgfeDrawable * sgfeNextDrawable( sgfeRenderBuffers *bufs );

/*
 * Get the buffer to be used by the producer thread. For obvious reasons, there
 * should only be one producer thread. This function must be called before any
 * buffer swaps, or its behavior is undefined.
 */
sgfeRenderBuffers * sgfeGetProducerBuffer();

/*
 * Get the buffer to be used by a consumer thread. There can be multiple
 * consumer threads running at once, with the caveat that none of them modify
 * the data in the draw buffer.
 */
sgfeRenderBuffers * sgfeGetConsumerBuffer();

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
sgfeRenderBuffers * sgfeSwapBuffers( sgfeRenderBuffers *buf );

#endif
