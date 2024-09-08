/*
 * =====================================================================================
 *
 *       Filename:  client.cpp
 *
 *    Description:  Multi-Chat Room Client Program
 *
 *        Version:  1.0
 *        
 *       Compiler:  g++ (Linux), MSVC (Windows)
 *
 *         Author:  Sahil Patra
 *   
 *
 * =====================================================================================
 *
 * Purpose:
 * This C++ client program is part of a multi-chat room application. The client connects
 * to a chat server and allows users to send and receive messages in real time. It is designed
 * to work on both Linux and Windows platforms using platform-specific socket APIs.
 *
 */

#include <iostream>
#include <thread>
#include <string>
#include <cstring>
#include <mutex>
#include <csignal>

#ifdef _WIN32
#include <winsock2.h>  // For Windows socket functionality
#include <ws2tcpip.h>  // For InetPton
#pragma comment(lib, "ws2_32.lib")  // Link the Winsock library
#define close closesocket
#define SOCKET_TYPE SOCKET  // Define SOCKET_TYPE for Windows
#else
#include <sys/types.h>      // For Linux socket functionality
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET_TYPE int  // Define SOCKET_TYPE for Linux
#endif

#define MAX_LEN 200  // Maximum message length
#define NUM_COLORS 6  // Number of colors for message output

using namespace std;

// Global variables for socket communication
bool exit_flag = false;  // To track when the client should exit
thread t_send, t_recv;  // Threads for sending and receiving messages
SOCKET_TYPE client_socket;  // Socket descriptor for the client

// Terminal color codes for message formatting
string def_col = "\033[0m";
string colors[] = { "\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m" };

// Function declarations
void catch_ctrl_c(int signal);
string color(int code);
void eraseText(int cnt);
void send_message(SOCKET_TYPE client_socket);
void recv_message(SOCKET_TYPE client_socket);

#ifdef _WIN32
// Initialize Winsock for Windows
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
    init_winsock();  // Initialize Winsock on Windows
#endif

    // Create a socket (AF_INET for IPv4, SOCK_STREAM for TCP)
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
#ifdef _WIN32
        cerr << "Socket creation failed. Error: " << WSAGetLastError() << endl;
#else
        perror("socket: ");  // Print error on Linux
#endif
        exit(EXIT_FAILURE);
    }

    // Define server address and port
    struct sockaddr_in client;
    client.sin_family = AF_INET;  // IPv4
    client.sin_port = htons(29001);  // Port number of the server

#ifdef _WIN32
    // Use InetPton to convert IP address on Windows
    InetPton(AF_INET, TEXT("127.0.0.1"), &client.sin_addr);
#else
    client.sin_addr.s_addr = inet_addr("127.0.0.1");  // Set server address to localhost on Linux
#endif

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&client, sizeof(client)) == -1) {
#ifdef _WIN32
        cerr << "Connect failed. Error: " << WSAGetLastError() << endl;
#else
        perror("connect: ");  // Print error on Linux
#endif
        exit(EXIT_FAILURE);
    }

    // Handle Ctrl+C to exit gracefully
    signal(SIGINT, catch_ctrl_c);

    // Get user's name and send it to the server
    char name[MAX_LEN];
    cout << "Enter your name: ";
    cin.getline(name, MAX_LEN);
    send(client_socket, name, sizeof(name), 0);

    // Welcome message
    cout << colors[NUM_COLORS - 1] << "\n\t  ====== Welcome to the chat-room ======   " << endl << def_col;

    // Launch threads for sending and receiving messages
    thread t1(send_message, client_socket);
    thread t2(recv_message, client_socket);
    t_send = move(t1);
    t_recv = move(t2);

    // Wait for both threads to finish
    if (t_send.joinable())
        t_send.join();
    if (t_recv.joinable())
        t_recv.join();

#ifdef _WIN32
    WSACleanup();  // Cleanup Winsock on Windows
#endif

    return 0;
}

// Signal handler for Ctrl+C (SIGINT)
void catch_ctrl_c(int signal) {
    char str[MAX_LEN] = "#exit";  // Send exit message to server
    send(client_socket, str, sizeof(str), 0);
    exit_flag = true;  // Set exit flag to true
    t_send.detach();  // Detach send thread
    t_recv.detach();  // Detach receive thread
    close(client_socket);  // Close the socket
    exit(signal);  // Exit the program
}

// Function to get a color from the array
string color(int code) {
    return colors[code % NUM_COLORS];
}

// Function to erase text from the terminal (backspaces)
void eraseText(int cnt) {
    char back_space = 8;  // ASCII code for backspace
    for (int i = 0; i < cnt; i++) {
        cout << back_space;
    }
}

// Function to send messages to the server
void send_message(SOCKET_TYPE client_socket) {
    while (1) {
        cout << colors[1] << "You: " << def_col;
        char str[MAX_LEN];
        cin.getline(str, MAX_LEN);  // Get message from the user
        send(client_socket, str, sizeof(str), 0);  // Send message to server

        // Check if the user wants to exit
        if (strcmp(str, "#exit") == 0) {
            exit_flag = true;  // Set exit flag to true
            t_recv.detach();  // Detach receive thread
            close(client_socket);  // Close socket
            return;
        }
    }
}

// Function to receive messages from the server
void recv_message(SOCKET_TYPE client_socket) {
    while (1) {
        if (exit_flag)
            return;  // Exit if the flag is set

        char name[MAX_LEN];
        char str[MAX_LEN];
        int color_code;

        // Receive the name of the sender
        int bytes_received = recv(client_socket, name, sizeof(name), 0);
        if (bytes_received <= 0)
            continue;  // If nothing received, continue

        recv(client_socket, reinterpret_cast<char*>(&color_code), sizeof(color_code), 0);  // Receive color code
        recv(client_socket, str, sizeof(str), 0);  // Receive message

        eraseText(6);  // Erase the "You: " prompt

        // Display the received message
        if (strcmp(name, "#NULL") != 0)
            cout << color(color_code) << name << " : " << def_col << str << endl;
        else
            cout << color(color_code) << str << endl;

        cout << colors[1] << "You: " << def_col;  // Prompt the user for the next message
        fflush(stdout);  // Force flush the output buffer
    }
}
