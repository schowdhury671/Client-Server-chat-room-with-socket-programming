#include <stdlib.h>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <csignal>
#include <chrono>
#include <ctime>

using namespace std;

#define ERROR -1
#define MAX_CLIENTS 1
#define HTTIMER 300
#define MAX_DATA 1024
#define BUFFER 1024
#define FILE_BUFFER 1024

struct socketInfo
{
    int nw;
    struct sockaddr_in cli;
};
const char *client_alias, *client_ip, *client_port, *server_ip, *server_port, *chat_recv_port;
struct sockaddr_in client, server, broadcastClient;
socklen_t sockaddr_len = sizeof(struct sockaddr_in);
vector<pthread_t> threadV;
int broadcastSock;

void signalHandler_KillingThreads(int signum) {
    std::vector<pthread_t>::iterator thit;
    for (thit = threadV.begin(); thit != threadV.end(); thit++)
    {
        //std::cout << "Killing thread : " << *thit << '\n';
        pthread_kill(*thit, signum);
    }
    threadV.empty();
    close(broadcastSock);
    exit(signum);
}

vector<string> split_string(const char *data, char *delimiter) {
    vector<string> retVect;
    char *pch = strtok((char*)data, delimiter);

    if (strcmp(pch, (char *)"reply") == 0 && data[6] == 34) {
        retVect.push_back(pch);
        int i = 7;
        string chatMessage = "";
        while(data[i] != 34) {
            chatMessage += data[i];
            i++;
        }
        retVect.push_back(chatMessage);
    }
    else
        while (pch != NULL) {
            retVect.push_back(pch);
            pch = strtok(NULL, delimiter);
        }

    
    if (*(retVect.begin()) == "reply" && retVect.size() == 3) {
        *(retVect.begin()) = *(retVect.end()-1);
        retVect.pop_back();
    }
    
    return retVect;
}

void receiveFile(int sock, const char *fileName)
{
    cout << "recv file started..." << endl;
    string slash = "./";
    FILE *fp = fopen(strcat((char *)slash.c_str(), fileName), "w+");
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    long totalByteTransfer = 0;
    size_t bytesRcvd;
    char in[FILE_BUFFER];
    rewind(fp);
    if (fp)
    {
        while (1)
        {
            // cout << "Next Block..." << endl;
            bytesRcvd = recv(sock, in, sizeof(in), 0);
            // cout<< "Recieved " << bytesRcvd << " bytes " << endl;
            if (bytesRcvd < 0)
                //perror("recv");
                std::cout << "FAILURE:FILE RECEIVE ERROR" << '\n'
                          << ">>> ";
            else if (bytesRcvd == 0)
                break;
            if (fwrite(in, 1, bytesRcvd, fp) != (size_t)bytesRcvd)
            {
                //perror("fwrite");
                std::cout << "FAILURE:FILE WRITE ERROR" << '\n';
                fclose(fp);
                return;
            }
            totalByteTransfer += (long)bytesRcvd;
        }
        fclose(fp);
        cout << "SUCCESS: FILE RECEIVED" << '\n';
    }
    else
        std::cout << "FAILURE:FILE OPENING ERROR IN RECEIVER" << '\n';
}

void sendFile(int sock, const char *fileName)
{
    // cout << "**FILE SENDING START** " << fileName << endl;
    FILE *fp = fopen(fileName, "r");

    size_t bytesRead, readSize;
    char buff[FILE_BUFFER];

    if (fp)
    {
        cout << "SENDING FILE TO SERVER..." << endl;
        while (!feof(fp))
        {
            // cout << "Next Block..." << endl;
            bytesRead = fread(buff, 1, FILE_BUFFER, fp);
            // std::cout << (int)bytesRead << " bytes will be sent.." << '\n';
            if (bytesRead <= 0)
                break;
            
            if (send(sock, buff, bytesRead, 0) != bytesRead)
            {
                perror("File send error");
                break;
            }
        }
        cout << "SUCCESS:FILE SHARED" << '\n';
    }
    else
    {
        std::cout << "FAILURE: FILE NOT FOUND" << '\n';
    }
    fclose(fp);
}

