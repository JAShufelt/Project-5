#include "philosopher.h"

int main(int argc, char* argv[])
{
    char message_buffer[BUFFERSIZE];
    int err;
    int connecting_socket;
    int connecting_port;
    bool coordinator = false;
    pthread_t acceptor_thread;
    pthread_mutex_init(&lock,NULL);
    pthread_mutex_init(&coord_philo_lock,NULL);
    pthread_mutex_init(&forks_lock,NULL);
    pthread_mutex_init(&output,NULL);
    pthread_cond_init(&coord_philo_cond, NULL);

    if(argc == 1)
    {
        printf("No home port provided, please provide home port number via command line argument.\n");
        exit(1);
    }
    printf("Arguments = %d\n", argc);

    node_num = atoi(argv[1]);
    int home_port = atoi(argv[1]) + 9000;

    //The port of a connection was provided for connection at execution via argument 3.
    if(argc == 3)
    {
        //Create the connecting socket to connect to successor philosopher
        connecting_port = atoi(argv[2]) + 9000;
        int connection_SD;

        connecting_socket = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in connecting_address;
        connecting_address.sin_family = AF_INET;
        connecting_address.sin_port = htons(connecting_port);
        connecting_address.sin_addr.s_addr = INADDR_ANY;

        int connection_status = connect(connecting_socket, (struct sockaddr *) & connecting_address, sizeof(connecting_address));
        printf("Connection Status = %d\n", connection_status);

        if(connection_status == -1)
        {
            printf("There was an error connecting to port: %d\n", connecting_port);
        }
        else
        {
            char temp[10];
            snprintf(temp,10,"%d",home_port);

            strcpy(message_buffer, temp);
            strcat(message_buffer, ",");
            printf("Connection was established to port %d\n", connecting_port);
            send(connecting_socket,message_buffer, sizeof(message_buffer), 0);
        }
    }

    //Create the serving socket to accept connection from predeccesor philosopher

    //create the server socket
    int serving_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // define the server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(home_port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    //bind the socket to our specified IP and port
    bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));

    //Listen and accept connection
    listen(server_socket, 1);
    serving_socket = accept(server_socket, NULL, NULL);

    //Forward connection message to successor
    int messages_received = 0;
    while(1)
    {
        recv(serving_socket,&message_buffer,sizeof(message_buffer), 0);
        printf("message received: %s\n", message_buffer);

        if((message_buffer[0] == 'A') || (message_buffer[0] == 'B'))
        {
            printf("Received Election Message, and forwarding\n");
            break;
        }

        //If philosopher with a successor
        if(argc == 3)
        {
            char temp[10];
            snprintf(temp,10,"%d",home_port);

            strcat(message_buffer, temp);
            strcat(message_buffer, ",");
            send(connecting_socket,message_buffer, sizeof(message_buffer), 0);
        }
        else
        {
            messages_received++;
            if(messages_received == 4)
                break;
        }
    }

    //If no successor, connect to youngest predecessor and start election.
    if(argc == 2)
    {
        printf("All predecessor philosophers connected\n");
        printf("Attempting to complete chain...\n");

        connecting_port = extract_port(message_buffer, BUFFERSIZE);
        connecting_socket = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in connecting_address;
        connecting_address.sin_family = AF_INET;
        connecting_address.sin_port = htons(connecting_port);
        connecting_address.sin_addr.s_addr = INADDR_ANY;

        int connection_status = connect(connecting_socket, (struct sockaddr *) & connecting_address, sizeof(connecting_address));

        srand(time(NULL));
        vector<int> ids;

        //Generate 5 unique IDs between 0 and 1000
        bool valid = false;
        while(!valid)
        {
            for(int i = 0; i < 5; i++)
            {
                int temp_int = (rand() % 999) + 1;
                ids.push_back(temp_int);
            }

            valid = true;
            for(int i = 0; i < ids.size(); i++)
            {
                for(int j = 0; j < ids.size(); j++)
                {
                    if(ids[i] == ids[j] && (j != i))
                    {
                        valid = false;
                    }
                }
            }
        }

        memset(message_buffer, 0, sizeof(message_buffer));

        //Storage for character representation of IDs
        char all_ids[BUFFERSIZE];
        char ids_0[5];
        char ids_1[5];
        char ids_2[5];
        char ids_3[5];
        char ids_4[5];

        //Convert integer IDs to characters
        snprintf(ids_0,10,"%d",ids[0]);
        snprintf(ids_1,10,"%d",ids[1]);
        snprintf(ids_2,10,"%d",ids[2]);
        snprintf(ids_3,10,"%d",ids[3]);
        snprintf(ids_4,10,"%d",ids[4]);

        //Concatenate all character IDs to one message.
        strcpy(all_ids, ids_0);
        strcat(all_ids, ",");
        strcat(all_ids, ids_1);
        strcat(all_ids, ",");
        strcat(all_ids, ids_2);
        strcat(all_ids, ",");
        strcat(all_ids, ids_3);
        strcat(all_ids, ",");
        strcat(all_ids, ids_4);
        strcat(all_ids, ",");

        strcpy(message_buffer, "F");
        strcat(message_buffer, all_ids);

        is_coordinator(coordinator,atoi(argv[1]),message_buffer, BUFFERSIZE);
        if(coordinator)
        {
            coordinator = true;
            message_buffer[0] = 'B';
        }
        else
        {
            message_buffer[0] = 'A';
        }

        printf("Connection was established to port %d\n", connecting_port);
        send(connecting_socket,message_buffer, sizeof(message_buffer), 0);
        printf("All philosophers connected, starting coordinator election.\n");
    }

    is_coordinator(coordinator,atoi(argv[1]),message_buffer,BUFFERSIZE);

    //If not philosopher who started election.
    if(argc == 3)
    {
        phil_id = extract_id(atoi(argv[1]),message_buffer,BUFFERSIZE);
        printf("Philosopher ID: %d\n", phil_id);
        printf("Received First Election Request...\n");
        printf("%s\n", message_buffer);

        if(message_buffer[0] == 'A')
        {
            if(coordinator)
            {
                printf("I am the coordinator :)\n");
                pthread_create(&acceptor_thread,NULL,&coordinatorAccept,(void*)&server_socket);
                message_buffer[0] = 'B';
                send(connecting_socket,message_buffer, sizeof(message_buffer), 0);
                recv(serving_socket,&message_buffer,sizeof(message_buffer), 0);
            }
            else
            {
                printf("Forwarding election message.\n");
                send(connecting_socket,message_buffer, sizeof(message_buffer), 0);
                recv(serving_socket,&message_buffer,sizeof(message_buffer), 0);
                printf("The coordinator has been established, connecting.\n");
                send(connecting_socket,message_buffer, sizeof(message_buffer), 0);
                int coord_port = extract_coordinatorPort(message_buffer,BUFFERSIZE);
                connectCoordinator(coord_port);
            }
        }
        else if(message_buffer[0] == 'B')
        {
            if(coordinator)
            {
                printf("WEIRD**Everyone know's I'm the coordinator! Done. **WEIRD\n");
                pthread_create(&acceptor_thread,NULL,&coordinatorAccept,(void*)&server_socket);
            }
            else
            {
                printf("The coordinator has been established, connecting.\n");
                send(connecting_socket,message_buffer, sizeof(message_buffer), 0);
                int coord_port = extract_coordinatorPort(message_buffer,BUFFERSIZE);
                connectCoordinator(coord_port);
            }
        }
    }
    //If philosopher who started election.
    else
    {
        phil_id = extract_id(atoi(argv[1]),message_buffer,BUFFERSIZE);
        printf("Philosopher ID: %d\n", phil_id);
        printf("Sent Initial Election Message...\n");
        printf("%s\n", message_buffer);

        if(coordinator)
        {
            printf("I am the coordinator :)\n");
            pthread_create(&acceptor_thread,NULL,&coordinatorAccept,(void*)&server_socket);
            recv(serving_socket,&message_buffer,sizeof(message_buffer), 0);
        }
        else
        {
            recv(serving_socket,&message_buffer,sizeof(message_buffer), 0);
            printf("The coordinator has been established, connecting.\n");
            send(connecting_socket,message_buffer, sizeof(message_buffer), 0);
            int coord_port = extract_coordinatorPort(message_buffer,BUFFERSIZE);
            connectCoordinator(coord_port);
        }
    }


    sleep(RUNDUR);
    return 0;
}

