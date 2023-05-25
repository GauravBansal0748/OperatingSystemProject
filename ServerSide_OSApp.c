// Raghav Gupta 2019B4A30927H
// Samyak Paharia 2020A8PS1824H
// Yash Saravgi 2019B2A31530H
// Anish Guruvelli 2020A2PS2418H
// Kashish 2019B2A31550H
// Zakia Firdous Munshi 2020A1PS2385H
// Adrian Jagnania 2019B5AA1498H
// Rahul Sahu 2019B4A30852H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "CSF372.h"

#define CONNECT_CHANNEL_KEY 1234
#define MAX_CLIENTS 100
#define MAX_NAME_LENGTH 32
#define SHM_SEG_SIZE 128
#define SUCCESS 0
#define ERR_NAME_NOT_UNIQUE -1
#define ERR_INVALID_KEY -2

typedef enum
{
    ACTION_REGISTER,
    ACTION_UNREGISTER,
    ACTION_CLIENT_REQUEST,
    ACTION_SERVER_RESPONSE,
    ACTION_NONE
} ActionType;

typedef enum
{
    REQ_EVEN_OR_ODD,
    REQ_ARTHMETICS,
    REQ_IS_PRIME,
    REQ_IS_NEGATIVE
} ClientRequestType;

typedef struct
{
    ActionType type;
    char name[MAX_NAME_LENGTH];
    int key;
    ClientRequestType request;
    int arg1;
    int arg2;
    char arg3;
} Message;

typedef struct
{
    pthread_mutex_t mutex;
    Message request;
    Message response;
} SharedMemorySegment;

