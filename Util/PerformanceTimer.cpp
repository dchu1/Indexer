#include "PerformanceTimer.h"

void Util::PerformanceTimer::create_timer(std::string name)
{
	_timers[name] = PerformanceEntry{};
}

void Util::PerformanceTimer::start_timer(std::string name)
{
	_timers[name].start_time = std::chrono::steady_clock::now();
}

double Util::PerformanceTimer::stop_timer(std::string name)
{
	return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - _timers[name].start_time).count();
}

double Util::PerformanceTimer::timer_total(std::string name)
{
	return _timers[name].total_time;
}