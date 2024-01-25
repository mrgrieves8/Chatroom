#include "Client.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sstream>
#include <thread>
#include "../common/Message.h"
#include <atomic>
#include <condition_variable>
#include <mutex>


Client::Client(const std::string& serverIP, int serverPort)
    : serverIP(serverIP), serverPort(serverPort), clientSocket(-1), state(ClientState::PreLogin) {
    // Constructor initialization if needed
}

Client::~Client() {
    // Cleanup resources if needed
    if (clientSocket != -1) {
        close(clientSocket);
    }
}

bool Client::connectToServer() {
    // Create a socket to connect to the server
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return false;
    }

    // Initialize server address structure
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr);

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Error connecting to server" << std::endl;
        return false;
    }

    return true;
}

void Client::startReceivingMessages() {
    std::thread receiveThread([this]() {
        while (true) {
            Message response = receiveMessage();
            switch (response.getType()) {
                case MessageType::JOIN:
                    std::cout << "got join" << std::endl;
                    state = ClientState::InChatroom;
                    std::cout << response.getBody() << std::endl;
                    break;
                case MessageType::LEAVE:
                    std::cout << "got leave" << std::endl;
                    state = ClientState::SelectingChatroom;
                    std::cout << response.getBody() << std::endl;
                    break;
                case MessageType::QUIT:
                    std::cout << "got quit" << std::endl;
                    std::cerr << "Connection closed." << std::endl;
                    return; // Exiting the thread
                case MessageType::POST:
                    std::cout << "got post" << std::endl;
                    std::cout << response.getBody() << std::endl;
                    break;
                default:
                    std::cerr << "Unknown message type received." << std::endl;
                    break;
            }
        }
    });

    receiveThread.detach(); // Detach the thread to run independently
}

void Client::startChatSession() {
    if (!connectToServer()) {
        std::cerr << "Failed to connect to server." << std::endl;
        return;
    }
    
    if (state == ClientState::PreLogin) {
        handleServerResponse(); // Handle initial setup like username
        std::lock_guard<std::mutex> lock(stateMutex);
        state = ClientState::SelectingChatroom;
    }
    startReceivingMessages(); // Start the message receiving thread

    std::string message;
    while (true) {
        std::unique_lock<std::mutex> lock(stateMutex);
        stateCondition.wait(lock, [this] { return state != ClientState::PreLogin; }); // Wait for the state to change
        if (state == ClientState::SelectingChatroom) {
            std::getline(std::cin, message);
            sendMessage(Message(MessageType::JOIN, message));
        } else if (state == ClientState::InChatroom) {
            std::getline(std::cin, message);
            if (message == "/leave") {
                sendMessage(Message(MessageType::LEAVE, ""));
                state = ClientState::SelectingChatroom;
            } else {
                sendMessage(Message(MessageType::POST, message));
            }
        }
    }
}

void Client::sendMessage(const Message& message) {
    std::string serializedMessage = message.serialize();
    send(clientSocket, serializedMessage.c_str(), serializedMessage.length(), 0);
}

Message Client::receiveMessage() {
    char buffer[1024] = {0};
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    
    if (bytesReceived <= 0) {
        return Message(MessageType::QUIT, "Connection error or server closed the connection");
    }

    std::string receivedData(buffer, bytesReceived);

    return Message::deserialize(receivedData);
}


void Client::handleServerResponse() {
    Message welcomeMessage = receiveMessage();
    std::cout << welcomeMessage.getBody() << std::endl; // Displaying the welcome message

    // Getting username from the user
    std::string username;
    std::getline(std::cin, username);
    sendMessage(Message(MessageType::LOGIN, username)); // Sending username to the server

    Message chatroomOptions = receiveMessage();
    std::cout << chatroomOptions.getBody(); // Displaying chatroom options
}
