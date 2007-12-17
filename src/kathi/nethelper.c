#include <stdio.h>
#include <stdlib.h>

/* socket and network stuff */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "nethelper.h"

int main( int argc, char** argv )
{
	int    sock;
	struct sockaddr_in  server;
	struct in_addr targetip;
	int targetport;

	char buffer[1024];
	int byteCount = 0;

	if ( argc < 2 )
	{
		printf( "Image utillity nethelper tool\n" );
		printf( "This tool should only be automatically called\n" );
		printf( "Manually calling is not encouraged\n" );
		exit( EXIT_FAILURE );
	}

	if ( inet_aton( argv[1], &targetip ) == 0 )
	{
		fprintf( stderr, "The given targetip \"%s\" could not be parsed\n", argv[1] );
		exit( EXIT_FAILURE );
	}

	if ( ( targetport = atoi( argv[2] ) ) == 0 ) 
	/* 0 is an invalid port therefore no check if a real 0 was given is needed */
	{
		fprintf( stderr, "The given targetport \"%s\" could not be parsed\n", argv[2] );
		exit( EXIT_FAILURE );
	}
	
	// Create our needed socket
	if ( ( sock = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
	{
		perror("Could not create socket" );
		exit( EXIT_FAILURE );
	}

	// Fill our connection information
	server.sin_addr = targetip;
	server.sin_family = AF_INET;
	server.sin_port = htons( targetport ); 
	
	// Try to connect to the kathrein telnetd
	if ( connect( sock, (struct sockaddr*)&server, sizeof( server ) ) == -1 )
	{
		perror( "Could not connect to target server" );
		close( sock );
		exit( EXIT_FAILURE );
	}

	// Read everything on stdin and send it to the server
	while ( ( byteCount = fread( buffer, 1, 1024, stdin ) ) != 0 )
	{
		if ( sendall( sock, buffer, byteCount, 0 ) != byteCount )
		{
			fprintf( stderr, "There was an error transmitting the read data.\n" );
			close( sock );
			exit( EXIT_FAILURE );
		}
	}

	if ( !feof( stdin ) )
	{
		perror( "Error during stdin reading" );
		close( sock );
		exit( EXIT_FAILURE );
	}
	
	close( sock );
	return EXIT_SUCCESS;
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
