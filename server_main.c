


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include "HELPER.h"
#define SHMSIZE 1024
#define MAX_CLIENTS 10

typedef struct {
    int num1;
    int num2;
    char operator;
} RequestA;
typedef struct {
    int num;
} RequestEO;
typedef struct {
    int num;
} RequestP;
typedef struct
{   char client_name[20];
    RequestA a;
    RequestEO eo;
    RequestP p;
} Request;

typedef struct {
    int result;
} Response;

typedef struct {
    pthread_mutex_t mutex;
    Request request;
    Response response;
} Channel;

typedef struct {
    char name[20];
    int is_used;
} ClientInfo;

int isprime(int number)
{
    int i;
    
    // condition for checking the
    // given number is prime or
    // not
    for (i = 2; i <= number / 2; i++) {
        if (number % i != 0)
            continue;
        else
            return 1;
    }
    return 2;
}

int main() {
    int shmd, shms, shmr, shmc;
    Channel *d, *s, *r;
    ClientInfo *c;
    pthread_mutexattr_t attr;

    // Create shared memory for client names
    shmc = shm_open("/client_names", O_CREAT | O_RDWR, 0666);
    ftruncate(shmc, sizeof(ClientInfo) * MAX_CLIENTS);
    c = (ClientInfo *) mmap(NULL, sizeof(ClientInfo) * MAX_CLIENTS, PROT_READ | PROT_WRITE, MAP_SHARED, shmc, 0);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        c[i].is_used = 0;
    }

    // Create shared memory for mutex
    shmd = shm_open("/channel_mutex", O_CREAT | O_RDWR, 0666);
    ftruncate(shmd, sizeof(pthread_mutex_t));
    s = (Channel *) mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmd, 0);
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(s->mutex), &attr);

    // Create shared memory for request channel
    shms = shm_open("/channel_request", O_CREAT | O_RDWR, 0666);
    ftruncate(shms, sizeof(Request));
    d = (Channel *) mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shms, 0);

    d->request.a.num1 = 0;
    d->request.a.num2 = 0;
    d->request.a.operator = ' ';
    d->request.eo.num = 0;
    d->request.p.num = 0;
    memset(d->request.client_name, 0, sizeof(d->request.client_name));

    // Create shared memory for response channel
    shmr = shm_open("/channel_response", O_CREAT | O_RDWR, 0666);
    ftruncate(shmr, sizeof(Response));
    r = (Channel *) mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmr, 0);
    r->response.result = 0;

    PRINT_INFO("Server started.\n");
    while (1) {
      
        pthread_mutex_lock(&(s->mutex));
       
        bool x = (d->request.a.num1 != 0 && d->request.a.num2 != 0 && d->request.a.operator != ' ');
        bool y = (d->request.eo.num!=0);
        bool z = (d->request.p.num!=0);
        if (x || y || z) {
            // Find a client name for this request
            int client_index = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (c[i].is_used && strcmp(c[i].name, d->request.client_name) == 0){
                client_index = i;
                break;
            }
        }

        // If client name not found, find an available index
        if (client_index == -1) {
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (c[i].is_used == 0) {
                    client_index = i;
                    strcpy(c[client_index].name, d->request.client_name);
                    c[client_index].is_used = 1;
                    PRINT_INFO("Client %s is connected\n",c[client_index].name);
                    break;
                }
                 if(c[i].is_used == -1){
                PRINT_INFO("Client %s has disconnected\n",c[i].name);
            }
            }
        }

        // If no available index, reject request
        if (client_index == -1) {
            PRINT_ERROR("Maximum clients reached. Rejecting request.\n");
            r->response.result = -__INT_MAX__;
        } else {
            // Process request and send response
           
            int result;
            if(d->request.a.operator != ' '){
                 printf("Processing arithmetic %d %c %d request from %s\n",d->request.a.num1,d->request.a.operator,d->request.a.num2,d->request.client_name);
            switch (d->request.a.operator) {
                case '+':
                    result = d->request.a.num1 + d->request.a.num2;
                    break;
                case '-':
                    result = d->request.a.num1 - d->request.a.num2;
                    break;
                case '*':
                    result = d->request.a.num1 * d->request.a.num2;
                    break;
                case '/':
                    if(d->request.a.num2 == 0){
                        PRINT_ERROR("Divide by zero, returning zero back to client\n");
                        result = 0;
                    }
                    result = d->request.a.num1 / d->request.a.num2;
                    break;
                default:
                    result = 0;
                    break;
            }
            }
            else if(d->request.eo.num!=0){
                 printf("Processing evenodd %d request from %s\n",d->request.eo.num,d->request.client_name);
                result = (d->request.eo.num & 1) + 1;
            }
            else if(d->request.p.num!=0){
                 printf("Processing prime %d request from %s\n",d->request.p.num,d->request.client_name);
                result = isprime(d->request.p.num);
            }
            r->response.result = result;
        }

        // Reset request channel
          d->request.a.num1 = 0;
    d->request.a.num2 = 0;
    d->request.a.operator = ' ';
    d->request.eo.num = 0;
    d->request.p.num = 0;
 
    memset(d->request.client_name, 0, sizeof(d->request.client_name));
     
    
    }
    
    pthread_mutex_unlock(&(s->mutex));
       printf("Do you want to quit the server?(y/n) ");
        char m;
        scanf("%c",&m);
        if(m == 'y')
          break;
}

// Unmap shared memory
munmap(c, sizeof(ClientInfo) * MAX_CLIENTS);
munmap(s, SHMSIZE);
munmap(d, SHMSIZE);
munmap(r, SHMSIZE);

// Close shared memory descriptors
close(shmc);
close(shmd);
close(shms);
close(shmr);

// Remove shared memory objects
shm_unlink("/client_names");
shm_unlink("/channel_mutex");
shm_unlink("/channel_request");
shm_unlink("/channel_response");

return 0;
}


