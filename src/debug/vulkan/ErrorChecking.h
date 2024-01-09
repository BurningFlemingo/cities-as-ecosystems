#pragma once

#include "debug/ErrorHandler.h"

#define CHECK_VK_FATAL(func, ...)  \
{ \
	if ((func) != VK_SUCCESS) { \
		DEBUG::error(__VA_ARGS__); \
	} \
} 

#ifdef ENABLE_WARNINGS
	#define CHECK_VK_WARNING(func, ...)  \
	{ \
		if ((func) != VK_SUCCESS) { \
			DEBUG::warn(__VA_ARGS__); \
		} \
	} 
	
#else
	#define CHECK_VK_WARNING(func, Args...) {func}
#endif

#ifdef ENABLE_INFO
	#define CHECK_VK_INFO(func, ...)  \
	{ \
		if ((func) != VK_SUCCESS) { \
			DEBUG::inform(__VA_ARGS__); \
		} \
	} 
#else
	#define CHECK_VK_INFO(func, Args...) {func}
#endif
