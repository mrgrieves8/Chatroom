#include "Server.h"
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <unordered_map>
#include <set>
#include <sstream>
#include <vector>
#include <set>
#include <atomic>
#include <csignal>
#include "../common/Message.h"


Server::Server(const std::string& ip, int port) : ip(ip), port(port), server_fd(-1), epoll_fd(-1) {
    std::cout << "Initializing server..." << std::endl;
    // Additional initialization if needed
}


Server::~Server() {
    if (server_fd != -1) {
        std::cout << "Closing server socket..." << std::endl;
        close(server_fd);
    }
    if (epoll_fd != -1) {
        std::cout << "Closing epoll file descriptor..." << std::endl;
        close(epoll_fd);
    }
}


bool Server::init() {
    std::cout << "Starting server initialization..." << std::endl;
    if (!createServerSocket()) {
        std::cerr << "Failed to create server socket." << std::endl;
        return false;
    }
    if (!initEpoll()) {
        std::cerr << "Failed to initialize epoll." << std::endl;
        return false;
    }
    std::cout << "Server initialization successful." << std::endl;

    // Use the createChatroom method to initialize the default chatroom
    createChatroom("default");
    std::cout << "Default chatroom created." << std::endl;

    return true;
}


bool Server::createServerSocket() {
    std::cout << "Creating server socket..." << std::endl;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Error creating a socket" << std::endl;
        return false;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_address.sin_addr);

    if (bind(server_fd, (sockaddr*)&server_address, sizeof(server_address)) == -1) {
        std::cerr << "Error binding to IP/port" << std::endl;
        return false;
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        std::cerr << "Error listening" << std::endl;
        return false;
    }
    std::cout << "Socket created and listening on IP: " << ip << ", Port: " << port << std::endl;
    return true;
}


bool Server::initEpoll() {
    std::cout << "Initializing epoll..." << std::endl;
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "Error creating epoll instance" << std::endl;
        return false;
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        std::cerr << "Error adding socket to epoll" << std::endl;
        return false;
    }
    std::cout << "Epoll instance created and configured." << std::endl;
    return true;
}


// Signal handler for graceful shutdown
std::atomic<bool> running(true);

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received. Shutting down..." << std::endl;
    running = false;
}

void Server::run() {
    std::cout << "Server is now running..." << std::endl;
    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];

    // Set up signal handler for graceful shutdown
    signal(SIGINT, signalHandler);

    while (running) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            if (errno == EINTR) {
                std::cout << "Server stopping due to interrupt." << std::endl;
                break; // Interrupted by signal
            }
            std::cerr << "Epoll wait error: " << strerror(errno) << std::endl;
            continue; // Or handle the error as appropriate
        }

        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == server_fd) {
                handleNewConnection();
            } else {
                handleClientData(events[i].data.fd);
            }
        }
    }

    closeAllConnections();
    std::cout << "Server shutdown complete." << std::endl;
}

void Server::handleNewConnection() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(server_fd, (sockaddr*)&client_addr, &client_len);

    if (client_socket == -1) {
        std::cerr << "Error accepting new connection: " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "New client connected: Socket FD " << client_socket << std::endl;
    handleUsernameRequest(client_socket);

    struct epoll_event client_event;
    client_event.events = EPOLLIN;
    client_event.data.fd = client_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &client_event) == -1) {
        std::cerr << "Error adding client socket to epoll: " << strerror(errno) << std::endl;
        close(client_socket); // Ensure the socket is closed on error
    }
}


void Server::closeAllConnections() {
    // Close all client sockets and clean up resources
    for (auto& pair : clientUsernames) {
        close(pair.first); // Close each client socket
    }
    close(server_fd);  // Close the server socket
    close(epoll_fd);   // Close the epoll file descriptor
}

void Server::sendWelcomeMessage(int client_socket) {
    Message welcomeMessage(MessageType::POST, "Welcome to the chat server!\nPlease enter username:\n");
    sendMessage(client_socket, welcomeMessage);
    std::cout << "Welcome message sent to client: Socket FD " << client_socket << std::endl;
}

void Server::createChatroom(const std::string& name) {
    // Implementation logic
    if (chatrooms.find(name) == chatrooms.end()) {
        Chatroom newChatroom;
        newChatroom.name = name;
        // Initialize other members if necessary
        chatrooms[name] = newChatroom;
        std::cout << "Chatroom '" << name << "' created successfully." << std::endl;
    } else {
        std::cout << "Chatroom '" << name << "' already exists." << std::endl;
    }
}


