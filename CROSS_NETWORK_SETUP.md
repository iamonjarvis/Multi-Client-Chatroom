# Cross-Network Setup Instructions

## Overview

To enable cross-network communication for the multi-client chatroom application, you need to ensure that the server is accessible over the internet and that the clients can connect to it from different networks. This requires configuring port forwarding on the server's network and updating the client configuration to use the server's public IP address.

## Steps to Configure Cross-Network Communication

### 1. Configure Port Forwarding

To allow clients outside your local network to connect to your server, you need to set up port forwarding on your router. Follow these steps:

1. **Access Router Settings:**
   - Open a web browser and enter your router’s IP address (often `192.168.1.1`, `192.168.0.1`, or similar). You can usually find this information in your router's manual or by checking your network settings.

2. **Log In:**
   - Enter your router’s username and password. If you haven't changed these, they are likely the default credentials provided by your router’s manufacturer.

3. **Locate Port Forwarding Section:**
   - Look for a section labeled "Port Forwarding," "Virtual Server," or something similar. This section allows you to create rules for forwarding specific ports to internal IP addresses.

4. **Create a Port Forwarding Rule:**
   - Add a new rule for the port you are using in your chatroom server application (the port number is based on Sahil's date of birth).
   - Set the protocol to TCP/UDP (or TCP if only one is available).
   - Enter the internal IP address of the server machine running the `chat_server` application. This is usually found in your network settings on the server machine.
   - Save the changes and restart your router if necessary.

### 2. Find Your Public IP Address

To allow clients to connect to your server, they need to use your public IP address. Follow these steps to find it:

1. **Find Public IP Address:**
   - Open a web browser and search for "What is my IP" or visit a site like [WhatIsMyIP.com](https://www.whatismyip.com).
   - Note down your public IP address displayed on the website.

### 3. Update Client Configuration

To ensure clients can connect to the server across different networks, update the client code or configuration:

1. **Modify Client Code:**
   - Open the client code file (`client.cpp`).
   - Find the section where the IP address is specified (usually `inet_addr("127.0.0.1")`).
   - Replace `127.0.0.1` with your server’s public IP address.

   Example:
   ```cpp
   client.sin_addr.s_addr = inet_addr("YOUR_PUBLIC_IP_ADDRESS");
2.Compile and Run the Client:
  - Recompile the client code.
  - Run the client application and enter your username when prompted.
    
### Troubleshooting
- Check Firewall Settings: Ensure that any firewall on your server machine or network is configured to allow traffic on the specified port.
- Verify Port Forwarding: Double-check the port forwarding rule and internal IP address.
- Test Connectivity: Use tools like ping or telnet to test connectivity to your server’s public IP address and port.
  
If you encounter issues, consult your router's documentation or contact your network administrator for assistance.

