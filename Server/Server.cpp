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
#include <sstream>
#include <set>
#include <atomic>
#include <csignal>
#include "Chatroom/Chatroom.h"
#include "../common/Message.h"
#include <thread>


Server::Server(const std::string& ip, int port) : ip(ip), port(port), server_fd(-1), epoll_fd(-1) {
    std::cout << "Initializing server..." << std::endl;
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
    createChatroom("defaultChat");
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

    // Set SO_REUSEADDR to ensure the port is freed immediately after server shutdown
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Error setting SO_REUSEADDR" << std::endl;
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

// Global variable 'running' to manage server state. It's global because signal handlers, 
// used with signal() calls, cannot access non-static class members.
std::atomic<bool> running(true);

// Signal handler 'signalHandler' for graceful shutdown. It's global to modify 'running' 
// and to be compatible with signal() system call requirements.
void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received. Shutting down..." << std::endl;
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
               // Create a new thread to handle the new connection
                std::thread([this] { this->handleNewConnection(); }).detach();
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
    std::cout << "Handled username request " << client_socket << std::endl;
    struct epoll_event client_event;
    client_event.events = EPOLLIN;
    client_event.data.fd = client_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &client_event) == -1) {
        std::cerr << "Error adding client socket to epoll: " << strerror(errno) << std::endl;
        close(client_socket); // Ensure the socket is closed on error
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

        if (username.length() > 25) {
            sendMessage(client_socket, Message(MessageType::QUIT, "Username too long. Please reconnect with a shorter username."));
        } else if (!isUsernameAvailable(username)) {
            sendMessage(client_socket, Message(MessageType::QUIT, "Username taken. Please reconnect with a different username."));
        } else {
            // If the username is valid and available, proceed to assign it to the client
            ClientInfo newClient;
            newClient.username = username;
            newClient.socketNum = client_socket;
            clientUsernames[client_socket] = newClient;
            std::cout << "Username '" << username << "' is valid and assigned to client: Socket FD " << client_socket << std::endl;

            // Display the chat menu for the client
            displayMenu(client_socket);
            return; // Continue with the normal flow
        }
    } else {
        if (bytesRead == -1) {
            std::cerr << "Error receiving username from client: Socket FD " << client_socket << ". Error: " << strerror(errno) << std::endl;
        } else {
            std::cerr << "Client: Socket FD " << client_socket << " closed the connection." << std::endl;
        }
    }
    
    // Close the connection if the username is invalid or if an error occurred
    close(client_socket);
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
    Message welcomeMessage(MessageType::POST, "Welcome to the chat server!\nPlease enter username:");
    sendMessage(client_socket, welcomeMessage);
    std::cout << "Welcome message sent to client: Socket FD " << client_socket << std::endl;
}


void Server::createChatroom(const std::string& name, const std::set<std::string>& forbiddenWords) {
    Chatroom newChatroom(name, forbiddenWords);
    std::string welcomeMessage = "\n[Server]: Welcome to the chatroom '" + name + "'.\nYou can send messages to the chat now.\nType '/leave' to exit the chatroom.";
    newChatroom.addMessage(welcomeMessage);
    chatrooms[name] = newChatroom;
    std::cout << "Chatroom '" << name << "' created successfully with welcome message." << std::endl;
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
        leaveChatroom(client_socket);
        return;
    }

    buffer[bytesRead] = '\0';
    std::string message(buffer);
    processClientMessage(client_socket, message);
}


void Server::displayMenu(int client_socket) {
    std::stringstream menu;
    // Greeting with username
    menu << "Hello " << clientUsernames[client_socket].username << "!\n";
    // Note about quitting
    menu << "At any time, use /quit to exit the chat server.\n\n";

    // Displaying available chatrooms
    menu << "Available chatrooms:\n";
    for (const auto& pair : chatrooms) {
        menu << "---- " << pair.first << "\n"; // Chatroom name
    }

    // Instructions for joining and creating chatrooms
    menu << "\nTo enter a chatroom, type its name and press Enter.\n";
    menu << "To create a new chatroom, use the command:\n\t /create [chatroom name];[forbidden words]\n";
    menu << "\nExample: /create myRoom;word1,word2\n";

    
    // Send the formatted menu to the client
    Message menuMessage(MessageType::MENU, menu.str());
    sendMessage(client_socket, menuMessage);
    std::cout << "Menu displayed to client: Socket FD " << client_socket << std::endl;
}


void Server::joinChatroom(int client_socket, const std::string& chatroomName) {
    chatrooms[chatroomName].addClient(client_socket);
    clientToChatroomMap[client_socket] = chatroomName;
    std::cout << "Socket FD " << client_socket << " has joined room " << chatroomName << std::endl;

    // Build the chat history as a single string
    std::stringstream chatHistory;
    for (const std::string& messageBody : chatrooms[chatroomName].getMessages()) {
        chatHistory << messageBody << "\n";  // Add each message to the stream
    }

    // Send the chat history as one POST message
    if (!chatHistory.str().empty()) {
        Message historyMessage(MessageType::JOIN, chatHistory.str());
        sendMessage(client_socket, historyMessage);
    }
}


