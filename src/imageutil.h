#define PROGRAM_TITLE "Kathrein UFS-910 Image Utillity"
#define PROGRAM_VERSION "v. 0.2"
#define PROGRAM_COPYRIGHT "(c) Jakob Westhoff <jakob@westhoffswelt.de>"

#define USERNAME "root"
#define PASSWORD "kathrein"

#define COMMANDPROMPT "-- AUTO IMAGE TOOLS --"

#define TRANSFERPORT 12345

struct imagefile
{
	char* pathname;
	char* filename;
	char* mtdblock;
	FILE* fp;
};

int main( int argc, char** argv );
void readImages( struct in_addr kathreinip, char* targetpath );
void writeImages( struct in_addr kathreinip, char* targetpath );
void sendAndWriteImage( struct imagefile* image );
void stripHeader( char* src, char* target );
void addHeader( char* header, char* src, char* target );
int file_exists( char* path );
void validatePath( char* path );
void openControlConnection( struct in_addr ipaddr, char* username, char* password );
void closeControlConnection();
int waitForControlConnection( int waitnum, ... );
void sendToControlConnection( char* data );
void sendCommandToNethelper( char* command );
void recieveNethelperErrorCode( int* errorCode, char** data );
int getSourceIpFromSocket( int sock, struct in_addr* ipaddr );
int getDestinationIpFromSocket( int sock, struct in_addr* ipaddr );
int installNetHelper();
void removeNetHelper();
void recieveAndStoreImage( char* image, char* targetpath );
void transferServer( struct in_addr* serverip, uint32_t port, char* out, int outlen );
int createHeader( char* image, char** header, int* headerlen );
int readHeader( FILE* fp, char** mtdblock );
ssize_t sendall( int s , const void * msg , size_t len , int flags );
void errorExit();
