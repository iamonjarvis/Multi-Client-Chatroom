#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <mutex>
#include <csignal>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define close closesocket
#define SOCKET_TYPE SOCKET
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET_TYPE int
#endif

#define MAX_LEN 200
#define NUM_COLORS 6

using namespace std;

bool exit_flag = false;
thread t_send, t_recv;
SOCKET_TYPE client_socket;
string def_col = "\033[0m";
string colors[] = { "\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m" };

void catch_ctrl_c(int signal);
string color(int code);
void eraseText(int cnt);
void send_message(SOCKET_TYPE client_socket);
void recv_message(SOCKET_TYPE client_socket);

#ifdef _WIN32
void init_winsock() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "WSAStartup failed. Error Code: " << WSAGetLastError() << endl;
        exit(EXIT_FAILURE);
    }
}
#endif

int main() {
#ifdef _WIN32
    init_winsock();
#endif

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
#ifdef _WIN32
        cerr << "Socket creation failed. Error: " << WSAGetLastError() << endl;
#else
        perror("socket: ");
#endif
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(29001);  // Port no. of server

#ifdef _WIN32
    // Use InetPton instead of inet_addr
    InetPton(AF_INET, TEXT("127.0.0.1"), &client.sin_addr);
#else
    client.sin_addr.s_addr = inet_addr("127.0.0.1");
#endif

    if (connect(client_socket, (struct sockaddr*)&client, sizeof(client)) == -1) {
#ifdef _WIN32
        cerr << "Connect failed. Error: " << WSAGetLastError() << endl;
#else
        perror("connect: ");
#endif
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, catch_ctrl_c);  // Handle Ctrl+C for graceful exit
    char name[MAX_LEN];
    cout << "Enter your name: ";
    cin.getline(name, MAX_LEN);
    send(client_socket, name, sizeof(name), 0);

    cout << colors[NUM_COLORS - 1] << "\n\t  ====== Welcome to the chat-room ======   " << endl << def_col;

    thread t1(send_message, client_socket);
    thread t2(recv_message, client_socket);

    t_send = move(t1);
    t_recv = move(t2);

    if (t_send.joinable())
        t_send.join();
    if (t_recv.joinable())
        t_recv.join();

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}

// Handler for "Ctrl + C"
void catch_ctrl_c(int signal) {
    char str[MAX_LEN] = "#exit";
    send(client_socket, str, sizeof(str), 0);
    exit_flag = true;
    t_send.detach();
    t_recv.detach();
    close(client_socket);
    exit(signal);
}

string color(int code) {
    return colors[code % NUM_COLORS];
}

// Erase text from terminal
void eraseText(int cnt) {
    char back_space = 8;
    for (int i = 0; i < cnt; i++) {
        cout << back_space;
    }
}

// Send message to everyone
void send_message(SOCKET_TYPE client_socket) {
    while (1) {
        cout << colors[1] << "You: " << def_col;
        char str[MAX_LEN];
        cin.getline(str, MAX_LEN);
        send(client_socket, str, sizeof(str), 0);
        if (strcmp(str, "#exit") == 0) {
            exit_flag = true;
            t_recv.detach();
            close(client_socket);
            return;
        }
    }
}

// Receive message
void recv_message(SOCKET_TYPE client_socket) {
    while (1) {
        if (exit_flag)
            return;

        char name[MAX_LEN];
        char str[MAX_LEN];
        int color_code;

        int bytes_received = recv(client_socket, name, sizeof(name), 0);
        if (bytes_received <= 0)
            continue;

        recv(client_socket, reinterpret_cast<char*>(&color_code), sizeof(color_code), 0);
        recv(client_socket, str, sizeof(str), 0);

        eraseText(6);
        if (strcmp(name, "#NULL") != 0)
            cout << color(color_code) << name << " : " << def_col << str << endl;
        else
            cout << color(color_code) << str << endl;

        cout << colors[1] << "You: " << def_col;
        fflush(stdout);
    }
}
