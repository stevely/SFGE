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

int sgfeInitDrawBuffers();

sgfeEntity * sgfeGetBuffer( int producer );

void sgfeSignalExit();

sgfeEntity * sgfeSwapBuffers( sgfeEntity *buf, int *size );

#endif