void Server::handleUsernameRequest(int client_socket) {
    sendWelcomeMessage(client_socket);
    char buffer[1024];
    int bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
    std::cout << "Handling username request for client: Socket FD " << client_socket << std::endl;

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string msg(buffer);
        Message usernameMessage = Message::deserialize(msg);
        std::string username = usernameMessage.getBody();
        std::cout << "Received username: " << username << " from client: Socket FD " << client_socket << std::endl;

        if (isValidUsername(username)) {
            ClientInfo newClient;
            newClient.username = username;
            newClient.socketNum = client_socket;
            clientUsernames[client_socket] = newClient;
            std::cout << "Username '" << username << "' is valid and assigned to client: Socket FD " << client_socket << std::endl;
            displayMenu(client_socket);
        } else {
            std::cerr << "Invalid username '" << username << "' received from client: Socket FD " << client_socket << std::endl;
            Message loginFailMsg(MessageType::POST, "Login failed: Invalid username.");
            sendMessage(client_socket, loginFailMsg);
            std::cout << "Invalid username message sent, closing connection for client: Socket FD " << client_socket << std::endl;
            close(client_socket); // Close the connection if login fails
        }
    } else {
        if (bytesRead == -1) {
            std::cerr << "Error receiving username from client: Socket FD " << client_socket << ". Error: " << strerror(errno) << std::endl;
        } else {
            std::cerr << "Client: Socket FD " << client_socket << " closed the connection." << std::endl;
        }
        // Handle error or disconnection
        close(client_socket);
    }
}



void Server::handleClientData(int client_socket) {
    char buffer[4096];
    int bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);

    if (bytesRead <= 0) {
        // Client disconnected
        std::cout << "Client disconnected: Socket FD " << client_socket << std::endl;

        // Remove client from chatroom and update chatroom mappings
        handleClientDisconnect(client_socket); 

        // Close the socket and remove from epoll monitoring
        close(client_socket);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr);

        // Remove client information
        clientUsernames.erase(client_socket);
        removeFromChatroomMapping(client_socket);
        return;
    }

    buffer[bytesRead] = '\0';
    std::string message(buffer);
    processClientMessage(client_socket, message);
}


void Server::displayMenu(int client_socket) {
    std::stringstream menu;
    menu << "Available chatrooms:\n";
    for (const auto& pair : chatrooms) {
        menu << pair.first << "\n"; // Chatroom name
    }
    menu << "Enter a chatroom name to join or create a new one:\n";

    Message menuMessage(MessageType::POST, menu.str());
    sendMessage(client_socket, menuMessage);
    std::cout << "Menu displayed to client: Socket FD " << client_socket << std::endl;
}


void Server::joinChatroom(int client_socket, const std::string& chatroomName) {
    chatrooms[chatroomName].clients.insert(client_socket);
    std::cout << "Socket FD " << client_socket << " has joined room " << chatroomName << std::endl;

    // Build the chat history as a single string
    std::stringstream chatHistory;
    for (const std::string& serializedMessage : chatrooms[chatroomName].messages) {
        Message message = Message::deserialize(serializedMessage);
        chatHistory << message.getBody() << "\n";
    }

    // Send the chat history as one message
    if (!chatHistory.str().empty()) {
        Message historyMessage(MessageType::POST, chatHistory.str());
        std::cout << chatHistory << std::endl;
        sendMessage(client_socket, historyMessage);
    }
}


void Server::broadcastMessage(const std::string& chatroomName, const Message& message) {
    std::string serializedMessage = message.getBody();
    
    // Send the message to all clients in the chatroom
    for (int client_socket : chatrooms[chatroomName].clients) {
        sendMessage(client_socket, message);
    }

    // Append the serialized message to the chat history
    chatrooms[chatroomName].messages.push_back(serializedMessage);
}




void Server::sendMessage(int client_socket, const Message& message) {
    std::string serializedMessage = message.serialize() + "\n";
    send(client_socket, serializedMessage.c_str(), serializedMessage.length(), 0);
}



void Server::processClientMessage(int client_socket, const std::string& serializedMessage) {
    Message message = Message::deserialize(serializedMessage);

    std::cout << "Received message from client " << client_socket 
              << ": Type=" << static_cast<int>(message.getType()) 
              << ", Body=" << message.getBody() << std::endl;


    switch (message.getType()) {
        case MessageType::JOIN:
            processJoinMessage(client_socket, message);
            break;
        case MessageType::LEAVE:
            processLeaveMessage(client_socket, message);
            break;
        case MessageType::QUIT:
            processQuitMessage(client_socket, message);
            break;
        case MessageType::POST:
            processPostMessage(client_socket, message);
            break;
        default:
            // Handle unknown message type
            break;
    }
}

