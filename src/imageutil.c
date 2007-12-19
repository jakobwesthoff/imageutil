
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include <stdarg.h>

#include <stdlib.h>

#include <string.h>

#include <time.h>

/* socket and network stuff */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "nethelper_resource.h"
#include "kathi/errorcodes.h"
#include "crc/crc.h"

#include "imageutil.h"

int controlConnection   = 0;
int nethelperConnection = 0;

int main( int argc, char** argv ) 
{
	// Ip address
	struct in_addr kathreinip;

	// targetpath
	char* targetpath;

	// Just output the program title
	printf( "%s %s\n", PROGRAM_TITLE, PROGRAM_VERSION );

	// Check for correct parameter count
	if ( argc < 3 ) {
		// Show usage
		printf( "Usage: %s ACTION KATHREINIP PATHNAME\n", *argv );
		printf( "Read or write images from or to the kathrein ufs-910 reciever\n" );
		printf( "\n" );
		printf( "Example: %s r 10.0.1.202 target\n", *argv );
		printf( "  This will read all the images from the box and store them in the target directory\n" );
		printf( "\n" );
		printf( "An action needs to be specified. It can be either \"r\" for read or \"w\" for write.\n" );
		printf( "Kathreinip specifies the ip addr of the kathrein reciever.\n" );
		printf( "Pathname specifies the directory containing the images to read from or write to.\n" );	
		exit( EXIT_FAILURE );
	}

	if ( inet_aton( argv[2], &kathreinip ) == 0 )
	{
		fprintf( stderr, "The given kathreinip \"%s\" could not be parsed\n", argv[2] );
		exit( EXIT_FAILURE );
	}

	// Target path validation
	{
		// Check if the target path exists
		struct stat stats;
		if ( stat( argv[3], &stats ) == -1 )
		{
			if ( errno == ENOENT )
			{
				// Create the directory if it does not exist
				if ( mkdir( argv[3], 0755 ) == -1 )
				{
					perror( "The target directory could not be created" );
					exit( EXIT_FAILURE );
				}
			}
			else
			{
				// An error occured during the stats call
				perror("An error occured during the validation of the target directory");
				exit( EXIT_FAILURE );
			}
		}
		else
		{
			// Check if the existing target is a directory
			if ( !S_ISDIR( stats.st_mode ) )
			{
				fprintf( stderr, "The supplied target path is not a directory.\n" );
				exit( EXIT_FAILURE );
			}
			// Check if the existing target is readable
			if ( access( argv[3], R_OK ) == -1 )
			{
				perror( "The supplied target directory is not readable" );
				exit( EXIT_FAILURE );
			}
			// Check if the existing target is writable
			if ( access( argv[3], W_OK ) == -1 )
			{
				perror( "The supplied target directory is not writable" );
				exit( EXIT_FAILURE );
			}
		}
		
		// The target path should be valid at this point
		targetpath = argv[3];
	}

	if ( !strcasecmp( argv[1], "w" ) ) 
	{
		writeImages( kathreinip, targetpath );
	}
	else 
	{
		readImages( kathreinip, targetpath );
	}

	printf("Everything done.\n");
	
	return EXIT_SUCCESS;
}

void readImages( struct in_addr kathreinip, char* targetpath ) 
{
	// Open control connection and install the nethelper
	openControlConnection( kathreinip, USERNAME, PASSWORD );
	installNetHelper();	

	// Fetch the images one after another
	recieveAndStoreImage( "kernel", targetpath );
	recieveAndStoreImage( "config", targetpath );
	recieveAndStoreImage( "root", targetpath );
	recieveAndStoreImage( "app", targetpath );
	recieveAndStoreImage( "emergency", targetpath );
	recieveAndStoreImage( "data", targetpath );

	// Maybe recieve the bootloader stuff, too?
	recieveAndStoreImage( "boot", targetpath );
	recieveAndStoreImage( "bootcfg", targetpath );	
	
	// Remove the nethelper and close the control connection
	removeNetHelper();
	closeControlConnection();
}

