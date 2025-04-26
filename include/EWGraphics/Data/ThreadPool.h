#pragma once
#include "EWGraphics/Data/KeyValueContainer.h"
#include "EWGraphics/Preprocessor.h"

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <cassert>


namespace EWE {
    //i want this to be statically accessible, aka dont want to pass around a reference or pointer
    //id prefer to make it data oriented rather than a class, but that's a little complicated with templates

    class ThreadPool {
    private:
        static ThreadPool* singleton;
        std::vector<std::thread> threads{};
        KeyValueContainer<std::thread::id, std::queue<std::function<void()>>, true> threadSpecificTasks;
        static thread_local std::queue<std::function<void()>>* localThreadTasks;
        static thread_local int myThreadIndex;

        std::vector<std::mutex> threadMutexesBase;
        KeyValueContainer<std::thread::id, std::mutex*, true> threadSpecificMutex;
#if EWE_DEBUG
        struct TaskStruct {
            std::string name;
            std::function<void()> func;
        };
        std::queue<TaskStruct> tasks{};
#else
        std::queue<std::function<void()>> tasks{};
#endif

        std::mutex queueMutex{};
        std::mutex counterMutex{};
        std::condition_variable condition{};
        std::condition_variable counterCondition{};
        std::size_t numTasksEnqueued{ 0 };
        std::size_t numTasksCompleted{ 0 };
        bool stop{ false };

        static bool WaitCondition();
        explicit ThreadPool(std::size_t numThreads);
        ~ThreadPool();


    public:
        enum TaskType {
            Task_None = 0,
            Task_ThreadLocal,
            Task_Generic,
        };

        template<typename F, typename... Args>
        static void EnqueueForEachThread(F&& f, Args&&... args) {
            assert(singleton != nullptr);
            const std::thread::id thisThreadID = std::this_thread::get_id();

            for (auto& thread : singleton->threads) {
                assert(thisThreadID != thread.get_id());
                singleton->threadSpecificTasks.GetValue(thread.get_id()).push(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            }
            for (auto& thread : singleton->threads) {
                thread.join();
            }
        }
        template<typename Value>
        static auto GetKVContainerWithThreadIDKeys() {
            KeyValueContainer<std::thread::id, Value, true> ret{};
            ret.reserve(singleton->threads.size());

            for (auto& threadID : singleton->threads) {
                ret.Add(threadID.get_id());
            }
            return ret;
        }

        void GiveTaskToAThread(std::thread::id id, std::function<void()> task);
        template<typename F, typename... Args>
        void GiveTaskToAThread(std::thread::id id, F&& func, Args&&... args) {
            auto task = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
            GiveTaskToAThread(id, task);
        }

        static void Construct();
        static void Deconstruct();

        static void EnqueueVoidFunction(std::function<void()> task);
        static void EnqueueVoidFunction(std::string const& threadName, std::function<void()> task);

        template<typename F, typename... Args>
        static auto Enqueue(F&& f, Args&&... args) {
            if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>) {
                auto task = std::make_shared<std::packaged_task<void()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

                std::future<std::invoke_result_t<F, Args...>> res = task->get_future();
                {
                    std::unique_lock<std::mutex> lock(singleton->queueMutex);
                    assert(!singleton->stop && "enqueue on stopped threadpool");
#if EWE_DEBUG
                    singleton->tasks.emplace("", [task]() { (*task)(); });
#else
                    singleton->tasks.emplace([task]() { (*task)(); });
#endif

                    // Increment the number of tasks enqueued
                    std::unique_lock<std::mutex> counterLock(singleton->counterMutex);
                    ++singleton->numTasksEnqueued;
                }
                singleton->condition.notify_one();
                return res;
            }
            else {
                auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
                EnqueueVoidFunction(task);
            }
        }

        template<typename F, typename... Args>
        static auto Enqueue(std::string const& threadName, F&& f, Args&&... args) {
#if THREAD_NAMING
            if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>) {
                auto task = std::make_shared<std::packaged_task<void()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

                std::future<std::invoke_result_t<F, Args...>> res = task->get_future();
                {
                    std::unique_lock<std::mutex> lock(singleton->queueMutex);
                    assert(!singleton->stop && "enqueue on stopped threadpool");
                    singleton->tasks.emplace(threadName, [task]() { (*task)(); });

                    // Increment the number of tasks enqueued
                    std::unique_lock<std::mutex> counterLock(singleton->counterMutex);
                    ++singleton->numTasksEnqueued;
                }
                singleton->condition.notify_one();
                return res;
            }
            else {
                auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
                EnqueueVoidFunction(threadName, task);
            }
#else
            Enqueue(std::forward<F>(f), std::forward<Args>(args)...);
#endif
        }

        template<typename F, typename... Args>
        static void EnqueueVoid(F&& f, Args&&... args) {
            auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            EnqueueVoidFunction(task);
        }
        template<typename F, typename... Args>
        static void EnqueueVoid(std::string const& threadName, F&& f, Args&&... args) {
            auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
#if THREAD_NAMING
            EnqueueVoidFunction(threadName, task);
#else
            EnqueueVoidFunction(task);
#endif
        }
        static void WaitForCompletion();
        static bool CheckEmpty();

        static std::vector<TaskType> const& GetThreadTasks() {
            return singleton->threadTasks;
        }
        private:
            std::vector<TaskType> threadTasks;
    };
}