// JOIN
void Server::processJoinMessage(int client_socket, const Message& message) {
    std::string chatroomName = message.getBody();

    if (!findClientChatroom(client_socket).empty() && findClientChatroom(client_socket) != chatroomName) {
        Message alreadyInMsg(MessageType::POST, "You are already in chatroom " + findClientChatroom(client_socket) + ". Please leave it first.");
        sendMessage(client_socket, alreadyInMsg);
    } else if (chatrooms.find(chatroomName) != chatrooms.end()) {
        joinChatroom(client_socket, chatroomName);
        std::string joinMsg = "[" + clientUsernames[client_socket].username + "] has joined " + chatroomName;
        Message joinMessage(MessageType::JOIN, joinMsg);
        broadcastMessage(chatroomName, joinMessage);
    } else {
        Message errorMsg(MessageType::POST, "Chatroom '" + chatroomName + "' does not exist.");
        sendMessage(client_socket, errorMsg);
    }
}

// LEAVE
void Server::processLeaveMessage(int client_socket, const Message& message) {
    std::string currentChatroom = findClientChatroom(client_socket);
    if (!currentChatroom.empty()) {
        chatrooms[currentChatroom].clients.erase(client_socket);
        removeFromChatroomMapping(client_socket);
        std::string leaveMsg = "[" + clientUsernames[client_socket].username + "] has left " + currentChatroom;
        Message leaveMessage(MessageType::LEAVE, leaveMsg);
        sendMessage(client_socket, leaveMessage);
        broadcastMessage(currentChatroom, leaveMessage);
        displayMenu(client_socket);
    } else {
        Message notInChatroomMessage(MessageType::POST, "You are not currently in a chatroom.");
        sendMessage(client_socket, notInChatroomMessage);
    }
}



// QUIT
void Server::processQuitMessage(int client_socket, const Message& message) {
    // Implementation of quit logic
    // Close the connection with the client
    close(client_socket);
}

// POST
void Server::processPostMessage(int client_socket, const Message& message) {
    std::string chatroomName = findClientChatroom(client_socket);
    if (!chatroomName.empty()) {
        std::string formattedMessage = "[" + clientUsernames[client_socket].username + "]: " + message.getBody();
        Message postMessage(MessageType::POST, formattedMessage);
        broadcastMessage(chatroomName, postMessage);
    } else {
        Message notInChatroomMessage(MessageType::POST, "You need to join a chatroom to send messages.");
        sendMessage(client_socket, notInChatroomMessage);
    }
}



void Server::removeFromChatroomMapping(int client_socket) {
    auto it = clientToChatroomMap.find(client_socket);
    if (it != clientToChatroomMap.end()) {
        // If the client is in the map, remove them.
        clientToChatroomMap.erase(it);
    }
}


void Server::handleClientDisconnect(int client_socket) {
    std::string chatroomName = findClientChatroom(client_socket);
    if (!chatroomName.empty()) {
        // Remove the client from the chatroom
        chatrooms[chatroomName].clients.erase(client_socket);

        // Create a message indicating that the client has left the chatroom
        std::string leaveMsg = "[" + clientUsernames[client_socket].username + "] has left " + chatroomName;
        Message leaveMessage(MessageType::LEAVE, leaveMsg);

        // Broadcast the leave message to other clients in the chatroom
        broadcastMessage(chatroomName, leaveMessage);

        // Remove client from the mapping
        removeFromChatroomMapping(client_socket);
    }

    // Remove client info
    clientUsernames.erase(client_socket);
}

std::string Server::findClientChatroom(int client_socket) {
    // Iterate through all chatrooms
    for (const auto& pair : chatrooms) {
        const std::string& chatroomName = pair.first;
        const Chatroom& chatroom = pair.second;

        // Check if the client is in the current chatroom
        if (chatroom.clients.find(client_socket) != chatroom.clients.end()) {
            return chatroomName;
        }
    }
    
    // Return an empty string if the client is not in any chatroom
    return "";
}

bool Server::isValidUsername(const std::string& username) {
    if (username.length() > 25) {
        return false;
    }

    // Check if username is already taken
    for (const auto& pair : clientUsernames) {
        if (pair.second.username == username) {
            return false;
        }
    }

    return true;
}
