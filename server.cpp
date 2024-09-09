/*
 * =====================================================================================
 *
 *       Filename:  server.cpp
 *
 *    Description:  Multi-Chat Room Server Program
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
 * This C++ server program is part of a multi-chat room application. It listens for client
 * connections and facilitates real-time communication between them. The server broadcasts
 * messages to all connected clients and manages client join/leave events. It is designed
 * to work on both Linux and Windows platforms using platform-specific socket APIs.
 */

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <cstring> // For strcpy and strcmp

#ifdef _WIN32
#include <winsock2.h>  // Windows-specific socket library
#include <ws2tcpip.h>  // For InetPton on Windows
#pragma comment(lib, "ws2_32.lib")  // Link Winsock library
#define close closesocket  // Close function for Windows
#define INVALID_SOCKET_VALUE INVALID_SOCKET  // Error value for invalid socket
#define SOCKET_TYPE SOCKET  // Define socket type for Windows
#define strcpy_safe(dest, src, size) strcpy_s(dest, size, src)  // Safe string copy for Windows
#else
#include <sys/types.h>  // Linux socket library
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET_TYPE int  // Define socket type for Linux
#define INVALID_SOCKET_VALUE -1  // Error value for invalid socket on Linux
#define strcpy_safe(dest, src, size) strncpy(dest, src, size)  // Safe string copy for Linux
#endif

#define MAX_LEN 200  // Maximum message length
#define NUM_COLORS 6  // Number of colors for message display

using namespace std;

// Structure to represent a connected client (terminal)
struct terminal {
    int id;  // Unique client ID
    string name;  // Client's name
    SOCKET_TYPE socket;  // Client's socket descriptor
    thread th;  // Thread handling client communication
    chrono::time_point<chrono::steady_clock> start_time;  // Start time of connection
};

// Global variables
vector<terminal> clients;  // List of connected clients
string def_col = "\033[0m";  // Default terminal color
string colors[] = { "\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m" };  // Color options for clients
int seed = 0;  // ID counter for clients
std::mutex cout_mtx, clients_mtx;  // Mutexes for thread-safe output and client list management

// Function declarations
string color(int code);
void set_name(int id, char name[]);
void shared_print(string str, bool endLine = true);
void broadcast_message(string message, int sender_id);
void broadcast_message(int num, int sender_id);
void end_connection(int id);
void handle_client(SOCKET_TYPE client_socket, int id);

#ifdef _WIN32
// Initialize Winsock on Windows
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

    // Create a server socket (IPv4, TCP)
    SOCKET_TYPE server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        cerr << "Socket creation failed. Error: " << WSAGetLastError() << endl;
#else
        perror("socket: ");  // Print error on Linux
