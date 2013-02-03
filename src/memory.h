/*
 * memory.h
 * By Steven Smith
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include "messages.h"
#include <stddef.h>

enum sgfeMemoryMode {
    sgfeMemWrite,
    sgfeMemRead,
    sgfeMemRW
};

typedef struct sgfeBuffer {
    sgfeChannel channel;
    enum sgfeMemoryMode mode;
    size_t size; /* Total size of buffer */
    size_t count; /* Number of items in buffer */
    size_t width; /* Size of each data item */
    unsigned char flags;
    void *data;
    void *ptr; /* Current pointer location */
    struct sgfeBuffer *next;
} sgfeBuffer;

int sgfeMemoryStart();

sgfeBuffer * sgfeAllocWrite( sgfeChannel chan, size_t size, size_t width );

int sgfeAllocWriteNoBlock( sgfeChannel chan, size_t size, size_t width );

sgfeBuffer * sgfeAllocReadWrite( sgfeChannel chan, size_t size, size_t width );

int sgfeAllocReadWriteNoBlock( sgfeChannel chan, size_t size, size_t width );

sgfeBuffer * sgfeAllocRead( sgfeChannel chan, size_t size, size_t width );

int sgfeAllocReadNoBlock( sgfeChannel chan, size_t size, size_t width );

void * sgfeGetNextWrite( sgfeBuffer *b );

void * sgfeGetNextRead( sgfeBuffer *b );

void * sgfeGetRWData( sgfeBuffer *b );

sgfeBuffer * sgfeTransformBufferToWrite( sgfeBuffer *b );

sgfeBuffer * sgfeTransformBufferToRead( sgfeBuffer *b );

sgfeBuffer * sgfeTransformBufferToRW( sgfeBuffer *b );

#endif
