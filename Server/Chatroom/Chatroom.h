#ifndef CHATROOM_H
#define CHATROOM_H

#include <string>
#include <set>
#include <vector>

class Chatroom {
public:
    std::string name;
    std::set<int> clients; // Set of client sockets in the chatroom
    std::vector<std::string> messages; // History of messages in the chatroom
    std::set<std::string> forbiddenWords; // Forbidden words in the chatroom

    Chatroom() = default;
    void addForbiddenWord(const std::string& word);
    bool isWordForbidden(const std::string& word) const;
    // void createChatroom(const std::string &name);
    // void joinChatroom(int client_socket, const std::string &chatroomName);
    // void leaveChatroom(int client_socket);
    // ... Other methods if needed ...
};

#endif // CHATROOM_H
