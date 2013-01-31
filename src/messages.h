/*
 * messages.h
 * By Steven Smith
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

typedef int sgfeChannel;

int sfgeMessengerStart();

sgfeChannel sgfeRegisterChannel( const char *id );

sgfeChannel sgfeGetChannel( const char *id );

sgfeChannel sgfeRequireChannel( const char *id );

int sgfeReadChannel( sgfeChannel channel, void *data, int size );

int sgfeReadChannelBlocking( sgfeChannel channel, void *data, int size );

int sgfeWriteChannel( sgfeChannel channel, void *data, int size );

int sgfeWriteChannelBlocking( sgfeChannel channel, void *data, int size );

#endif
