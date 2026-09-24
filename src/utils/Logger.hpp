#pragma once
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

namespace formaia::utils {

// Logger sencillo, con salida a stdout y timestamp + hilo, protegido por
// mutex porque el sistema es multi-hilo (event loop + workers). En
// producción esto se reemplaza fácilmente por spdlog sin tocar el resto
// del código: toda la app llama a Logger::info/warn/error.
class Logger {
public:
    static void info(const std::string& msg)  { log("INFO ", msg); }
    static void warn(const std::string& msg)  { log("WARN ", msg); }
    static void error(const std::string& msg) { log("ERROR", msg); }

private:
    static void log(const char* level, const std::string& msg) {
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream ts;
        ts << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
        std::cout << "[" << ts.str() << "] [" << level << "] "
                  << "[hilo " << std::this_thread::get_id() << "] "
                  << msg << std::endl;
    }
};

} // namespace formaia::utils
