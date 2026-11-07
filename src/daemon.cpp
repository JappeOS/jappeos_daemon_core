#include "daemon.h"

#include <utility>

namespace JappeStudios::JappeOS::DaemonCore
{

    const char* DaemonState_ToString(const DaemonState state)
    {
        switch (state)
        {
#define X(name) case DaemonState::name: return #name;
#include "daemon_state.def"
#undef X
            default: return "Unknown";
        }
    }

    Daemon::Daemon(
        std::vector<std::string> startupArgs,
        std::function<DaemonClass*()> factory,
        std::optional<DaemonDBusConfig> dbusConfig)
        :   _startupArgs(std::move(startupArgs)),
            _factory(std::move(factory)),
            _dbusConfig(std::move(dbusConfig))
    {
        if (s_instance != nullptr)
            throw JosException(
                "SINGLETON_ALREADY_INITIALIZED",
                "An instance of Daemon already initialized.");

        s_instance = this;
    }

    Daemon::~Daemon()
    {
        s_instance = nullptr;
    }

    void Daemon::Run()
    {
        _exitCode = EXIT_SUCCESS;
        try
        {
            _state = DaemonState::Initializing;
            Initialize();
            _state = DaemonState::Running;
            while (_shouldRun)
            {
                // Update the application
                Update();

                // Handle signals
                if (s_signalCaught)
                {
                    NULL_SAFE_CALL(_logger, Info("Received signal " + std::to_string(s_signalCaught) + ", exiting..."));
                    switch (s_signalCaught)
                    {
                        case SIGALRM: throw JosException("SIG_ALRM", "");
                        case SIGABRT: throw JosException("SIG_ABRT", "");
                        case SIGBUS:  throw JosException("SIG_BUS", "");
                        case SIGFPE:  throw JosException("SIG_FPE", "");
                        case SIGHUP:  throw JosException("SIG_HUP", "");
                        case SIGILL:  throw JosException("SIG_ILL", "");
                        case SIGPIPE: throw JosException("SIG_PIPE", "");
                        case SIGQUIT: throw JosException("SIG_QUIT", "");
                        case SIGSEGV: throw JosException("SIG_SEGV", "");
                        case SIGSYS:  throw JosException("SIG_SYS", "");
                        case SIGUSR1: throw JosException("SIG_USR1", "");
                        case SIGUSR2: throw JosException("SIG_USR2", "");
                        case SIGXCPU: throw JosException("SIG_XCPU", "");
                        case SIGXFSZ: throw JosException("SIG_XFSZ", "");
                        default: Quit();
                    }
                }
            }
            _state = DaemonState::Stopping;
            Cleanup();
            _state = DaemonState::Stopped;
        }
        catch (const std::exception& e)
        {
            NULL_SAFE_CALL(_logger, Alert(""));
            NULL_SAFE_CALL(_logger, Alert("An unhandled exception was thrown! Printing exception information below ..."));
            NULL_SAFE_CALL(_logger, Alert("+-- BEGIN EXCEPTION INFO --+"));
            NULL_SAFE_CALL(_logger, Alert(""));
            NULL_SAFE_CALL(_logger, Alert(std::string("Application State: ") + DaemonState_ToString(_state)));
            NULL_SAFE_CALL(_logger, Alert(""));
            NULL_SAFE_CALL(_logger, Alert(std::string("Exception: ") + e.what()));
            NULL_SAFE_CALL(_logger, Alert(""));
            NULL_SAFE_CALL(_logger, Alert("+-- END EXCEPTION INFO --+"));
            NULL_SAFE_CALL(_logger, Alert(""));

            if (_state == DaemonState::Running)
            {
                NULL_SAFE_CALL(_logger, Info("Application was running. Attempting cleanup."));
                try
                {
                    _state = DaemonState::Stopping;
                    Cleanup();
                }
                catch (const std::exception&)
                {
                    NULL_SAFE_CALL(_logger, Err("Cleanup failed."));
                }
            }

            _state = DaemonState::Stopped;
            _exitCode = EXIT_FAILURE;

            std::string errorCode = "EXCEPTION_UNHANDLED";
            std::string stackTrace;
            if (auto* se = dynamic_cast<const std::system_error*>(&e))
            {
                errorCode = std::to_string(se->code().value());
            }
            else if (auto* je = dynamic_cast<const Utils::JosException*>(&e))
            {
                errorCode = je->ShortErrorCode();
                stackTrace = je->Stacktrace();
            }

            FailFastFatalError(
                errorCode,
                e.what(),
                stackTrace.empty() ? StackTrace::Print() : stackTrace
            );
        }

        exit(_exitCode);
    }

    void Daemon::Initialize()
    {
        if (pipe(s_pipeFds) == -1)
        {
            throw JosException("SIGPIPE_CREATE_FAIL", "Failed to create signal pipe.");
        }

        _logger = new Logging::StdoutLogger();
        NULL_SAFE_CALL(_logger, Debug("Initializing..."));

        struct sigaction sa{};
        sa.sa_handler = SignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        // TODO
        sigaction(SIGALRM, &sa, nullptr);
        sigaction(SIGABRT, &sa, nullptr);
        //sigaction(SIGBUS, &sa, nullptr);
        //sigaction(SIGFPE, &sa, nullptr);
        sigaction(SIGHUP, &sa, nullptr);
        //sigaction(SIGILL, &sa, nullptr);
        sigaction(SIGPIPE, &sa, nullptr);
        sigaction(SIGQUIT, &sa, nullptr);
        //sigaction(SIGSEGV, &sa, nullptr);
        sigaction(SIGSYS, &sa, nullptr);
        sigaction(SIGUSR1, &sa, nullptr);
        sigaction(SIGUSR2, &sa, nullptr);
        sigaction(SIGXCPU, &sa, nullptr);
        sigaction(SIGXFSZ, &sa, nullptr);
        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);