typedef struct
{
    char name[MAX_NAME_LENGTH];
    int key;
    int shmid;
    int request_count;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t client_list_mutex = PTHREAD_MUTEX_INITIALIZER;

int generate_key()
{
    static int key = CONNECT_CHANNEL_KEY + 1;
    return key++;
}

int find_client_by_name(const char *name)
{
    for (int i = 0; i < client_count; i++)
    {
        if (strcmp(clients[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

void print_summary(int key)
{
    int total_requests = 0;
    PRINT_INFO("***SUMMARY***\n\n");
    for (int i = 0; i < client_count; i++)
    {
        total_requests += clients[i].request_count;
        PRINT_INFO("Name: %s | Request Count: %d \n", clients[i].name, clients[i].request_count);
    }
    PRINT_INFO("Total Request Count: %d \n", total_requests);
}

int find_client_by_key(int key)
{
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].key == key)
        {
            return i;
        }
    }
    return -1;
}

int register_client(const char *name)
{
    pthread_mutex_lock(&client_list_mutex);

    if (find_client_by_name(name) >= 0)
    {
        pthread_mutex_unlock(&client_list_mutex);
        return ERR_NAME_NOT_UNIQUE;
    }

    int key = generate_key();
    int shmid = shmget(key, SHM_SEG_SIZE, IPC_CREAT | 0666);
    if (shmid < 0)
    {
        perror("shmget");
        exit(1);
    }

    SharedMemorySegment *segment = (SharedMemorySegment *)shmat(shmid, NULL, 0);
    if (segment == (SharedMemorySegment *)(-1))
    {
        perror("shmat");
        exit(1);
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&segment->mutex, &attr);

    strncpy(clients[client_count].name, name, MAX_NAME_LENGTH);
    clients[client_count].key = key;
    clients[client_count].shmid = shmid;
    clients[client_count].request_count = 0;
    client_count++;

    pthread_mutex_unlock(&client_list_mutex);

    return key;
}

void unregister_client(int key)
{
    pthread_mutex_lock(&client_list_mutex);
    int index = find_client_by_key(key);
    if (index >= 0)
    {
        int shmid = clients[index].shmid;
        SharedMemorySegment *segment = (SharedMemorySegment *)shmat(shmid, NULL, 0);

        pthread_mutex_destroy(&segment->mutex);
        shmdt(segment);
        shmctl(shmid, IPC_RMID, NULL);

        for (int i = index; i < client_count - 1; i++)
        {
            clients[i] = clients[i + 1];
        }
        client_count--;
        PRINT_INFO("Server successfully unregistered client with key: %d\n", key);
    }

    pthread_mutex_unlock(&client_list_mutex);
}

void process_request(int key, Message *request, Message *response)
{
    int client_index = find_client_by_key(key);
    if (client_index < 0)
    {
        response->type = ACTION_SERVER_RESPONSE;
        response->key = ERR_INVALID_KEY;
        return;
    }

    clients[client_index].request_count++;

    response->type = ACTION_SERVER_RESPONSE;
    response->key = SUCCESS;

    switch (request->request)
    {
    case REQ_ARTHMETICS:
        response->arg3 = 'V';
        if (request->arg3 == '+')
        {
            response->arg1 = request->arg1 + request->arg2;
        }
        else if (request->arg3 == '-')
        {
            response->arg1 = request->arg1 - request->arg2;
        }
        else if (request->arg3 == '/')
        {
            response->arg1 = request->arg1 / request->arg2;
        }
        else if (request->arg3 == '*')
        {
            response->arg1 = request->arg1 * request->arg2;
        } else {
            response->arg3 = 'I';
        }
        break;
    case REQ_IS_PRIME:
    {
        int n = request->arg1;
        int is_prime = 1;
        if (n < 2)
        {
            is_prime = 0;
        }
        else
        {
            for (int i = 2; i * i <= n; i++)
            {
                if (n % i == 0)
                {
                    is_prime = 0;
                    break;
                }
            }
        }
        response->arg1 = is_prime;
        break;
    }

    case REQ_EVEN_OR_ODD:
        response->arg1 = request->arg1 % 2;
        break;
    case REQ_IS_NEGATIVE:
        response->arg1 = 1;
        break;
    default:
        response->key = -1;
        break;
    }
}

void *client_thread(void *arg)
{
    int key = *((int *)arg);
    free(arg);

    while (1)
    {
        int client_index = find_client_by_key(key);
        if (client_index < 0)
        {
            break;
        }

        SharedMemorySegment *segment = (SharedMemorySegment *)shmat(clients[client_index].shmid, NULL, 0);

        pthread_mutex_lock(&segment->mutex);

        if (segment->request.type == ACTION_CLIENT_REQUEST)
        {
            PRINT_INFO("Server received a service request on the comm channel from client: %d.\n", key);
            process_request(key, &segment->request, &segment->response);
            PRINT_INFO("Server responded to the client on the comm channel.\n");
            segment->request.type = ACTION_NONE; // Reset request type
        }

        pthread_mutex_unlock(&segment->mutex);

        shmdt(segment);
        usleep(10000); // Sleep for 10ms
    }

    return NULL;
}

void handle_client(const char *name, int *key_ptr)
{
    pthread_t tid;
    pthread_create(&tid, NULL, client_thread, (void *)key_ptr);
}

int main()
{
    int shmid = shmget(CONNECT_CHANNEL_KEY, SHM_SEG_SIZE, IPC_CREAT | 0666);
    if (shmid < 0)
    {
        perror("shmget");
        exit(1);
    }

    SharedMemorySegment *connect_channel = (SharedMemorySegment *)shmat(shmid, NULL, 0);
    if (connect_channel == (SharedMemorySegment *)(-1))
    {
        perror("shmat");
        exit(1);
    }
    else
    {
        PRINT_INFO("Server initiated and created the connect channel. \n");
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&connect_channel->mutex, &attr);

    while (1)
    {
        pthread_mutex_lock(&connect_channel->mutex);
        if (connect_channel->request.type == ACTION_REGISTER)
        {
            PRINT_INFO("Server received a register request. \n");
            int key = register_client(connect_channel->request.name);
            connect_channel->response.type = ACTION_SERVER_RESPONSE;
            connect_channel->response.key = key;
            connect_channel->request.type = ACTION_NONE; // Reset request type

            if (key >= 0)
            {
                PRINT_INFO("Successful creation of comm channel for the client with key: %d\n", key);
                int *key_ptr = malloc(sizeof(int));
                *key_ptr = key;
                handle_client(connect_channel->request.name, key_ptr);
                PRINT_INFO("Successful response made to the client's register request.\n");
            }
        }
        else if (connect_channel->request.type == ACTION_UNREGISTER)
        {
            PRINT_INFO("Server received a unregister request. \n");
            print_summary(connect_channel->request.key);
            unregister_client(connect_channel->request.key);
            connect_channel->request.type = ACTION_NONE; // Reset request type
        }

        pthread_mutex_unlock(&connect_channel->mutex);
        usleep(10000); // Sleep for 10ms
    }

    pthread_mutex_destroy(&connect_channel->mutex);
    shmdt(connect_channel);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}