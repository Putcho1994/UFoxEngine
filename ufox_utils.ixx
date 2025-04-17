//
// Created by Putcho on 16.04.2025.
//

module;
#include <chrono>

#include <iomanip>

#include <map>
#include <sstream>
#include <string>
#include <fstream>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#include <unistd.h>
#endif

export module ufox_utils;

import fmt;

export namespace ufox::utils::logger {
    inline constexpr const char* SeparatorLine = "--------------------------------------------------------------------------------------------------";
    inline constexpr int DefaultPrecision = 3;
    inline constexpr double BytesToMB = 1'000'000.0;

    enum class VerbosityLevels {
        None = 0,
        Debug = 4
    };
    struct TimerData {
        int64_t total_elapsed = 0;
        int64_t last_elapsed = 0;
        int64_t first_tick = 0;
        int64_t last_tick = 0;
        std::map<std::string, int64_t> laps;
    };

    inline VerbosityLevels verbosity = VerbosityLevels::Debug;
    inline TimerData timer_data;

    // Helper function: Get current time in nanoseconds since epoch
    inline int64_t get_nanoseconds_since_epoch() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }

    // Helper function: Convert ticks (nanoseconds) to formatted string in milliseconds
    inline std::string format_ticks_to_ms(int64_t ticks) {
        double ms = ticks / 1'000'000.0;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(DefaultPrecision) << ms << "ms";
        return oss.str();
    }

    // Helper function: Get process memory usage
    inline std::string get_memory_usage_in_mb() {
        uint64_t memory = 0;
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
            memory = pmc.WorkingSetSize;
        }
#else
        struct rusage usage;
        if (getrusage(RUSAGE_SELF, &usage) == 0) {
            memory = usage.ru_maxrss * 1024; // Convert KB to bytes
        }
#endif
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << (memory / BytesToMB) << "MB";
        return oss.str();
    }

    inline void show_headers() {
        fmt::print("{}\n", SeparatorLine);
        fmt::print("{:<8} | {:<14} | {:<12} | {:<12} | {:<25} | Result\n",
               "Memory", "Total", "Last", "Lap", "Name");
        fmt::print("{}\n", SeparatorLine);

    }

    inline void restart_timer() {
        if (verbosity == VerbosityLevels::None) return;
        timer_data.first_tick = get_nanoseconds_since_epoch();
        timer_data.last_tick = timer_data.first_tick;
        timer_data.total_elapsed = 0;
        timer_data.last_elapsed = 0;

    }

    inline void BeginDebugBlog(const std::string& name) {
        fmt::print("\n{}\n", name);
        show_headers();
    }

    inline void EndDebugBlog() {
        fmt::print("{}\n", SeparatorLine);
    }

    inline void start_lap(const std::string& lap_name) {
        timer_data.laps[lap_name] = get_nanoseconds_since_epoch();
    }

    inline void log_debug(const std::string& lap, const std::string& msg, bool skip_timer = false) {
        if (verbosity == VerbosityLevels::None) return;

        std::string memory = get_memory_usage_in_mb();
        if (!skip_timer) {
            int64_t now = get_nanoseconds_since_epoch();

            if (timer_data.laps.find(lap) == timer_data.laps.end()) {
                timer_data.laps[lap] = now;
            }

            int64_t lap_ticks = now - timer_data.laps[lap];
            fmt::print("{:<8} | {:<14} | {:<12} | {:<12} | {:<25} | {}\n",
                memory,
                format_ticks_to_ms(now - timer_data.first_tick),
                format_ticks_to_ms(now - timer_data.last_tick),
                format_ticks_to_ms(lap_ticks),
                lap,
                msg);

            timer_data.last_tick = now;
        } else {
            fmt::print("{:<8} | {:<14} | {:<12} | {:<12} | {:<25} | {}\n",memory,"","","","",msg);
        }
    }

}
