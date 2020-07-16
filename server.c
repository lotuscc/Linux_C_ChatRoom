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

#include <sqlite3.h>


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
sqlite3* db = NULL;

int RegtoDatabase(char* name, char* passwd){

    char *zErrMsg = 0;
    int  rc;

    /* Create SQL statement */
    char* sql1 = "INSERT INTO COMPANY (NAME,PASSWD) "  \
                "VALUES ('";

    char* sql2 = "',";
    char* sql3 = "); ";

    char* sql = (char*)malloc(strlen(sql1) + strlen(name) + strlen(sql2) + \
                                                        strlen(passwd) + strlen(sql3));

    strcpy(sql, sql1);
    strcat(sql, name);
    strcat(sql, sql2);
    strcat(sql, passwd);
    strcat(sql, sql3);

    printf("sql: %s\n", sql);

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);

        return -1;
    }else{
        fprintf(stdout, "Records created successfully\n\n");
    }


    free(sql);
    return 1;
}


int InquirefromDatabase(char* name, char* passwd){

    char *zErrMsg = 0;
    int nrow=0;
    int ncolumn = 0;
    char **azResult=NULL; 
    int ret = 0;

    /* Create merged SQL statement */
    char* sql1 = "SELECT * from COMPANY WHERE NAME='";
    char* sql2 = "';";

    char* sql = (char*)malloc(strlen(sql1) + strlen(name) + strlen(sql2));

    strcpy(sql, sql1);
    strcat(sql, name);
    strcat(sql, sql2);

    printf("sql: %s\n", sql);

    sqlite3_get_table( db , sql , &azResult , &nrow , &ncolumn , &zErrMsg );
    
    printf("nrow = %d ncolumn = %d\n", nrow, ncolumn);
    printf("the result is:\n");
    for(int i = 0; i < (nrow + 1) * ncolumn; i++){
         printf("azResult[%d]=%s\n", i, azResult[i]);
    }

    if(nrow * ncolumn == 0){
        ret = -1;
    }else if (strcmp(azResult[3], passwd) == 0)
    {
        ret = 1;
    }else{
        ret = -1;
    }


	sqlite3_free_table(azResult);

    return ret;
}



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
            //从数据库中查询用户密码是否匹配
            if(InquirefromDatabase(msg->name, msg->msg) == 1){
                printf("%s logon successful\n", msg->name);
                msg->type = -1;

                // 注册到在线用户表
                user = (struct UserOnlineNode*)malloc(sizeof(struct UserOnlineNode));

                user->sockfd = sockfd;
                strcpy(user->name, msg->name);
                user->next = NULL;

                Insert_online(&Head, user);
                send(sockfd, msg, sizeof(struct Message), 0);

            }else{
                printf("%s logon failed\n", msg->name);
            }
            

            break;
        case 2:
            //注册到数据库中    
            if(RegtoDatabase(msg->name, msg->msg) < 0){
                printf("reg failed\n");
                break;
            }
            


            msg->type = -2;

            send(sockfd, msg, sizeof(struct Message), 0);

            break;

        case 3:

            printf("debug receive a message\n");

            user = Find_online_byname(&Head, msg->name);

            from = Find_online_bysockfd(&Head, sockfd);

            if(from != NULL){
                strcpy(msg->name, from->name);
            }
            
            if(user == NULL || from == NULL){
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
            if(from != NULL){
                strcpy(msg->name, from->name);
            }
            
            user = Head.next;

            while (user != NULL && from != NULL){
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

    // init database
    char *zErrMsg = 0;
    char* sql;
    int rc;

    /* Open database */
    int len = sqlite3_open("ChatRoomdb", &db);
    
    if(len){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(0);
    }
    else{
        printf("You have opened a sqlite3 database successfully!\n");
    } 

    /* Create SQL statement */
    sql = "CREATE TABLE COMPANY("  \
          "NAME           TEXT  PRIMARY KEY  NOT NULL," \
          "PASSWD         TEXT    NOT NULL);";

     /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }else{
        fprintf(stdout, "Table created successfully\n\n");
    }



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
    addr.sin_addr.s_addr = inet_addr("192.168.43.106"); //自己主机IP

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























