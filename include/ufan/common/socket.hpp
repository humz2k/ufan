#pragma once

#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <tuple>
#include <unistd.h>

namespace ufan::common {

struct Endpoint {
    sockaddr_in addr{};

    static Endpoint any(uint16_t port) {
        Endpoint ep{};
        ep.addr.sin_family = AF_INET;
        ep.addr.sin_port = htons(port);
        ep.addr.sin_addr.s_addr = htonl(INADDR_ANY);
        return ep;
    }

    static Endpoint ip(std::string_view dotted_quad, uint16_t port) {
        Endpoint ep{};
        ep.addr.sin_family = AF_INET;
        ep.addr.sin_port = htons(port);
        if (::inet_pton(AF_INET, std::string(dotted_quad).c_str(),
                        &ep.addr.sin_addr) != 1) {
            throw std::runtime_error("inet_pton(AF_INET) failed for ip=" +
                                     std::string(dotted_quad));
        }
        return ep;
    }

    static Endpoint ip_u32(uint32_t ip_host_order, uint16_t port) {
        Endpoint ep{};
        ep.addr.sin_family = AF_INET;
        ep.addr.sin_port = htons(port);
        ep.addr.sin_addr.s_addr = htonl(ip_host_order);
        return ep;
    }

    uint32_t ip_host_order() const noexcept {
        return ntohl(addr.sin_addr.s_addr);
    }
    uint16_t port_host_order() const noexcept { return ntohs(addr.sin_port); }

    bool operator==(const Endpoint& other) const noexcept {
        return ip_host_order() == other.ip_host_order() &&
               port_host_order() == other.port_host_order();
    }

    bool operator<(const Endpoint& other) const noexcept {
        return std::tuple(ip_host_order(), port_host_order()) <
               std::tuple(other.ip_host_order(), other.port_host_order());
    }

    uint64_t id() const {
        return (static_cast<uint64_t>(ip_host_order()) << 16) |
               static_cast<uint64_t>(port_host_order());
    }
};

struct RecvFrom {
    std::size_t size{};
    Endpoint from{};
};

class Socket {
  private:
    int m_fd{-1};

    static std::string err(const char* what) {
        return std::string(what) + " failed: " + std::strerror(errno);
    }

  public:
    Socket() = default;

    static Socket open(bool non_blocking = true) {
        Socket s;
        s.m_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (s.m_fd < 0)
            throw std::runtime_error(err("socket"));
        if (non_blocking)
            s.set_non_blocking(true);
        return s;
    }

    ~Socket() { close(); }

    Socket(Socket&& o) noexcept : m_fd(o.m_fd) { o.m_fd = -1; }
    Socket& operator=(Socket&& o) noexcept {
        if (this != &o) {
            close();
            m_fd = o.m_fd;
            o.m_fd = -1;
        }
        return *this;
    }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    int fd() const noexcept { return m_fd; }
    explicit operator bool() const noexcept { return m_fd >= 0; }

    void close() noexcept {
        if (m_fd >= 0) {
            ::close(m_fd);
            m_fd = -1;
        }
    }

    void set_non_blocking(bool on) {
        if (m_fd < 0)
            throw std::runtime_error("set_non_blocking on closed socket");
        int flags = ::fcntl(m_fd, F_GETFL, 0);
        if (flags < 0)
            throw std::runtime_error(err("fcntl(F_GETFL)"));
        if (on)
            flags |= O_NONBLOCK;
        else
            flags &= ~O_NONBLOCK;
        if (::fcntl(m_fd, F_SETFL, flags) < 0)
            throw std::runtime_error(err("fcntl(F_SETFL)"));
    }

    void bind(const Endpoint& local) {
        if (m_fd < 0)
            throw std::runtime_error("bind on closed socket");
        if (::bind(m_fd, reinterpret_cast<const sockaddr*>(&local.addr),
                   sizeof(sockaddr_in)) < 0) {
            throw std::runtime_error(err("bind"));
        }
    }

    std::size_t send_to(const Endpoint& to, std::span<const std::byte> data) {
        if (m_fd < 0)
            throw std::runtime_error("send_to on closed socket");
        auto n = ::sendto(m_fd, data.data(), data.size(), 0,
                          reinterpret_cast<const sockaddr*>(&to.addr),
                          sizeof(sockaddr_in));
        if (n < 0)
            throw std::runtime_error(err("sendto"));
        return static_cast<std::size_t>(n);
    }

    std::optional<RecvFrom> recv_from(std::span<std::byte> out) {
        if (m_fd < 0)
            throw std::runtime_error("recv_from on closed socket");

        Endpoint from{};
        socklen_t from_len = sizeof(sockaddr_in);

        auto n = ::recvfrom(m_fd, out.data(), out.size(), 0,
                            reinterpret_cast<sockaddr*>(&from.addr), &from_len);

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return std::nullopt;
            throw std::runtime_error(err("recvfrom"));
        }

        if (n == 0)
            return std::nullopt;
        return RecvFrom{static_cast<std::size_t>(n), from};
    }
};

} // namespace ufan::common

namespace std {
template <> struct hash<ufan::common::Endpoint> {
    std::size_t operator()(const ufan::common::Endpoint& ep) const noexcept {
        const uint64_t key = (static_cast<uint64_t>(ep.ip_host_order()) << 16) |
                             static_cast<uint64_t>(ep.port_host_order());
        return std::hash<uint64_t>{}(key);
    }
};
} // namespace std
