#pragma once

#include <ufan/common/socket.hpp>
#include <ufan/protocol/message.hpp>

#include <chrono>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>

namespace ufan {

class Publisher {
  private:
    common::Endpoint m_server;
    common::Socket m_socket;
    protocol::MessageConstructor m_constructor;

  public:
    Publisher(const common::Endpoint& server)
        : m_server(server),
          m_socket(common::Socket::open(/*non_blocking=*/true)) {}

    bool publish(protocol::Topic topic, std::span<const std::byte> data) {
        auto packet =
            m_constructor.construct(protocol::Header::publish(topic), data);
        return packet.size() == m_socket.send_to(m_server, packet);
    }

    bool publish(protocol::Topic topic, std::string_view data) {
        auto packet =
            m_constructor.construct(protocol::Header::publish(topic), data);
        return packet.size() == m_socket.send_to(m_server, packet);
    }
};

class Subscriber {
  private:
    common::Endpoint m_server;
    common::Socket m_socket;
    protocol::MessageConstructor m_constructor;
    protocol::Topic m_topic;
    protocol::Topic m_subscribed_topic;

    int64_t m_time_now;
    int64_t m_next_heartbeat = 0;
    int64_t m_last_heartbeat = 0;
    static constexpr int64_t m_heartbeat_frequency = 3000;
    static constexpr int64_t m_heartbeat_timeout = 10000;

    std::vector<std::byte> m_recv_buf;

    int64_t time_now() const { return m_time_now; }

    void cache_time_now() {
        m_time_now = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    }

    void send_heartbeat() {
        m_socket.send_to(
            m_server,
            m_constructor.construct(protocol::Header::heartbeat(time_now())));
        if (!subscribed()) {
            m_socket.send_to(
                m_server,
                m_constructor.construct(protocol::Header::subscribe(m_topic)));
        }
    }

    void handle_heartbeat(std::span<const std::byte> data) {
        if (data.size() !=
            (sizeof(protocol::Header) + sizeof(protocol::Topic))) {
            throw std::runtime_error("invalid heartbeat");
        }

        m_last_heartbeat = protocol::MessageParser::header(data).timestamp();
        m_subscribed_topic = *(
            (protocol::Topic*)
                protocol::MessageParser::data<std::span<const std::byte>>(data)
                    .data());
    }

  public:
    Subscriber(const common::Endpoint& server, protocol::Topic topic)
        : m_server(server),
          m_socket(common::Socket::open(/*non_blocking=*/true)),
          m_topic(topic) {
        cache_time_now();
        std::memset(m_subscribed_topic.keys, 0,
                    sizeof(m_subscribed_topic.keys));
        m_recv_buf.resize(65535);
    }

    template <typename RecvType = std::string_view>
    std::optional<RecvType> process() {
        static_assert(std::is_same_v<RecvType, std::span<const std::byte>> ||
                      std::is_same_v<RecvType, std::string_view>);

        cache_time_now();
        if (time_now() > m_next_heartbeat) {
            m_next_heartbeat = time_now() + m_heartbeat_frequency;
            send_heartbeat();
        }

        if (auto r = m_socket.recv_from(m_recv_buf)) {
            if (r->from != m_server) {
                throw std::runtime_error("wtf");
            }

            std::span<const std::byte> data(m_recv_buf.data(), r->size);
            auto header = protocol::MessageParser::header(data);
            switch (header.type()) {
            case protocol::MessageType::heartbeat:
                handle_heartbeat(data);
                break;
            case protocol::MessageType::publish:
                if (header.topic().matches(m_topic)) {
                    return protocol::MessageParser::data<RecvType>(data);
                }
                break;
            default:
                break;
            }
        }

        return std::nullopt;
    }

    bool subscribed() const noexcept {
        return connected() && (m_subscribed_topic == m_topic);
    }

    bool connected() const noexcept {
        return (time_now() - m_last_heartbeat) < m_heartbeat_timeout;
    }
};

} // namespace ufan
