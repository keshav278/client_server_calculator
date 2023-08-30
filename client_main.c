






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

int main() {
    int shmd, shms, shmr, shmc;
    Channel *d, *s, *r;
    ClientInfo *c;
    pthread_mutexattr_t attr;

    // Connect to shared memory for client names
    shmc = shm_open("/client_names", O_RDWR, 0666);
    c = (ClientInfo *) mmap(NULL, sizeof(ClientInfo) * MAX_CLIENTS, PROT_READ | PROT_WRITE, MAP_SHARED, shmc, 0);

    // Connect to shared memory for mutex
    shmd = shm_open("/channel_mutex", O_RDWR, 0666);
    s = (Channel *) mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmd, 0);

    // Connect to shared memory for request channel
    shms = shm_open("/channel_request", O_RDWR, 0666);
    d = (Channel *) mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shms, 0);

    // Connect to shared memory for response channel
    shmr = shm_open("/channel_response", O_RDWR, 0666);
    r = (Channel *) mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmr, 0);

    // Get client name from user
    char client_name[20];
    PRINT_INFO("Enter client name: ");
    scanf("%s", client_name);

    // Send request to server
    int choice = 0;
    do {
        // Display menu
        printf("\nSelect an option:\n");
        printf("1. Arithmetic\n");
        printf("2. Even or Odd\n");
        printf("3.Prime Number\n");
        printf("4.Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: {
                // Send request to server
                PRINT_INFO("Enter expression: ");
                int num1, num2;
                char operator;
                scanf("%d %c %d", &num1, &operator, &num2);

                pthread_mutex_lock(&(s->mutex));

                d->request.a.num1 = num1;
                d->request.a.num2 = num2;
                d->request.a.operator = operator;
                strcpy(d->request.client_name, client_name);

                pthread_mutex_unlock(&(s->mutex));

                // Wait for response from server
                while (r->response.result == 0) {
                    usleep(1000);
                }

                // Print response
                if (r->response.result == -__INT_MAX__) {
                    PRINT_ERROR("Request rejected. Maximum clients reached.\n");
                }else if(r->response.result == -__INT_MAX__ + 1){
                    PRINT_ERROR("Cannot divide by zero, send another request.\n");
                    r->response.result=0;
                } else {
                    printf("Result: %d\n", r->response.result);
                    r->response.result = 0;
                }

                break;
            }
            case 2: {
                PRINT_INFO("Enter a number: ");
                int num;
                scanf("%d",&num);
                 pthread_mutex_lock(&(s->mutex));

                d->request.eo.num = num;
                strcpy(d->request.client_name, client_name);

                pthread_mutex_unlock(&(s->mutex));

                // Wait for response from server
                while (r->response.result == 0) {
                    usleep(1000);
                }

                // Print response
                if (r->response.result == -__INT_MAX__) {
                    PRINT_ERROR("Request rejected. Maximum clients reached.\n");
                } else {
                    if(r->response.result == 1){
                        PRINT_INFO("Number is even\n");
                    }
                    else{
                         PRINT_INFO("Number is odd\n");
                    }
                    r->response.result = 0;
                }

                break;

            }
            case 3:{
                 printf("Enter a number: ");
                int num;
                scanf("%d",&num);
                 pthread_mutex_lock(&(s->mutex));

                d->request.p.num = num;
                strcpy(d->request.client_name, client_name);

                pthread_mutex_unlock(&(s->mutex));

                // Wait for response from server
                while (r->response.result == 0) {
                    usleep(1000);
                }

                // Print response
                if (r->response.result == -__INT_MAX__) {
                    PRINT_ERROR("Request rejected. Maximum clients reached.\n");
                } else {
                    if(r->response.result == 1){
                        PRINT_INFO("Number is not prime\n");
                    }
                    else{
                         PRINT_INFO("Number is prime\n");
                    }
                    r->response.result = 0;
                }

                break;

            }
            case 4: {
                // Exit program
                 pthread_mutex_lock(&(s->mutex));
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (c[i].is_used && strcmp(c[i].name, client_name) == 0){
            c[i].is_used = -1;
            break;
        }
    }
    pthread_mutex_unlock(&(s->mutex));
                PRINT_INFO("Exiting...\n");
                break;
            }
            default: {
                printf("Invalid choice. Please try again.\n");
                break;
            }
        }
    } while (choice != 4);
   

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

return 0;
}







