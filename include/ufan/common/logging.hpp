#pragma once

#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>

#include <string>

namespace ufan::common {

using Logger = quill::Logger*;

void init_logging();

void set_logfile(const std::string& filename);

Logger create_logger(const std::string& name);

class ClassLogger {
  private:
    Logger m_logger;

  public:
    ClassLogger(const std::string& name) : m_logger(create_logger(name)) {}
    Logger logger() { return m_logger; }
    const Logger logger() const { return m_logger; }
};

} // namespace ufan::common
