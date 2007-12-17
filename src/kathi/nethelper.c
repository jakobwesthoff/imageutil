#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* socket and network stuff */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "nethelper.h"
#include "errorcodes.h"

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
			return EXIT_SUCCESS;
		}
		else if ( !strcasecmp( command, "read" ) )
		{
			char* mtdblock;
			char mtddevice[256];
			char mtdsize[128];
			char* buffer;
			int bytesRead = 0;
			FILE* fp;

			memset( mtddevice, 0, 256 );
			memset( mtdsize, 0, 128 );

			mtdblock = command + strlen(command) + 1;
			
			sprintf( mtddevice, "/dev/mtdblock%s", mtdblock );

			if ( ( fp = fopen( mtddevice, "r" ) ) == 0 )
			{
				sendErrorResponse( sock, E_COULD_NOT_OPEN_MTDBLOCK, 0, 0 );
				return -1;
			}

			fseek( fp, 0, SEEK_END );
			sprintf( mtdsize, "%i", ftell( fp ) + 1 );
			fseek( fp, 0, SEEK_SET );

			sendErrorResponse( sock, E_TRANSFERING_MTDBLOCK, mtdsize, strlen(mtdsize) );

			buffer = (char*)malloc(32 * 1024);
			
			while( ( bytesRead = fread( buffer, 1, 32 * 1024, fp ) ) != 0 )
			{
				if ( sendall( sock, buffer, bytesRead, 0 ) != bytesRead )
				{
					fprintf( stderr, "The data could not be send to the pc.\n" );
					return -1;
				}
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
