#pragma once

#include <string>

namespace JappeStudios::JappeOS::DaemonCore::Logging
{
    class DaemonLogger
    {
    public:
        virtual ~DaemonLogger() = default;

        virtual void Emerg (const std::string& str) = 0;
        virtual void Alert (const std::string& str) = 0;
        virtual void Crit  (const std::string& str) = 0;
        virtual void Err   (const std::string& str) = 0;
        virtual void Warn  (const std::string& str) = 0;
        virtual void Notice(const std::string& str) = 0;
        virtual void Info  (const std::string& str) = 0;
        virtual void Debug (const std::string& str) = 0;
    };
}
