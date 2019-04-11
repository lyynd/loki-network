#ifndef LLARP_UTIL_WIN32_LOGGER_HPP
#define LLARP_UTIL_WIN32_LOGGER_HPP
#if defined(_WIN32)
#include <util/ostream_logger.hpp>
#define VC_EXTRALEAN
#include <windows.h>

namespace llarp
{
  struct Win32LogStream : public OStreamLogStream
  {
    Win32LogStream(std::ostream& out);

    void
    PreLog(std::stringstream& s, LogLevel lvl, const char* fname,
           int lineno) const override;

    void
    PostLog(std::stringstream& s) const override;

    bool isConsoleModern =
        true;  // qol fix so oldfag clients don't see ugly escapes
    HANDLE fd1 = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    short old_attrs;
  };
}  // namespace llarp
#endif
#endif
