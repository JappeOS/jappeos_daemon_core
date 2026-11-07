#pragma once

#include <atomic>
#include <condition_variable>
#include <queue>
#include <thread>

#include "daemon_logger.h"

namespace JappeStudios::JappeOS::DaemonCore::Logging
{

#define SD_EMERG   "<0>"  /* system is unusable */
#define SD_ALERT   "<1>"  /* action must be taken immediately */
#define SD_CRIT    "<2>"  /* critical conditions */
#define SD_ERR     "<3>"  /* error conditions */
#define SD_WARNING "<4>"  /* warning conditions */
#define SD_NOTICE  "<5>"  /* normal but significant condition */
#define SD_INFO    "<6>"  /* informational */
#define SD_DEBUG   "<7>"  /* debug-level messages */

    class StdoutLogger : public DaemonLogger
    {
    public:
        explicit StdoutLogger();
        ~StdoutLogger() override;

        void Emerg (const std::string &str) override;
        void Alert (const std::string &str) override;
        void Crit  (const std::string &str) override;
        void Err   (const std::string &str) override;
        void Warn  (const std::string &str) override;
        void Notice(const std::string &str) override;
        void Info  (const std::string &str) override;
        void Debug (const std::string &str) override;

    private:
        std::atomic<bool>       _done = false;
        std::thread             _loggerThread;
        std::queue<std::string> _logQueue;
        std::mutex              _queueMutex;
        std::condition_variable _cv;

        void Log(const std::string& prefix, const std::string& str);
        void Process();
    };
}
