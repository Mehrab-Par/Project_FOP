#include "Logger.h"
#include <iostream>
#include <fstream>
#include <iomanip>

namespace Logger {

std::string levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:      return "INFO";
        case LogLevel::WARNING:   return "WARNING";
        case LogLevel::ERROR_LVL: return "ERROR";
    }
    return "UNKNOWN";
}

void logEvent(GameState& state, LogLevel level, int line,
              const std::string& cmd, const std::string& op,
              const std::string& data) {
    LogEntry entry;
    entry.cycle    = state.globalCycle;
    entry.line     = line;
    entry.command  = cmd;
    entry.operation = op;
    entry.data     = data;
    entry.level    = level;
    state.logs.push_back(entry);

    // Also print to console
    std::cout << "[Cycle:" << std::setw(5) << entry.cycle << "]"
              << " [Line:" << std::setw(3) << entry.line << "]"
              << " [" << levelToString(level) << "]"
              << " [CMD:" << cmd << "]"
              << " -> " << op << " | " << data
              << std::endl;
}

void logInfo(GameState& state, int line,
             const std::string& cmd, const std::string& op,
             const std::string& data) {
    logEvent(state, LogLevel::INFO, line, cmd, op, data);
}

void logWarning(GameState& state, int line,
                const std::string& cmd, const std::string& op,
                const std::string& data) {
    logEvent(state, LogLevel::WARNING, line, cmd, op, data);
}

void logError(GameState& state, int line,
              const std::string& cmd, const std::string& op,
              const std::string& data) {
    logEvent(state, LogLevel::ERROR_LVL, line, cmd, op, data);
}

void clearLogs(GameState& state) {
    state.logs.clear();
}

void exportLogsToFile(GameState& state, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[Logger] Failed to open file: " << filename << std::endl;
        return;
    }
    for (const auto& entry : state.logs) {
        file << "[Cycle:" << entry.cycle << "]"
             << " [Line:" << entry.line << "]"
             << " [" << levelToString(entry.level) << "]"
             << " [CMD:" << entry.command << "]"
             << " -> " << entry.operation << " | " << entry.data
             << "\n";
    }
    file.close();
    std::cout << "[Logger] Logs exported to: " << filename << std::endl;
}

}