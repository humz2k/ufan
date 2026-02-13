#pragma once

#include <ufan/common/interrupts.hpp>
#include <ufan/common/logging.hpp>
#include <ufan/common/socket.hpp>
#include <ufan/protocol/message.hpp>

#include <chrono>
#include <cstddef>
#include <flat_map>

namespace ufan {

class Server : common::ClassLogger {
  private:
    struct ClientData {
        protocol::Topic topic{};
        int64_t last_heartbeat;

        ClientData() { std::memset(topic.keys, 0, sizeof(topic.keys)); }
    };

    common::Endpoint m_endpoint;
    common::Socket m_socket;

    protocol::MessageConstructor m_constructor;

    std::vector<std::byte> m_recv_buf;
    std::flat_map<ufan::common::Endpoint, ClientData> m_clients;

    int64_t m_time_now;
    static constexpr int64_t m_heartbeat_timeout = 10000;

    void cache_time_now() {
        m_time_now = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    }

    int64_t time_now() const { return m_time_now; }

    void send(const common::Endpoint& endpoint,
              std::span<const std::byte> data) {
        LOG_INFO(this->logger(), "[{}] < ({}) {} bytes", endpoint.id(),
                 (char)protocol::MessageParser::header(data).type(),
                 data.size());
        m_socket.send_to(endpoint, data);
    }

    void handle_heartbeat(const common::Endpoint& endpoint,
                          std::span<const std::byte> data) {
        auto& client_data = m_clients[endpoint];
        client_data.last_heartbeat =
            protocol::MessageParser::header(data).timestamp();

        if (client_data.last_heartbeat > time_now()) {
            client_data.last_heartbeat = time_now();
        }

        send(endpoint,
             m_constructor.construct(
                 protocol::Header::heartbeat(client_data.last_heartbeat),
                 std::span<const std::byte>((std::byte*)&client_data.topic,
                                            sizeof(client_data.topic))));
    }

    void handle_subscribe(const common::Endpoint& endpoint,
                          std::span<const std::byte> data) {
        m_clients[endpoint].topic =
            protocol::MessageParser::header(data).topic();
    }

    void handle_publish(const common::Endpoint& endpoint,
                        std::span<const std::byte> data) {
        auto to_publish = protocol::MessageParser::data<std::string_view>(data);
        auto topic = protocol::MessageParser::header(data).topic();

        for (auto it = m_clients.begin(); it != m_clients.end();) {
            const auto& [endpoint, client_data] = *it;

            if ((time_now() - client_data.last_heartbeat) >
                m_heartbeat_timeout) {
                LOG_INFO(this->logger(), "[{}] timed out", endpoint.id());
                it = m_clients.erase(it);
                continue;
            }

            if (client_data.topic.matches(topic)) {
                send(endpoint,
                     m_constructor.construct(protocol::Header::publish(topic),
                                             to_publish));
            }
            ++it;
        }
    }

    void process() {
        auto info_opt = m_socket.recv_from(m_recv_buf);
        if (!info_opt.has_value())
            return;

        cache_time_now();

        const auto& info = info_opt.value();
        std::span<const std::byte> buf(m_recv_buf.data(), info.size);

        try {
            auto header = protocol::MessageParser::header(buf);
            LOG_INFO(this->logger(), "[{}] > ({}) {} bytes", info.from.id(),
                     (char)header.type(), info.size);

            switch (header.type()) {
            case protocol::MessageType::heartbeat:
                handle_heartbeat(info.from, buf);
                break;
            case protocol::MessageType::subscribe:
                handle_subscribe(info.from, buf);
                break;
            case protocol::MessageType::publish:
                handle_publish(info.from, buf);
                break;
            default:
                break;
            }
        } catch (const std::exception& e) {
            LOG_ERROR(this->logger(), "parse failed with {}", e.what());
        }
    }

  public:
    Server(common::Endpoint endpoint)
        : common::ClassLogger("server"), m_endpoint(endpoint),
          m_socket(common::Socket::open(/*non_blocking=*/true)) {
        m_socket.bind(m_endpoint);
        m_recv_buf.resize(65535);
    }

    void run() {
        LOG_INFO(this->logger(), "starting server");
        common::run_forever([&]() { this->process(); });
        LOG_INFO(this->logger(), "stopping server");
    }
};

} // namespace ufan
