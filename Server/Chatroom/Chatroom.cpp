#include "Chatroom.h"

void Chatroom::addForbiddenWord(const std::string& word) {
    forbiddenWords.insert(word);
}

bool Chatroom::isWordForbidden(const std::string& word) const {
    return forbiddenWords.find(word) != forbiddenWords.end();
}

