
#include <stdio.h>

#include <stdlib.h>

#include <string.h>

/* socket and network stuff */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "imageutil.h"

int main( int argc, char** argv ) 
{
	// Ip addresses
	struct in_addr serverip, kathreinip;

	// targetpath
	char* targetpath;

	// Just output the program title
	printf( "%s %s\n", PROGRAM_TITLE, PROGRAM_VERSION );

	// Check for correct parameter count
	if ( argc < 4 ) {
		// Show usage
		printf( "Usage: %s ACTION SERVERIP KATHREINIP PATHNAME\n", *argv );
		printf( "Read or write images from or to the kathrein ufs-910 reciever\n" );
		printf( "\n" );
		printf( "Example: %s r 10.0.1.5 10.0.1.202 target\n", *argv );
		printf( "  This will read all the images from the box and store them in the target directory\n" );
		printf( "\n" );
		printf( "An action needs to be specified. It can be either \"r\" for read or \"w\" for write.\n" );
		printf( "Serverip specifies the ip addr which this computer can be reached from the kathrein box.\n" );
		printf( "Kathreinip specifies the ip addr of the kathrein reciever.\n" );
		printf( "Pathname specifies the directory containing the images to read from or write to.\n" );	
	}

	if ( inet_aton( argv[2], &serverip ) == 0 )
	{
		fprintf( stderr, "The given serverip \"%s\" could not be parsed\n", argv[2] );
		exit( EXIT_FAILURE );
	}
	if ( inet_aton( argv[3], &kathreinip ) == 0 )
	{
		fprintf(stderr, "The given kathreinip \"%s\" could not be parsed\n", argv[3] );
		exit( EXIT_FAILURE );
	}

	targetpath = argv[4];

	if ( !strcasecmp( argv[1], "w" ) ) 
	{
		return writeImages( serverip, kathreinip, targetpath );
	}
	else 
	{
		return readImages( serverip, kathreinip, targetpath );
	}
}

int readImages( struct in_addr serverip, struct in_addr kathreinip, char* targetpath ) 
{
	int controlConnection;
	
	controlConnection = openControlConnection( kathreinip, USERNAME, PASSWORD );
}

int writeImages( struct in_addr serverip, struct in_addr kathreinip, char* targetpath )
{
}

int openControlConnection( struct in_addr ipaddr, char* username, char* password )
{
	int    sock;
	struct sockaddr_in  kathrein;
	
	// Create our needed socket
	sock = socket( AF_INET, SOCK_STREAM, 0 );
	if ( sock == -1 ) 
	{
		perror("Could not create socket for control connection" );
		exit( EXIT_FAILURE );
	}

	// Fill our connection information
	kathrein.sin_addr = ipaddr;
	kathrein.sin_family = AF_INET;
	kathrein.sin_port = htons( 23 ); 
	
	// Try to connect to the kathrein telnetd
	if ( connect( sock, (struct sockaddr*)&kathrein, sizeof( kathrein ) ) == -1 )
	{
		perror( "Could not connect to the kathrein telnetd" );
	}

	// Login to the recievers telnet session
	waitForControlConnection( sock, "login:" );
	sendToControlConnection( sock, USERNAME );
	waitForControlConnection( sock, "Password:" );
	// Send the password manually because it is not echoed back
	send( sock, PASSWORD, strlen(PASSWORD), 0 );
	send( sock, "\n", 1, 0 );
	waitForControlConnection( sock, "~ # " );
	
	// We are logged in set a new console prompt for identification
	sendToControlConnection( sock, "PS1=\"-- AUTO IMAGE TOOLS -- \"" );
	waitForControlConnection( sock, "\n-- AUTO IMAGE TOOLS -- " );

	// We are ready to proceed 
	return sock;
}

void waitForControlConnection( int sock, char* waitfor )
{
	char *data;
	int datalen = 1024;
	char buf[1024];
	int bytesRead = 0;
	int lastRead = 0;	

	// Allocate 1024 bytes for our data buffer
	data = (char*)malloc( datalen );
	memset(data, 0, datalen);

	while( ( lastRead = recv( sock, buf, 1024, 0 ) ) != 0 ) 
	{
		// Check if we need to get more space for the data
		if ( bytesRead + lastRead > datalen ) 
		{
			// Reallocate our data buffer
			data = (char*)realloc( data, datalen + 1024 );
			memset( data + datalen, 0, 1024 );
			datalen += 1024;
		}

		// Add the read data to our data buffer
		memcpy( data + bytesRead, buf, lastRead );

		// Update byte counter
		bytesRead += lastRead;

		// Check if the string we are waiting for has appeared
		if ( strstr( data, waitfor ) != 0 ) 
		{
			printf("%s \n -- \n\n", data);
			// We found the string let's cleanup and go on
			free( data );
			return;
		}
	}

	// The connection terminated while we were waiting for a string
	// This should not happen, therefore bail out
	fprintf( stderr, "The control connection to the kathrein reciever terminated unexpectedly.\n" );
	close( sock );
	exit( EXIT_FAILURE );
}

void sendToControlConnection( int sock, char* data )
{
	char* echobuf;
	int recvBytes   = 0;
	int neededBytes = 0;
	echobuf = (char*)malloc( strlen(data) + 1 );

	// Send the command followed by a newline
	send( sock, data, strlen(data), 0 );
	send( sock, "\n", 1, 0 );

	// Recieve the echoed input and just throw it away
	while ( neededBytes < strlen(data) + 1 ) 
	{
		// DEBUG
		printf(">> Waiting for echo\n");
		// DEBUG
		memset( echobuf, 0, strlen(data) + 1 );

		recvBytes = recv( sock, echobuf, strlen(data) + 1 - neededBytes, 0 );
		
		// DEBUG
		printf(">> Recieved part of echo: \"%s\"\n", echobuf );
	
		// Something went wrong
		if ( recvBytes == 0 )
		{
			fprintf( stderr, "The control connection to the kathrein reciever terminated unexpectedly.\n" );
			close( sock );
			exit( EXIT_FAILURE );
		}

		neededBytes += recvBytes;
		
		// DEBUG
		printf(">> Recieved %i bytes of %i bytes\n", neededBytes, strlen(data) + 1 );

	}
	// Free the allocated buffer
	free( echobuf );
}
