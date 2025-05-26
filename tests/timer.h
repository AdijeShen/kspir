#pragma once
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <list>
#include <mutex>
#include <ostream>
#include <string>
#include <utility>

class Timer {
public:
  typedef std::chrono::system_clock::time_point timeUnit;

  std::list<std::pair<timeUnit, std::string>> mTimes;
  bool mLocking;
  std::mutex mMtx;

  Timer(bool locking = false) : mLocking(locking) { reset(); }

  const timeUnit &setTimePoint(const std::string &msg) {
    mTimes.push_back(std::make_pair(timeUnit::clock::now(), msg));
    return mTimes.back().first;
  }

  // 模板化的 getTotalTime，默认单位为 milliseconds，支持浮点精度
  template <typename Duration = std::chrono::milliseconds>
  double getTotalTime() const {
    auto totalDuration = mTimes.back().first - mTimes.front().first;
    return std::chrono::duration_cast<std::chrono::duration<double>>(
               totalDuration)
               .count() /
           std::chrono::duration_cast<std::chrono::duration<double>>(
               Duration{1})
               .count();
  }

  // 模板化的 getTimePoint，默认单位为 milliseconds，支持浮点精度
  template <typename Duration = std::chrono::milliseconds>
  std::pair<double, double> getTimePoint(const std::string &msg) const {
    auto iter = std::prev(mTimes.end());
    auto prev = std::prev(std::prev(mTimes.end()));

    while (iter != mTimes.begin()) {
      if (iter->second == msg) {
        // 使用高精度单位（例如 microseconds）计算时间差
        auto totalDuration = mTimes.back().first - iter->first;
        auto diffDuration = iter->first - prev->first;

        // 将时间差转换为目标单位 Duration 的浮点数
        double time = std::chrono::duration_cast<std::chrono::duration<double>>(
                          totalDuration)
                          .count() /
                      std::chrono::duration_cast<std::chrono::duration<double>>(
                          Duration{1})
                          .count();
        double diff = std::chrono::duration_cast<std::chrono::duration<double>>(
                          diffDuration)
                          .count() /
                      std::chrono::duration_cast<std::chrono::duration<double>>(
                          Duration{1})
                          .count();

        return std::make_pair(time, diff);
      }
      --iter;
      --prev;
    }
    return std::make_pair(0.0, 0.0);
  }

  friend std::ostream &operator<<(std::ostream &out, const Timer &timer);

  void reset() {
    setTimePoint("__Begin__");
    mTimes.clear();
  }

  std::string to_string() const {
    std::ostringstream stream;
    stream << *this;
    return stream.str();
  }
};

const Timer gTimer(true);

class TimerAdapter {
public:
  virtual ~TimerAdapter() = default;

  virtual void setTimer(Timer &timer) { mTimer = &timer; }

  Timer &getTimer() {
    if (mTimer)
      return *mTimer;
    throw std::runtime_error("Timer not set.");
  }

  Timer::timeUnit setTimePoint(const std::string &msg) {
    if (mTimer)
      return getTimer().setTimePoint(msg);
    else
      return Timer::timeUnit::clock::now();
  }

  Timer *mTimer = nullptr;
};

inline std::ostream &operator<<(std::ostream &out, const Timer &timer) {
  if (timer.mTimes.size() > 1) {
    uint64_t maxStars = 10;
    uint64_t p = 9;
    uint64_t width = 0;
    auto maxLog = 1.0;

    {
      auto prev = timer.mTimes.begin();
      auto iter = timer.mTimes.begin();
      ++iter;

      while (iter != timer.mTimes.end()) {
        width = std::max<uint64_t>(width, iter->second.size());
        auto diff = std::chrono::duration_cast<std::chrono::microseconds>(
                        iter->first - prev->first)
                        .count() /
                    1000.0;
        maxLog = std::max(maxLog, std::log2(diff));
        ++iter;
        ++prev;
      }
    }
    width += 3;

    out << std::left << std::setfill(' ') << std::setw(width) << "Label  "
        << "  " << std::setw(p) << "Time (ms)"
        << "  " << std::setw(p) << "diff (ms)\n"
        << std::string(width + 2 * p + 6, '_') << std::endl;

    auto prev = timer.mTimes.begin();
    auto iter = timer.mTimes.begin();
    ++iter;

    while (iter != timer.mTimes.end()) {
      auto time = std::chrono::duration_cast<std::chrono::microseconds>(
                      iter->first - timer.mTimes.front().first)
                      .count() /
                  1000.0;
      auto diff = std::chrono::duration_cast<std::chrono::microseconds>(
                      iter->first - prev->first)
                      .count() /
                  1000.0;
      uint64_t numStars = static_cast<uint64_t>(
          std::round(std::max(0.1, std::log2(diff)) * maxStars / maxLog));

      out << std::setw(width) << std::left << iter->second << "  " << std::right
          << std::fixed << std::setprecision(1) << std::setw(p) << time << "  "
          << std::right << std::fixed << std::setprecision(3) << std::setw(p)
          << diff << "  " << std::string(numStars, '*') << std::endl;

      ++prev;
      ++iter;
    }
  }
  return out;
}

template <> struct fmt::formatter<Timer> {
  constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(const Timer &v, FormatContext &ctx) {
    return format_to(ctx.out(), "Timer\n{}", v.to_string());
  }
};