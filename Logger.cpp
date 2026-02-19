#include "Logger.h"
#include <ctime>

namespace Logger {
    static std::ofstream logFile;
    static bool initialized = false;

    std::string getTime() {
        time_t now = time(0);
        char buf[80];
        strftime(buf, 80, "%Y-%m-%d %H:%M:%S", localtime(&now));
        return buf;
    }

    void init(const std::string& filename) {
        logFile.open(filename, std::ios::app);
        if (logFile.is_open()) {
            initialized = true;
            info("Logger started");
        }
    }

    void log(Level level, const std::string& message) {
        std::string prefix;
        switch(level) {
            case Level::INFO: prefix = "[INFO] "; break;
            case Level::WARNING: prefix = "[WARN] "; break;
            case Level::ERROR_LVL: prefix = "[ERROR] "; break;
        }

        std::string msg = getTime() + " " + prefix + message;
        std::cout << msg << std::endl;

        if (initialized && logFile.is_open()) {
            logFile << msg << std::endl;
            logFile.flush();
        }
    }

    void info(const std::string& msg) { log(Level::INFO, msg); }
    void warning(const std::string& msg) { log(Level::WARNING, msg); }
    void error(const std::string& msg) { log(Level::ERROR_LVL, msg); }

    void close() {
        if (initialized) {
            info("Logger closing");
            logFile.close();
            initialized = false;
        }
    }
}
