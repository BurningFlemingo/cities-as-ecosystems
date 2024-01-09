#pragma once

#include "Logging.h"

namespace DEBUG {
	template <typename ...Args>
	inline void error(Args... args) {
		logFatal(args...);
		std::abort();
	}
	
	#ifdef ENABLE_WARNINGS
		template <typename ...Args>
		inline void warn(Args... args) {
			logWarning(args...);
		}
	#else
		template <typename ...Args>
		void warn(Args... args) {};
	#endif
	
	#ifdef ENABLE_INFO
		template <typename ...Args>
		inline void inform(Args... args) {
			logInfo(args...);
		}
	#else
		template <typename ...Args>
		void inform(Args... args) {};
	#endif
}
