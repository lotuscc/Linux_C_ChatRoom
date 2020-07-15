#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#define PORT 33333

struct Message 
{
    int type;
    char name[20];
    char msg[1024];
};

struct UserOnlineNode
{
    char name[20];
    int sockfd;
    struct UserOnlineNode* next;
};

//typedef struct UserOnlineNode* PUserOnlineNode;

struct UserOnlineNode Head;


void Insert_online(struct UserOnlineNode* Head, struct UserOnlineNode* new){

    Head->sockfd++;

    new->next = Head->next;
    Head->next = new;
}


struct UserOnlineNode* Find_online_byname(struct UserOnlineNode* Head, char* name){

    Head = Head->next;
    
    while (Head != NULL){
        if(strcmp(Head->name, name) == 0){
            return Head;
        }
        Head = Head->next;
    }
    
    return NULL;
}

struct UserOnlineNode* Find_online_bysockfd(struct UserOnlineNode* Head, int sockfd){

    Head = Head->next;
    
    while (Head != NULL){
        if(Head->sockfd == sockfd){
            return Head;
        }
        Head = Head->next;
    }
    
    return NULL;
}


void* send_message(void* arg){

    int sockfd = *((int*)arg);

    int ret;

    while (1)
    {

        // send会引发异常，出现 Program received signal SIGPIPE, Broken pipe
        // 需要重载SIGPIPE信号
        // ret = send(sockfd, "hello world", 12, 0);   
        
        // if(ret == 0){
        //     printf("%d is close on send\n", sockfd);
        //     pthread_exit(NULL);
        // }
        
        // sleep(1);
    }
    
    pthread_exit(NULL);
}


void* recv_message(void* arg){
    int sockfd = *((int*)(arg));
    int ret;

    struct Message* msg = (struct Message*)malloc(sizeof(struct Message));

    while (1){   
        
        if((ret = recv(sockfd, msg, sizeof(struct Message), 0)) < 0){
            perror("recv error");
            exit(1);
        }
        if(ret == 0){
            printf("%d is close on recv\n", sockfd);

            shutdown(sockfd, SHUT_RDWR);
            pthread_exit(NULL);
        }
        
        struct UserOnlineNode* user = NULL;
        struct UserOnlineNode* from = NULL;
        
        switch (msg->type){
        
        case 1:

            user = (struct UserOnlineNode*)malloc(sizeof(struct UserOnlineNode));

            user->sockfd = sockfd;
            strcpy(user->name, msg->msg);
            user->next = NULL;

            Insert_online(&Head, user);

            msg->type = -1;

            send(sockfd, msg, sizeof(struct Message), 0);

            break;
        case 3:

            printf("debug receive a message\n");

            user = Find_online_byname(&Head, msg->name);

            from = Find_online_bysockfd(&Head, sockfd);
            strcpy(msg->name, from->name);

            if(user == NULL){
                printf("user is not log on\n");
            }else{
                msg->type = -3;            

                send(user->sockfd, msg, sizeof(struct Message), 0);

                printf("debug: %d, %s\n", user->sockfd, user->name);
            }

            break;
        
        case 4:
            
            msg->type = -4;

            from = Find_online_bysockfd(&Head, sockfd);
            strcpy(msg->name, from->name);

            user = Head.next;

            while (user != NULL){
                send(user->sockfd, msg, sizeof(struct Message), 0);
                user = user->next;
                usleep(3);
            }


            break;

        default:
            break;
        }

    }

    usleep(3);
}


int main()
{
    //重载SIGPIPE信号
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction( SIGPIPE, &sa, 0 );


    // init user online head
    strcpy(Head.name, "Head");
    Head.sockfd = 0;
    Head.next = NULL;


    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        perror("socket create failed!");
        exit(1);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));


    struct sockaddr_in addr;

    bzero(&addr, sizeof (struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("192.168.1.105"); //自己主机IP

    if(bind(sockfd, (struct sockaddr*)(&addr), sizeof (struct sockaddr_in)) < 0){
        perror("bind failed!");
        exit(1);
    }

    if(listen(sockfd, 3) < 0){
        perror("listen failed!");
        exit(1);
    }

    printf("listen successful\n");


    char buffer[1024];
    int cfd;
    pthread_t send_pid;
    pthread_t recv_pid;
    struct sockaddr_in c_addr;
    socklen_t c_len;

    while (1) {

        memset(buffer, 0, sizeof(buffer));

        c_len = sizeof (struct sockaddr_in);
        

        printf("before accept\n");
        bzero(&c_addr, sizeof (struct sockaddr_in));
        cfd = accept(sockfd, (struct sockaddr*)(&c_addr), &c_len);
        if(cfd == -1){
            perror("accept failed");
            exit(1);
        }

        printf("%d accept successful\n", cfd);
        printf("port = %d, ip = %s\n", ntohs(c_addr.sin_port), inet_ntoa(c_addr.sin_addr));


        if(pthread_create(&recv_pid, NULL, recv_message, (void*)(&cfd)) != 0){
            perror("pthread create error!\n");
		    exit(1);
        } 

        if(pthread_create(&send_pid, NULL, send_message, (void*)(&cfd)) != 0){
            perror("pthread create error!\n");
		    exit(1);
        }
    }

    
    shutdown(sockfd, SHUT_RDWR);


    return 0;
}























