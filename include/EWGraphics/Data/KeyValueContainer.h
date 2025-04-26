#pragma once

#include "EWGraphics/Preprocessor.h"

#include <cstdint>
#include <cassert>
#include <type_traits>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <memory>

#include <vector>

//#include <thread>

#ifndef INITIAL_SIZE_LIMIT
#define INITIAL_SIZE_LIMIT 64
#endif

#define TO_STRING(str) #str
#define INITIAL_SIZE_STR(str) TO_STRING(str)


namespace EWE {
	namespace KV_Helper {
		template <typename, typename = void>
		struct is_container : std::false_type {};

		template <typename T>
		struct is_container<T, std::void_t<
			decltype(std::declval<T>().begin()),
			decltype(std::declval<T>().end()),
			decltype(std::declval<T>().size())
			>> : std::true_type {};

		template <bool ThreadSafe>
		struct ConditionalMutex {};

		template <>
		struct ConditionalMutex<true> {
			std::recursive_mutex mutex{};

			void lock() {
				mutex.lock();
			}

			void unlock() {
				mutex.unlock();
			}
		};

		template <bool ThreadSafe>
		struct ConditionalLock {
			ConditionalLock(ConditionalMutex<false>& mut) {}
		};

		template<>
		struct ConditionalLock<true> {
			ConditionalMutex<true>& mutex;
			ConditionalLock(ConditionalMutex<true>& mut) : mutex{ mut } { 
				mutex.lock(); 
			}
			~ConditionalLock() {
				mutex.unlock();
			}
		};

		constexpr void CheckSizeCondition(const std::size_t size) {
#if defined(__GNUC__) || defined(__clang__)
#if size > INITIAL_SIZE_LIMIT
			#warning "Warning: initialSize exceeds the INITIAL_SIZE_LIMIT (" INITIAL_SIZE_STR(INITIAL_SIZE_LIMIT) "), consider using a hash table"
#endif
#elif defined (_MSC_VER)
#if size > INITIAL_SIZE_LIMIT
#pragma message("Warning: initialSize exceeds the INITIAL_SIZE_LIMIT (" INITIAL_SIZE_STR(INITIAL_SIZE_LIMIT) "), consider using a hash table")
#endif
#endif
		}
	}

	template<typename Key, typename Value>
	struct KeyValuePair {
		Key key;
		Value value;

		using KeyParamType = typename std::conditional_t<
			std::is_pointer_v<Key> || std::is_lvalue_reference_v<Key>,
			Key,
			typename std::conditional_t<
				sizeof(Key) <= sizeof(void*) && !KV_Helper::is_container<Key>::value,
				Key,
				const Key&
			>
		>;
		using ValueParamType = typename std::conditional_t<
			std::is_pointer_v<Value> || std::is_lvalue_reference_v<Value>,
			Value,
			typename std::conditional_t<
				sizeof(Value) <= sizeof(void*) && !KV_Helper::is_container<Value>::value,
				Value,
				const Value&
			>
		>;


		KeyValuePair(KeyParamType key, ValueParamType value) : key{ key }, value{ value } {}

		template<typename = std::enable_if_t<std::is_default_constructible_v<Value>>>
		KeyValuePair(KeyParamType key) : key{ key }, value{ } {}

		template<typename K = Key, typename V = Value, typename = std::enable_if_t<std::is_default_constructible_v<K> && std::is_default_constructible_v<V>>>
		KeyValuePair() : key{}, value{} {}

		~KeyValuePair() = default;

		template <typename K = Key, typename V = Value, typename = std::enable_if_t<std::is_copy_constructible_v<K>&& std::is_copy_constructible_v<V>>>
		KeyValuePair(const KeyValuePair& copySource)
			: key{ copySource.key }, value{ copySource.value } {
		}

		template <typename K = Key, typename V = Value, typename = std::enable_if_t<std::is_move_constructible_v<K>&& std::is_move_constructible_v<V>>>
		KeyValuePair(KeyValuePair&& moveSource) noexcept
			: key{ std::move(moveSource.key) }, value{ std::move(moveSource.value) } {
		}

		template <typename K = Key, typename V = Value, typename = std::enable_if_t<std::is_copy_assignable_v<K>&& std::is_copy_assignable_v<V>>>
		KeyValuePair& operator=(const KeyValuePair& copySource) {
			assert(this != &copySource);
			key = copySource.key;
			value = copySource.value;
			return *this;
		}

