/*
 * messages.c
 * By Steven Smith
 */

#include "messages.h"
#include "tinycthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHAN_BUF_SIZE 512

typedef struct {
    const char *id;
    void *data;
    int readIndex;
    int writeIndex;
    cnd_t readCond;
    cnd_t writeCond;
    mtx_t mutex;
} sgfeChan;

static mtx_t messengerLock;
static cnd_t requireCond;

static sgfeChan channels[10]; /* TODO: Dynamic resizing */
static int channelCount;

int sfgeMessengerStart() {
    mtx_init(&messengerLock, mtx_plain);
    cnd_init(&requireCond);
    channelCount = 0;
    return 1;
}

sgfeChannel sgfeRegisterChannel( const char *id ) {
    int c;
    mtx_lock(&messengerLock);
    c = channelCount;
    channels[c].id = id;
    channels[c].data = malloc(CHAN_BUF_SIZE);
    channels[c].readIndex = 0;
    channels[c].writeIndex = 0;
    cnd_init(&channels[c].readCond);
    cnd_init(&channels[c].writeCond);
    mtx_init(&channels[c].mutex, mtx_plain);
    channelCount++;
    mtx_unlock(&messengerLock);
    /* If any threads are waiting on a specific channel, signal them to check
     * again. */
    cnd_broadcast(&requireCond);
    return c;
}

sgfeChannel sgfeGetChannel( const char *id ) {
    int i;
    mtx_lock(&messengerLock);
    for( i = 0; i < channelCount; i++ ) {
        if( strcmp(id, channels[i].id) == 0 ) {
            mtx_unlock(&messengerLock);
            return i;
        }
    }
    mtx_unlock(&messengerLock);
    return -1;
}

sgfeChannel sgfeRequireChannel( const char *id ) {
    int i;
    mtx_lock(&messengerLock);
    while( 1 ) {
        for( i = 0; i < channelCount; i++ ) {
            if( strcmp(id, channels[i].id) == 0 ) {
                mtx_unlock(&messengerLock);
                return i;
            }
        }
        /* If the channel is required, we simply block and check again on the
         * next channel registration. */
        cnd_wait(&requireCond, &messengerLock);
    }
    return -1;
}

/* Dev note about sgfeRead/WriteChannel[Blocking]:
 * cnd_broadcast is used when both reading and writing.
 * In the case of reading, there's no technical reason why multiple threads
 * couldn't be reading from a single channel (This would make things such as
 * work-stealing possible).
 * In the case of writing, while reading from a channel will usually free up
 * exactly one slot in the buffer, justifying the use of signal instead of
 * broadcast, there are two cases where doing so would be sub-optimal:
 * 1) There are two writes waiting of different sizes. A read could free up
 *    enough buffer space for one write but not the other. Using signal and
 *    unfortunate scheduling could lead to both writes continuing to block.
 * 2) A read frees up enough buffer space for two waiting writes. Not only is
 *    one write needlessly waiting, but this would cause at least one write to
 *    *always* be waiting henceforth. These hanging writes can accumulate,
 *    potentially dead-locking the channel.
 * Worse still, the first problem happening will likely cause the second problem
 * to also occur.
 */

int sgfeReadChannel( sgfeChannel channel, void *data, int size ) {
    char i;
    sgfeChan chan;
    if( channel < channelCount ) {
        chan = channels[channel];
        mtx_lock(&chan.mutex);
        if( chan.readIndex != chan.writeIndex ) {
            i = ((char*)chan.data)[chan.readIndex];
            /* First byte = size of message. If it's zero, then the writer has
             * wrapped around. */
            if( i == 0 ) {
                chan.readIndex = 0;
                i = ((char*)chan.data)[chan.readIndex];
                if( chan.readIndex != chan.writeIndex ) {
                    if( size == i ) {
                        memcpy(data, chan.data, size);
                        /* Plus one for the leading size byte */
                        chan.readIndex += size + 1;
                        mtx_unlock(&chan.mutex);
                        cnd_broadcast(&chan.writeCond);
                        return 1;
                    }
                }
            }
            else {
                if( size == i ) {
                    memcpy(data, chan.data + chan.readIndex, size);
                    /* Plus one for the leading size byte */
                    chan.readIndex += size + 1;
                    mtx_unlock(&chan.mutex);
                    cnd_broadcast(&chan.writeCond);
                    return 1;
                }
            }
        }
        /* Read and write cursors met, no data to read. Or error. */
        mtx_unlock(&chan.mutex);
        return 0;
    }
    else {
        printf("WARN: Attempted to read from non-existant channel: %d!\n",
               channel);
        return 0;
    }
}