void writeImages( struct in_addr kathreinip, char* targetpath )
{
	DIR* dir;
	struct dirent* entry;
	FILE* fp;

	int imagelistlen = 0;
	struct imdata
	{
		char* pathname;
		char* filename;
		char* mtdblock;
	} imagelist[8];

	printf( "WARNING: Image writing support is highly EXPERIMENTAL\n" );

	// Read all the image headers and create list of flashable images
	printf( "Reading image headers..." );
	fflush( stdout );
	
	errno = 0;
	if ( ( dir = opendir( targetpath ) ) == 0 )
	{
		printf( "\n" );
		perror( "Could not open the target directory for reading" );
		return;
	}

	while( ( entry = readdir( dir ) ) != 0 )
	{
		char* pathname;
		char* mtdblock;

		if ( !strcmp( entry->d_name, "." ) || !strcmp( entry->d_name, ".." ) )
		{
			continue;
		}

		pathname = malloc( strlen(targetpath) + strlen(entry->d_name) + 2 );
		sprintf( pathname, "%s/%s", targetpath, entry->d_name );
		
		if ( ( fp = fopen( pathname, "r" ) ) == 0 )
		{
			printf( "\n" );
			fprintf( stderr, "Could not open imagefile \"%s\" for reading.\n", entry->d_name );
			return;
		}

		if ( readHeader( fp, &mtdblock ) == -1 )
		{
			continue;
		}
		
		imagelist[imagelistlen].filename = entry->d_name;
		imagelist[imagelistlen].pathname = pathname;
		imagelist[imagelistlen++].mtdblock = mtdblock;
	}

	if ( errno != 0 )
	{
		printf( "\n" );
		perror( "Could not read target directory contents" );
	}

	// Output flashing information and warning	
	{		
		int i;
		
		printf( "\rThe following images will be flashed to the shown mtdblocks:\n" );

		for (i=0; i<imagelistlen; i++)
		{
			printf( "File: %s -> mtdblock%s\n", imagelist[i].filename, imagelist[i].mtdblock );
		}
				
		printf( "\n" );
		printf( "WARNING -   The following process may damage your kathrein severly    - WARNING\n" );
		printf( "WARNING - The flashing will start in 10 seconds ( Abort with CTRL-C ) - WARNING\n" );
		fflush( stdout );
		sleep(10);
	}

	// @todo: Add flashing procedure

	// Cleanup
	{
		int i;
		for( i=0; i<imagelistlen; i++ )
		{
			free( imagelist[i].pathname );
		}
	}
}

