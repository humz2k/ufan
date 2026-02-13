#pragma once

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

namespace ufan::protocol {

enum class MessageType : uint8_t {
    heartbeat = 'H',
    subscribe = 'S',
    publish = 'P',
    error = 'E',
};

struct [[gnu::packed]] Topic {
    uint8_t keys[8];

    bool operator==(const Topic& other) const noexcept {
        static_assert(sizeof(Topic) == sizeof(uint64_t));
        return (*((const uint64_t*)keys)) == (*((const uint64_t*)other.keys));
    };

    bool matches(const Topic& other) const noexcept {
        for (size_t i = 0; i < 8; i++) {
            if ((keys[i] != other.keys[i]) && (!(keys[i] & other.keys[i])))
                return false;
        }
        return true;
    }

    static Topic from_string(const std::string topic) {
        Topic out;
        std::memset(out.keys, 0, sizeof(out.keys));

        std::string token;
        std::stringstream stream(topic);

        size_t idx = 0;
        while (std::getline(stream, token, '.')) {
            if (token == "*") {
                out.keys[idx] = 0xFF;
            } else if (token == ">") {
                for (size_t i = idx; i < 8; i++) {
                    out.keys[i] = 0xFF;
                }
                break;
            } else {
                for (auto c : token) {
                    out.keys[idx] |= 1 << (c - 'a');
                }
            }
            ++idx;
            if (idx >= 8)
                break;
        }

        return out;
    }
};

struct [[gnu::packed]] Header {
  private:
    const int8_t reserved = 0;
    MessageType type_;
    union {
        Topic topic;
        int64_t timestamp;
    } topic_or_timestamp_;

  public:
    Header(MessageType type, Topic topic) : type_(type) {
        topic_or_timestamp_.topic = topic;
    }
    Header(MessageType type, int64_t timestamp) : type_(type) {
        topic_or_timestamp_.timestamp = timestamp;
    }

    static Header heartbeat(int64_t timestamp) {
        return Header(MessageType::heartbeat, timestamp);
    }
    static Header heartbeat(Topic topic) {
        return Header(MessageType::heartbeat, topic);
    }
    static Header publish(Topic topic) {
        return Header(MessageType::publish, topic);
    }
    static Header subscribe(Topic topic) {
        return Header(MessageType::subscribe, topic);
    }
    static Header error() { return Header(MessageType::error, 0); }

    MessageType type() const { return type_; }
    Topic topic() const { return topic_or_timestamp_.topic; }
    int64_t timestamp() const { return topic_or_timestamp_.timestamp; }
};

static_assert(sizeof(Header) == 10ULL);

} // namespace ufan::protocol