#endif
        exit(EXIT_FAILURE);
    }

    // Define server address and port
    struct sockaddr_in server;
    server.sin_family = AF_INET;  // IPv4
    server.sin_port = htons(29001);  // Server port number (29001)
    server.sin_addr.s_addr = INADDR_ANY;  // Bind to any available interface (public IP)

    // Bind socket to the server address
    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) == -1) {
#ifdef _WIN32
        cerr << "Bind failed. Error: " << WSAGetLastError() << endl;
#else
        perror("bind error: ");
#endif
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming client connections (backlog 8)
    if (listen(server_socket, 8) == -1) {
#ifdef _WIN32
        cerr << "Listen failed. Error: " << WSAGetLastError() << endl;
#else
        perror("listen error: ");
#endif
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client;  // Client address
#ifdef _WIN32
    int len = sizeof(sockaddr_in);  // Use int for Windows
#else
    socklen_t len = sizeof(sockaddr_in);  // Use socklen_t for Linux
#endif

    cout << colors[NUM_COLORS - 1] << "\n\t  ====== Welcome to the chat-room ======   " << endl << def_col;

    // Main loop to accept new client connections
    while (true) {
        SOCKET_TYPE client_socket;
        if ((client_socket = accept(server_socket, (struct sockaddr*)&client, &len)) == INVALID_SOCKET_VALUE) {
#ifdef _WIN32
            cerr << "Accept failed. Error: " << WSAGetLastError() << endl;
#else
            perror("accept error: ");
#endif
            exit(EXIT_FAILURE);
        }

        // Assign a unique ID to each client and start handling the client in a new thread
        seed++;
        auto start_time = chrono::steady_clock::now();
        thread t(handle_client, client_socket, seed);
        std::lock_guard<std::mutex> guard(clients_mtx);
        clients.push_back(terminal{ seed, "Anonymous", client_socket, std::move(t), start_time });
    }

    // Close the server socket when done
    for (auto& client : clients) {
        if (client.th.joinable())
            client.th.join();
    }

    close(server_socket);
#ifdef _WIN32
    WSACleanup();  // Cleanup Winsock on Windows
#endif

    return 0;
}

// Function to return a color based on the client's ID
string color(int code) {
    return colors[code % NUM_COLORS];
}

// Function to set a client's name after connecting
void set_name(int id, char name[]) {
    for (auto& client : clients) {
        if (client.id == id) {
            client.name = string(name);  // Update the client's name
        }
    }
}

// Thread-safe function to print messages to the console
void shared_print(string str, bool endLine) {
    lock_guard<mutex> guard(cout_mtx);  // Lock mutex for thread-safe printing
    cout << str;
    if (endLine)
        cout << endl;
}

// Function to broadcast a message to all connected clients, except the sender
void broadcast_message(string message, int sender_id) {
    lock_guard<mutex> guard(clients_mtx);  // Lock mutex for thread-safe client list access
    char temp[MAX_LEN];
    strcpy_safe(temp, message.c_str(), MAX_LEN);  // Copy message into buffer
    for (const auto& client : clients) {
        if (client.id != sender_id) {
            send(client.socket, temp, sizeof(temp), 0);  // Send message to clients
        }
    }
}

// Function to broadcast an integer (such as an ID) to all clients
void broadcast_message(int num, int sender_id) {
    lock_guard<mutex> guard(clients_mtx);
    for (const auto& client : clients) {
        if (client.id != sender_id) {
            send(client.socket, reinterpret_cast<const char*>(&num), sizeof(num), 0);
        }
    }
}

// Function to handle a client disconnection and broadcast the leave message
void end_connection(int id) {
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->id == id) {
            auto end_time = chrono::steady_clock::now();
            chrono::duration<double> duration = end_time - it->start_time;
            double duration_seconds = duration.count();
            int minutes = static_cast<int>(duration_seconds) / 60;
            int seconds = static_cast<int>(duration_seconds) % 60;

            string message = it->name + " has left the chat after " +
                to_string(minutes) + " minute(s) and " +
                to_string(seconds) + " second(s)";
            shared_print(message);  // Print message to server console

            // Notify clients about the disconnection
            broadcast_message("#NULL", id);
            broadcast_message(id, id);
            broadcast_message(message, id);

            it->th.detach();
            close(it->socket);  // Close the client's socket
            clients.erase(it);  // Remove client
            break;  // Exit the loop after removing the client
        }
    }
}

// Function to handle communication with a client
void handle_client(SOCKET_TYPE client_socket, int id) {
    char name[MAX_LEN], str[MAX_LEN];
    
    // Receive and set the client's name
    recv(client_socket, name, sizeof(name), 0);
    set_name(id, name);

    // Broadcast the join message to all other clients
    string welcome_message = string(name) + " has joined";
    broadcast_message("#NULL", id);
    broadcast_message(id, id);
    broadcast_message(welcome_message, id);
    shared_print(color(id) + welcome_message + def_col);

    // Main loop to receive and handle client messages
    while (true) {
        int bytes_received = recv(client_socket, str, sizeof(str), 0);
        if (bytes_received <= 0)  // Connection closed or error
            return;

        // Check for the exit command
        if (strcmp(str, "#exit") == 0) {
            string message = string(name) + " has left";
            broadcast_message("#NULL", id);
            broadcast_message(id, id);
            broadcast_message(message, id);
            shared_print(color(id) + message + def_col);
            end_connection(id);  // Handle client disconnection
            return;
        }

        // Broadcast the received message to all other clients
        broadcast_message(string(name), id);
        broadcast_message(id, id);
        broadcast_message(string(str), id);
        shared_print(color(id) + name + " : " + def_col + str);
    }
}
