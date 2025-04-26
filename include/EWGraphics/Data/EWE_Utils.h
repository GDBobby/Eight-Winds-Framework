#pragma once

#include <cstdint>
#include <functional>

#include <LAB/Vector/Hash.h>

namespace EWE {

	template <typename T, typename... Rest>
	void HashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		(HashCombine(seed, rest), ...);
	};
}


