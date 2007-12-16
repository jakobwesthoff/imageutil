#define PROGRAM_TITLE "Kathrein UFS-910 Image Utillity"
#define PROGRAM_VERSION "- PRE-ALPHA VERSION NOT FOR DISTRIBUTION -"

#define USERNAME "root"
#define PASSWORD "kathrein"

#define COMMANDPROMPT "-- AUTO IMAGE TOOLS --"

#define TRANSFERPORT 12345

int main( int argc, char** argv );
int readImages( struct in_addr kathreinip, char* targetpath );
int writeImages( struct in_addr kathreinip, char* targetpath );
int openControlConnection( struct in_addr ipaddr, char* username, char* password );
void closeControlConnection( int sock );
int waitForControlConnection( int sock, int waitnum, ... );
void sendToControlConnection( int sock, char* data );
int getSourceIpFromSocket( int sock, struct in_addr* ipaddr );
int transferServer( struct in_addr* serverip, uint32_t port, char* out, int outlen, char** in, int* inlen, void (*callback)(int, void*), void* data );
void installNetHelper( int controlConnection );
void recieveAndStoreImage( int controlConnection, char* image, char* targetpath );
void removeNetHelper( int controlConnection );
void recieveImageCallback( int bytes, void* data );
ssize_t sendall( int s , const void * msg , size_t len , int flags );
