#pragma once

#include <chrono>
#include <unordered_map>

namespace Util
{
	struct PerformanceEntry
	{
		bool running;
		std::chrono::steady_clock::time_point start_time;
		double total_time;
	};

	class PerformanceTimer
	{
	public:
		void create_timer(std::string name);
		void start_timer(std::string name);
		double stop_timer(std::string name);
		double timer_total(std::string name);
	private:
		std::unordered_map<std::string, PerformanceEntry> _timers;
	};
}