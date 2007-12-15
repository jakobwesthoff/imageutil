#define PROGRAM_TITLE "Kathrein UFS-910 Image Utillity"
#define PROGRAM_VERSION "v 0.1"

#define USERNAME "root"
#define PASSWORD "kathrein"

#define COMMANDPROMPT "-- AUTO IMAGE TOOLS --"

#define TRANSFERPORT 12345

int main( int argc, char** argv );
int readImages( struct in_addr kathreinip, char* targetpath );
int writeImages( struct in_addr kathreinip, char* targetpath );
int openControlConnection( struct in_addr ipaddr, char* username, char* password );
void closeControlConnection( int sock );
void waitForControlConnection( int sock, char* waitfor );
void sendToControlConnection( int sock, char* data );
int getSourceIpFromSocket( int sock, struct in_addr* ipaddr );
int transferServer( struct in_addr* serverip, uint32_t port, char* out, int outlen, char** in, int* inlen );
void installNetHelper( int controlConnection );
