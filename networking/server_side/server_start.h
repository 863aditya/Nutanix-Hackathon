#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <pthread.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include "../constants.h"
#include "../helper.h"
#include <shared_mutex>
#include <thread>
#include <mutex>

// can be brought through a db
std::set<std::string> USER_NAMES = {"thunder", "cypher", "a"};
std::map<std::string, std::string> PASSWORDS = {{"thunder", "thunder"}, {"cypher", "cypher"}, {"a", "a"}};

// file name with their hashes
std::shared_mutex lock;
std::map<std::string, std::string> hash_maintainence;
std::map<std::string, std::string> last_modified;
//

void *handleClientResponses(void *args)
{
    char buffer[BUFFER_SIZE] = {0};
    int *client_socket = (int *)args;
    int y = recv((*client_socket), buffer, sizeof(buffer), 0);
    if (y > 0)
    {
        std::vector<std::string> tokens;
        tokenize_buffer_response(buffer, tokens);
        // for (auto &e : tokens)
        // {
        //     std::cout << e << std::endl;
        // }
        // protocol for login is LOGIN USERNAME PASSWORD
        if (tokens[0] == LOGIN)
        {
            std::string username = tokens[1], password = tokens[2];
            if (USER_NAMES.count(username) && PASSWORDS[username] == password)
            {
                const char *reply = LOGGED_SUCC;
                send((*client_socket), reply, strlen(reply), 0);
                std::cout << "success login" << std::endl;
            }
        }

        // protocol for adding user
        if (tokens[0] == ADD_USER)
        {
            USER_NAMES.insert(tokens[1]);
            PASSWORDS[tokens[1]] = tokens[2];
            const char *reply = "CREDENTIALS CREATED SUCCESSFULL!";
            send((*client_socket), reply, strlen(reply), 0);
        }

        if (tokens[0] == DATA_STREAM)
        {
            auto filename = tokens[1], timestamp = tokens[2], hash = tokens[3], extension = tokens[4];
            auto created_file_name = SERVER_FILE + hash + "." + extension;
            std::ofstream outfile(created_file_name, std::ios::binary);
            int bytes_recieved;
            char buf[BUFFER_SIZE] = {0};
            while ((bytes_recieved = recv((*client_socket), buf, BUFFER_SIZE, 0)) > 0)
            {
                std::cout << buf << '\n';
                outfile.write(buf, bytes_recieved);
            }
            std::unique_lock writelock(lock);
            if (last_modified[filename] < timestamp)
            {
                last_modified[filename] = timestamp;
                std::string prev_file_name = SERVER_FILE + hash_maintainence[filename] + "." + extension;
                std::remove(prev_file_name.c_str());
                hash_maintainence[filename] = hash;
            }
            else
            {
                writelock.unlock();
                if (hash_maintainence[filename] != hash)
                {
                    std::remove(created_file_name.c_str());
                }
            }
            outfile.close();
        }

        if (tokens[0] == GET)
        {
            std::string data = "";
            std::shared_lock<std::shared_mutex> readlock(lock);
            for (auto ele : hash_maintainence)
            {
                data += ele.first + ":" + ele.second + ";";
            }
            readlock.unlock();
            char *reply = convert_to_char(data);
            send((*client_socket), reply, strlen(reply), 0);
        }

        if (tokens[0] == REQ_FILE)
        {
            std::string filename = tokens[1];
            std::ifstream file(SERVER_FILE + hash_maintainence[filename] + ".txt", std::ios::binary);
            char buffer[BUFFER_SIZE];
            while (!file.eof())
            {
                file.read(buffer, BUFFER_SIZE);
                int bytes_read = file.gcount();
                std::cout << bytes_read << ' ' << buffer << '\n';
                send((*client_socket), buffer, bytes_read, 0);
            }
            // readlock.unlock();
            file.close();
            usleep(1000000);
        }
        close((*client_socket));
        printf("closing\n");
    }
}

void *run_server(void *arg)
{
    std::cout << "starting server" << std::endl;
    int socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_server < 0)
    {
        perror("Socket creation failed");
        return nullptr;
    }
    sockaddr_in address_server;
    address_server.sin_family = AF_INET;
    address_server.sin_port = htons(SERVER_PORT_N);
    address_server.sin_addr.s_addr = INADDR_ANY;
    int opt = 1;
    // setsockopt(socket_server, SOL_SOCKET, SO_RCVTIMEO, &opt, optlen);
    setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // bind(socket_server, (struct sockaddr *)&address_server, sizeof(address_server));
    if (bind(socket_server, (struct sockaddr *)&address_server, sizeof(address_server)) < 0)
    {
        perror("Bind failed");
        close(socket_server);
        return nullptr;
    }
    listen(socket_server, MAX_NO_MACHINES);
    socklen_t optlen = sizeof(opt);
    std::cout << "Listening" << std::endl;

    while (true)
    {
        int temp = accept(socket_server, nullptr, nullptr);
        if (temp < 0)
        {
            perror("Accept failed");
            close(socket_server);
            return nullptr;
        }
        int *client_socket = new int(temp);
        pthread_t t;

        void *arg = client_socket;
        pthread_create(&t, NULL, handleClientResponses, arg);
        std::cout << "starting a new thread" << std::endl;
        pthread_detach(t);
    }
}