int sgfeReadChannelBlocking( sgfeChannel channel, void *data, int size ) {
    char i;
    sgfeChan chan;
    if( channel < channelCount ) {
        chan = channels[channel];
        mtx_lock(&chan.mutex);
        while( 1 ) {
            if( chan.readIndex != chan.writeIndex ) {
                i = ((char*)chan.data)[chan.readIndex];
                /* First byte = size of message. If it's zero, then the writer has
                 * wrapped around. */
                if( i == 0 ) {
                    chan.readIndex = 0;
                    i = ((char*)chan.data)[chan.readIndex];
                    if( chan.readIndex != chan.writeIndex ) {
                        if( size == i ) {
                            memcpy(data, chan.data, size);
                            /* Plus one for the leading size byte */
                            chan.readIndex += size + 1;
                            mtx_unlock(&chan.mutex);
                            cnd_broadcast(&chan.writeCond);
                            return 1;
                        }
                        else {
                            /* If there's data in the pipe but the wrong struct
                             * passed in, then immediately return 0. */
                            mtx_unlock(&chan.mutex);
                            return 0;
                        }
                    }
                }
                else {
                    if( size == i ) {
                        memcpy(data, chan.data + chan.readIndex, size);
                        /* Plus one for the leading size byte */
                        chan.readIndex += size + 1;
                        mtx_unlock(&chan.mutex);
                        cnd_broadcast(&chan.writeCond);
                        return 1;
                    }
                    else {
                        /* If there's data in the pipe but the wrong struct
                         * passed in, then immediately return 0. */
                        mtx_unlock(&chan.mutex);
                        return 0;
                    }
                }
            }
            /* Read and write cursors met, no data to read. */
            cnd_wait(&chan.readCond, &chan.mutex);
        }
    }
    else {
        printf("WARN: Attempted to read from non-existant channel: %d!\n",
               channel);
        return 0;
    }
}

int sgfeWriteChannel( sgfeChannel channel, void *data, int size ) {
    sgfeChan chan;
    if( channel < channelCount ) {
        chan = channels[channel];
        mtx_lock(&chan.mutex);
        if( chan.writeIndex + size > CHAN_BUF_SIZE ) {
            /* Mark the size byte to indicate that we've wrapped around */
            ((char*)chan.data)[chan.writeIndex] = 0;
            chan.writeIndex = 0;
        }
        if( chan.writeIndex > chan.readIndex
         || chan.writeIndex + size < chan.readIndex ) {
            ((char*)chan.data)[chan.writeIndex] = (char)size;
            /* Plus 1 for the leading size byte */
            memcpy(chan.data+1, data, size);
            chan.writeIndex += size + 1;
            mtx_unlock(&chan.mutex);
            cnd_broadcast(&chan.readCond);
            return 1;
        }
        else {
            mtx_unlock(&chan.mutex);
            return 0;
        }
    }
    else {
        printf("WARN: Attempted to read from non-existant channel: %d!\n",
               channel);
        return 0;
    }
}

int sgfeWriteChannelBlocking( sgfeChannel channel, void *data, int size ) {
    sgfeChan chan;
    if( channel < channelCount ) {
        chan = channels[channel];
        mtx_lock(&chan.mutex);
        while( 1 ) {
            if( chan.writeIndex + size > CHAN_BUF_SIZE ) {
                /* Mark the size byte to indicate that we've wrapped around */
                ((char*)chan.data)[chan.writeIndex] = 0;
                chan.writeIndex = 0;
            }
            if( chan.writeIndex > chan.readIndex
             || chan.writeIndex + size < chan.readIndex ) {
                ((char*)chan.data)[chan.writeIndex] = (char)size;
                /* Plus 1 for the leading size byte */
                memcpy(chan.data+1, data, size);
                chan.writeIndex += size + 1;
                mtx_unlock(&chan.mutex);
                cnd_broadcast(&chan.readCond);
                return 1;
            }
            else {
                cnd_wait(&chan.writeCond, &chan.mutex);
            }
        }
    }
    else {
        printf("WARN: Attempted to read from non-existant channel: %d!\n",
               channel);
        return 0;
    }
}
