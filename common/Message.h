#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <set>

enum class MessageType {
    JOIN,
    LEAVE,
    QUIT,
    POST,
    LOGIN
};

class Message {
private:
    MessageType type;
    std::string body;

public:
    Message(MessageType messageType, const std::string& messageBody);
    ~Message();

    MessageType getType() const;
    const std::string& getBody() const;

    std::string serialize() const;
    static Message deserialize(const std::string& serializedData);
    void censorWords(const std::set<std::string>& forbiddenWords);
};

#endif