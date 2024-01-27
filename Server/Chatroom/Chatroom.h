#ifndef CHATROOM_H
#define CHATROOM_H

#include <string>
#include <set>
#include <vector>

class Chatroom {
public:
    Chatroom() = default;
    explicit Chatroom(const std::string& name);
    Chatroom(const std::string& name, const std::set<std::string>& forbiddenWords);

    void addClient(int clientSocket);
    void removeClient(int clientSocket);
    void addMessage(const std::string& message);
    const std::string& getName() const;
    const std::set<int>& getClients() const;
    const std::vector<std::string>& getMessages() const;
    const std::set<std::string>& getForbiddenWords() const;

    void addForbiddenWord(const std::string& word);
    bool isWordForbidden(const std::string& word) const;

private:
    std::string name;
    std::set<int> clients;
    std::vector<std::string> messages;
    std::set<std::string> forbiddenWords;
};

#endif // CHATROOM_H
