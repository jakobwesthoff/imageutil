#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/ioctl.h>

/* socket and network stuff */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* Reboot stuff */
#include <unistd.h>
#include <linux/reboot.h>

/* Mtd stuff */
#include "mtd/mtd-user.h"

#include "nethelper.h"
#include "../crc/crc.h"
#include "errorcodes.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int sock; 

int main( int argc, char** argv )
{
	int listensock;
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
	if ( commandLoop() == -1)
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

int commandLoop()
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
		else if ( !strcasecmp( command, "reboot" ) )
		{
			pid_t pid;
			
			// Fork because the connection maybe terminated during the command execution
			pid = fork();
			if ( pid == 0 ) // Child
			{
				setVfdText( "REBOOTING" );
				sleep(1);
				sync();
				sleep(1);
				reboot( LINUX_REBOOT_CMD_RESTART );
			}
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
		else if ( !strcasecmp( command, "erase" ) )
		{
			char* mtdblock;
			int imtdsize;
			char mtdsize[256];

			mtdblock = command + strlen(command) + 1;
			sendErrorResponse( sock, E_ERASING_MTDBLOCK, 0, 0 );
			imtdsize = eraseMtdDevice( mtdblock );
			sprintf( mtdsize, "%i", imtdsize );
			sendErrorResponse( sock, E_ERASED_MTDBLOCK, mtdsize, strlen( mtdsize ) );
		}
		else if ( !strcasecmp( command, "write" ) )
		{
			char* mtdblock;
			char* mtdsize;
			char mtddevice[256];
			int  imtdsize;
			char* buffer;
			FILE* fp;
			unsigned long crc = ~0L;
			char writtencrc[32];
			int completed = 0; 
			char vfdtext[17];

			memset( mtddevice, 0, 256 );

			mtdblock = command + strlen(command) + 1;
			mtdsize = mtdblock + strlen(mtdblock) + 1;
			
	
			imtdsize = atoi( mtdsize );
			
			sprintf( mtddevice, "/dev/mtd%s", mtdblock );

			if ( ( fp = fopen( mtddevice, "w" ) ) == 0 )
			{
				sendErrorResponse( sock, E_COULD_NOT_OPEN_MTDBLOCK, 0, 0 );
				return -1;
			}

			sendErrorResponse( sock, E_READY_TO_WRITE_MTDBLOCK, 0, 0 );

			buffer = (char*)malloc(32*1024);

			while( completed < imtdsize )
			{
				readBytes = recv( sock, buffer, ( ((imtdsize - completed) > 32 * 1024) ? (32*1024) : (imtdsize - completed) ), 0 );
				if ( readBytes == 0 ) 
				{
					fprintf( stderr, "The connection to the image server died unexpectedbly while fetching image data.\n" );
					fclose( fp );
					free( buffer );
					close( sock );
					return -1;
				}

				if ( fwrite( buffer, sizeof(char), readBytes, fp ) != readBytes ) 
				{
					fprintf( stderr, "The image could not be written to the mtd device \"%s\".\n", mtddevice );
					fclose( fp );
					free( buffer );
					close( sock );
					return -1;
				}
					
				crc = crc32( buffer, readBytes, crc );

				completed += readBytes;
				sprintf( vfdtext, "WRITE %s %i", mtdblock, ((completed*100)/imtdsize) );
				setVfdText( vfdtext );
			}

			fclose( fp );
			free( buffer );

			memset(writtencrc, 0, 32 );
			sprintf( writtencrc, "%i", crc ^ ~0L );
			sendErrorResponse( sock, E_WRITTEN_MTDBLOCK, writtencrc, strlen(writtencrc) );
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

int eraseMtdDevice( char* mtdblock )
{
    int fd;
    mtd_info_t mtdinfo;
    erase_info_t eraseinfo;
	char mtddevice[256];
	char vfdtext[17];

	sprintf( mtddevice, "/dev/mtd%s", mtdblock );

	// Open the mtdblock
	if ( ( fd = open( mtddevice, O_RDWR ) ) < 0 )
	{
		sendErrorResponse( sock, E_COULD_NOT_OPEN_MTDBLOCK, 0, 0 );
		close( sock );
		exit( EXIT_FAILURE );
	}
	
	// Retrieve some info about our mtddevice	
	if( ioctl( fd, MEMGETINFO, &mtdinfo ) != 0 ) {
		sendErrorResponse( sock, E_COULD_NOT_GET_MTDINFO, 0, 0 );
		exit( EXIT_FAILURE );
	}

	// Erase the rom block by block
	eraseinfo.length = mtdinfo.erasesize;
	for ( eraseinfo.start = 0; eraseinfo.start < mtdinfo.size; eraseinfo.start += mtdinfo.erasesize ) {
		// Update the status text on the display
		sprintf( vfdtext, "ERASE %s %i", mtdblock, ((eraseinfo.start*100)/mtdinfo.size) );
		setVfdText( vfdtext );

		// Erase the current block
		if( ioctl( fd, MEMERASE, &eraseinfo ) != 0 )
		{
			sprintf( vfdtext, "ERROR" );
			setVfdText( vfdtext );
			sendErrorResponse( sock, E_ERASING_MTDBLOCK_FAILED, 0, 0 );
			exit( EXIT_FAILURE );
		}
	}

	close( fd );

	return mtdinfo.size;
}
