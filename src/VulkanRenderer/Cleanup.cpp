#include "Cleanup.h"

void DeletionQueue::pushFunction(std::function<void()>&& deletionProc) {
	m_DeletionProcedures.emplace_back(deletionProc);
}

void DeletionQueue::flush() {
	for (auto itt{ m_DeletionProcedures.rbegin() };
		 itt < m_DeletionProcedures.rend();
		 ++itt) {
		(*itt)();
	}
	m_DeletionProcedures.clear();
}