        int dbusFd = -1;
        InitDBus(dbusFd);

        _fds[0].fd = dbusFd;
        _fds[0].events = POLLIN;

        _fds[1].fd = s_pipeFds[0];
        _fds[1].events = POLLIN;

        _daemonInstance = _factory();

        NULL_SAFE_CALL(_logger, Debug("Initialization finished!"));
    }

    void Daemon::InitDBus(int& dbusFd)
    {
        dbusFd = -1;
        if (!_dbusConfig.has_value())
            return;

        const auto& config = _dbusConfig.value();
        _conn = config.connectionFactory();
        _conn->AcquireName(config.name, config.flags);
        NULL_SAFE_CALL(_logger, Debug(std::string("D-Bus name acquired: ") + config.name));
        dbusFd = _conn->GetUnixFd();
    }

    void Daemon::Cleanup()
    {
        NULL_SAFE_CALL(_logger, Debug("Cleaning up..."));

        delete _daemonInstance;
        _daemonInstance = nullptr;
        delete _conn;
        _conn = nullptr;

        NULL_SAFE_CALL(_logger, Debug("Cleanup done!"));
        delete _logger;
        _logger = nullptr;
    }

    void Daemon::Update()
    {
        auto* mainContext = g_main_context_default();

        std::vector<GPollFD> glibFds;
        gint glibMaxPriority = 0;
        gint glibTimeoutMs = -1;
        gint glibN = 0;
        bool glibAcquired = false;

        if (mainContext)
        {
            glibAcquired = g_main_context_acquire(mainContext);
            if (glibAcquired)
            {
                const bool glibReady = g_main_context_prepare(mainContext, &glibMaxPriority);

                glibFds.resize(16);
                glibN = g_main_context_query(
                    mainContext,
                    glibMaxPriority,
                    &glibTimeoutMs,
                    glibFds.data(),
                    static_cast<gint>(glibFds.size())
                );

                if (glibN > static_cast<gint>(glibFds.size()))
                {
                    glibFds.resize(static_cast<size_t>(glibN));
                    glibN = g_main_context_query(
                        mainContext,
                        glibMaxPriority,
                        &glibTimeoutMs,
                        glibFds.data(),
                        static_cast<gint>(glibFds.size())
                    );
                }

                if (glibReady)
                    glibTimeoutMs = 0;
            }
        }

        std::vector<pollfd> pollFds;
        pollFds.reserve(2 + static_cast<size_t>(std::max(0, glibN)));
        pollFds.push_back(_fds[0]);
        pollFds.push_back(_fds[1]);

        for (gint i = 0; i < glibN; ++i)
        {
            pollfd pfd{};
            pfd.fd = glibFds[i].fd;
            pfd.events = static_cast<short>(glibFds[i].events);
            pollFds.push_back(pfd);
        }

        if (poll(pollFds.data(), pollFds.size(), glibTimeoutMs) < 0)
        {
            if (glibAcquired)
                g_main_context_release(mainContext);
            return;
        }

        _fds[0].revents = pollFds[0].revents;
        _fds[1].revents = pollFds[1].revents;

        // GLib support
        if (glibAcquired)
        {
            for (gint i = 0; i < glibN; ++i)
                glibFds[i].revents = static_cast<gushort>(pollFds[2 + i].revents);

            if (g_main_context_check(mainContext, glibMaxPriority, glibFds.data(), glibN))
                g_main_context_dispatch(mainContext);

            g_main_context_release(mainContext);
        }

        // Signals
        if (_fds[1].revents & POLLIN)
        {
            char buf[64];
            read(s_pipeFds[0], buf, sizeof(buf));

            NULL_SAFE_CALL(_logger, Info("Received signal, exiting..."));
            Quit();
            return;
        }

        // D-Bus messages
        UpdateDBus();
    }

    void Daemon::UpdateDBus() const
    {
        if (_fds[0].revents & POLLIN)
        {
            while (_conn->ReadWrite(0))
            {
                auto msg = _conn->PopMessage();
                if (!msg.has_value()) break;
                _conn->HandleMessage(msg.value());
            }
        }
    }

    void Daemon::FailFastFatalError(const std::string& errCode,
                                    const std::string& message,
                                    const std::string& stack) const
    {
        auto newErrCode = errCode;
        for (auto& c: newErrCode) c = toupper(c);
        std::erase(newErrCode, ' ');

        const auto newStack = stack.empty() ? StackTrace::Print() : stack;

        NULL_SAFE_CALL(
            _logger,
            Emerg(
                "[Application::FailFastFatalError] FailFastFatalError called! "
                "ErrCode: " + newErrCode
                + ", Message: " + message
                + ", Stack: " + newStack
                + ". This indicates that a fatal system error has occurred."
            )
        );
        NULL_SAFE_CALL(
            _logger,
            Crit("[Application::FailFastFatalError] Exiting immediately because of unrecoverable fatal error.")
        );
        exit(EXIT_FAILURE);
    }

    Daemon* Daemon::s_instance = nullptr;
    volatile std::sig_atomic_t Daemon::s_signalCaught = 0;
    int Daemon::s_pipeFds[2] = { -1, -1 };

}