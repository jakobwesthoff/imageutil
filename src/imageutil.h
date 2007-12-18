#define PROGRAM_TITLE "Kathrein UFS-910 Image Utillity"
#define PROGRAM_VERSION "- PRE-ALPHA VERSION NOT FOR DISTRIBUTION -"

#define USERNAME "root"
#define PASSWORD "kathrein"

#define COMMANDPROMPT "-- AUTO IMAGE TOOLS --"

#define TRANSFERPORT 12345

int main( int argc, char** argv );
int readImages( struct in_addr kathreinip, char* targetpath );
int writeImages( struct in_addr kathreinip, char* targetpath );
void openControlConnection( struct in_addr ipaddr, char* username, char* password );
void closeControlConnection();
int waitForControlConnection( int waitnum, ... );
void sendToControlConnection( char* data );
void sendCommandToNethelper( char* command );
int getSourceIpFromSocket( int sock, struct in_addr* ipaddr );
int getDestinationIpFromSocket( int sock, struct in_addr* ipaddr );
int installNetHelper();
void removeNetHelper();
void recieveAndStoreImage( char* image, char* targetpath );
void transferServer( struct in_addr* serverip, uint32_t port, char* out, int outlen );
int createHeader( char* image, char** header, int* headerlen )
ssize_t sendall( int s , const void * msg , size_t len , int flags );
void errorExit();
