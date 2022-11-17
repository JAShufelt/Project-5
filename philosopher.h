#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <vector>

#ifndef BUFFERSIZE
#define BUFFERSIZE 100
#endif // BUFFERSIZE

#ifndef RUNDUR
#define RUNDUR 99999999
#endif // RUNDUR

using namespace std;

int extract_port(char* buffer, int buffer_size);

void is_coordinator(bool &coordinator,int home_port_num, char* buffer, int buffer_size);

int extract_id(int home_port_num, char* buffer, int buffer_size);

void* coordinatorAccept(void* server_socket);

void* philosopher_handler(void* nothing);

void connectCoordinator(int coordinator_port);

int extract_coordinatorPort(char* buffer, int buffer_size);

void* philosopherRoutine(int connecting_socket);

void* coordinatorPhilosopherRoutine(void* nothing);

int calcLeftFork(char* message_buffer, int buffer_size);

int calcRightFork(char* message_buffer, int buffer_size);

int calcSocketDescriptor(char* message_buffer, int buffer_size);

int calcPhiloID(char* message_buffer, int buffer_size);

vector<int> philo_SDs;

vector<int> forks;

int fd[2];

int node_num;

pthread_mutex_t lock;
pthread_mutex_t coord_philo_lock;
pthread_mutex_t forks_lock;
pthread_mutex_t output;
pthread_cond_t coord_philo_cond;

int phil_id;

//Request requestTranslator(char* message_buffer, int buffer_size);

void coordinatorRoutine();

int server_socket;
