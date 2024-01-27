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
#include <termios.h>

// Terminal control sequences
const std::string MOVE_CURSOR_UP = "\033[A";
const std::string CLEAR_LINE = "\033[2K";

Client::Client(const std::string& serverIP, int serverPort)
    : serverIP(serverIP), serverPort(serverPort), clientSocket(-1), state(ClientState::PreLogin) {
}


Client::~Client() {
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
    // Create a separate thread for receiving messages
    std::thread receiveThread([this]() { 
        while (true) {
            // Receive a message from the server
            Message response = receiveMessage();

            // Process the received message based on its type
            switch (response.getType()) {
                case MessageType::JOIN:
                    system("clear");
                    state = ClientState::InChatroom;
                    std::cout << response.getBody() << std::endl;
                    notifyReadyToSend();
                    break;
                case MessageType::MENU:
                    system("clear");
                    state = ClientState::SelectingChatroom;
                    std::cout << response.getBody() << std::endl;
                    notifyReadyToSend();
                    break;
                case MessageType::QUIT:
                    state = ClientState::Quitting;
                    std::cerr << response.getBody() << std::endl;
                    return; // Exiting the thread
                case MessageType::POST:
                    std::cout << response.getBody() << std::endl;
                    notifyReadyToSend();
                    break;
                default:
                    std::cerr << "Unknown message type received." << std::endl;
                    break;
            }
        }
    });
    receiveThread.detach();
}


void Client::startChatSession() {
    // Attempt to connect to the server
    if (!connectToServer()) {
        std::cerr << "Failed to connect to server." << std::endl;
        return; // Exit the function if the connection fails
    }

    // Start a separate thread to receive messages from the server
    startReceivingMessages();

    if (state == ClientState::PreLogin) {
        handleServerResponse(); // Handle initial setup like username
        state = ClientState::SelectingChatroom;
    }
    
     // Continue running as long as the client is not in the "Quitting" state
    while (state != ClientState::Quitting) {
        waitForMessageReady();

        switch (state) {
            case ClientState::SelectingChatroom:
                handleSelectingChatroom();
                break;

            case ClientState::InChatroom:
                handleInChatroom();
                break;

            case ClientState::Quitting:
                handleQuitting();
                return;

        }
        setNotReadyToSend();
    }
}


void Client::handleQuitting() {
    // Close the socket and perform any necessary cleanup
    
    if (clientSocket != -1) {
        close(clientSocket);
        clientSocket = -1;
    }
    std::cout << "Client disconnected and resources cleaned up." << std::endl;

    exit(0);
}


void Client::handleSelectingChatroom() {
    std::string message;
    std::getline(std::cin, message);
    if (message.rfind("/create ", 0) == 0) {
        std::string chatroomInfo = message.substr(8); // Extract chatroom info
        sendMessage(Message(MessageType::CREATE, chatroomInfo));
    } else if (message == "/quit") {
        system("clear");
        sendMessage(Message(MessageType::QUIT, ""));
        state = ClientState::Quitting;
    } else {
        sendMessage(Message(MessageType::JOIN, message));
    }
}


// Function to read input line with echo, then clear the line after sending
std::string Client::getInputAndClearLine() {
    std::string input;
    std::getline(std::cin, input); // Echoes the input as usual

    // Move the cursor up one line and clear the line
    std::cout << MOVE_CURSOR_UP << CLEAR_LINE;
    return input;
}


void Client::handleInChatroom() {
    std::string message = getInputAndClearLine();
    if (message == "/leave") {
        sendMessage(Message(MessageType::MENU, ""));
    } else if (message == "/quit") {
        system("clear");
        sendMessage(Message(MessageType::QUIT, ""));
        state = ClientState::Quitting;
    } else {
        sendMessage(Message(MessageType::POST, message));
    }
}

// Notify that the client is ready to send a message
void Client::notifyReadyToSend() {
    std::lock_guard<std::mutex> lock(mtx);
    readyToSend = true;
    cv.notify_one();
}

// Wait until the client is ready to send a message
void Client::waitForMessageReady() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this]{ return readyToSend; });
}

// Set the client as not ready to send a message
void Client::setNotReadyToSend() {
    std::lock_guard<std::mutex> lock(mtx);
    readyToSend = false;
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
    Message deserializedMessage = Message::deserialize(receivedData);
    return deserializedMessage;
}


void Client::handleServerResponse() {
    // Getting username from the user
    std::string username;
    std::getline(std::cin, username);
    sendMessage(Message(MessageType::LOGIN, username)); // Sending username to the server
    
    Message serverResponse = receiveMessage();
    if (serverResponse.getType() == MessageType::QUIT) {
        std::cerr << serverResponse.getBody() << std::endl;
        handleQuitting();
    } else {
        system("clear");
        std::cout << serverResponse.getBody(); // Displaying chatroom options or other messages
    }
}
