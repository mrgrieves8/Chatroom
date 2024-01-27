#include "Message.h"
#include <sstream>

Message::Message(MessageType messageType, const std::string& messageBody)
    : type(messageType), body(messageBody) {}

Message::~Message() {}

MessageType Message::getType() const {
    return type;
}

const std::string& Message::getBody() const {
    return body;
}

std::string Message::serialize() const {
    return std::to_string(static_cast<int>(type)) + ";" + body;
}

Message Message::deserialize(const std::string& serializedData) {
    size_t delimiterPos = serializedData.find(";");
    if (delimiterPos != std::string::npos) {
        int messageType = std::stoi(serializedData.substr(0, delimiterPos));
        std::string messageBody = serializedData.substr(delimiterPos + 1);
        return Message(static_cast<MessageType>(messageType), messageBody);
    } else {
        return Message(MessageType::POST, "Invalid Message");
    }
}

