#include <ufan/common/logging.hpp>

#include <quill/Backend.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h>

#include <optional>
#include <string>

namespace ufan::common {

void init_logging() { quill::Backend::start(); }

static std::optional<std::string> logfile;

void set_logfile(const std::string& filename) { logfile = filename; }

Logger create_logger(const std::string& name) {
    if (logfile) {
        return quill::Frontend::create_or_get_logger(
            name,
            quill::Frontend::create_or_get_sink<quill::FileSink>(*logfile));
    } else {
        return quill::Frontend::create_or_get_logger(
            name,
            quill::Frontend::create_or_get_sink<quill::ConsoleSink>("default"));
    }
}

} // namespace ufan::common
