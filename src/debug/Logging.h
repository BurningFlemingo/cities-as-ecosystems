#pragma once

#include <pch.h>
#include <iostream>
#include <initializer_list>
#include <string>

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

		if (severity == SEVERITY::FATAL) {
			std::cout << "::" << __FILE__ << "::" << __LINE__ << std::endl;
			std::abort();
		}
		std::cout << std::endl;
	}
}


template <typename ...Args>
void logFatal(Args... args) {
	LOGGING::severityLog(LOGGING::SEVERITY::FATAL, args...);
}

#ifdef ENABLE_WARNINGS
	template <typename ...Args>
	void logWarning(Args... args) {
		LOGGING::severityLog(LOGGING::SEVERITY::WARNING, args...);
	}
#else
	template <typename ...Args>
	void logWarning(Args... args) {};
#endif

#ifdef ENABLE_INFO
	template <typename ...Args>
	void logInfo(Args... args) {
		LOGGING::severityLog(LOGGING::SEVERITY::INFO, args...);
	}
#else
	template <typename ...Args>
	void logInfo(Args... args) {};
#endif
