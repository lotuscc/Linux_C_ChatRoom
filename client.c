#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>



#define PORT 33333



struct Message 
{
    int type;
    char name[20];
    char msg[1024];
};



void* recv_message(void* arg){
    int sockfd = *((int*)(arg));
    int ret;

    struct Message* msg = (struct Message*)malloc(sizeof(struct Message));

    while (1)
    {   
        if((ret = recv(sockfd, msg, sizeof(struct Message), 0)) < 0){
            perror("recv error");
            exit(1);
        }

        // 连接关闭
        if(ret == 0){
            printf("%d is close\n", sockfd);
            pthread_exit(NULL);
        }
        
        switch (msg->type){
        
        case -1:
            printf("%s login successful\n", msg->msg);

            break;
        case -2:
            printf("%s reg successful\n", msg->name);

            break;
        case -3:
        case -4:
            printf("from %s : %s\n", msg->name, msg->msg);

            break;
        
        default:
            printf("%s\n", msg->msg);    
            break;
        }
        

    }

}


int main()
{
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1){
        perror("socket create failed!");
        exit(1);
    }

    struct sockaddr_in s_addr;
    bzero(&s_addr, sizeof (struct sockaddr_in));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(PORT);
    s_addr.sin_addr.s_addr = inet_addr("192.168.1.105");

    if(connect(sockfd, (struct sockaddr*)(&s_addr), sizeof (struct sockaddr_in)) < 0){
        perror("connect failed!");
        exit(1);
    }
    
    pthread_t recv_pid;

    if(pthread_create(&recv_pid, NULL, recv_message, (void*)(&sockfd)) != 0){
            perror("pthread create error!\n");
		    exit(1);
    }

    printf("cmd: logon, reg, send, all\n");

    
    char cmd[20];

    struct Message* msg = (struct Message*)malloc(sizeof(struct Message));

    while (scanf("%s", cmd)){

        if(strcmp(cmd, "logon") == 0){
            msg->type = 1;

            printf("name: \n");
            scanf("%s", msg->msg);
            
            send(sockfd, msg, sizeof(struct Message), 0);

        }else if (strcmp(cmd, "reg") == 0){
            msg->type = 2;
            printf("name:\n");
            scanf("%s", msg->name);
            printf("passwd:\n");
            scanf("%s", msg->msg);

            send(sockfd, msg, sizeof(struct Message), 0);

        }else if(strcmp(cmd, "send") == 0){
            msg->type = 3;
            
            printf("to who?\n");
            scanf("%s", msg->name);

            printf("msg ?\n");
            scanf("%s", msg->msg);

            send(sockfd, msg, sizeof(struct Message), 0);

        }else if(strcmp(cmd, "all") == 0){
            msg->type = 4;

            printf("msg ?\n");
            scanf("%s", msg->msg);

            send(sockfd, msg, sizeof(struct Message), 0);

        }else{
            printf("cmd error!\n");
        }
    }
    
    

    shutdown(sockfd, SHUT_RDWR);

    return 0;
}























