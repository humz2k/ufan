// DISCLAIMER: this cli tool is AI generated

#include <ufan/client.hpp>
#include <ufan/common/interrupts.hpp>
#include <ufan/protocol/message.hpp>

#include <arpa/inet.h>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using ufan::common::Endpoint;
using ufan::protocol::Topic;

struct ParsedEndpoint {
    std::string ip;
    uint16_t port;
};

void print_usage(const char* prog) {
    std::cerr << "Usage:\n"
              << "  " << prog
              << " publish <server_ip>:<server_port> <topic> <data>\n"
              << "  " << prog
              << " subscribe <server_ip>:<server_port> <topic>\n\n"
              << "Examples:\n"
              << "  " << prog
              << " publish 127.0.0.1:42069 a.b.f.a.c.e.g.h \"hello\"\n"
              << "  " << prog << " subscribe 127.0.0.1:42069 a.b.>\n\n"
              << "Topic rules:\n"
              << "  - Up to 8 tokens separated by '.'\n"
              << "  - Token is one of: [a-h]+, '*', or '>'\n"
              << "  - '>' must be the last token\n";
}

std::optional<ParsedEndpoint> parse_endpoint(std::string_view s) {
    const auto colon = s.rfind(':');
    if (colon == std::string_view::npos || colon == 0 ||
        colon == s.size() - 1) {
        return std::nullopt;
    }

    std::string ip(s.substr(0, colon));
    std::string port_str(s.substr(colon + 1));

    in_addr addr{};
    if (::inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
        return std::nullopt;
    }

    char* end = nullptr;
    errno = 0;
    unsigned long p = std::strtoul(port_str.c_str(), &end, 10);
    if (errno != 0 || end == port_str.c_str() || *end != '\0' || p > 65535UL) {
        return std::nullopt;
    }

    return ParsedEndpoint{ip, static_cast<uint16_t>(p)};
}

bool validate_topic(std::string_view topic, std::string& why_bad) {
    if (topic.empty()) {
        why_bad = "topic is empty";
        return false;
    }

    std::vector<std::string> tokens;
    {
        std::string t(topic);
        std::stringstream ss(t);
        std::string tok;
        while (std::getline(ss, tok, '.')) {
            tokens.push_back(tok);
        }
    }

    if (tokens.empty()) {
        why_bad = "topic has no tokens";
        return false;
    }

    if (tokens.size() > 8) {
        why_bad = "topic has more than 8 tokens";
        return false;
    }

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& tok = tokens[i];
        if (tok.empty()) {
            why_bad = "topic has empty token (e.g. consecutive dots or "
                      "leading/trailing dot)";
            return false;
        }

        if (tok == "*") {
            continue;
        }
        if (tok == ">") {
            if (i != tokens.size() - 1) {
                why_bad = "'>' wildcard must be the last token";
                return false;
            }
            continue;
        }

        for (unsigned char c : tok) {
            if (c < 'a' || c > 'h') {
                why_bad = "token '" + tok + "' contains invalid char '" +
                          std::string(1, static_cast<char>(c)) +
                          "' (allowed: a..h, '*', '>')";
                return false;
            }
        }
    }

    return true;
}

void print_bytes_hex_ascii(std::span<const std::byte> bytes) {
    constexpr size_t kWidth = 16;

    for (size_t i = 0; i < bytes.size(); i += kWidth) {
        const size_t n = std::min(kWidth, bytes.size() - i);

        std::cout << std::setw(8) << std::setfill('0') << std::hex << i << "  ";

        for (size_t j = 0; j < kWidth; ++j) {
            if (j < n) {
                auto v = static_cast<unsigned>(
                    std::to_integer<uint8_t>(bytes[i + j]));
                std::cout << std::setw(2) << std::setfill('0') << std::hex << v
                          << ' ';
            } else {
                std::cout << "   ";
            }
        }

        std::cout << " |";

        for (size_t j = 0; j < n; ++j) {
            unsigned char c = std::to_integer<uint8_t>(bytes[i + j]);
            std::cout << (std::isprint(c) ? static_cast<char>(c) : '.');
        }
        std::cout << "|\n";
    }

    std::cout << std::dec << std::setfill(' ');
}

int run_publish(std::string_view endpoint_sv, std::string_view topic_sv,
                std::string_view data_sv) {
    auto ep = parse_endpoint(endpoint_sv);
    if (!ep) {
        std::cerr << "Invalid endpoint: '" << endpoint_sv
                  << "' (expected <ipv4>:<port>)\n";
        return 2;
    }

    std::string why_bad;
    if (!validate_topic(topic_sv, why_bad)) {
        std::cerr << "Invalid topic: '" << topic_sv << "' - " << why_bad
                  << "\n";
        return 2;
    }

    Endpoint server = Endpoint::ip(ep->ip, ep->port);
    Topic topic = Topic::from_string(std::string(topic_sv));

    ufan::Publisher publisher(server);

    auto* ptr = reinterpret_cast<const std::byte*>(data_sv.data());
    std::span<const std::byte> payload(ptr, data_sv.size());

    publisher.publish(topic, payload);

    std::cout << "published " << payload.size() << " bytes to " << endpoint_sv
              << " topic=" << topic_sv << "\n";

    return 0;
}

int run_subscribe(std::string_view endpoint_sv, std::string_view topic_sv) {
    auto ep = parse_endpoint(endpoint_sv);
    if (!ep) {
        std::cerr << "Invalid endpoint: '" << endpoint_sv
                  << "' (expected <ipv4>:<port>)\n";
        return 2;
    }

    std::string why_bad;
    if (!validate_topic(topic_sv, why_bad)) {
        std::cerr << "Invalid topic: '" << topic_sv << "' - " << why_bad
                  << "\n";
        return 2;
    }

    Endpoint server = Endpoint::ip(ep->ip, ep->port);
    Topic topic = Topic::from_string(std::string(topic_sv));

    ufan::Subscriber sub(server, topic);

    std::cout << "subscribed to " << endpoint_sv << " topic=" << topic_sv
              << " (Ctrl-C to exit)\n";

    ufan::common::run_forever([&]() {
        auto msg = sub.process<std::span<const std::byte>>();
        if (!msg)
            return;

        auto bytes = msg.value();
        std::cout << "---- message (" << bytes.size() << " bytes) ----\n";
        print_bytes_hex_ascii(bytes);
        std::cout.flush();
    });

    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 2;
    }

    std::string mode = argv[1];

    try {
        if (mode == "publish") {
            if (argc != 5) {
                print_usage(argv[0]);
                return 2;
            }
            return run_publish(argv[2], argv[3], argv[4]);
        }

        if (mode == "subscribe") {
            if (argc != 4) {
                print_usage(argv[0]);
                return 2;
            }
            return run_subscribe(argv[2], argv[3]);
        }

        std::cerr << "Unknown command: " << mode << "\n";
        print_usage(argv[0]);
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
