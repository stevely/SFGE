/*
 * memory.c
 * By Steven Smith
 */

#include "memory.h"
#include "messages.h"
#include "tinycthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_CHAN_NAME "memory"

#define MEMORY_FLAG_ALLOC_SIGNALED 1
#define MEMORY_FLAG_ALLOC_RECEIVED (1 << 1)

typedef struct {
    sgfeChannel source;
    enum sgfeMemoryMode mode;
    size_t size;
    size_t width;
} sgfeMemoryMessage;

static thrd_t thread;
static sgfeChannel channel;

static sgfeBuffer * sgfeAlloc( sgfeChannel chan, enum sgfeMemoryMode mode,
size_t size, size_t width ) {
    sgfeBuffer *result;
    result = malloc(sizeof(sgfeBuffer));
    result->channel = chan;
    result->mode = mode;
    result->size = size;
    result->count = 0;
    result->width = width;
    result->flags = 0;
    result->data = malloc(size * width);
    result->ptr = result->data;
    result->next = NULL;
    return result;
}

static int sgfeMemoryRun( void *args ) {
    int status;
    sgfeMemoryMessage message;
    sgfeBuffer *response;
    if( args ) {} /* Avoid unused arg warning */
    while( 1 ) {
        status = sgfeReadChannelBlocking(channel, &message,
                                         sizeof(sgfeMemoryMessage));
        if( status ) {
            switch( message.mode ) {
            /* The only usecase for creating a buffer with no write access is
             * to transform it into a write-only buffer, so we treat its
             * allocation as if it were a write-only buffer. */
            case sgfeMemRead:
            case sgfeMemWrite:
                response = sgfeAlloc(message.source, message.mode,
                                     message.size * 2, message.width);
                break;
            case sgfeMemRW:
                response = sgfeAlloc(message.source, message.mode, message.size,
                                     message.width);
                break;
            }
            /* We have to touch the memory to ensure that the OS actually
             * performs the allocation before we hand it over, otherwise the
             * recipient might suffer a pause when it first accesses the
             * buffer. */
            *((char*)response->data) = 0;
            sgfeWriteChannelBlocking(message.source, &response,
                                     sizeof(sgfeBuffer*));
        }
    }
}

int sgfeMemoryStart() {
    channel = sgfeRegisterChannel(MEMORY_CHAN_NAME);
    return thrd_create(&thread, sgfeMemoryRun, NULL);
}

static int sgfeAllocNoBlock( sgfeChannel chan, enum sgfeMemoryMode mode,
size_t size, size_t width ) {
    sgfeMemoryMessage message;
    message.source = chan;
    message.mode = mode;
    message.size = size;
    message.width = width;
    return sgfeWriteChannel(channel, &message, sizeof(sgfeMemoryMessage));
}

sgfeBuffer * sgfeAllocWrite( sgfeChannel chan, size_t size, size_t width ) {
    return sgfeAlloc(chan, sgfeMemWrite, size, width);
}

int sgfeAllocWriteNoBlock( sgfeChannel chan, size_t size, size_t width ) {
    return sgfeAllocNoBlock(chan, sgfeMemWrite, size, width);
}

sgfeBuffer * sgfeAllocReadWrite( sgfeChannel chan, size_t size, size_t width ) {
    return sgfeAlloc(chan, sgfeMemRW, size, width);
}

int sgfeAllocReadWriteNoBlock( sgfeChannel chan, size_t size, size_t width ) {
    return sgfeAllocNoBlock(chan, sgfeMemRW, size, width);
}

sgfeBuffer * sgfeAllocRead( sgfeChannel chan, size_t size, size_t width ) {
    return sgfeAlloc(chan, sgfeMemRead, size, width);
}

int sgfeAllocReadNoBlock( sgfeChannel chan, size_t size, size_t width ) {
    return sgfeAllocNoBlock(chan, sgfeMemRead, size, width);
}

void * sgfeGetNextWrite( sgfeBuffer *b ) {
    sgfeBuffer *buf;
    int status;
    void *p;
    if( !b->ptr ) {
        return sgfeGetNextWrite(b->next);
    }
    else if( b->count > b->size / b->width / 2 ) {
        p = b->ptr;
        b->ptr += b->width;
        b->count++;
        if( !(b->flags & MEMORY_FLAG_ALLOC_SIGNALED) ) {
            sgfeAllocNoBlock(b->channel, sgfeMemWrite, b->size * 2, b->width);
            b->flags |= MEMORY_FLAG_ALLOC_SIGNALED;
            return p;
        }
        else if( !(b->flags & MEMORY_FLAG_ALLOC_RECEIVED) ) {
            if( b->count > b->size / b->width ) {
                /* We've run out of padding and our buffer isn't here, block */
                sgfeReadChannelBlocking(b->channel, &buf, sizeof(sgfeBuffer*));
                b->flags |= MEMORY_FLAG_ALLOC_RECEIVED;
                b->next = buf;
                b->ptr = NULL;
                return buf->data;
            }
            else {
                /* We have padding, so check if our buffer is here yet */
                status = sgfeReadChannel(b->channel, &buf, sizeof(sgfeBuffer*));
                if( status ) {
                    /* We got our new buffer */
                    b->flags |= MEMORY_FLAG_ALLOC_RECEIVED;
                    b->next = buf;
                }
                return p;
            }
        }
        else {
            if( b->count > b->size / b->width ) {
                b->ptr = NULL;
                b->next->ptr = b->next->data + b->width;
                b->next->count++;
                return b->next->data;
            }
            else {
                return p;
            }
        }
    }
    else {
        p = b->ptr;
        b->ptr += b->width;
        b->count++;
        return p;
    }
}