int extract_port(char* buffer, int buffer_size)
{
    char temp[buffer_size];

    for(int i = 0; i < buffer_size; i++)
    {
        if(buffer[i] != ',')
        {
            temp[i] = buffer[i];
        }
        else
        {
            temp[i] = '\0';
            break;
        }
    }

    return atoi(temp);
}

void is_coordinator(bool &coordinator,int home_port_num, char* buffer, int buffer_size)
{
    int return_int = 0;
    int largest_int = -1;
    vector<int> ids;
    char temp[5];

    int j = 0;
    for(int i = 1; i < strlen(buffer); i++)
    {
        if(buffer[i] != ',')
        {
            temp[j] = buffer[i];
            j++;
        }
        else
        {
            temp[j] = '\0';
            ids.push_back(atoi(temp));
            j = 0;
        }
    }

    for(int i = 0; i < ids.size(); i++)
    {
        if(ids[i] > largest_int)
        {
            largest_int = ids[i];
        }
    }

    if(ids[home_port_num] == largest_int)
    {
        coordinator = true;
    }
    else
    {
        coordinator = false;
    }
}

int extract_id(int home_port_num, char* buffer, int buffer_size)
{  
    int j = 0;
    char temp[5];
    vector<int> ids;

    for(int i = 1; i < strlen(buffer); i++)
    {
        if(buffer[i] != ',')
        {
            temp[j] = buffer[i];
            j++;
        }
        else
        {
            temp[j] = '\0';
            ids.push_back(atoi(temp));
            j = 0;
        }
    }

    return ids[home_port_num];
}

