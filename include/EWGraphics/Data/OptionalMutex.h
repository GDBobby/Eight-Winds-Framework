#pragma once

/*
#include <mutex>
#include <type_traits>

namespace EWE {
	template <bool ThreadSafe>
	struct OptionalMutex {};

	template<>
	struct OptionalMutex<true> {
		std::recursive_mutex mutex{};

		void lock() {
			mutex.lock();
		}

		void unlock() {
			mutex.unlock();
		}
	};

	template <bool ThreadSafe>
	struct OptionalLock {
		ConditionalLock(OptionalMutex<false>&) {}
	};

	template<>
	struct OptionalLock<true> {
		OptionalMutex<true>& mutex;
		OptionalLock(OptionalMutex<true>& mut) : mutex{ mut } {
			mutex.lock();
		}
		~OptionalLock() {
			mutex.unlock();
		}
	};
}
*/