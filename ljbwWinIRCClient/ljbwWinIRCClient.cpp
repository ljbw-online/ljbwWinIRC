#include <iostream>
#include <fstream>
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h> // Also necessary for Windows sockets
#include <stdio.h>
//#include <thread> // std::this_thread::sleep_for
//#include <chrono> // std::chrono::seconds

// "... indicates to the linker that the Ws2_32.lib file is needed"
#pragma comment(lib, "Ws2_32.lib")

// File mapping stuff
#include <windows.h>
#include <conio.h>
#include <tchar.h>
#pragma comment(lib, "user32.lib")

#define DEFAULT_SERVER "irc.chat.twitch.tv"
//#define DEFAULT_SERVER "192.168.178.55"
#define DEFAULT_PORT "6667"
#define DEFAULT_BUFLEN 512

int sendIRC(SOCKET sock, std::string buffer) {
    return send(sock, buffer.c_str(), buffer.size(), 0);
}

int main(int argc, char* argv[])
{
    const char* server = DEFAULT_SERVER;
    const char* port = DEFAULT_PORT;

    std::ifstream token("..\\..\\..\\..\\Documents\\twitch_oauth.txt");
    std::string serverPass;
    getline(token, serverPass); // converts the first line of an ifstream to a string

    //std::string User = "ljbw_online";
    std::string Nick = "ljbw_online";
    std::string Channel = "#ljbw_online";
    
    //std::string channelPass = ""; // Not necessary for Twitch

    WSADATA wsaData;
    int iResult;

    // Initialise the socket library
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }

    struct addrinfo *result = NULL, *ptr = NULL, hints;
    /*
    addrinfo is a struct type
    result is a pointer to an addrinfo
    ptr is a pointer to an addrinfo
    hints is an addrinfo

    What I don't understand here is why the struct keyword has been used.
    This file compiles without it, which isn't surprising seen as
    addrinfo is a type name.

    Is this something from a very old version of C++?
    */

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo(server, port, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ConnectSocket = INVALID_SOCKET;

    ptr = result;

    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
        ptr->ai_protocol);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Connect to server
    iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
    }

    // Should really try the next address returned by getaddrinfo
    // if the connect call failed
    // But for this simple example we just free the resources
    // returned by getaddrinfo and print an error message

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server\n");
        WSACleanup();
        return 1;
    }

    int recvbuflen = DEFAULT_BUFLEN;
    const char* sendbuf = "this is a test";
    char recvbuf[DEFAULT_BUFLEN];

    iResult = sendIRC(ConnectSocket, "PASS " + serverPass + " \r\n");
    
    if (iResult == SOCKET_ERROR) {
        printf("send failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    sendIRC(ConnectSocket, "NICK " + Nick + " \r\n");

    /*
    This isn't necessary for Twitch because we use an authentication
    token. Something like this probably is necessary for a normal IRC
    server:
    sendIRC(ConnectSocket, "USER " + User + " 0 * :" + User + " \r\n");
    */

    sendIRC(ConnectSocket, "JOIN " + Channel + " \r\n");
    
    std::string recvstring, usr, msg;
    size_t pos;

    // This runs indefinitely. You'll have to press Ctrl-C to quit the
    // program :).
    while (true) {
        // Set the buffer to zeros to prevent garbage characters from being
        // present in recvbuf and recvstring.
        memset(&recvbuf, '\0', sizeof(recvbuf));
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        //printf(recvbuf);

        if (iResult == SOCKET_ERROR) {
            printf("recv failed: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }

        recvstring = std::string(recvbuf);
        if (recvstring.find("PING", 0) == 0) {
            std::string pong = "PONG" + recvstring.substr(4, recvstring.length());
            sendIRC(ConnectSocket, pong);
            std::cout << pong;
        }

        pos = recvstring.find("!", 0);

        usr = recvstring.substr(1, pos - 1);

        //:<usr>!<usr>@<usr>.tmi.twitch.tv PRIVMSG #<channel> :
        // Channel includes the "#"
        pos = 3 + usr.length() * 3 + 23 + Channel.length() + 2;

        // substr returns a std::out_of_range exception if the first 
        // argument is greater than the length of the string.
        if (pos <= recvstring.length()) {
            msg = recvstring.substr(pos, std::string::npos);

            std::size_t s = msg.length();

            //memcpy(pBuf, msg.c_str(), s);

            std::cout << usr + ": " + msg;
        }

        if (msg == "!commands\r\n") {
            std::string sendMsg = "PRIVMSG #ljbw_online :!reset | !paint | !raise_wanted | !lower_wanted | !forward | !backward | w | a | s | d\r\n";
            sendIRC(ConnectSocket, sendMsg);
        }

        /*
        If your Twitch channel were called "ljbw_online" then this is how 
        you would send the message "yo" into chat, check for errors, and 
        then wait for one second:

        iSendResult = sendIRC(ConnectSocket, "PRIVMSG #ljbw_online :yo\r\n");
        if (iSendResult == SOCKET_ERROR) {
            std::cout << "iSendResult == SOCKET_ERROR\n";
            std::cout << WSAGetLastError() << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        */
    }

    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