		template <typename K = Key, typename V = Value, typename = std::enable_if_t<std::is_move_assignable_v<K>&& std::is_move_assignable_v<V>>>
		KeyValuePair& operator=(KeyValuePair&& moveSource) noexcept {
			assert(this != &moveSource);
			key = std::move(moveSource.key);
			value = std::move(moveSource.value);
			
			return *this;
		}

	};

	template<typename Key, typename Value, bool ThreadSafe = false/*, class Allocator = std::allocator<KeyValuePair<Key, Value>>*/>
	class KeyValueContainer {
	private:

		using KVPair = KeyValuePair<Key, Value>;
		std::vector<KVPair> inner_data;
		KV_Helper::ConditionalMutex<ThreadSafe> mutex;
	public:
		
		constexpr KeyValueContainer(std::size_t count) : inner_data{ count } { KV_Helper::CheckSizeCondition(count); }
		constexpr KeyValueContainer(std::size_t count, KVPair const& value) : inner_data{ count, value } { KV_Helper::CheckSizeCondition(count); }
		constexpr KeyValueContainer(std::initializer_list<KVPair> init) : inner_data{ init } { KV_Helper::CheckSizeCondition(init.size()); }

		KeyValueContainer(KeyValueContainer& copySource) : inner_data{ copySource.inner_data } {}
		KeyValueContainer(KeyValueContainer&& moveSource) : inner_data{ std::move(moveSource.inner_data) } {}
		KeyValueContainer& operator=(KeyValueContainer& other) = delete;
		KeyValueContainer& operator=(KeyValueContainer&& other) = delete;
		~KeyValueContainer() = default;

		template <typename T = void, typename = std::enable_if_t<ThreadSafe, T>>
		void Lock() {
			mutex.lock();
		}
		template <typename T = void, typename = std::enable_if_t<ThreadSafe, T>>
		void Unlock() {
			mutex.unlock();
		}

		using ValueReturnType = typename std::conditional_t<
			std::is_pointer_v<Value> || std::is_lvalue_reference_v<Value>,
			Value,
			Value&
		>;


		KVPair& at(KVPair::KeyParamType key) {
			KV_Helper::ConditionalLock<ThreadSafe>{mutex};
			for (auto& point : inner_data) {
				if (point.key == key) {
					return point;
				}
			}
			EWE_UNREACHABLE;
		}
		auto operator[](std::size_t i) {
			KV_Helper::ConditionalLock<ThreadSafe>{mutex};
			return inner_data[i];
		}
		ValueReturnType GetValue(KVPair::KeyParamType key) {
			return at(key).value;
		}

		void* data() {
			return inner_data.data();
		}
		auto begin() {
			return inner_data.begin();
		}
		auto cbegin() {
			return inner_data.cbegin();
		}
		auto end() {
			return inner_data.end();
		}
		auto cend() {
			return inner_data.cend();
		}
		std::size_t size() {
			return inner_data.size();
		}
		void reserve(std::size_t res) {
			inner_data.reserve(res);
		}

		void erase(std::vector<KVPair>::iterator iter)
		{
			inner_data.erase(iter);
		}

		void clear() {
			KV_Helper::ConditionalLock<ThreadSafe>{mutex};
			inner_data.clear();
		}
		template<typename = std::enable_if_t<std::is_default_constructible_v<Value>>>
		Value& Add(KVPair::KeyParamType key) {
			KV_Helper::ConditionalLock<ThreadSafe>{mutex};
		
			return inner_data.emplace_back(key).value;
		}
		void Add(KVPair const& kvPair) {
			KV_Helper::ConditionalLock<ThreadSafe>{mutex};
			inner_data.push_back(kvPair);
		}
		void Add(KVPair::KeyParamType key, KVPair::ValueParamType value) {
			KV_Helper::ConditionalLock<ThreadSafe>{mutex};
			inner_data.emplace_back(key, value);
		}

		void Remove(KVPair::KeyParamType key) {
			KV_Helper::ConditionalLock<ThreadSafe>{mutex};
			for (auto iter = inner_data.begin(); iter != inner_data.end(); iter++) {
				if (iter->key == key) {
					inner_data.erase(iter);
					return;
				}
			}
			assert(false);
		}
		bool Contains(KVPair::KeyParamType key) {
			KV_Helper::ConditionalLock<ThreadSafe>{mutex};
			for (auto iter = inner_data.begin(); iter != inner_data.end(); iter++) {
				if (iter->key == key) {
					return true;
				}
			}
			return false;
		}
	};

}