int extract_coordinatorPort(char* buffer, int buffer_size)
{
    int element_num = 0;
    int largest_int = -1;
    vector<int> ids;
    char temp[5];

    int j = 0;
    for(int i = 1; i < strlen(buffer); i++)
    {
        if(buffer[i] != ',')
        {
            temp[j] = buffer[i];
            j++;
        }
        else
        {
            temp[j] = '\0';
            ids.push_back(atoi(temp));
            j = 0;
        }
    }

    for(int i = 0; i < ids.size(); i++)
    {
        if(ids[i] > largest_int)
        {
            largest_int = ids[i];
            element_num = i;
        }
    }

    return (element_num + 9000);

}

//Run by philosophers when connecting to the coordinator
void connectCoordinator(int coordinator_port)
{
    int connecting_port;
    int connecting_socket;

    char temp[5];
    char message_buffer[BUFFERSIZE];
    snprintf(temp,10,"%d",phil_id);
    strcpy(message_buffer, "Philospher ID: ");
    strcat(message_buffer, temp);
    strcat(message_buffer, " has connected.");


    //Create the connecting socket to connect to successor philosopher
    connecting_port = coordinator_port;
    int connection_SD;

    connecting_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in connecting_address;
    connecting_address.sin_family = AF_INET;
    connecting_address.sin_port = htons(connecting_port);
    connecting_address.sin_addr.s_addr = INADDR_ANY;

    sleep(1);
    int connection_status = connect(connecting_socket, (struct sockaddr *) & connecting_address, sizeof(connecting_address));

    if(connection_status == -1)
    {
        pthread_mutex_lock(&output);
        printf("Issue encountered attempting connection to the Coordinator.\n");
        pthread_mutex_unlock(&output);
    }
    else
    {
        recv(connecting_socket,message_buffer,sizeof(message_buffer), 0);
        printf("%s\n", message_buffer);
        philosopherRoutine(connecting_socket);
    }
}

//Run by the coordinator to accept incoming philosopher connections
void* coordinatorAccept(void* server_socket)
{
    int SD[10];
    int temp;
    pipe(fd);

    int server_sock = *(int*)server_socket;
    char message_buffer[BUFFERSIZE];
    strcpy(message_buffer, "Message from coordinator\n");

    //Establish pipe towards main thread.
    pipe(fd);

    pthread_mutex_lock(&output);
    printf("Coordinator Accept Thread started.\n");
    pthread_mutex_unlock(&output);

    pthread_t threads[6];

    pthread_t new_thread0;
    threads[0] = new_thread0;
    pthread_t new_thread1;
    threads[1] = new_thread1;
    pthread_t new_thread2;
    threads[2] = new_thread2;
    pthread_t new_thread3;
    threads[3] = new_thread3;
    pthread_t new_thread4;
    threads[4] = new_thread4;
    pthread_t new_thread5;

    for(int i = 0; i < 4; i++)
    {
        temp = accept(server_sock, NULL, NULL);
        send(temp,message_buffer,sizeof(message_buffer),0);
        SD[i] = temp;
        pthread_create(&threads[i],NULL,&philosopher_handler,(void*)&SD[i]);

        if(temp == -1)
        {
            pthread_mutex_lock(&output);
            printf("FAILED TO ACCEPT CONNECTION.");
            pthread_mutex_unlock(&output);
            exit(3);
        }
    }

    pthread_create(&threads[4],NULL,&coordinatorPhilosopherRoutine,NULL);

    coordinatorRoutine();
}

