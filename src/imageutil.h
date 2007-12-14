#define PROGRAM_TITLE "Kathrein UFS-910 Image Utillity"
#define PROGRAM_VERSION "v 0.1"

#define USERNAME "root"
#define PASSWORD "kathrein"

int main( int argc, char** argv );
int readImages( struct in_addr serverip, struct in_addr kathreinip, char* targetpath );
int writeImages( struct in_addr serverip, struct in_addr kathreinip, char* targetpath );
int openControlConnection( struct in_addr ipaddr, char* username, char* password );
void waitForControlConnection( int sock, char* waitfor );
void sendToControlConnection( int sock, char* data );
