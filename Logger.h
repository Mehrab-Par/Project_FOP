#pragma once
#include <string>
#include <fstream>
#include <iostream>

namespace Logger {
    enum class Level { INFO, WARNING, ERROR_LVL };
    
    void init(const std::string& filename = "scratch.log");
    void log(Level level, const std::string& message);
    void info(const std::string& msg);
    void warning(const std::string& msg);
    void error(const std::string& msg);
    void close();
}