void* philosopher_handler(void* nothing)
{
    int left_fork;
    int right_fork;
    int philo_SD = *(int*)nothing;
    char begin_message[BUFFERSIZE];
    strcpy(begin_message,"Begin philosopher routine.");

    char message_buffer[BUFFERSIZE];
    strcpy(message_buffer, "DEFAULT VALUE");

    char temp[11];
    snprintf(temp,10,"%d",philo_SD);

    //Receive connection confirmation from philosopher
    recv(philo_SD,message_buffer,BUFFERSIZE, 0);
    if(message_buffer[0] == 'D')
    {
        message_buffer[0] = 'F';
        send(philo_SD, message_buffer,sizeof(message_buffer), 0);
    }

    pthread_mutex_lock(&lock);
    pthread_mutex_lock(&output);
    fflush(stdout);
    printf("|COORDINATOR| Connection handling subthread created...\n");
    printf("%s\n", message_buffer);
    pthread_mutex_unlock(&output);
    pthread_mutex_unlock(&lock);

    //Send begin message to philosopher
    send(philo_SD, begin_message,sizeof(begin_message),0);
    while(1)
    {
        //Receive eating request
        recv(philo_SD,message_buffer,BUFFERSIZE,0);
        strcat(message_buffer,temp);
        strcat(message_buffer,",");

        //Print eating request
        pthread_mutex_lock(&output);
        printf("|COORDINATOR| RECEIVED REQUEST: %s\n", message_buffer);
        pthread_mutex_unlock(&output);

        //Forward eating request to main thread
        write(fd[1],message_buffer,sizeof(message_buffer));

        //Receive deallocation request
        recv(philo_SD,message_buffer,BUFFERSIZE,0);
        left_fork = calcLeftFork(message_buffer,sizeof(message_buffer));
        right_fork = calcRightFork(message_buffer, sizeof(message_buffer));

        //Print deallocation request
        pthread_mutex_lock(&output);
        printf("|COORDINATOR| Deallocating, LEFT: %d RIGHT: %d\n", left_fork, right_fork);
        pthread_mutex_unlock(&output);

        pthread_mutex_lock(&forks_lock);
        forks[left_fork] = 1;
        forks[right_fork] = 1;
        pthread_mutex_unlock(&forks_lock);
    }

}

void* philosopherRoutine(int connecting_socket)
{
    int left_fork;
    int right_fork;

    char temp[10];
    char sd[11];
    char left[10];
    char right[10];

    char begin_message[BUFFERSIZE];
    char message_buffer[BUFFERSIZE];
    char request_message[BUFFERSIZE];
    char deallocate_message[BUFFERSIZE];

    snprintf(temp,10,"%d",phil_id);
    snprintf(sd,10,"%d",connecting_socket);

    strcpy(message_buffer, "Philospher ID: ");
    strcat(message_buffer, temp);
    strcat(message_buffer, " has connected.");

    printf("Connection to Coordinator was Success!\n");
    printf("Sending message: %s\n", message_buffer);
    send(connecting_socket,message_buffer, sizeof(message_buffer), 0);

    //Thinking and eating duration randomly set between 2 and 9 seconds for each philosopher.
    srand(phil_id);
    int think_duration = 2 + (rand() % 8);
    int eat_duration = 2 + (rand() % 8);

    //Assign Forks
    left_fork = node_num;   //Home port number
    if((left_fork - 1) == -1)
    {
        right_fork = 4;
    }
    else
    {
        right_fork = left_fork - 1;
    }

    snprintf(left,10,"%d",left_fork);
    snprintf(right,10,"%d",right_fork);

    //Construct requesting message
    memset(request_message,0,sizeof(request_message));
    strcpy(request_message, "R");
    strcat(request_message,temp);
    strcat(request_message, ",");
    strcat(request_message,sd);
    strcat(request_message, ",");
    strcat(request_message,left);
    strcat(request_message, ",");
    strcat(request_message,right);
    strcat(request_message, ",");

    //Construct deallocate message
    memset(deallocate_message,0,sizeof(deallocate_message));
    strcpy(deallocate_message,request_message);
    deallocate_message[0] = 'D';

    recv(connecting_socket,&begin_message,sizeof(begin_message),0);
    printf("BEGIN MESSAGE: %s", begin_message);

    int a = 1;
    while(a == 1)
    {
        printf("Begin thinking for %d seconds\n", think_duration);
        sleep(think_duration);
        printf("Done thinking\n");

        printf("Requesting left fork\n");
        printf("Requesting right fork\n");

        //Send fork/eat request to monitoring thread.
        send(connecting_socket,request_message,sizeof(request_message), 0);

        //Receive fork permission from main thread.
        recv(connecting_socket,&message_buffer,sizeof(message_buffer), 0);

        printf("Left fork acquired\n");
        printf("Right fork acquired\n");
        printf("Begin eating for %d seconds\n", eat_duration);
        sleep(eat_duration);
        printf("Done eating\n");

        //Send deallocation request to monitoring thread.
        send(connecting_socket,deallocate_message,sizeof(deallocate_message), 0);
    }
}

