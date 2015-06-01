#include	<sys/types.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<signal.h>
#include 	<pthread.h>
// Miniature client to exercise getaddrinfo(2).

int
connect_to_server( const char * server, const char * port )
{
	int			sd;
	struct addrinfo		addrinfo;
	struct addrinfo *	result;
	char			message[256];

	addrinfo.ai_flags = 0;
	addrinfo.ai_family = AF_INET;		// IPv4 only
	addrinfo.ai_socktype = SOCK_STREAM;	// Want TCP/IP
	addrinfo.ai_protocol = 0;		// Any protocol
	addrinfo.ai_addrlen = 0;
	addrinfo.ai_addr = NULL;
	addrinfo.ai_canonname = NULL;
	addrinfo.ai_next = NULL;
	if ( getaddrinfo( server, port, &addrinfo, &result ) != 0 )
	{
		fprintf( stderr, "\x1b[1;31mgetaddrinfo( %s ) failed.  File %s line %d.\x1b[0m\n", server, __FILE__, __LINE__ );
		return -1;
	}
	else if ( errno = 0, (sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 )
	{
		freeaddrinfo( result );
		return -1;
	}
	else
	{
		do {
			if ( errno = 0, connect( sd, result->ai_addr, result->ai_addrlen ) == -1 )
			{
				sleep( 3 );
				write( 1, message, sprintf( message, "\x1b[2;33mConnecting to server %s ...\x1b[0m\n", server ) );
			}
			else
			{
				freeaddrinfo( result );
				return sd;		// connect() succeeded
			}
		} while ( errno == ECONNREFUSED );
		freeaddrinfo( result );
		return -1;
	}
}

void *HeartBeat(void* t){
	int sd = *(int*)t;
	free(t);
	while(1){
		write(sd,"thump",6);
		sleep(1);
		}

}

void *FromServer(void* t){
	pthread_detach(pthread_self());
	int sd = *(int*)t;
	free(t);
	char			buffer[512];
	int num;
	while (1){
		num=read( sd, buffer, sizeof(buffer));
		buffer[num]='\0';
		//write(1,check,sprintf(check,"Client recieved:%s",&buffer));
		if(strcmp(buffer,"thump")==0){
			alarm(5);
			continue;
		}
		write(1, buffer, strlen(buffer) );
		buffer[0]='\0';
		 }
}

void *FromClient(void*t){
		pthread_detach(pthread_self());
		int sd= *(int*)t;
		free(t);
		int			len;
		char			string[512];
		while ( write( 1, ">>", 2) ,(len = read( 0, string, sizeof(string) )) > 0 ){
			string[len-1]= '\0';
			write( sd, string, strlen( string ) + 1 );
			if(strcmp(string,"quit")==0){break;}//this is just to speed up the process, without it it waits a bit before exiting, and it was annoying me during testing
			sleep(2);
		}
}


void handler(int signal){
	if(signal==SIGALRM){write(1,"Server Disconnected. Terminating process.\n",43);raise(SIGINT);}
}

int
main( int argc, char ** argv )
{
	int			check,sd;
	char			message[256];
	pthread_t		hbThread,writeThread,readThread;
	
	if ( argc < 2 )
	{
		fprintf( stderr, "\x1b[1;31mNo host name specified.  File %s line %d.\x1b[0m\n", __FILE__, __LINE__ );
		exit( 1 );
	}
	else if ( (sd = connect_to_server( argv[1], "33400" )) == -1 )
	{
		write( 1, message, sprintf( message,  "\x1b[1;31mCould not connect to server %s errno %s\x1b[0m\n", argv[1], strerror( errno ) ) );
		return 1;
	}
	else
	{
		printf( "Welcome to GENERIC BANK. If you know what you're doing, feel free to start. If you don't, you can use the help command.\n");
		/*while ( write( 1, prompt, sizeof(prompt) ) ,(len = read( 0, string, sizeof(string) )) > 0 ){
			//write( 1, prompt, sizeof(prompt) );
			string[len-1]= '\0';
			write( sd, string, strlen( string ) + 1 );
			num=read( sd, buffer, sizeof(buffer));
			//sprintf(output,"%[^\n]\n",buffer);
			buffer[num]='\0';
			write( 1, buffer, strlen(buffer) );
			buffer[0]='\0';
		}
		*/
		signal(SIGALRM, handler);
		int* sdwrite = (int *)malloc(sizeof(int));
		int* sdread  = (int *)malloc(sizeof(int));
		int* hb =      (int *)malloc(sizeof(int));
		*hb=*sdwrite=*sdread=sd;
		check = pthread_create(&readThread,0,FromClient,(void*)sdread);
		if(check){printf("Pthread failed. Bad things. Going to die now."); return (-1);}
		check = pthread_create(&writeThread,0,FromServer,(void*)sdwrite);
		if(check){printf("Pthread failed. Bad things. Going to die now."); return (-1);}
		check = pthread_create(&hbThread,0,HeartBeat,(void*)hb);
		if(check){printf("Pthread failed. Bad things. Going to die now."); return (-1);}
		//close(sd);
		pthread_exit(0);
	}
	
	

}
