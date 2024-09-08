# Multi-Client Chatroom

## Description

This project implements a multi-client chatroom using socket programming in C++. It includes both server and client applications, allowing multiple clients to connect and communicate in real-time. The chatroom supports basic functionalities like sending and receiving messages, handling user connections, and disconnecting clients.

The project is designed to be cross-platform and works on both Linux and Windows environments.

## Features

- Real-time communication between multiple clients.
- Support for multiple colors to distinguish messages.
- Graceful handling of client disconnections.
- Cross-platform compatibility (Linux and Windows).

## Prerequisites

- **For Linux:**

  - A C++ compiler (e.g., g++)
  - POSIX libraries (usually included in most Linux distributions)

- **For Windows:**
  - Microsoft Visual Studio(recommended) or MinGW (for compiling C++ code)
  - Winsock library (included in Windows SDK)

## Setup

### Server

1. **Compile the Server Code:**
   ```sh
   g++ -o chat_server server.cpp -lpthread
   ```
2. **Run the Server:**
   ./chat_server

Note: To enable cross-network communication, you need to:

    Change the client code to use the public IP address of the server.
    Configure port forwarding on the router of the server's network.
    Use the port you specify in the server code (in this code i have used 29001 which dob).

### Client

1. **Compile the Client Code:**
   g++ -o chat_client client.cpp -lpthread

2. **Run the Client:**
   ./chat_client

3. Enter Your Name: When prompted, enter your desired username. The client will then connect to the chatroom server.

4.**Configure for Cross-Network:**

    Modify the client code to replace the local IP address with the public IP address of the server.
    Ensure that the server's network allows port forwarding and uses the correct port.

### Usage

- Start the server application (chat_server).
- Start one or more instances of the client application (chat_client).
- Each client will prompt for a username. Enter a unique name to join the chatroom.
- Once connected, you can send messages to other clients. Type your message and press Enter.
  Type #exit to disconnect from the chatroom.

### Code Documentation

**Server Code (server.cpp):**

        Manages client connections.
        Handles incoming messages and broadcasts them to other clients.
        Provides functionality to gracefully disconnect clients and clean up resources.

**Client Code (client.cpp):**
Connects to the chatroom server.
Provides a user interface for sending and receiving messages.
Handles disconnection and clean-up operations.

### Example

Here's an example of a typical interaction:
Client A: Enter your name: Alice
Client B: Enter your name: Bob

Server: Alice has joined
Server: Bob has joined

Alice: Hello, Bob!
Bob: Hi Alice! How are you?

Alice: I'm good, thanks!
Bob: Great to hear!

Alice: #exit

### Contributing

Contributions are welcome! Please fork the repository and submit a pull request with any changes or improvements.
License

This project is licensed under the MIT License - see the LICENSE file for details.
Author

This project was developed by Sahil Patra. For any questions or feedback, please reach out to sahilp123456@gmail.com
