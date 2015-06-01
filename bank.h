#include <pthread.h>

struct Customer{
	pthread_mutex_t lock;
	float balance;	
	char name[100];
	int inService;
};

struct Bank{
	pthread_mutex_t banklock;
	struct Customer Accounts[20];
	int numAccounts;
};
