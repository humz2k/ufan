#include <ufan/common/logging.hpp>
#include <ufan/common/socket.hpp>
#include <ufan/server.hpp>

int main() {
    ufan::common::init_logging();
    ufan::Server server(ufan::common::Endpoint::ip("0.0.0.0", 42069));
    server.run();
    return 0;
}
