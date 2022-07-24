#include "exception.h"

void exceptionHandle(
	const std::exception &Ex, const std::string &szOperation
)
{
		auto Message = Ex.what();
		if(Message == nullptr) {
			nLog::error("Exception while {}", szOperation);
		}
		else {
			nLog::error("Exception while {}: {}", szOperation, Message);
		}
};