void* coordinatorPhilosopherRoutine(void* nothing)
{
    int left_fork;
    int right_fork;

    char temp[10];
    char sd[11];
    char left[10];
    char right[10];
    char request_message[BUFFERSIZE];

    snprintf(temp,10,"%d",phil_id);
    strcpy(sd, "-1");

    //Sleeping and eating duration randomly set between 2 and 9 seconds for each philosopher.
    srand(phil_id);
    int think_duration = 2 + (rand() % 8);
    int eat_duration = 2 + (rand() % 8);

    //Assign Forks
    left_fork = node_num;  //Home port number
    if((left_fork - 1) == -1)
    {
        right_fork = 4;
    }
    else
    {
        right_fork = left_fork - 1;
    }

    snprintf(left,10,"%d",left_fork);
    snprintf(right,10,"%d",right_fork);

    //Construct request message.
    memset(request_message,0,sizeof(request_message));
    strcpy(request_message, "R");
    strcat(request_message,temp);
    strcat(request_message, ",");
    strcat(request_message,sd);
    strcat(request_message, ",");
    strcat(request_message,left);
    strcat(request_message, ",");
    strcat(request_message,right);
    strcat(request_message, ",");
    strcat(request_message, sd);
    strcat(request_message, ",");

    int a = 1;
    while(a == 1)
    {
        pthread_mutex_lock(&output);
        printf("Begin thinking for %d seconds\n", think_duration);
        pthread_mutex_unlock(&output);

        sleep(think_duration);
        pthread_mutex_lock(&output);
        printf("Done thinking\n");
        pthread_mutex_unlock(&output);

        pthread_mutex_lock(&output);
        printf("Requesting left fork\n");
        printf("Requesting right fork\n");
        printf("|COORDINATOR| RECEIVED REQUEST: %s\n", request_message);
        pthread_mutex_unlock(&output);

        write(fd[1],request_message,sizeof(request_message));

        pthread_cond_wait(&coord_philo_cond,&coord_philo_lock); //Wait on signal from Coordinator thread
        pthread_mutex_unlock(&coord_philo_lock);

        pthread_mutex_lock(&output);
        printf("Left fork acquired\n");
        printf("Right fork acquired\n");
        printf("Begin eating for %d seconds\n", eat_duration);
        pthread_mutex_unlock(&output);

        sleep(eat_duration);

        pthread_mutex_lock(&output);
        printf("Done eating\n");
        pthread_mutex_unlock(&output);

        //Deallocate resources
        pthread_mutex_lock(&forks_lock);
        forks[left_fork] = 1;
        forks[right_fork] = 1;
        pthread_mutex_unlock(&forks_lock);

    }

}

