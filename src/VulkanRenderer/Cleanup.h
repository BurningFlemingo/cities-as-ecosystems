#pragma once

#include <deque>
#include <functional>

class DeletionQueue {
   public:
	void pushFunction(std::function<void()>&& deletionProc);
	void flush();

   private:
	std::deque<std::function<void()>> m_DeletionProcedures;
};

template<typename T>
struct Deletable {
	T obj;
	std::function<void()> deleter;
};
