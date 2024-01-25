#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include "../common/Message.h" // Include the Message class

class Chatroom {
public:
    std::string name;
    int adminSocket; // Socket number of the admin
    std::set<int> clients;
    std::vector<std::string> messages;
    std::set<std::string> forbiddenWords; // Store forbidden words

    // Constructor
    Chatroom() : adminSocket(-1) {}

    // Methods to manage forbidden words, admin, etc.
    void addForbiddenWord(const std::string& word) {
        forbiddenWords.insert(word);
    }

    bool isWordForbidden(const std::string& word) const {
        return forbiddenWords.find(word) != forbiddenWords.end();
    }

    void setAdmin(int socket) {
        adminSocket = socket;
    }

    bool isAdmin(int socket) const {
        return socket == adminSocket;
    }
    
    // ... Other methods ...
};

class ClientInfo {
public:
    std::string username;
    int socketNum;
    // Add more fields or methods as necessary
};

class Server {
public:
    Server(const std::string& ip, int port);
    virtual ~Server();
    bool init();
    void run();
    std::string findClientChatroom(int client_socket);

    bool isTakenUsername(const std::string &username);

private:
    std::string ip;
    int port;
    int server_fd;
    int epoll_fd;
    std::unordered_map<int, ClientInfo> clientUsernames; // Map socket FD to ClientInfo
    std::unordered_map<std::string, Chatroom> chatrooms; // Map chatroom name to Chatroom
    std::unordered_map<int, std::string> clientToChatroomMap; // Maps client socket to chatroom name

    bool createServerSocket();
    bool initEpoll();
    void handleClientData(int client_socket);
    void processClientMessage(int client_socket, const std::string& serializedMessage);
    void sendWelcomeMessage(int client_socket);
    void handleUsernameRequest(int client_socket);
    void displayMenu(int client_socket);
    void createChatroom(const std::string& name);
    void joinChatroom(int client_socket, const std::string& chatroomName);
    void broadcastMessage(const std::string& chatroomName, const Message& message);
    void handleClientDisconnect(int client_socket);
    void closeClientConnection(int client_socket);
    void leaveChatroom(int client_socket);
    bool containsForbiddenWords(const std::string& chatroomName, const std::string& message);
    void handleNewConnection();
    void closeAllConnections();
    void processJoinMessage(int client_socket, const Message& message);
    void processCreateChatroomMessage(int client_socket, const Message &message);
    void setForbiddenWords(const std::string &chatroomName, const std::string &words);
    void processMenuMessage(int client_socket, const Message &message);
    void processQuitMessage(int client_socket, const Message& message);
    void processPostMessage(int client_socket, const Message& message);
    void sendMessage(int client_socket, const Message& message);
    // Add other private helper methods as necessary
};

#endif // SERVER_H

std::string replaceForbiddenWords(const std::string &messageBody, const std::set<std::string> &forbiddenWords);
