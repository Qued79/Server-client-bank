#include	<sys/types.h>
#include 	<sys/wait.h>
#include	<semaphore.h>
#include	<sys/shm.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<pthread.h>
#include 	"bank.h"
sem_t sema4;
// Miniature server to exercise getaddrinfo(2).

int
claim_port( const char * port )
{
	struct addrinfo		addrinfo;
	struct addrinfo *	result;
	int			sd;
	char			message[256];
	int			on = 1;

	addrinfo.ai_flags = AI_PASSIVE;		// for bind()
	addrinfo.ai_family = AF_INET;		// IPv4 only
	addrinfo.ai_socktype = SOCK_STREAM;	// Want TCP/IP
	addrinfo.ai_protocol = 0;		// Any protocol
	addrinfo.ai_addrlen = 0;
	addrinfo.ai_addr = NULL;
	addrinfo.ai_canonname = NULL;
	addrinfo.ai_next = NULL;
	if ( getaddrinfo( 0, port, &addrinfo, &result ) != 0 )		// want port 33400
	{
		fprintf( stderr, "\x1b[1;31mgetaddrinfo( %s ) failed errno is %s.  File %s line %d.\x1b[0m\n", port, strerror( errno ), __FILE__, __LINE__ );
		return -1;
	}
	else if ( errno = 0, (sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 )
	{
		write( 1, message, sprintf( message, "socket() failed.  File %s line %d.\n", __FILE__, __LINE__ ) );
		freeaddrinfo( result );
		return -1;
	}
	else if ( setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) == -1 )
	{
		write( 1, message, sprintf( message, "setsockopt() failed.  File %s line %d.\n", __FILE__, __LINE__ ) );
		freeaddrinfo( result );
		close( sd );
		return -1;
	}
	else if ( bind( sd, result->ai_addr, result->ai_addrlen ) == -1 )
	{
		freeaddrinfo( result );
		close( sd );
		write( 1, message, sprintf( message, "\x1b[2;33mBinding to port %s ...\x1b[0m\n", port ) );
		return -1;
	}
	else
	{
		write( 1, message, sprintf( message,  "\x1b[1;32mSUCCESS : Bind to port %s\x1b[0m\n", port ) );
		freeaddrinfo( result );		
		return sd;			// bind() succeeded;
	}
}

void handler(int signal){
	if(signal==SIGCHLD){
	wait(0);
	}
	else if(signal==SIGALRM){sem_post(&sema4);}
}

void handler2(int signal){
	if(signal==SIGALRM){write(1,"User Disconnected. Terminating child process.\n",47);raise(SIGINT);}
}


void *HeartBeat(void* t){
	int sd = *(int*)t;
	free(t);
	while(1){
		write(sd,"thump",6);
		sleep(1);
		}

}


