#include "logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace {
std::mutex g_logMutex;

long long nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string nowDate() {
    const auto t = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmv, "%Y-%m-%d");
    return oss.str();
}

std::string nowClock() {
    const auto t = std::time(nullptr);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    std::ostringstream oss;
    oss << (tmv.tm_year + 1900) << "-" << (tmv.tm_mon + 1) << "-" << tmv.tm_mday << "_"
        << std::setfill('0') << std::setw(2) << tmv.tm_hour << ":" << std::setw(2) << tmv.tm_min << ":" << std::setw(2) << tmv.tm_sec;
    return oss.str();
}

unsigned int processId() {
#ifdef _WIN32
    return static_cast<unsigned int>(_getpid());
#else
    return static_cast<unsigned int>(getpid());
#endif
}
}  // namespace

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

bool Logger::init(const std::string& baseDir, bool mirrorToStdout) {
    std::lock_guard<std::mutex> lk(g_logMutex);
    baseDir_ = baseDir;
    mirrorToStdout_ = mirrorToStdout;
    std::filesystem::create_directories(baseDir_);
    sharePath_ = buildLogPath("ShareMemory");
    configPath_ = buildLogPath("Config");
    debugPath_ = buildLogPath("Debug");
    startMs_ = nowMs();
    initialized_ = true;
    return true;
}

void Logger::logShare(const std::string& message) {
    if (!initialized_) return;
    logLine(sharePath_, message, mirrorToStdout_);
}

void Logger::logConfig(const std::string& message) {
    if (!initialized_) return;
    logLine(configPath_, message, mirrorToStdout_);
}

void Logger::logDebug(const std::string& message) {
    if (!initialized_) return;
    logLine(debugPath_, message, false);
}

void Logger::logLine(const std::string& filePath, const std::string& message, bool mirrorStdout) {
    std::lock_guard<std::mutex> lk(g_logMutex);
    const std::string line = formatLine(message);
    std::ofstream out(filePath, std::ios::app);
    if (out.is_open()) {
        out << line << '\n';
    }
    if (mirrorStdout) {
        std::cout << line << '\n';
        std::cout.flush();
    }
}

std::string Logger::formatLine(const std::string& message) const {
    const auto threadHash = static_cast<unsigned int>(std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0xffffffffu);
    const double t1 = static_cast<double>(nowMs() - startMs_) / 1000.0;

    std::ostringstream oss;
    oss << message
        << " (" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << threadHash << std::dec << ")"
        << "(T0=" << nowClock()
        << " T1=" << std::fixed << std::setprecision(4) << t1 << ")";
    return oss.str();
}

std::string Logger::buildLogPath(const std::string& prefix) const {
    std::ostringstream oss;
    oss << baseDir_ << "/" << prefix << "_" << nowDate() << "." << processId() << ".log";
    return oss.str();
}
