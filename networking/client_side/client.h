#include <iostream>
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
#include <fstream>
#include <filesystem>
#include "../constants.h"
#include "../helper.h"
#include <sys/stat.h>
#include <iomanip>
#include <ctime>
#include <netinet/tcp.h>
#include <shared_mutex>
#include <mutex>

#define CLIENT_SIDE_PORT 4600
#define CLIENT_SIDE_IP "172.16.64.54"

int client_socket;
sockaddr_in address;
sockaddr_in server_address;
const std::string space = " ";
std::map<std::string, std::string> hashes;

bool is_logged_in = false;
void init()
{
    // client_socket = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_port = htons(CLIENT_SIDE_PORT);
    address.sin_addr.s_addr = inet_addr(CLIENT_SIDE_IP);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT_N);
    server_address.sin_addr.s_addr = inet_addr(SERVER_ADDR);
}

void send_data(char *data)
{
    int new_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    int connection_response = connect(new_client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    printf("%d\n", connection_response);
    while (connection_response < 0)
    {
        connection_response = connect(new_client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    }
    send(new_client_socket, data, strlen(data), 0);
    client_socket = new_client_socket;
    printf("connection estab and data sent\n");
}

void change_client(std::string &username)
{
}

void handle_new_file(std::string path_to_file, std::string path_to_folder, std::string file_without_extension)
{
    std::string track_file_location = path_to_folder + TRACK + file_without_extension + TXT;
    hash_each_line_to_file(path_to_file, track_file_location);
}

void handle_file_change(std::string path_to_file, std::string path_to_folder, std::string file_without_extension, std::string file_name_complete, std::string group)
{
    std::string track_file_location = path_to_folder + TRACK + file_without_extension + TXT;
    std::ifstream file(path_to_file);
    std::ifstream track_file(track_file_location);
    std::vector<std::string> initial_line_hashes;
    std::vector<std::string> final_line_hashes;
    std::string line;
    std::vector<std::string> complete_lines;
    while (std::getline(file, line))
    {
        std::string new_hash = sha256(line);
        complete_lines.emplace_back(line);
        final_line_hashes.emplace_back(new_hash);
    }
    while (std::getline(track_file, line))
    {
        std::string old_hash = sha256(line);
        initial_line_hashes.emplace_back(old_hash);
    }

    for (int i = 0; i < std::min(final_line_hashes.size(), initial_line_hashes.size()); i++)
    {
        if (final_line_hashes[i] != initial_line_hashes[i])
        {
            std::string protocol_data = FILE_DIFF + space + group + space + file_name_complete + space + num_to_string(i + 1) + space + complete_lines[i];
            char *data = convert_to_char(protocol_data);
            send_data(data);
            delete data;
        }
    }
    if (final_line_hashes.size() < initial_line_hashes.size())
    {
        for (int i = final_line_hashes.size(); i < initial_line_hashes.size(); i++)
        {
            std::string protocol_data = FILE_DIFF + space + group + space + file_name_complete + space + num_to_string(i + 1) + space + NOLINE;
            char *data = convert_to_char(protocol_data);
            send_data(data);
            delete data;
        }
    }
    else if (final_line_hashes.size() != initial_line_hashes.size())
    {
        for (int i = initial_line_hashes.size(); i < final_line_hashes.size(); i++)
        {
            std::string protocol_data = FILE_DIFF + space + group + space + file_name_complete + space + num_to_string(i + 1) + space + complete_lines[i];
            char *data = convert_to_char(protocol_data);
            send_data(data);
            delete data;
        }
    }
}

void folder_checking_for_groups(std::vector<std::string> &tokens)
{
    std::string group_name;
    std::string path = "./data/";
    int len = tokens.size();
    for (int i = 1; i < len; i++)
    {
        std::string str = tokens[i];
        int tmp_len = str.size();
        int idx = str.find(';');
        std::string name = str.substr(0, idx);
        std::string hash = str.substr(idx + 1);
        hashes[name] = hash;
    }
    while (true)
    {
        for (const auto &entry : std::filesystem::directory_iterator(path))
        {
            if (entry.path().filename() == CONTROL_INFO)
                continue;
            if (std::filesystem::is_regular_file(entry))
                std::cout << "File: " << entry.path().filename() << '\n';
            else if (std::filesystem::is_directory(entry))
                std::cout << "Directory: " << entry.path().filename() << '\n';
            std::string current_file_name = entry.path().filename();
            std::string current_file_hash = sha256_file(entry.path());
            std::string timestamp = getLastModifiedTime(entry.path());
            std::cout << current_file_name << ' ' << current_file_hash << '\n';
            if (hashes[current_file_name] != current_file_hash)
            {
                std::string protocol_data = DATA_STREAM + space + current_file_name + space + timestamp + space + current_file_hash + space + "txt" + space;
                char *result = convert_to_char(protocol_data);
                int new_client_socket = socket(AF_INET, SOCK_STREAM, 0);
                int flag = 1;
                setsockopt(new_client_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
                int connection_response = connect(new_client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
                printf("%d\n", connection_response);
                while (connection_response < 0)
                {
                    connection_response = connect(new_client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
                }
                send(new_client_socket, result, strlen(result), 0);
                usleep(1000000);
                std::ifstream file(entry.path(), std::ios::binary);
                char buffer[BUFFER_SIZE];
                while (!file.eof())
                {
                    file.read(buffer, BUFFER_SIZE);
                    int bytes_read = file.gcount();
                    send(new_client_socket, buffer, bytes_read, 0);
                }
                file.close();
                close(new_client_socket);
                hashes[current_file_name] = current_file_hash;
            }
        }
        usleep(2000000);

        std::string get_data = GET;
        char *result = convert_to_char(get_data);
        int new_client_socket = socket(AF_INET, SOCK_STREAM, 0);
        int flag = 1;
        setsockopt(new_client_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
        int connection_response = connect(new_client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
        printf("Response from get %d\n", connection_response);
        while (connection_response < 0)
        {
            connection_response = connect(new_client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
        }
        send(new_client_socket, result, strlen(result), 0);
        char buffer[BUFFER_SIZE] = {0};
        recv(new_client_socket, buffer, BUFFER_SIZE, 0);
        close(new_client_socket);
        std::cout << buffer << '\n';
        std::stringstream ss(buffer);
        std::string item;
        while (std::getline(ss, item, ';'))
        {
            std::string str = item;
            int tmp_len = str.size();
            int idx = str.find(':');
            std::string filename = str.substr(0, idx);
            std::string hash = str.substr(idx + 1);
            std::cout << filename << ' ' << hash << '\n';
            if (hashes[filename] == hash)
            {
                std::cout << filename + " is present with hash as " + hash << '\n';
                continue;
            }
            hashes[filename] = hash;
            int file_client_socket = socket(AF_INET, SOCK_STREAM, 0);
            int flag_in = 1;
            setsockopt(file_client_socket, IPPROTO_TCP, TCP_NODELAY, &flag_in, sizeof(int));
            int connection_response_in = connect(file_client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
            printf("Response from get_in %d\n", connection_response_in);
            while (connection_response_in < 0)
            {
                connection_response_in = connect(file_client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
            }
            std::string data = REQ_FILE + space + filename;
            char *result_in = convert_to_char(data);
            send(file_client_socket, result_in, strlen(result_in), 0);
            std::cout << "Requesting file : " << filename << '\n';
            std::string created_file_name = "./" + DATA + filename;
            std::ofstream outfile(created_file_name, std::ios::binary);
            std::cout << created_file_name << '\n';
            int bytes_recieved;
            char buf[BUFFER_SIZE] = {0};
            while ((bytes_recieved = recv(file_client_socket, buf, BUFFER_SIZE, 0)) > 0)
            {
                std::cout << "here\n";
                std::cout << bytes_recieved << ' ' << buf << '\n';
                outfile.write(buf, bytes_recieved);
            }
            outfile.close();
            close(file_client_socket);
        }
        usleep(2000000);
    }
}

void check_control_info(std::string &username)
{
    std::ifstream file(CONTROL_INFO_FILE);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file\n";
        return;
    }
    std::string line;
    std::vector<std::string> tokens;
    while (std::getline(file, line))
    {
        tokens.emplace_back(line);
    }
    printf("check\n");
    if (tokens[0] == username)
    {
        // pthread_t t;
        // pthread_create(&t, NULL, get_data, NULL);
        // std::cout << "starting a new thread for GET_DATA" << std::endl;
        // pthread_detach(t);
        folder_checking_for_groups(tokens);
    }
    else
    {
        change_client(username);
    }
}

void handleLogin(char *data, std::string username)
{
    send_data(data);
    printf("sent data\n");
    char buffer[BUFFER_SIZE] = {0};
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    printf("recv\n");
    std::cout << buffer << std::endl;
    if (buffer[0] == 'L')
    {
        check_control_info(username);
    }
}

void *initialise_groups(void *arg)
{
    while (std::cin.get())
    {
        std::string file_name;
        std::cin >> file_name;
        std::string file_path = PARENT_DIRECTORY + DATA + file_name;
        std::filesystem::create_directory(file_path);
    }
}

void *start_client(void *arg)
{
    init();
    std::cout << "You have the following options\n1.LOGIN\n2.CREATE NEW USER\n"
              << std::endl;
    int option;
    std::cin >> option;
    std::string username, password;
    std::string message;
    char *result;
    switch (option)
    {
    case 1:
        std::cout << "ENTER USERNAME AND PASSWORD" << std::endl;
        std::cin >> username >> password;
        message = LOGIN + space + username + space + password;
        result = new char[message.size() + 1];
        std::strcpy(result, message.c_str());
        handleLogin(result, username);
        delete result;
        break;
    case 2:
        std::cout << "ENTER NEW USERNAME AND PASSWORD" << std::endl;
        std::cin >> username >> password;
        message = ADD_USER + space + username + space + password;
        result = new char[message.size() + 1];
        std::strcpy(result, message.c_str());

        delete result;
        break;
    default:
        break;
    }
}