#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>

/* socket and network stuff */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "nethelper.h"
#include "../crc/crc.h"
#include "errorcodes.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int main( int argc, char** argv )
{
	int listensock, sock;
	struct sockaddr_in server;
	struct sockaddr_in client;
	int clientlen;

	if ( ( listensock = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) 
	{
		perror( "Could not create server socket for transfer" );		
		return -1;
	}

	server.sin_family      = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port        = htons( TRANSFERPORT );
	if ( bind( listensock, (struct sockaddr *)&server, sizeof(server) ) == -1 )
	{
		perror( "Could not bind server socket for transfer" );
		close( listensock );
		return -1;
	}

	if ( listen( listensock, 1 ) == -1 )
	{
		perror( "Could not enter listen state for transfer" );
		close( listensock );
		return -1;
	}

	clientlen = sizeof( client );
	if ( ( sock = accept( listensock, (struct sockaddr*)&client, &clientlen ) ) == -1 )
	{
		perror( "Could not accept connection for transfer" );
		close( listensock );
		return -1;
	}

	// Handle the commands
	if ( commandLoop( sock ) == -1)
	{
		close( sock );
		close( listensock );
		exit( EXIT_FAILURE );
	}
	
	// Cleanup
	close( sock );
	close( listensock );
	return EXIT_SUCCESS;
}

int commandLoop( int sock )
{
	while ( 1 )
	{
		char buffer;
		char command[1024];
		int commandlength = 0;
		int readBytes = 0;

		memset( command, 0, 1024 );

		// Read incoming data one character by another until a newline occurs
		while ( ( readBytes = recv( sock, &buffer, 1, 0 ) != 0 ) )
		{
			if ( buffer == '\n' )
			{
				command[commandlength++] = 0;
				break;
			}
			else if ( buffer == ' ' )
			{
				command[commandlength++] = 0;
			}
			else
			{
				command[commandlength++] = buffer;
			}
		}

		if ( readBytes == 0 )
		{
			fprintf( stderr, "The transfer connection died unexpectedly.\n" );
			return -1;
		}

		// Decide which command was given and what needs to be done
		if ( !strcasecmp( command, "exit" ) )
		{
			setVfdText( "FINISHED" );
			return EXIT_SUCCESS;
		}
		else if ( !strcasecmp( command, "read" ) )
		{
			char* mtdblock;
			char mtddevice[256];
			char mtdsize[256];
			int  imtdsize;
			char* buffer;
			FILE* fp;
			unsigned long crc = ~0L;
			int completed = 0; 
			char vfdtext[17];

			memset( mtddevice, 0, 256 );
			memset( mtdsize, 0, 256 );

			mtdblock = command + strlen(command) + 1;
			
			sprintf( mtddevice, "/dev/mtdblock%s", mtdblock );

			if ( ( fp = fopen( mtddevice, "r" ) ) == 0 )
			{
				sendErrorResponse( sock, E_COULD_NOT_OPEN_MTDBLOCK, 0, 0 );
				return -1;
			}

			// Allocate read buffer
			buffer = (char*)malloc( 32 * 1024 );

			sprintf( vfdtext, "READ %s CRC", mtdblock );
			setVfdText( vfdtext );

			// Calculate crc32
			while( ( readBytes = fread( buffer, 1, 32 * 1024, fp ) ) != 0 )
			{
				crc = crc32( buffer, readBytes, crc );
			}

			imtdsize = ftell( fp );
			sprintf( mtdsize, "%i %i", imtdsize, crc ^ ~0L );
			fseek( fp, 0, SEEK_SET );

			sendErrorResponse( sock, E_TRANSFERING_MTDBLOCK, mtdsize, strlen(mtdsize) );
			
			while( ( readBytes = fread( buffer, 1, 32 * 1024, fp ) ) != 0 )
			{
				if ( sendall( sock, buffer, readBytes, 0 ) != readBytes )
				{
					fprintf( stderr, "The data could not be send to the pc.\n" );
					return -1;
				}

				completed += readBytes;
				sprintf( vfdtext, "READ %s %i", mtdblock, ((completed*100)/imtdsize) );
				setVfdText( vfdtext );
			}

			free( buffer );	

			fclose( fp );

		}
	}
}

ssize_t sendall( int s , const void * msg , size_t len , int flags )
{
	ssize_t completed = 0;
	ssize_t bytesSend;

	while ( completed < len )
	{
		bytesSend = send( s, msg, len, flags );
		
		if ( bytesSend == -1 )
		{
			return -1;
		}

		completed += bytesSend;
	}
}

int sendErrorResponse( int sock, int code, char* data, int datalen )
{
	char* errorString;

	errorString = (char*)malloc( datalen + 32 );
	memset( errorString, 0, datalen + 32 );
	sprintf( errorString, "E %i ", code );
	errorString[strlen(errorString) + datalen] = '\n';	
	memcpy( errorString + strlen(errorString), data, datalen );

	if ( sendall( sock, errorString, strlen(errorString), 0 ) != strlen(errorString) )
	{
		fprintf( stderr, "The data could not be send to the pc.\n" );
		return -1;
	}
}

void setVfdText( char* text )
{
	struct ioctl_data
	{
		unsigned char begin;
		unsigned char data[64];
		unsigned char len;
	}
	vfddata;

	int fd;

	// Open the device
	if ( ( fd = open( "/dev/vfd", O_RDWR ) ) == -1 )
	{
		fprintf( stderr, "The /dev/vfd device could not be opened.\n" );
		return;
	}

	// Clear every char
	memset( vfddata.data, ' ', 16 );

	// Copy given string to data structure
	memcpy( vfddata.data, text, MIN( strlen(text), 16) );
	vfddata.begin = 0;
	vfddata.len   = 16;

	if ( ioctl( fd, 0xc0425a00, &vfddata ) == -1 )
	{
		perror( "Could not use ioctl to set display text" );
		close( fd );
		return;
	}

	close( fd );
}
