#pragma once

#include "debug/Logging.h"
#include <string>

template <typename ...Args>
void assertFatal(bool condition, Args... args) {
	if (!condition) {
		LOGGING::log("\tFATAL::ASSERTION_FAILURE::", args..., "::",  __FILE__, "::", __LINE__); \
			std::abort();
		std::cout << std::endl;
	}
}

#ifdef ENABLE_WARNINGS
	template <typename ...Args>
	void assertWarning(bool condition, Args... args) {
		if (!condition) {
			LOGGING::log("\tWARNING::ASSERTION_FAILURE::", args..., "::",  __FILE__, "::", __LINE__); \
			std::cout << std::endl;
		}
	}
#else 
#define ASSERT_WARNING(condition, ...) {condition}
#endif

#ifdef ENABLE_INFO
	template <typename ...Args>
	void assertInfo(bool condition, Args... args) {
		if (!condition) {
			LOGGING::log("\tINFO::ASSERTION_FAILURE::", args..., "::",  __FILE__, "::", __LINE__); \
			std::cout << std::endl;
		}
	}
#else
	template <typename ...Args>
	void assertInfo(bool condition, Args... args) {condition}
#endif