void coordinatorRoutine()
{
    int requested_left;
    int requested_right;
    int requested_SD;
    int requested_ID;
    char message_buffer[BUFFERSIZE];
    char grant_request[BUFFERSIZE];

    memset(message_buffer,0,sizeof(message_buffer));

    memset(grant_request,0,sizeof(grant_request));
    strcpy(grant_request, "Eating request granted...\n");

    for(int i = 0; i < 5; i++)
    {
        forks.push_back(1);
    }

    while(1)
    {
        read(fd[0],message_buffer,sizeof(message_buffer));

        pthread_mutex_lock(&output);
        printf("|COORDINATOR| HANDLING REQUEST: %s\n", message_buffer);
        requested_ID = calcPhiloID(message_buffer, sizeof(message_buffer));

        requested_left = calcLeftFork(message_buffer, sizeof(message_buffer));

        requested_right = calcRightFork(message_buffer, sizeof(message_buffer));

        requested_SD = calcSocketDescriptor(message_buffer, sizeof(message_buffer));

        pthread_mutex_unlock(&output);

        while(1)
        {
            //If request came from another process
            if(requested_SD != -1)
            {
                if((forks[requested_left] == 1) && (forks[requested_right] == 1))
                {
                    pthread_mutex_lock(&forks_lock);
                    forks[requested_left] = 0;
                    forks[requested_right] = 0;
                    pthread_mutex_unlock(&forks_lock);

                    //Send request grant to requesting process
                    send(requested_SD,grant_request,sizeof(grant_request),0);
                    pthread_mutex_lock(&output);
                    printf("PHIL ID: %d, may begin eating.\n", requested_ID);
                    printf("ALLOCATING LEFT FORK: %d RIGHT FORK: %d\n", requested_left, requested_right);
                    pthread_mutex_unlock(&output);
                    break;
                }
            }
            //If request came from this process (philosopher thread)
            else
            {
                if((forks[requested_left] == 1) && (forks[requested_right] == 1))
                {
                    pthread_mutex_lock(&forks_lock);
                    forks[requested_left] = 0;
                    forks[requested_right] = 0;
                    pthread_mutex_unlock(&forks_lock);

                    pthread_mutex_lock(&coord_philo_lock);
                    pthread_cond_broadcast(&coord_philo_cond);
                    pthread_mutex_unlock(&coord_philo_lock);
                    break;
                }
            }
        }
    }
}

int calcLeftFork(char* message_buffer, int buffer_size)
{
    int left_fork;
    int start_index = 0;
    int comma_count = 0;
    char temp[11];

    //Find start index of second comma
    for(int i = 0; i < buffer_size; i++)
    {
        if(message_buffer[i] == ',')
        {
            comma_count++;
            if(comma_count == 2)
            {
                start_index = i + 1;
                break;
            }
        }
    }

    int j = 0;
    for(int i = start_index; i < buffer_size; i++)
    {
        if(message_buffer[i] != ',')
        {
            temp[j] = message_buffer[i];
            j++;
        }
        else
        {
            temp[j] = '\0';
            break;
        }
    }

    return atoi(temp);
}

int calcRightFork(char* message_buffer, int buffer_size)
{
    int right_fork;
    int start_index = 0;
    int comma_count = 0;
    char temp[11];

    //Find start index of second comma
    for(int i = 0; i < buffer_size; i++)
    {
        if(message_buffer[i] == ',')
        {
            comma_count++;
            if(comma_count == 3)
            {
                start_index = i + 1;
                break;
            }
        }
    }

    int j = 0;
    for(int i = start_index; i < buffer_size; i++)
    {
        if(message_buffer[i] != ',')
        {
            temp[j] = message_buffer[i];
            j++;
        }
        else
        {
            temp[j] = '\0';
            break;
        }
    }

    return atoi(temp);
}

int calcSocketDescriptor(char* message_buffer, int buffer_size)
{
    int socket_descriptor;
    int start_index = 0;
    int comma_count = 0;
    char temp[11];

    //Find start index of second comma
    for(int i = 0; i < buffer_size; i++)
    {
        if(message_buffer[i] == ',')
        {
            comma_count++;
            if(comma_count == 4)
            {
                start_index = i + 1;
                break;
            }
        }
    }

    int j = 0;
    for(int i = start_index; i < buffer_size; i++)
    {
        if(message_buffer[i] != ',')
        {
            temp[j] = message_buffer[i];
            j++;
        }
        else
        {
            temp[j] = '\0';
            break;
        }
    }

    return atoi(temp);
}

int calcPhiloID(char* message_buffer, int buffer_size)
{
    int philo_id;
    int start_index = 1;
    int comma_count = 0;
    char temp[11];

    int j = 0;
    for(int i = start_index; i < buffer_size; i++)
    {
        if(message_buffer[i] != ',')
        {
            temp[j] = message_buffer[i];
            j++;
        }
        else
        {
            temp[j] = '\0';
            break;
        }
    }

    return atoi(temp);
}
