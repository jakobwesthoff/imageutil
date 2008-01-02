#define TRANSFERPORT 12345 

int main( int argc, char** argv );
int commandLoop();
ssize_t sendall( int s , const void * msg , size_t len , int flags );
int sendErrorResponse( int sock, int code, char* data, int datalen );
void setVfdText( char* text );
int eraseMtdDevice( char* mtdblock );
