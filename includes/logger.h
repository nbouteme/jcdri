#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

namespace jcdri {
	enum log_level {
		TRACE,
		DEBUG,
		VERBOSE,
		INFO,
		ERROR,
		WARNING,
		NONE
	};

	log_level current = TRACE;

	template <typename ...Args>
	void log(log_level l, Args ... rest) {
		if (l >= current)
			printf(rest...);
	}
}

#endif /* LOGGER_H */