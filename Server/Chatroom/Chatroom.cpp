#include "Chatroom.h"
#include <iostream>


Chatroom::Chatroom(const std::string& name) : name(name) {}

Chatroom::Chatroom(const std::string& name, const std::set<std::string>& forbiddenWords) 
    : name(name), forbiddenWords(forbiddenWords) {}

void Chatroom::addForbiddenWord(const std::string& word) {
    forbiddenWords.insert(word);
}

bool Chatroom::isWordForbidden(const std::string& word) const {
    return forbiddenWords.find(word) != forbiddenWords.end();
}

void Chatroom::addClient(int clientSocket) {
    clients.insert(clientSocket);
    std::cout << "Client " << clientSocket << " joined chatroom: " << name << std::endl;
}

void Chatroom::removeClient(int clientSocket) {
    clients.erase(clientSocket);
    std::cout << "Client " << clientSocket << " left chatroom: " << name << std::endl;
}

void Chatroom::addMessage(const std::string& message) {
    messages.push_back(message);
}

const std::string& Chatroom::getName() const {
    return name;
}

const std::set<int> &Chatroom::getClients() const {
    return clients;
}

const std::vector<std::string>& Chatroom::getMessages() const {
    return messages;
}

const std::set<std::string> &Chatroom::getForbiddenWords() const {
    return forbiddenWords;
}

std::string Chatroom::censorMessage(const std::string& messageBody) const {
    std::string modifiedMessage = messageBody;

    for (const auto& word : forbiddenWords) {
        std::size_t found = modifiedMessage.find(word);
        while (found != std::string::npos) {
            // Replace each occurrence of the forbidden word with ***
            modifiedMessage.replace(found, word.length(), "****");
            found = modifiedMessage.find(word, found + 3);
        }
    }

    return modifiedMessage;
}