void Server::broadcastMessage(const std::string& chatroomName, const Message& message) {
    // Replace forbidden words
    std::string modifiedMessageBody = chatrooms[chatroomName].censorMessage(message.getBody());

    // Broadcast modified message
    Message modifiedMessage(message.getType(), modifiedMessageBody);
    for (int client_socket : chatrooms[chatroomName].getClients()) {
        sendMessage(client_socket, modifiedMessage);
    }

    // Add to chat history
    chatrooms[chatroomName].addMessage(modifiedMessageBody);
}


void Server::sendMessage(int client_socket, const Message& message) {
    std::string serializedMessage = message.serialize();
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
        case MessageType::CREATE:
            processCreateChatroomMessage(client_socket, message);
            break;
        case MessageType::MENU:
            processMenuMessage(client_socket, message);
            break;
        case MessageType::QUIT:
            handleClientDisconnect(client_socket);
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
    } else {
        if (chatrooms.find(chatroomName) != chatrooms.end()) {
            
            std::string joinMsg = "[" + clientUsernames[client_socket].username + "] has joined " + chatroomName;
            Message joinMessage(MessageType::POST, joinMsg);
            broadcastMessage(chatroomName, joinMessage);
            joinChatroom(client_socket, chatroomName);

        } else {
            Message errorMsg(MessageType::POST, "Chatroom '" + chatroomName + "' does not exist.");
            sendMessage(client_socket, errorMsg);
        }
    }
}


void Server::processCreateChatroomMessage(int client_socket, const Message& message) {
    std::string chatroomInfo = message.getBody();
    std::istringstream ss(chatroomInfo);
    std::string chatroomName;
    std::getline(ss, chatroomName, ';');

    if (chatrooms.find(chatroomName) == chatrooms.end()) {
        std::set<std::string> forbiddenWords;
        std::string word;
        while (std::getline(ss, word, ',')) {
            forbiddenWords.insert(word);
        }
        createChatroom(chatroomName, forbiddenWords);
        joinChatroom(client_socket, chatroomName); // Automatically join the creator to the chatroom
        std::cout << "New chatroom '" << chatroomName << "' created and forbidden words set by client: " << client_socket << std::endl;
    } else {
        Message errorMsg(MessageType::POST, "Chatroom '" + chatroomName + "' already exists.");
        sendMessage(client_socket, errorMsg);
    }
}



// MENU
void Server::processMenuMessage(int client_socket, const Message& message) {
    std::string currentChatroom = findClientChatroom(client_socket);
    std::cout << "Processing menu message for client " << client_socket << ". In chatroom: " << currentChatroom << std::endl; 
    
    if (!currentChatroom.empty()) {
        displayMenu(client_socket);
        leaveChatroom(client_socket);
    } else {
        Message notInChatroomMessage(MessageType::POST, "You are not currently in a chatroom.");
        sendMessage(client_socket, notInChatroomMessage);
    }
}

// QUIT
void Server::processQuitMessage(int client_socket, const Message& message) {
    std::string currentChatroom = findClientChatroom(client_socket);
    if (!currentChatroom.empty()) {
        leaveChatroom(client_socket);
    }

    closeClientConnection(client_socket);
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


void Server::leaveChatroom(int client_socket) {
    // Find the chatroom that the client is in

    auto it = clientToChatroomMap.find(client_socket);
    if (it != clientToChatroomMap.end()) {
        const std::string& chatroomName = it->second;
        auto& chatroom = chatrooms[chatroomName];

        // Remove the client from the chatroom's client list
        chatroom.removeClient(client_socket);

        // Erase the client from the clientToChatroomMap
        clientToChatroomMap.erase(it);

        // Broadcast a message that the client has left the chatroom
        std::string leaveMsg = "[" + clientUsernames[client_socket].username + "] has left " + chatroomName;
        Message leaveMessage(MessageType::POST, leaveMsg);
        std::cout << "Broadcasting leave message for client " << client_socket << " in chatroom " << chatroomName << std::endl;
        broadcastMessage(chatroomName, leaveMessage);
        
        std::cout << "Client " << client_socket << " has left the chatroom: " << chatroomName << std::endl;
    }
}


void Server::handleClientDisconnect(int client_socket) {
    std::string chatroomName = findClientChatroom(client_socket);
    std::cout << "Handling client disconnect for client " << client_socket << ". In chatroom: " << chatroomName << std::endl;
    if (!chatroomName.empty()) {
        leaveChatroom(client_socket);
    }
    closeClientConnection(client_socket);
}


void Server::closeClientConnection(int client_socket) {
    std::cout << "Closing Socket FD " << client_socket << std::endl;
    close(client_socket);
    clientUsernames.erase(client_socket);
}


std::string Server::findClientChatroom(int client_socket) {
    // Iterate through all chatrooms
    for (const auto& pair : chatrooms) {
        const std::string& chatroomName = pair.first;
        const Chatroom& chatroom = pair.second;

        // Check if the client is in the current chatroom
        if (chatroom.getClients().find(client_socket) != chatroom.getClients().end()) {
            return chatroomName;
        }
    }
    
    // Return an empty string if the client is not in any chatroom
    return "";
}


bool Server::isUsernameAvailable(const std::string& username) {
    // Check if username is already taken
    for (const auto& pair : clientUsernames) {
        if (pair.second.username == username) {
            return false;
        }
    }

    return true;
}
