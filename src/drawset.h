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
} sfgeEntity;

typedef struct {
    GLfloat pos[3];
    GLfloat rot[3];
    sstDrawableSet *set;
} sfgeDrawable;

typedef struct {
    GLfloat dir[3];
    GLfloat color[3];
} sfgeDirectionalLight;

typedef struct {
    GLfloat pos[3];
    GLfloat color[3];
    GLfloat r_start; /* Distance to begin falloff */
    GLfloat r_end; /* Max light distance */
} sfgePointLight;

typedef struct {
    GLfloat pos[3];
    GLfloat dir[3];
    GLfloat color[3];
    GLfloat penumbra; /* Angle to begin falloff */
    GLfloat umbra; /* Max angle illuminated */
} sfgeSpotlight;

typedef struct {
    enum {
        directionalL,
        pointL,
        spotlightL,
        noneL
    } type;
    union {
        sfgeDirectionalLight directional;
        sfgePointLight point;
        sfgeSpotlight spotlight;
    } light;
} sfgeLight;

typedef struct {
    sfgeLight *lights;
    sfgeDrawable *drawables;
    int light_count; /* count = Number of filled slots */
    int draw_count;
    /* These fields should only be modified by code in drawset.c */
    int light_size; /* size = Total size of buffer */
    int draw_size;
} sfgeRenderBuffers;

typedef struct sfgeEntityList {
    sfgeEntity entity;
    struct sfgeEntityList *next;
} sfgeEntityList;

/*
 * Initializes the draw buffers for the given number of threads.
 */
int sfgeInitDrawBuffers();

void sfgeResetLights( sfgeRenderBuffers *bufs );

void sfgeResetDrawables( sfgeRenderBuffers *bufs );

sfgeLight * sfgeNextLight( sfgeRenderBuffers *bufs );

sfgeDrawable * sfgeNextDrawable( sfgeRenderBuffers *bufs );

/*
 * Get the buffer to be used by the producer thread. For obvious reasons, there
 * should only be one producer thread. This function must be called before any
 * buffer swaps, or its behavior is undefined.
 */
sfgeRenderBuffers * sfgeGetProducerBuffer();

/*
 * Get the buffer to be used by a consumer thread. There can be multiple
 * consumer threads running at once, with the caveat that none of them modify
 * the data in the draw buffer.
 */
sfgeRenderBuffers * sfgeGetConsumerBuffer();

/*
 * Signal to the other threads that they should exit. sfgeSwapBuffers() must be
 * called before the signalling thread can exit to ensure that every thread is
 * notified.
 */
void sfgeSignalExit();

/*
 * Thread-safe function for swapping between the two entity buffers. The second
 * parameter is a pointer to an integer that should contain the size of the
 * buffer the calling thread is working on, and will be set to the size of the
 * new buffer.
 * Returns NULL when an exit has been signaled.
 */
sfgeRenderBuffers * sfgeSwapBuffers( sfgeRenderBuffers *buf );

#endif
