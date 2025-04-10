#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<cstring>
#include<pthread.h>
#include<arpa/inet.h>
#include<string>
#include<cstring>
#include<sstream>
#include<vector>
#include<set>
#include<map>
#include "../constants.h"
#include "../helper.h"




//can be brought through a db
std::set<std::string>USER_NAMES = {"thunder","cypher"};
std::map<std::string,std::string> PASSWORDS={{"thunder","thunder"},{"cypher","cypher"}};
//corresponding to group_id what are the usernames associated with it
std::map<std::string,std::vector<std::string>>groups;
//corresponding to a usernames what are the group_ids it is maintaining
std::map<std::string,std::set<int>>reverse_groups;
//list of file names and their sha256 hash corresponding to a group
std::map<std::string,std::map<std::string,std::string>>files_in_groups;

void* handleClientResponses(void* args){
    char buffer[BUFFER_SIZE]={0};
    int* client_socket =(int*)args;
    int y=recv((*client_socket),buffer,sizeof(buffer),0);
    if(y>0){
        std::vector<std::string>tokens;
        tokenize_buffer_response(buffer,tokens);
        for(auto &e:tokens){
            std::cout<<e<<std::endl;
        }
        //protocol for login is LOGIN USERNAME PASSWORD
        if(tokens[0]==LOGIN){
            std::string username=tokens[1],password = tokens[2];
            if(USER_NAMES.count(username) && PASSWORDS[username]==password){
                const char* reply = LOGGED_SUCC;
                send((*client_socket),reply, strlen(reply),0);
            }
        }
        //protocol for adding user
        if(tokens[0]==ADD_USER){
            USER_NAMES.insert(tokens[1]);
            PASSWORDS[tokens[1]]=tokens[2];
            const char* reply = "CREDENTIALS CREATED SUCCESSFULL!";
            send((*client_socket),reply,strlen(reply),0);
        }

        if(tokens[0]==DATA_STREAM){
            
        }

        //protocol for getting the sha256 file in a grp is GET_SHA_GRP_FILE GRP_ID FILENAME
        if(tokens[0]==GET_SHA_GRP_FILE){
            std::string response = files_in_groups[tokens[1]][tokens[2]];
            char* reply = new char[response.size()+1];
            std::strcpy(reply,response.c_str());
            send((*client_socket),reply,strlen(reply),0);
        }

        //protocol for file diffs is FILE_DIFF GROUP_ID FILENAME LINE_NUMBER CHANGED_LINE
        if(tokens[0]==FILE_DIFF){

        }
        close((*client_socket));
    }
}

void* run_server(void* arg){

    int socket_server =  socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in address_server;
    address_server.sin_family=AF_INET;
    address_server.sin_port=htons(SERVER_PORT_N);
    address_server.sin_addr.s_addr=inet_addr(SERVER_ADDR);
    bind(socket_server,(struct sockaddr*)&address_server,sizeof(address_server));
    int opt=1;
    socklen_t optlen=sizeof(opt);
    setsockopt(socket_server,SOL_SOCKET,SO_RCVTIMEO,&opt,optlen);
    listen(socket_server,MAX_NO_MACHINES);


    while(true){
        int client_socket = accept(socket_server,(struct sockaddr *)&address_server,&optlen);
        if(client_socket<0){
            continue;
        }
        pthread_t t;
        void* arg=&client_socket;
        pthread_create(&t,NULL,handleClientResponses,arg);
        pthread_detach(t);
    }
}