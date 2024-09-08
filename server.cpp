#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <cstring> // For strcpy and strcmp

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define close closesocket
#define INVALID_SOCKET_VALUE INVALID_SOCKET
#define SOCKET_TYPE SOCKET
#define strcpy_safe(dest, src, size) strcpy_s(dest, size, src)
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET_TYPE int
#define INVALID_SOCKET_VALUE -1
#define strcpy_safe(dest, src, size) strncpy(dest, src, size)
#endif

#define MAX_LEN 200
#define NUM_COLORS 6

using namespace std;

struct terminal {
    int id;
    string name;
    SOCKET_TYPE socket;
    thread th;
    chrono::time_point<chrono::steady_clock> start_time;
};

vector<terminal> clients;
string def_col = "\033[0m";
string colors[] = { "\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m" };
int seed = 0;
std::mutex cout_mtx, clients_mtx;

string color(int code);
void set_name(int id, char name[]);
void shared_print(string str, bool endLine = true);
void broadcast_message(string message, int sender_id);
void broadcast_message(int num, int sender_id);
void end_connection(int id);
void handle_client(SOCKET_TYPE client_socket, int id);

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

    SOCKET_TYPE server_socket;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET_VALUE) {
#ifdef _WIN32
        cerr << "Socket creation failed. Error: " << WSAGetLastError() << endl;
#else
        perror("socket: ");
#endif
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(29001);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) == -1) {
#ifdef _WIN32
        cerr << "Bind failed. Error: " << WSAGetLastError() << endl;
#else
        perror("bind error: ");
#endif
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 8) == -1) {
#ifdef _WIN32
        cerr << "Listen failed. Error: " << WSAGetLastError() << endl;
#else
        perror("listen error: ");
#endif
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client;
#ifdef _WIN32
    int len = sizeof(sockaddr_in);  // Use int on Windows
#else
    socklen_t len = sizeof(sockaddr_in);  // Use socklen_t on Linux/macOS
#endif

    cout << colors[NUM_COLORS - 1] << "\n\t  ====== Welcome to the chat-room ======   " << endl
        << def_col;

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
        seed++;

        auto start_time = chrono::steady_clock::now();

        thread t(handle_client, client_socket, seed);
        std::lock_guard<std::mutex> guard(clients_mtx);
        clients.push_back(terminal{ seed, "Anonymous", client_socket, std::move(t), start_time });
    }

    for (auto& client : clients) {
        if (client.th.joinable())
            client.th.join();
    }

    close(server_socket);
#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}

string color(int code) {
    return colors[code % NUM_COLORS];
}

void set_name(int id, char name[]) {
    for (auto& client : clients) {
        if (client.id == id) {
            client.name = string(name);
        }
    }
}

void shared_print(string str, bool endLine) {
    lock_guard<mutex> guard(cout_mtx);
    cout << str;
    if (endLine)
        cout << endl;
}

void broadcast_message(string message, int sender_id) {
    lock_guard<mutex> guard(clients_mtx);
    char temp[MAX_LEN];
    strcpy_safe(temp, message.c_str(), MAX_LEN);
    for (const auto& client : clients) {
        if (client.id != sender_id) {
            send(client.socket, temp, sizeof(temp), 0);
        }
    }
}

void broadcast_message(int num, int sender_id) {
    lock_guard<mutex> guard(clients_mtx);
    for (const auto& client : clients) {
        if (client.id != sender_id) {
            send(client.socket, reinterpret_cast<const char*>(&num), sizeof(num), 0);
        }
    }
}

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
            shared_print(message);

            broadcast_message("#NULL", id);
            broadcast_message(id, id);
            broadcast_message(message, id);

            it->th.detach();
            close(it->socket);
            clients.erase(it);
            break;
        }
    }
}

void handle_client(SOCKET_TYPE client_socket, int id) {
    char name[MAX_LEN], str[MAX_LEN];
    recv(client_socket, name, sizeof(name), 0);
    set_name(id, name);

    string welcome_message = string(name) + " has joined";
    broadcast_message("#NULL", id);
    broadcast_message(id, id);
    broadcast_message(welcome_message, id);
    shared_print(color(id) + welcome_message + def_col);

    while (true) {
        int bytes_received = recv(client_socket, str, sizeof(str), 0);
        if (bytes_received <= 0)
            return;
        if (strcmp(str, "#exit") == 0) {
            string message = string(name) + " has left";
            broadcast_message("#NULL", id);
            broadcast_message(id, id);
            broadcast_message(message, id);
            shared_print(color(id) + message + def_col);
            end_connection(id);
            return;
        }
        broadcast_message(string(name), id);
        broadcast_message(id, id);
        broadcast_message(string(str), id);
        shared_print(color(id) + name + " : " + def_col + str);
    }
}