void openControlConnection( struct in_addr ipaddr, char* username, char* password )
{
	struct sockaddr_in  kathrein;
	
	// Create our needed socket
	if ( ( controlConnection = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
	{
		perror("Could not create socket for control connection" );
		errorExit();
	}

	// Fill our connection information
	kathrein.sin_addr = ipaddr;
	kathrein.sin_family = AF_INET;
	kathrein.sin_port = htons( 23 ); 
	
	// Try to connect to the kathrein telnetd
	if ( connect( controlConnection, (struct sockaddr*)&kathrein, sizeof( kathrein ) ) == -1 )
	{
		perror( "Could not connect to the kathrein telnetd" );
		errorExit();
	}

	// Login to the recievers telnet session
	if ( waitForControlConnection( 2, "login:", "# " ) == 0 )
	{
		sendToControlConnection( USERNAME );
		waitForControlConnection( 1, "Password:" );
		// Send the password manually because it is not echoed back
		sendall( controlConnection, PASSWORD, strlen(PASSWORD), 0 );
		sendall( controlConnection, "\n", 1, 0 );
		waitForControlConnection( 1, "# " );
	}
	
	// We are logged in set a new console prompt for identification
	sendToControlConnection( "PS1=\""COMMANDPROMPT"\"" );
	waitForControlConnection( 1, COMMANDPROMPT );
}

void closeControlConnection()
{
	sendToControlConnection( "exit" );
	close( controlConnection );
}

int waitForControlConnection( int waitnum, ... )
{
	char *data;
	int datalen = 1024;
	char buf[1024];
	int bytesRead = 0;
	int lastRead = 0;	
	va_list va;
	int i;

	// Allocate 1024 bytes for our data buffer
	data = (char*)malloc( datalen );
	memset(data, 0, datalen);

	//@todo: integrate timeout by using select instead of recv directly

	while( ( lastRead = recv( controlConnection, buf, 1024, 0 ) ) != 0 ) 
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

		// Check if one of the string we are waiting for has appeared
		va_start( va, waitnum );
		for ( i = 0; i < waitnum; i++ )
		{
			char* waitfor = va_arg( va, char* );
			if ( strstr( data, waitfor ) != 0 ) 
			{
				// We found the string let's cleanup and go on
				free( data );
				return i;
			}
		}
		va_end( va );
	}

	// The connection terminated while we were waiting for a string
	// This should not happen, therefore bail out
	fprintf( stderr, "The control connection to the kathrein reciever terminated unexpectedly.\n" );
	errorExit();
}

void sendToControlConnection( char* data )
{
	char* echobuf;
	int recvBytes   = 0;
	int neededBytes = 0;
	echobuf = (char*)malloc( strlen(data) + 1 );

	// Send the command followed by a newline
	sendall( controlConnection, data, strlen(data), 0 );
	sendall( controlConnection, "\n", 1, 0 );

	// Recieve the echoed input and just throw it away
	while ( neededBytes < strlen(data) + 1 ) 
	{
		recvBytes = recv( controlConnection, echobuf, strlen(data) + 1 - neededBytes, 0 );
		
		// Something went wrong
		if ( recvBytes == 0 )
		{
			fprintf( stderr, "The control connection to the kathrein reciever terminated unexpectedly.\n" );
			errorExit();
		}

		neededBytes += recvBytes;
	}
	// Free the allocated buffer
	free( echobuf );
}

void sendCommandToNethelper( char* command )
{
	int cmdlen = strlen( command );

	if ( sendall( nethelperConnection, command, cmdlen, 0 ) != cmdlen )
	{
		fprintf( stderr, "Could not send command to the nethelper.\n" );
		errorExit();
	}

}

void recieveNethelperErrorCode( int* errorCode, char** data )
{
	int readBytes = 0;
	char buffer;
	char errorString[1024];
	char* ecode;
	char* edata;
	int errorlen = 0;	

	memset( errorString, 0, 1024 );

	// Read incoming data one character by another until a newline occurs
	while ( ( readBytes = recv( nethelperConnection, &buffer, 1, 0 ) != 0 ) )
	{
		if ( buffer == '\n' )
		{
			errorString[errorlen++] = 0;
			break;
		}
		else if ( buffer == ' ' )
		{
			errorString[errorlen++] = 0;
		}
		else
		{
			errorString[errorlen++] = buffer;
		}
	}

	if ( readBytes == 0 )
	{
		fprintf( stderr, "The nethelper connection died while an error code was expected.\n" );
		errorExit();
	}

	// Check if this a real error message
	if ( strcmp( errorString, "E" ) )
	{
		fprintf( stderr, "A errorstring was excepted but something different was recieved.\n" );
		errorExit();
	}

	ecode = errorString + strlen( errorString ) + 1;
	edata = ecode + strlen( ecode ) + 1;

	*errorCode = atoi( ecode );
	*data = (char*)malloc( errorlen - 2 - strlen(ecode) - 1 ); 
	memcpy( *data, edata, errorlen - 2 - strlen(ecode) - 1 );
}

int getSourceIpFromSocket( int sock, struct in_addr* ipaddr )
{
	int retval;
	struct sockaddr_in saddr_in;
	socklen_t slen;
	
	slen = sizeof(saddr_in);
	retval = getsockname( sock, (struct sockaddr*)&saddr_in, &slen );	
	memcpy( ipaddr, &saddr_in.sin_addr, sizeof(struct in_addr) );
	return retval;
}

int getDestinationIpFromSocket( int sock, struct in_addr* ipaddr )
{
	int retval;
	struct sockaddr_in saddr_in;
	socklen_t slen;
	
	slen = sizeof(saddr_in);
	retval = getpeername( sock, (struct sockaddr*)&saddr_in, &slen );	
	memcpy( ipaddr, &saddr_in.sin_addr, sizeof(struct in_addr) );
	return retval;
}

int installNetHelper()
{
	struct in_addr serverip;
	char port[7];
	char commandline[1024];
	char* outdata;
	int outlen;
	struct sockaddr_in kathrein;
	struct in_addr kathreinip;

	printf( "Installing nethelper on the box.\n" );

	// Prepare the "fake" http data
	outdata = (char*)malloc( NETHELPER_DATA_LEN + 512 );
	memset( outdata, 0, NETHELPER_DATA_LEN + 512 );
	sprintf( outdata, "HTTP/1.1 200 OK\nContent-Type: application/octet-stream\nContent-Length: %i\n\n", NETHELPER_DATA_LEN );
	outlen = NETHELPER_DATA_LEN + strlen(outdata);
	memcpy( outdata + strlen(outdata), NETHELPER_DATA, NETHELPER_DATA_LEN );

	if ( getSourceIpFromSocket( controlConnection, &serverip ) == -1 )
	{
		fprintf( stderr, "The ip address of this system could not be determined.\n" );
		errorExit();
	}

	// Get the port as string
	memset( port, 0, 7 );
	sprintf( port, "%d", TRANSFERPORT );

	// Create the needed commandline
	memset( commandline, 0, 1024 );
	strcat( commandline, "sleep 2; wget http://" );
	strcat( commandline, (char*)inet_ntoa( serverip ) );
	strcat( commandline, ":" );
	strcat( commandline, port );
	strcat( commandline, "/nethelper" );

	// Send the commandline and start the transfer server
	sendToControlConnection( "cd /tmp; rm -rf nethelper" );
	waitForControlConnection( 1, COMMANDPROMPT );
	sendToControlConnection( commandline );
	
	// Start the transfer server to get the nethelper to the box
	transferServer( &serverip, htons( TRANSFERPORT ),  outdata, outlen );
	
	// The helper has been sent, therefore free its data structure in memory
	free( outdata );
	
	waitForControlConnection( 1, COMMANDPROMPT );
	sendToControlConnection( "chmod u+x nethelper" );
	waitForControlConnection( 1, COMMANDPROMPT );
	sendToControlConnection( "/tmp/nethelper" );

	sleep( 2 );

	// Get the kathrein ip based on the controlConnection
	if ( getDestinationIpFromSocket( controlConnection, &kathreinip ) == -1 )
	{
		fprintf( stderr, "The ip address of the kathrein could not be determined.\n" );
		errorExit();
	}

	// Connect to the nethelper
	if ( ( nethelperConnection = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
	{
		perror("Could not create socket for nethelper connection" );
		errorExit();
	}

	// Fill our connection information
	kathrein.sin_addr = kathreinip;
	kathrein.sin_family = AF_INET;
	kathrein.sin_port = htons( TRANSFERPORT ); 
	
	// Try to connect to the nethelper
	if ( connect( nethelperConnection, (struct sockaddr*)&kathrein, sizeof( kathrein ) ) == -1 )
	{
		perror( "Could not connect to the nethelper" );
		errorExit();
	}
}

void removeNetHelper()
{

	printf("Removing nethelper from the box.\n");
	sendCommandToNethelper( "exit" );
	close( nethelperConnection );
	waitForControlConnection( 1, COMMANDPROMPT );
	sendToControlConnection( "cd /tmp; rm -rf nethelper" );
	waitForControlConnection( 1, COMMANDPROMPT );
}

void recieveAndStoreImage( char* image, char* targetpath )
{
	char *mtdblock;
	char commandline[32];

	int ecode;
	char *edata;

	unsigned long origcrc;
	unsigned long recievedcrc = ~0L;

	// Determine which mtdblock is needed
	if ( !strcmp( image, "boot" ) )
	{
		mtdblock = "0";
	}
	else if ( !strcmp( image, "kernel" ) )
	{
		mtdblock = "1";
	}
	else if ( !strcmp( image, "config" ) )
	{
		mtdblock = "2";
	}
	else if ( !strcmp( image, "root" ) )
	{
		mtdblock = "3";
	}
	else if ( !strcmp( image, "app" ) )
	{
		mtdblock = "4";
	}
	else if ( !strcmp( image, "emergency" ) )
	{
		mtdblock = "5";
	}
	else if ( !strcmp( image, "data" ) )
	{
		mtdblock = "6";
	}
	else if ( !strcmp( image, "bootcfg" ) )
	{
		mtdblock = "7";
	}

	// Construct the needed commandline
	memset( commandline, 0, 32 );
	strcat( commandline, "read " );
	strcat( commandline, mtdblock );
	strcat( commandline, "\n" );

	printf( "Calculating %s image checksum...", image );
	fflush( stdout );

	// Execute the commandline
	sendCommandToNethelper( commandline );
	recieveNethelperErrorCode( &ecode, &edata );

	origcrc = atol( edata + strlen(edata) + 1 );
	
	// Handle possible responses
	switch ( ecode )
	{
		case E_COULD_NOT_OPEN_MTDBLOCK:
			printf( "\n" );
			fprintf( stderr, "The mtdblock containing the %s image could not be opened.\n", image );
			errorExit();
		break;
		case E_TRANSFERING_MTDBLOCK:
			{
				FILE* fp;
				char* targetfile;
				char* indata;
				int inlen;
				int recievedBytes    = 0;
				int recievedComplete = 0;

				int headerAvailable  = 0;
				int headerlen;
				char* header;

				targetfile = (char*)malloc(strlen(targetpath) + 32);
				
				memset( targetfile, 0, strlen(targetpath) + 32 );
				strcat( targetfile, targetpath );
				strcat( targetfile, "/" );
				strcat( targetfile, image );
				strcat( targetfile, ".img" );

				if( ( fp = fopen( targetfile, "w+" ) ) == 0 )
				{
					printf("\n");
					fprintf( stderr, "The targetfile \"%s\" could not be opened for writing.\n", targetfile );
					free( targetfile );
					errorExit();
				}

				// Try to add the header to the file
				if ( createHeader( image, &header, &headerlen ) == 0 )
				{
					headerAvailable = 1;
					if ( fwrite( header, sizeof(char), headerlen, fp ) != headerlen )
					{
						printf("\n");
						fprintf( stderr, "The header could not be written to the targetfile \"%s\".\n", targetfile );
						fclose( fp );
						free( targetfile );
						free( header );
						errorExit();
					}
					free( header );
				}

				inlen = atoi( edata );
				indata = (char*)malloc( 32 * 1024 );

				while ( recievedComplete < inlen )
				{
					recievedBytes = recv( nethelperConnection, indata, ((inlen - recievedComplete) > 32 * 1024) ? (32 * 1024) : (inlen - recievedComplete), 0 );
				    if ( recievedBytes == 0 )
					{
						printf("\n");
						fprintf( stderr, "The connection to the nethelper died unexpectedly while fetching image data.\n" );
						fclose( fp );
						free( targetfile );
						free( indata );
						errorExit();
					}
					
					if ( fwrite( indata, sizeof(char), recievedBytes, fp ) != recievedBytes ) 
					{
						printf("\n");
						fprintf( stderr, "The image could not be written to the targetfile \"%s\".\n", targetfile );
						fclose( fp );
						free( targetfile );
						free( indata );
						errorExit();
					}

					recievedcrc = crc32( indata, recievedBytes, recievedcrc );

					recievedComplete += recievedBytes;
					printf( "\rTransfering %s image... (%i kb / %i kb)         ", image, recievedComplete/1024, inlen/1024 );
				}

				free( indata );

				// Write the checksum if an appropriate header could be created
				if ( headerAvailable )
				{
					char chksum[4];
					int i;

					for( i=0; i<4; i++ )
					{
						chksum[i] = ( origcrc >>  i * 8 ) & 0xff;
					}

					if ( fwrite( chksum, sizeof(char), 4, fp ) != 4 ) 
					{
						printf("\n");
						fprintf( stderr, "The checksum could not be written to the targetfile \"%s\".\n", targetfile );
						fclose( fp );
						free( targetfile );
						errorExit();
					}
				}

				free( targetfile );
				fclose( fp );
				printf( "\rTransfered %s image. (%i kb)                    \n", image, recievedComplete/1024 );

				recievedcrc = recievedcrc ^ ~0L;
				if ( recievedcrc != origcrc )
				{
					fprintf( stderr, "CRC mismatch the image should have a checksum of 0x%06x but 0x%06x was found.\nA transfer error seems to have occured.\n", origcrc, recievedcrc );
					errorExit();
				}
			}
		break;
	}
}

void transferServer( struct in_addr* serverip, uint32_t port, char* out, int outlen )
{
	int listensock, sock;
	struct sockaddr_in server;
	struct sockaddr_in kathrein;
	int kathreinlen;	

	if ( ( listensock = socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 ) 
	{
		perror( "Could not create server socket for transfer" );		
		errorExit();		
	}

	server.sin_family      = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port        = port;
	if ( bind( listensock, (struct sockaddr *)&server, sizeof(server) ) == -1 )
	{
		perror( "Could not bind server socket for transfer" );
		close( listensock );
		errorExit();
	}

	if ( listen( listensock, 1 ) == -1 )
	{
		perror( "Could not enter listen state for transfer" );
		close( listensock );
		errorExit();
	}

	kathreinlen = sizeof( kathrein );
	if ( ( sock = accept( listensock, (struct sockaddr*)&kathrein, &kathreinlen ) ) == -1 )
	{
		perror( "Could not accept connection for transfer" );
		close( listensock );
		errorExit();
	}

	// Send all our given data
	if ( sendall( sock, out, outlen, 0 ) != outlen )
	{
		fprintf( stderr, "The given data could not be send.\n" );
		close( listensock );
		close( sock );
		errorExit();
	}
	
	// Recieve all the given data and just ignore it :)
	{	
		char databuf[1024];

		while ( recv( sock, databuf, 1024, 0 ) != 0 )
		{
			// Just do nothing :)
		}
	}

	// Cleanup
	close( sock );
	close( listensock );
}

int createHeader( char* image, char** header, int* headerlen )
{
	char* imgname;
	time_t seconds = time(0);
	struct tm *mytime = localtime( &seconds );

	*header = malloc( sizeof(char) * 17 );
	*headerlen = 16;
	

	if ( !strcmp( image, "boot" ) )
	{
		free( *header );
		*headerlen = 0;
		return -1;
	}
	else if ( !strcmp( image, "kernel" ) )
	{
		imgname = ".ker";
	}
	else if ( !strcmp( image, "config" ) )
	{
		imgname = "conf";
	}
	else if ( !strcmp( image, "root" ) )
	{
		imgname = "root";
	}
	else if ( !strcmp( image, "app" ) )
	{
		imgname = ".app";
	}
	else if ( !strcmp( image, "emergency" ) )
	{
		imgname = ".eme";
	}
	else if ( !strcmp( image, "data" ) )
	{
		imgname = ".dat";
	}
	else if ( !strcmp( image, "bootcfg" ) )
	{
		imgname = "btcf";
	}

	sprintf( *header, "MARU%04i%02i%02i%s", (int)mytime->tm_year + 1900, (int)mytime->tm_mon + 1, (int)mytime->tm_mday, imgname );
	return 0;
}

int readHeader( FILE* fp, char** mtdblock )
{
	char buffer[17]; 

	memset( buffer, 0, 17 );
	
	// Read 16 byte header
	if ( fread( buffer, sizeof(char), 16, fp ) != 16 )
	{
		fclose( fp );
		fprintf( stderr, "Could not read header from image file.\n" );
		errorExit();
	}

	// Check if we are really dealing with a correct header
	if ( buffer[0] != 'M' || buffer[1] != 'A' || buffer[2] != 'R' || buffer[3] != 'U' )
	{
		return -1;
	}

	// Check which image we are dealing with and set the appropriate mtdblock
	if ( !strcmp( buffer + 12, ".ker" ) )
	{
		*mtdblock = "1";
	}
	else if ( !strcmp( buffer + 12, "conf" ) )
	{
		*mtdblock = "2";
	}
	else if ( !strcmp( buffer + 12, "root" ) )
	{
		*mtdblock = "3";
	}
	else if ( !strcmp( buffer + 12, ".app" ) )
	{
		*mtdblock = "4";
	}
	else if ( !strcmp( buffer + 12, ".eme" ) )
	{
		*mtdblock = "5";
	}
	else if ( !strcmp( buffer + 12, ".dat" ) )
	{
		*mtdblock = "6";
	}
	else if ( !strcmp( buffer + 12, "btcf" ) )
	{
		*mtdblock = "7";
	}
	
	return 0;
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

void errorExit()
{
	if ( nethelperConnection != 0 )
	{
		close( nethelperConnection );
	}
	if ( controlConnection != 0 )
	{
		close( controlConnection );
	}
	exit( EXIT_FAILURE );
}



