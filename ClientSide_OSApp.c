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

int connect_to_server(const char *name)
{
    int shmid = shmget(CONNECT_CHANNEL_KEY, SHM_SEG_SIZE, 0666);
    SharedMemorySegment *connect_channel = (SharedMemorySegment *)shmat(shmid, NULL, 0);

    pthread_mutex_lock(&connect_channel->mutex);
    connect_channel->request.type = ACTION_REGISTER;
    strncpy(connect_channel->request.name, name, MAX_NAME_LENGTH);
    pthread_mutex_unlock(&connect_channel->mutex);

    while (1)
    {
        pthread_mutex_lock(&connect_channel->mutex);
        if (connect_channel->response.type == ACTION_SERVER_RESPONSE)
        {
            int key = connect_channel->response.key;
            connect_channel->response.type = ACTION_NONE; // Reset response type
            pthread_mutex_unlock(&connect_channel->mutex);
            shmdt(connect_channel);
            return key;
        }
        pthread_mutex_unlock(&connect_channel->mutex);
        usleep(1000); // Sleep for 1ms
    }
}

void disconnect_from_server(int *key_ptr)
{
    int shmid = shmget(CONNECT_CHANNEL_KEY, SHM_SEG_SIZE, 0666);
    SharedMemorySegment *connect_channel = (SharedMemorySegment *)shmat(shmid, NULL, 0);

    pthread_mutex_lock(&connect_channel->mutex);
    connect_channel->request.type = ACTION_UNREGISTER;
    connect_channel->request.key = *key_ptr;
    pthread_mutex_unlock(&connect_channel->mutex);

    shmdt(connect_channel);
}

int send_request(int key, Message *request, Message *response)
{
    int shmid = shmget(key, SHM_SEG_SIZE, 0666);
    SharedMemorySegment *comm_channel = (SharedMemorySegment *)shmat(shmid, NULL, 0);

    pthread_mutex_lock(&comm_channel->mutex);
    comm_channel->request = *request;
    pthread_mutex_unlock(&comm_channel->mutex);

    int result = -1;
    while (1)
    {

        pthread_mutex_lock(&comm_channel->mutex);
        if (comm_channel->response.type == ACTION_SERVER_RESPONSE)
        {
            *response = comm_channel->response;
            comm_channel->response.type = ACTION_NONE;
            comm_channel->request.type = ACTION_NONE; // Reset response type
            pthread_mutex_unlock(&comm_channel->mutex);
            shmdt(comm_channel);
            return result;
        }
        pthread_mutex_unlock(&comm_channel->mutex);

        usleep(1000); // Sleep for 1ms
    }
}

void even_odd(int key)
{
    Message request, response;
    request.key = key;

    PRINT_INFO("Enter a number to check if it's even or odd: ");
    char buffer[32];
    fgets(buffer, sizeof(buffer), stdin);
    sscanf(buffer, "%d", &request.arg1);

    request.type = ACTION_CLIENT_REQUEST;
    request.request = REQ_EVEN_OR_ODD;

    PRINT_INFO("Client sends a even_odd request to the server on the comm channel...\n");

    send_request(key, &request, &response);

    PRINT_INFO("Client received a response from the server.\n\n");
    if (response.arg1 == 0)
    {
        PRINT_INFO("The number is even.\n");
    }
    else
    {
        PRINT_INFO("The number is odd.\n");
    }
}

void is_prime(int key)
{
    Message request, response;
    request.key = key;

    PRINT_INFO("Enter a number to check if it's a prime number: ");
    char buffer[32];
    fgets(buffer, sizeof(buffer), stdin);
    sscanf(buffer, "%d", &request.arg1);

    request.type = ACTION_CLIENT_REQUEST;
    request.request = REQ_IS_PRIME;

    PRINT_INFO("Client sends a is_prime request to the server on the comm channel...\n");

    send_request(key, &request, &response);

    PRINT_INFO("Client received a response from the server.\n\n");
    if (response.arg1 == 1)
    {
        PRINT_INFO("The number is prime.\n");
    }
    else if (response.arg1 == 0)
    {
        PRINT_INFO("The number is not prime.\n");
    }
    else
    {
        PRINT_INFO("Please try again.\n");
    }
}

void is_negative(int key)
{
    Message request, response;
    request.key = key;

    request.type = ACTION_CLIENT_REQUEST;
    request.request = REQ_IS_NEGATIVE;

    PRINT_INFO("Client sends a is_negative request to the server on the comm channel...\n");

    send_request(key, &request, &response);

    PRINT_INFO("Client received a response from the server.\n\n");
    if (response.arg1 == 1)
    {
        PRINT_INFO("Is Negative is not supported at the moment.\n");
    }
    else
    {
        PRINT_INFO("Please try again.\n");
    }
}

void arthmetics(int key)
{
    Message request, response;
    request.key = key;

    PRINT_INFO("Enter two numbers and an operator (+, -, *, /) separated by space: ");
    scanf("%d %d %c", &request.arg1, &request.arg2, &request.arg3);

    request.type = ACTION_CLIENT_REQUEST;
    request.request = REQ_ARTHMETICS;

    PRINT_INFO("Client sends a arthmetics request to the server on the comm channel...\n");

    send_request(key, &request, &response);

    PRINT_INFO("Client received a response from the server.\n\n");
    if(response.arg3 == 'I') {
        PRINT_INFO("Invalid Input... Please try again!\n");
    } else {
        PRINT_INFO("%d %c %d = %d\n", request.arg1, request.arg3, request.arg2, response.arg1);
    }
}

int main()
{
    char buffer[64];

    char name[MAX_NAME_LENGTH];
    PRINT_INFO("Enter client name: ");
    fgets(name, MAX_NAME_LENGTH, stdin);
    name[strcspn(name, "\n")] = 0;

    PRINT_INFO("Client making register request to the server on connect channel...\n");
    int key = connect_to_server(name);
    if (key < 0)
    {
        PRINT_INFO("Error connecting to server: %d\n", key);
        return 1;
    }

    int *key_ptr = malloc(sizeof(int));
    *key_ptr = key;

    PRINT_INFO("Client connected to server.\n");

    while (1)
    {
        int option;
        PRINT_INFO("\nMenu:\n");
        PRINT_INFO("1. Arthmetic Operation on two numbers\n");
        PRINT_INFO("2. Check if a number is even or odd\n");
        PRINT_INFO("3. Check if a number is prime or not\n");
        PRINT_INFO("4. Check if a number is negative or not\n");
        PRINT_INFO("5. Unregister and exit\n");
        PRINT_INFO("Enter your option: ");
        scanf("%d", &option);
        getchar(); // Add this line to consume the newline character left by scanf

        switch (option)
        {
        case 1:
            arthmetics(key);
            break;
        case 2:
            even_odd(key);
            break;
        case 3:
            is_prime(key);
            break;
        case 4:
            is_negative(key);
            break;
        case 5:
            PRINT_INFO("Client sends a unregister request to the server on the comm channel...\n");
            disconnect_from_server(key_ptr);
            PRINT_INFO("Disconnected from server.\n");
            free(key_ptr); // Free the allocated memory for key_ptr
            return 0;
        default:
            PRINT_INFO("Invalid option.\n");
            break;
        }
    }

    return 0;
}
