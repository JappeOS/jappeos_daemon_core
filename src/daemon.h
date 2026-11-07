/*
 * jappeos_core, Core system management daemon for JappeOS.
 * Copyright (C) 2026  The JappeOS team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <string>
#include <system_error>
#include <utility>
#include <glib.h>
#include <sys/poll.h>

#include "globals.h"
#include "dbus/dbus_wrapper.h"
#include "logging/daemon_logger.h"
#include "logging/stdout_logger.h"
#include "utils/jos_exception.h"
#include "utils/null_check_exception.h"
#include "utils/stack_trace.h"

using namespace JappeStudios::JappeOS::DaemonCore::Utils;

namespace JappeStudios::JappeOS::DaemonCore
{
#define CREATE_DAEMON(factory, dbusConfig) \
    int main(int argc, char* argv[]) \
    { \
        std::vector<std::string> arguments(argv + 1, argv + argc); \
        JappeStudios::JappeOS::DaemonCore::Daemon daemon(arguments, (factory), (dbusConfig)); \
        daemon.Run(); \
    }

    enum class DaemonState
    {
#define X(name) name,
#include "daemon_state.def"
#undef X
    };
    const char* DaemonState_ToString(DaemonState state);

    struct DaemonDBusConfig
    {
        std::function<DBus::Connection*()> connectionFactory;
        const std::string& name;
        unsigned int flags = DBUS_NAME_FLAG_DO_NOT_QUEUE;
    };

    class DaemonClass {};

    class Daemon
    {
    public:
        explicit Daemon(
            std::vector<std::string> startupArgs,
            std::function<DaemonClass*()> factory,
            std::optional<DaemonDBusConfig> dbusConfig = std::nullopt);

        ~Daemon();

        Daemon(const Daemon&) = delete;
        Daemon& operator=(const Daemon&) = delete;

        Daemon(Daemon&&) = delete;
        Daemon& operator=(Daemon&&) = delete;

        void Quit(const uint8_t exitCode = EXIT_SUCCESS) { _shouldRun = false; _exitCode = exitCode; }

        void Run();

    public:
        [[nodiscard]] static Logging::DaemonLogger& GetLogger()
        {
            NULL_CHECK_VAR(s_instance, "s_instance");
            NULL_CHECK_VAR(s_instance->_logger, "_logger");
            return *s_instance->_logger;
        }

        [[nodiscard]] static DBus::Connection& GetConnection()
        {
            NULL_CHECK_VAR(s_instance, "s_instance");
            NULL_CHECK_VAR(s_instance->_conn, "_conn");
            return *s_instance->_conn;
        }

        [[nodiscard]] static bool HasConnection()
        {
            NULL_CHECK_VAR(s_instance, "s_instance");
            return s_instance->_conn != nullptr;
        }

        [[nodiscard]] static DBus::InterfaceName GetBaseInterface()
        {
            if (!s_instance->_baseInterfacePath.has_value())
                throw std::logic_error("GetBaseInterface called but DBus was not initialized");
            return s_instance->_baseInterfacePath.value();
        }

        [[nodiscard]] static DBus::ObjectPath GetBaseObject()
        {
            if (!s_instance->_baseObjectPath.has_value())
                throw std::logic_error("GetBaseObject called but DBus was not initialized");
            return s_instance->_baseObjectPath.value();
        }

    private:
        bool             _shouldRun = true;
        uint8_t          _exitCode = EXIT_SUCCESS;
        DaemonState      _state = DaemonState::Initializing;
        pollfd           _fds[2]{};

        std::vector<std::string>        _startupArgs;
        std::function<DaemonClass*()>   _factory;
        std::optional<DaemonDBusConfig> _dbusConfig;

        DaemonClass*           _daemonInstance = nullptr;
        Logging::DaemonLogger* _logger = nullptr;
        DBus::Connection*      _conn = nullptr;
        std::optional<DBus::InterfaceName> _baseInterfacePath;
        std::optional<DBus::ObjectPath>    _baseObjectPath;

        void Initialize();
        void InitDBus(int& dbusFd);
        void Cleanup();

        void Update();
        void UpdateDBus() const;

        void FailFastFatalError(const std::string& errCode,
                                const std::string& message,
                                const std::string& stack) const;

    private:
        static Daemon* s_instance;
        static volatile std::sig_atomic_t s_signalCaught;
        static int s_pipeFds[2]; // pipe_fds[0] = read, pipe_fds[1] = write

        static void SignalHandler(const int signal)
        {
            s_signalCaught = signal;

            constexpr char signal_byte = 1;
            write(s_pipeFds[1], &signal_byte, 1);
        }
    };
}
