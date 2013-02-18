/*
 * memory.h
 * By Steven Smith
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include "messages.h"
#include <stddef.h>

enum sfgeMemoryMode {
    sfgeMemWrite,
    sfgeMemRead,
    sfgeMemRW
};

typedef struct sfgeBuffer {
    sfgeChannel channel;
    enum sfgeMemoryMode mode;
    size_t size; /* Total size of buffer */
    size_t count; /* Number of items in buffer */
    size_t width; /* Size of each data item */
    unsigned char flags;
    void *data;
    void *ptr; /* Current pointer location */
    struct sfgeBuffer *next;
} sfgeBuffer;

int sfgeMemoryStart();

sfgeBuffer * sfgeAllocWrite( sfgeChannel chan, size_t size, size_t width );

int sfgeAllocWriteNoBlock( sfgeChannel chan, size_t size, size_t width );

sfgeBuffer * sfgeAllocReadWrite( sfgeChannel chan, size_t size, size_t width );

int sfgeAllocReadWriteNoBlock( sfgeChannel chan, size_t size, size_t width );

sfgeBuffer * sfgeAllocRead( sfgeChannel chan, size_t size, size_t width );

int sfgeAllocReadNoBlock( sfgeChannel chan, size_t size, size_t width );

void * sfgeGetNextWrite( sfgeBuffer *b );

void * sfgeGetNextRead( sfgeBuffer *b );

void * sfgeGetRWData( sfgeBuffer *b );

sfgeBuffer * sfgeTransformBufferToWrite( sfgeBuffer *b );

sfgeBuffer * sfgeTransformBufferToRead( sfgeBuffer *b );

sfgeBuffer * sfgeTransformBufferToRW( sfgeBuffer *b );

#endif
