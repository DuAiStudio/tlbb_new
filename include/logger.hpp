#pragma once

#include <string>

class Logger {
public:
    static Logger& instance();

    /// If true, `logShare` and `logConfig` are also written to stdout (matches legacy foreground output).
    bool init(const std::string& baseDir, bool mirrorToStdout = true);
    void logShare(const std::string& message);
    void logConfig(const std::string& message);
    void logDebug(const std::string& message);

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void logLine(const std::string& filePath, const std::string& message, bool mirrorStdout);
    std::string formatLine(const std::string& message) const;
    std::string buildLogPath(const std::string& prefix) const;

    bool initialized_{false};
    bool mirrorToStdout_{true};
    std::string baseDir_;
    std::string sharePath_;
    std::string configPath_;
    std::string debugPath_;
    long long startMs_{0};
};