void * sgfeGetNextRead( sgfeBuffer *b ) {
    void *p;
    if( b->count == 0 ) {
        if( b->next ) {
            return sgfeGetNextRead(b->next);
        }
        else {
            printf("ERROR: Access past end of read-only buffer!\n");
            return NULL;
        }
    }
    else {
        b->count--;
        p = b->ptr;
        b->ptr += b->width;
        return p;
    }
}

void * sgfeGetRWData( sgfeBuffer *b ) {
    if( b->mode == sgfeMemRW ) {
        return b->data;
    }
    else {
        printf("WARN: Attempted direct access to raw data of non-read/write "
               "buffer!\n");
        return NULL;
    }
}

sgfeBuffer * sgfeTransformBufferToWrite( sgfeBuffer *b ) {
    sgfeBuffer *temp;
    while( b->next ) {
        temp = b;
        b = b->next;
        free(temp->data);
        free(temp);
    }
    if( (b->flags & MEMORY_FLAG_ALLOC_SIGNALED)
     && !(b->flags & MEMORY_FLAG_ALLOC_RECEIVED) ) {
        /* Have to block for read to ensure the mode is set to write */
        sgfeReadChannelBlocking(b->channel, &temp, sizeof(sgfeBuffer*));
        temp->mode = sgfeMemWrite;
        b->next = temp;
        b->flags |= MEMORY_FLAG_ALLOC_RECEIVED;
    }
    b->mode = sgfeMemWrite;
    b->ptr = b->data;
    b->count = 0;
    return b;
}

sgfeBuffer * sgfeTransformBufferToRead( sgfeBuffer *b ) {
    sgfeBuffer *temp, *temp2;
    /* Find last entry */
    for( temp = b; temp->next; temp = temp->next ) {
        temp->mode = sgfeMemRead;
    }
    temp->mode = sgfeMemRead;
    if( (temp->flags & MEMORY_FLAG_ALLOC_SIGNALED)
     && !(temp->flags & MEMORY_FLAG_ALLOC_RECEIVED) ) {
        /* Have to block for read to set the access mode */
        sgfeReadChannelBlocking(temp->channel, &temp2, sizeof(sgfeBuffer*));
        temp2->mode = sgfeMemRead;
        temp->next = temp2;
        temp->flags |= MEMORY_FLAG_ALLOC_RECEIVED;
    }
    return b;
}

sgfeBuffer * sgfeTransformBufferToRW( sgfeBuffer *b ) {
    /* RW buffers don't work like read- or write-only buffers. We need to
     * consolidate the whole data set and make a new buffer to hold everything.
     */
    size_t size;
    sgfeBuffer *buf, *temp, *temp2;
    for( buf = b; buf->next; buf = buf->next ); /* Find last entry */
    /* We can't just use buf->size because the last buffer might not be full. */
    if( (buf->flags & MEMORY_FLAG_ALLOC_SIGNALED)
     && !(buf->flags & MEMORY_FLAG_ALLOC_RECEIVED) ) {
        /* Need that memory now */
        sgfeReadChannelBlocking(buf->channel, &temp, sizeof(sgfeBuffer*));
        temp->mode = sgfeMemRW;
        buf->next = temp;
        buf = temp;
    }
    /* Fun fact: the numeric series 1/(2^1) + 1/(2^2) + 1/(2^3) ... + 1/(2^n)
     * converges to 1 as n approaches infinity.
     * This is important because we double buffer size whenever we make a new
     * buffer. Thanks to the magic of math, this tells us that consolidating all
     * the data from all of the previous buffers into the last buffer will leave
     * an amount of space equivalent to the buffer size of the first buffer.
     * Thus, if the last buffer is filled with less data that the total size of
     * the first buffer, then we don't need to allocate a new buffer. If it is,
     * then we do.
     * Math. */
    size = buf->count * buf->width;
    if( buf != b && size <= b->size ) {
        /* No realloc needed. Also, thanks to math, we know exactly where the
         * last buffer's data needs to go without computing anything. */
        /* We can also guarantee there's no overlap here, naturally. */
        memcpy(buf->data + buf->size - b->size - 1, buf->data, size);
        buf->count += (buf->size - b->size) / buf->width;
    }
    else {
        /* Realloc needed. The new buffer will be the same size as the last
         * buffer, plus the difference between the amount of data in the last
         * buffer and the total size of the first buffer. */
        buf->next = sgfeAllocReadWrite(buf->channel, buf->size +
                                       (size - b->size),
                                       buf->width);
        memcpy(buf->next->data + buf->size - b->size - 1, buf->data, size);
        buf->count += (buf->size - b->size) / buf->width;
        buf = buf->next;
    }
    buf->ptr = buf->data;
    /* Copy over data and free buffers */
    for( temp = b; temp->next; ) {
        memcpy(buf->ptr, temp->data, temp->size);
        buf->ptr += temp->size;
        temp2 = temp;
        temp = temp->next;
        free(temp2->data);
        free(temp2);
    }
    return buf;
}
