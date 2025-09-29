#pragma once

#include <cstdint>
#include <functional>

#include <LAB/Vector/Hash.h>

#include <concepts>

namespace EWE {

	template <typename T, typename... Rest>
	void HashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
		seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		(HashCombine(seed, rest), ...);
	};

	template <typename Stream, typename T>
	void Serialize(Stream& s, T* obj, const std::size_t size) {
		if constexpr (requires { s.read((char*)nullptr, size); }) {
			s.read(reinterpret_cast<char*>(obj), size);
		}
		else if constexpr (requires { s.Read((char*)nullptr, size); }) {
			s.Read(reinterpret_cast<char*>(obj), size);
		}        
		else if constexpr (requires { s.write((const char*)nullptr, size); }) {
			s.write(reinterpret_cast<const char*>(obj), size);
		}
		else if constexpr (requires { s.Write((const char*)nullptr, size); }) {
			s.Write(reinterpret_cast<const char*>(obj), size);
		}
		else {
			static_assert(std::is_void_v<Stream>, "Unsupported stream type");
		}
	}
}


