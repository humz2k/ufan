#include <ufan/client.hpp>
#include <ufan/common/interrupts.hpp>
#include <ufan/common/logging.hpp>
#include <ufan/common/socket.hpp>
#include <ufan/protocol/message.hpp>

#include <chrono>
#include <iostream>

int main() {
    ufan::common::init_logging();

    // ufan::protocol::MessageConstructor message_constructor;

    auto server = ufan::common::Endpoint::ip("127.0.0.1", 42069);
    // auto socket = ufan::common::Socket::open();

    auto logger = ufan::common::create_logger("client");

    auto topic_publish = ufan::protocol::Topic::from_string("a.b.f.a.c.e.g.h");
    auto topic_subscribe = ufan::protocol::Topic::from_string("a.b.>");

    ufan::Publisher publisher(server);
    ufan::Subscriber subscriber(server, topic_subscribe);

    int64_t next_send = 0;

    // std::vector<std::byte> recv_buf;
    // recv_buf.resize(65535);

    ufan::common::run_forever([&]() {
        int64_t time_now =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        if (time_now > next_send) {
            int64_t time =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
            publisher.publish(
                topic_publish,
                std::span<const std::byte>((std::byte*)&time, sizeof(int64_t)));
            next_send = time_now + 5000;
        }

        auto message = subscriber.process<std::span<const std::byte>>();
        if (message) {
            auto recv = message.value();
            int64_t time = *((int64_t*)recv.data());
            int64_t recv_time =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
            LOG_INFO(logger, "recv {} latency={}", time,
                     static_cast<double>(recv_time - time) * 0.001);
        }
    });
    return 0;
}
