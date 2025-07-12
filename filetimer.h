#pragma once
//用于记录文件接收所需要的全部时间

#include <chrono>
#include <iostream>

class FileTimer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::nanoseconds;

    FileTimer() : m_isRunning(false), m_duration(0) {}

    // 启动计时器
    void start() {
        if (!m_isRunning) {
            m_startPoint = Clock::now();
            m_isRunning = true;
            std::cout << "计时器启动\n";
        } else {
            std::cout << "警告: 计时器已在运行中\n";
        }
    }

    // 停止计时器并保存持续时间
    void stop() {
        if (m_isRunning) {
            TimePoint end = Clock::now();
            m_duration = std::chrono::duration_cast<Duration>(end - m_startPoint);
            m_isRunning = false;
            std::cout << "计时器停止\n";
        } else {
            std::cout << "警告: 计时器尚未启动\n";
        }
    }

    // 重置计时器（清除保存的时间）
    void reset() {
        m_duration = Duration(0);
        m_isRunning = false;
        std::cout << "计时器已重置\n";
    }

    // 获取总时间（纳秒）
    long long getNanoseconds() const {
        return m_duration.count();
    }

    // 获取总时间（毫秒）
    double getMilliseconds() const {
        return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(m_duration).count();
    }

    // 获取总时间（秒）
    double getSeconds() const {
        return std::chrono::duration_cast<std::chrono::duration<double>>(m_duration).count();
    }

    // 检查计时器是否正在运行
    bool isRunning() const {
        return m_isRunning;
    }

    // 获取当前已经过的时间（不停止计时）
    double getElapsedMilliseconds() const {
        if (!m_isRunning) return 0.0;
        auto current = Clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current - m_startPoint);
        return elapsed.count();
    }

private:
    TimePoint m_startPoint;
    Duration m_duration;
    bool m_isRunning;
};
