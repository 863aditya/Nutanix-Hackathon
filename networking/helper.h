#pragma once
#include<sstream>
#include<iostream>
#include<string>
#include<cstring>
#include<vector>
#include<fstream>
#include <openssl/sha.h>
#include <iomanip>
#include<algorithm>

std::string num_to_string(int val){
    std::string answer="";
    while(val){
        answer+=(val%10 + '0');
        val/=10;
    }
    std::reverse(answer.begin(),answer.end());
    return answer;
}
char* convert_to_char(std::string data){
    char* response = new char[data.size()+1];
    std::strcpy(response,data.c_str());
    return response;
}
std::string sha256_file( std::string path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    char buffer[4096];
    while (file.read(buffer, sizeof(buffer)))
        SHA256_Update(&ctx, buffer, file.gcount());
    if (file.gcount())
        SHA256_Update(&ctx, buffer, file.gcount());

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);

    std::ostringstream result;
    for (unsigned char c : hash)
        result << std::hex << std::setw(2) << std::setfill('0') << (int)c;

    return result.str();
}

std::string sha256(const std::string& line) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, line.c_str(), line.size());
    SHA256_Final(hash, &sha256);

    std::ostringstream out;
    for (unsigned char c : hash) {
        out << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    return out.str();
}

void hash_each_line_to_file(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream inFile(inputPath);
    std::ofstream outFile(outputPath);

    if (!inFile.is_open() || !outFile.is_open()) {
        std::cerr << "Error opening file(s)." << std::endl;
        return;
    }

    std::string line;
    while (std::getline(inFile, line)) {
        std::string hash = sha256(line);
        outFile << hash << std::endl;
    }

    inFile.close();
    outFile.close();
}

void tokenize_buffer_response(char* buffer,std::vector<std::string>&tokens){
    std::istringstream iss(buffer);
    std::string word;
    while(iss>>word){
        tokens.push_back(word);
    }
}