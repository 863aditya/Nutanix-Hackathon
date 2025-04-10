#include "networking/client_side/client.h"
#include "networking/server_side/server_start.h"
#include<iostream>
#include<pthread.h>


int main(){
    std::cout<<"Enter 1 to enable server and enter 2 to enable client side"<<std::endl;
    int option;std::cin>>option;
    if(option==1){
        pthread_t client_thread;
        pthread_create(&client_thread,NULL,start_client,nullptr);
        pthread_join(client_thread,NULL);
    }
    else{
        pthread_t server_thread;
        pthread_create(&server_thread,NULL,run_server,NULL);
        pthread_join(server_thread,NULL);
    }
    std::cin.get();
    return 0;
}