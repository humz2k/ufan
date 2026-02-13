#pragma once

#include "header.hpp"

#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ufan::protocol {

class MessageConstructor {
  private:
    std::vector<std::byte> m_message;

  public:
    std::span<const std::byte> construct(Header header) {
        m_message.resize(sizeof(Header));
        std::copy((std::byte*)&header, ((std::byte*)&header) + sizeof(Header),
                  m_message.data());
        return m_message;
    }

    std::span<const std::byte> construct(Header header, std::string_view data) {
        m_message.resize(sizeof(Header) + data.size());
        std::copy((std::byte*)&header, ((std::byte*)&header) + sizeof(Header),
                  m_message.data());
        std::copy((std::byte*)data.data(),
                  ((std::byte*)data.data()) + data.size(),
                  m_message.data() + sizeof(Header));
        return m_message;
    }

    std::span<const std::byte> construct(Header header,
                                         std::span<const std::byte> data) {
        m_message.resize(sizeof(Header) + data.size());
        std::copy((std::byte*)&header, ((std::byte*)&header) + sizeof(Header),
                  m_message.data());
        std::copy((std::byte*)data.data(),
                  ((std::byte*)data.data()) + data.size(),
                  m_message.data() + sizeof(Header));
        return m_message;
    }
};

class MessageParser {
  public:
    static Header header(std::span<const std::byte> data) {
        if (data.size() < sizeof(Header)) {
            throw std::runtime_error("invalid header");
        }
        return *((Header*)data.data());
    }

    template <typename OutType>
    static OutType data(std::span<const std::byte> data) {
        static_assert(std::is_same_v<OutType, std::string_view> ||
                      std::is_same_v<OutType, std::span<const std::byte>>);

        if (data.size() < sizeof(Header)) {
            throw std::runtime_error("invalid header");
        }
        const auto* message_start = data.data() + sizeof(Header);
        size_t data_size = data.size() - sizeof(Header);

        if constexpr (std::is_same_v<OutType, std::string_view>) {
            return std::string_view((const char*)message_start, data_size);
        } else {
            return std::span<const std::byte>(message_start, data_size);
        }
    }
};

} // namespace ufan::protocol
