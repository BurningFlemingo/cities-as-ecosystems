#pragma once

#include <iostream>
#include <initializer_list>
#include <string>

#define LOCATION_STRING_ARGS  "::", __FILE__, "::", __LINE__

namespace LOGGING {

	enum class SEVERITY {
		INFO, 
		WARNING,
		FATAL, 
	};

	template <typename T>
	void log(T msg) {
		std::cout << msg;
	}

	template <typename T, typename... Args>
	void log(T msg, Args... args) {
		std::cout << msg;
		log(args...);
	}

	template <typename... Args>
	void severityLog(SEVERITY severity, Args... args) {
		std::string severityStrings[3]{"INFO", "WARNING", "FATAL"};

		std::cout << "\t" << severityStrings[(uint32_t)severity] << "::";

		log(args...);

		std::cout << std::endl;
	}

	template <typename... Args>
	void severityLog(SEVERITY severity) {
		std::string severityStrings[3]{"INFO", "WARNING", "FATAL"};

		std::cout << "\t" << severityStrings[(uint32_t)severity] << "::" << std::endl;
	}
}


template <typename ...Args>
inline void logFatal(Args... args) {
	LOGGING::severityLog(LOGGING::SEVERITY::FATAL, args...);
}

#ifdef ENABLE_WARNINGS
	template <typename ...Args>
	inline void logWarning(Args... args) {
		LOGGING::severityLog(LOGGING::SEVERITY::WARNING, args...);
	}
#else
	template <typename ...Args>
	void logWarning(Args... args) {};
#endif

#ifdef ENABLE_INFO
	template <typename ...Args>
	inline void logInfo(Args... args) {
		LOGGING::severityLog(LOGGING::SEVERITY::INFO, args...);
	}
#else
	template <typename ...Args>
	void logInfo(Args... args) {};
#endif
