/*
 * messages.h
 * By Steven Smith
 */

#ifndef MESSAGES_H_
#define MESSAGES_H_

typedef int sfgeChannel;

int sfgeMessengerStart();

sfgeChannel sfgeRegisterChannel( const char *id );

sfgeChannel sfgeGetChannel( const char *id );

sfgeChannel sfgeRequireChannel( const char *id );

int sfgeReadChannel( sfgeChannel channel, void *data, int size );

int sfgeReadChannelBlocking( sfgeChannel channel, void *data, int size );

int sfgeWriteChannel( sfgeChannel channel, void *data, int size );

int sfgeWriteChannelBlocking( sfgeChannel channel, void *data, int size );

#endif
