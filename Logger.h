

#pragma once
#include "GameState.h"
#include <string>

// ─────────────────────────────────────────────
// Logger — "Black Box" system
// ─────────────────────────────────────────────
namespace Logger {
    void logEvent(GameState& state, LogLevel level, int line,
                  const std::string& cmd, const std::string& op,
                  const std::string& data);

    void logInfo(GameState& state, int line,
                 const std::string& cmd, const std::string& op,
                 const std::string& data);

    void logWarning(GameState& state, int line,
                    const std::string& cmd, const std::string& op,
                    const std::string& data);

    void logError(GameState& state, int line,
                  const std::string& cmd, const std::string& op,
                  const std::string& data);

    void clearLogs(GameState& state);
    void exportLogsToFile(GameState& state, const std::string& filename);
    std::string levelToString(LogLevel level);
}