void* broadcast_recv(void * arg) {
    socketInfo sck;
    //Start server
    if ((broadcastSock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
        perror("SERVER SOCKET: ");

    bzero(&server.sin_zero, 8);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(chat_recv_port));
    server.sin_addr.s_addr = inet_addr(client_ip);

    if ((bind(broadcastSock, (struct sockaddr *)&server, sockaddr_len)) == ERROR)
        perror("BIND : ");

    std::cout << "BROADCAST RECEIVER STARTED..." << '\n';

    if ((listen(broadcastSock, MAX_CLIENTS)) == ERROR)
        perror("LISTEN : ");

    while (1)
    {
        if ((sck.nw = accept(broadcastSock, (struct sockaddr *)&broadcastClient, &sockaddr_len)) == ERROR)
        {
            std::cout << "Can't connect to client " << inet_ntoa(broadcastClient.sin_addr) << '\n';
        }
        else
        {
            sck.cli = broadcastClient;
            char chatMessage[MAX_DATA] = {'\0'};

            int data_len = recv(sck.nw, chatMessage, MAX_DATA, 0);

            if (data_len) {
                if (strcmp(chatMessage, "chat") == 0)
                {
                    send(sck.nw, "OK", 2, 0);
                    data_len = recv(sck.nw, chatMessage, MAX_DATA, 0);
                    std::cout << '\n'
                              << chatMessage << '\n';
                }
                else
                {
                    send(sck.nw, "OK", 2, 0);
                    data_len = recv(sck.nw, chatMessage, MAX_DATA, 0);
                    receiveFile(broadcastSock, chatMessage);
                }
                std::cout << ">> ";
            }
            
        }
    }
    close(broadcastSock);
}

int main(int argc, char const *argv[]) {
    
    if (argc != 7) {
        std::cout << "INVALID ARGUMENTS!" << endl;
        return -1;
    }
    fflush(stdin);
    fflush(stdout);
    client_alias = argv[1]; client_ip = argv[2]; client_port = argv[3];
    chat_recv_port = argv[4]; server_ip = argv[5]; server_port = argv[6];
	pthread_t thread;
    string messageToServer, fstr, fileName;
	int cliSock;

    signal(SIGINT, signalHandler_KillingThreads); //registers the signal handler

    

    //Start client
    memset(&client, '0', sizeof(client));
    client.sin_family = AF_INET;
    client.sin_port = htons(atoi(server_port));
    client.sin_addr.s_addr = inet_addr(server_ip);
    // User 1st time registration
    if ((cliSock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
    {
        std::cout << "SOCKET CREATION ERROR!" << endl;
        exit(-1);
    }

    if ((connect(cliSock, (struct sockaddr *)&client, sizeof(struct sockaddr_in))) == ERROR)
    {
        std::cout << "CONNECTION FAILURE: SERVER OFFLINE!" << '\n';
        exit(-1);
    }
    // std::cout << "FIRST CONNECTION WITH SERVER ESTABLISHED..." << '\n';
    
    messageToServer = "NEWUSER#@#Garbage#@#" + (string)client_alias + "#@#" + 
                            (string)client_ip + "#@#" + (string)chat_recv_port;
    std::cout << messageToServer << endl;
    send(cliSock, messageToServer.c_str(), messageToServer.length(), 0);
    close(cliSock);

    // Server Thread to receive broadcasts
    pthread_create(&thread, NULL, &broadcast_recv, NULL);
    threadV.push_back(thread);
    sleep(1);

    while(1) {

        std::cout << "\n>> ";
        getline(cin, fstr);
        vector<string> userResponseVect = split_string(fstr.c_str(), (char *)" ");
        messageToServer = "";
        if ((*(userResponseVect.begin())).compare("create") == 0)
            messageToServer = "CREATE#@#" + userResponseVect[2];
        else if ((*(userResponseVect.begin())).compare("leave") == 0)
            messageToServer = "LEAVE#@#Garbage";
        else if ((*(userResponseVect.begin())).compare("reply") == 0){
            messageToServer = userResponseVect[0] + "#@#" + userResponseVect[1];
        }
        else if ((*(userResponseVect.begin())).compare("tcp") == 0 
                || (*(userResponseVect.begin())).compare("udp") == 0)
        {
            fileName = userResponseVect[1];
            vector<string> filePathVect = split_string(userResponseVect[1].c_str(), (char *)"/");
            messageToServer = userResponseVect[2] + "#@#" + *(filePathVect.end() - 1);
        }
        else messageToServer = userResponseVect[0] + "#@#" + userResponseVect[1];

        messageToServer += "#@#" + (string)client_alias + "#@#" + (string)client_ip + "#@#" + (string)chat_recv_port;

        if ((cliSock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
        {
            std::cout << "SOCKET CREATION ERROR!" << endl;
            exit(-1);
        }

        if ((connect(cliSock, (struct sockaddr *)&client, sizeof(struct sockaddr_in))) == ERROR)
        {
            std::cout << "CONNECTION FAILURE: SERVER OFFLINE!" << '\n';
            exit(-1);
        }
        // std::cout << "CONNECTION WITH SERVER ESTABLISHED..." << '\n';

        send(cliSock, messageToServer.c_str(), messageToServer.length(), 0);
        char rcvdData[MAX_DATA];
        int data_len = recv(cliSock, rcvdData, MAX_DATA, 0);
        if(data_len)
        {

            if ((*(userResponseVect.begin())).compare("tcp") == 0 
                    || (*(userResponseVect.begin())).compare("udp") == 0)
            {
                sendFile(cliSock, fileName.c_str());
            }
            
            else
                std::cout << rcvdData << endl;
        }
        close(cliSock);
    }
    
	
    return 0;
}