int main( int argc, char ** argv ){

	int			sd;
	char			message[256];
	char			request[256];
	char			cmd[10];
	char			nameArg[100];
	float 			numArg;
	int			numArg2;
	pthread_attr_t		kernel_attr;
	socklen_t		ic;
	int			fd;
	struct sockaddr_in      senderAddr;
	pid_t pid;

	if ( pthread_attr_init( &kernel_attr ) != 0 )
	{
		printf( "pthread_attr_init() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}
	else if ( pthread_attr_setscope( &kernel_attr, PTHREAD_SCOPE_SYSTEM ) != 0 )
	{
		printf( "pthread_attr_setscope() failed in file %s line %d\n", __FILE__, __LINE__ );
		return 0;
	}
	else if ( (sd = claim_port( "33400" )) == -1 )
	{
		write( 1, message, sprintf( message,  "\x1b[1;31mCould not bind to port %s errno %s\x1b[0m\n", "33400", strerror( errno ) ) );
		return 1;
	}
	else if ( listen( sd, 100 ) == -1 )
	{
		printf( "listen() failed in file %s line %d\n", __FILE__, __LINE__ );
		close( sd );
		return 0;
	}
	else
	{
		ic = sizeof(senderAddr);
				
		struct Bank *aBank;			/*begin by creating and initializing the bank in shared memory*/
		int id,i,j,num;
		int size = sizeof(struct Bank)+10;	
		pthread_mutex_t *lockptr;
		
		key_t key = ftok("bank.c",'R');
		if((id=shmget(key, size, 0777 | IPC_CREAT))==-1){
			write(1,message,sprintf(message, "%d", errno));
			return -1;
			}
		if((aBank=(struct Bank*)shmat(id,(void*)0,0))==(void *)-1){return -1;}
		
			
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);	
		pthread_mutexattr_setpshared(&attr,PTHREAD_PROCESS_SHARED);
		
		lockptr = &(aBank->banklock);
		
		
		pthread_mutex_init(lockptr,&attr);
		
		
		signal(SIGALRM, handler);
		signal(SIGCHLD, handler);
		pid=fork();
		/*
 *		For practicality purposes I will not be putting the semaphore in shared memory. Instead it will be local to the print process.
 *		This is because accessing shared memory while in a signal handler is a headache
 * 		*/

		if(pid==0){sem_init(&sema4,0,1);sleep(20);}
		while(pid==0){/*print process infinite loop*/
			sem_wait(&sema4);
			pthread_mutex_lock(&(aBank->banklock));
			write(1, message, sprintf( message,  "Bank Printout:\n" ) );
			for(i=0;i<aBank->numAccounts;i++){	
				pthread_mutex_lock(&(aBank->Accounts[i].lock));
				write(1, message, sprintf( message,  "Name:  %s\nBalance:   %f\n",aBank->Accounts[i].name,aBank->Accounts[i].balance) );
				if(aBank->Accounts[i].inService==1){
					write(1, message, sprintf( message,  "IN SERVICE\n") );
								    }
				pthread_mutex_unlock(&(aBank->Accounts[i].lock));
			}
			pthread_mutex_unlock(&(aBank->banklock));
			alarm(20);
		}
	
		
		while ( (fd = accept( sd, (struct sockaddr *)&senderAddr, &ic )) != -1 )
		{
			pid=fork();
			if(pid==-1){printf("Fork failed! Bad things! Loud noises!!"); }
			else if(pid==0){break;}
			else{close(fd);
			write(1,message,sprintf(message,"A new connection has been established. PID:%d\n", &pid));	
			continue;
			}
			/*Parent remains here. Basically a session acceptor process  */
		}
		
		pthread_t		hbThread;
		int* hb =      (int *)malloc(sizeof(int));
		*hb=fd;
		pthread_create(&hbThread,0,HeartBeat,(void*)hb);
		signal(SIGALRM, handler2);
		while ( read( fd, request, sizeof(request) ) > 0 ){
				
				if(sscanf(request, "%s %[^\n]\n", &cmd, &nameArg)!=2){
					if(strcmp(cmd,"thump")==0){	
						alarm(10);
						continue;
					}
					else if(strcmp(request,"quit")==0){
						write( fd, message, sprintf( message,  "Goodbye.\n" ) );
						close(fd);
						return 0;
						      	             }
					else if(strcmp(request,"help")==0){
					write(fd,message,sprintf(message, "Commands for the current menu:\n\tcreate: Create a new account at GENERIC BANK. Must provide a name for the account\n\tserve: Access an existing account at GENERIC BANK. Must provide a name for the account\n\tquit: Exit our fine e-stablishment.\n"));
						 	      continue;
								       }
					else{
					write(fd, message, sprintf( message,  "Invalid command. If you don't know what to do, please use the \"help\" command\n" ) );
					message[0]='\0';
					continue;
					    }		
					 					     }
				
				if(strcmp(cmd,"create")==0){
					pthread_mutex_lock(&(aBank->banklock));
					num = aBank->numAccounts;
					if(strlen(nameArg)>=99){
						write( fd, message, sprintf( message,  "Your name is too long. Try using a nickname maybe? Surely you have one with a name like that\n" ) );
						message[0]='\0';
						pthread_mutex_unlock(&(aBank->banklock));
						continue;
						}
					else if(num==20){
						write( fd, message, sprintf( message,  "The bank can not handle any more accounts.\n" ) );
						message[0]='\0';
						pthread_mutex_unlock(&(aBank->banklock));
						continue;
						   }
					j=0;
					for(i=0;i<num;i++){
						if(strcmp(aBank->Accounts[i].name,nameArg)==0){
							write( fd, message, sprintf( message,  "There is already an account with this name.\n"  ) );
							message[0]='\0';
							j=1;
							pthread_mutex_unlock(&(aBank->banklock));
							break;			        
								       			      }
							  }
					if(j==1){continue;}
        
					strcpy(aBank->Accounts[num].name,nameArg);
					aBank->Accounts[num].balance = 0;
					aBank->Accounts[num].inService = 0;
					aBank->numAccounts++;	
					pthread_mutex_unlock(&(aBank->banklock));
					write(fd,message,sprintf(message,"Created Account\n"));
					message[0]='\0';
							   }/*end create*/
				
				else if(strcmp(cmd,"serve")==0){
					j=0;					
					pthread_mutex_lock(&(aBank->banklock));
					for(i=0;i<aBank->numAccounts;i++){
						if(strcmp(nameArg,aBank->Accounts[i].name)==0){j=1;break;}
					}
					pthread_mutex_unlock(&(aBank->banklock));
					if(j!=1){
						write( fd, message, sprintf( message,  "There is no such account in the bank.\n") );
						message[0]='\0';
						continue;
						}
					if(aBank->Accounts[i].inService==1){
						write(fd, message, sprintf(message, "There is already a session open for this account, you're either an imposter or a victim of identity fraud. Either way you have to wait.\n"));
						message[0]='\0';
						continue;
									   }
					//Wait on further commands, call deposit, withdraw, query, end appropriately
					write( fd, message, sprintf( message,  "Now accessing account with name %s\n", aBank->Accounts[i].name) );
					message[0]='\0';
					
						
					pthread_mutex_lock(&(aBank->Accounts[i].lock));
					aBank->Accounts[i].inService=1;
					pthread_mutex_unlock(&(aBank->Accounts[i].lock));
					
					while ( read( fd, request, sizeof(request) ) > 0 ){

						
						if(sscanf(request, "%s %s", &cmd, &nameArg)!=2){		
							if(strcmp(cmd,"thump")==0)    {	
								alarm(10);
								continue;
										      }
							
							else if(strcmp(cmd,"end")==0){
								pthread_mutex_lock(&(aBank->Accounts[i].lock));
								aBank->Accounts[i].inService=0;
								pthread_mutex_unlock(&(aBank->Accounts[i].lock));
								break;
						      	             		     }
							else if(strcmp(cmd,"query")==0){	
								pthread_mutex_lock(&(aBank->Accounts[i].lock));
								write( fd, message, sprintf( message,  "\nName:    %s\nBalance:   %.2f$\n", aBank->Accounts[i].name, aBank->Accounts[i].balance));
								message[0]='\0';
								pthread_mutex_unlock(&(aBank->Accounts[i].lock));
								continue;			   
								}

							else if(strcmp(cmd,"help")==0){
			write(fd,message,sprintf(message, "Commands for the current menu:\n\tdeposit: Deposit money into your account. Must provide an amount.\n\twithdraw: Withdraw funds from your account. Must provide an amount.\n\tquery: Retrieve your account information\n\tend: Log out of your account\n\tquit: Exit our fine e-stablishment.\n"));
							continue;
								 		      }
							

							else if(strcmp(cmd,"quit")==0){
								write( fd, message, sprintf( message,  "Goodbye.\n" ) );
								close(fd);
								return 0;
										      }
		
							else{
								write( fd, message, sprintf( message, "Invalid command or incorrect number of arguments\n") );
								message[0]='\0';
								continue;
							    }		
					 					     		}
					

						if(sscanf(nameArg,"%f",&numArg)!=1){
							if(sscanf(nameArg,"%d",&numArg2)==1){numArg=(float)numArg2;}
							else{
								write( fd, message, sprintf( message, "Invalid command or incorrect type of argument\n") );
								message[0]='\0';
								continue;
							    }
										   }



						if(strcmp(cmd,"deposit")==0)    {
							if(numArg<=0){	
								write( fd, message, sprintf( message, "Hey, you tried to trick me. That's not a deposit...\n") );
								message[0]='\0';
								continue;
								    }
							pthread_mutex_lock(&(aBank->Accounts[i].lock));
							aBank->Accounts[i].balance += numArg;
							pthread_mutex_unlock(&(aBank->Accounts[i].lock));
							write(fd,message,sprintf(message,"Deposited %.2f$ in your account, bringing you to a balance of %.2f$\n",numArg,aBank->Accounts[i].balance));
							message[0]='\0';
										}
						
						else if(strcmp(cmd,"withdraw")==0){
							if(numArg<=0){
								write( fd, message, sprintf( message, "Hey, you tried to trick me. That's not a withdraw...\n") );
								message[0]='\0';
								continue;
								     }
							pthread_mutex_lock(&(aBank->Accounts[i].lock));
							if((aBank->Accounts[i].balance-numArg)<=(-0.00001) ){
								write( fd, message, sprintf( message, "You can't afford that. You're just going to have to hold out until pay day.\n") );
								message[0]='\0';
								pthread_mutex_unlock(&(aBank->Accounts[i].lock));
								continue;
													    }
							aBank->Accounts[i].balance -= numArg;
							pthread_mutex_unlock(&(aBank->Accounts[i].lock));
							write(fd,message,sprintf(message,"Withdrew %.2f$ from your account, bringing you to a balance of %.2f$\n",numArg,aBank->Accounts[i].balance));				
							message[0]='\0';
						      }


						else{		
							write( fd, message, sprintf( message, "Invalid command for this menu, please try again. If you don't know what to do, try using \"help\"\n"  ) );
							message[0]='\0';
							continue;
						    }
											  }
						write( fd, message, sprintf( message,  "Done servicing account with name %s\n", aBank->Accounts[i].name) ) ;
						message[0]='\0';
						
								   }/*end serve portion*/



				else{
					write( fd, message, sprintf( message, "Invalid command for this menu, please try again. If you don't know what to do, try using \"help\"\n"  ) );
					message[0]='\0';
					continue;
				    }
				
				}/*End while loop for client session*/	
				write( fd, message, sprintf( message,  "Goodbye.\n" ) );
				close(fd);
	}/*end else*/
	return 0;
}
