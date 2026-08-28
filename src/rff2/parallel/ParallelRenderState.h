//
// Created by Merutilm on 2025-05-09.
//

#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <latch>
#include <memory>
#include <mutex>
#include <thread>

namespace merutilm::rff2 {
    class ParallelRenderState final {
        std::mutex mutex;
        mutable std::mutex pauseMutex;
        mutable std::condition_variable pauseCondition;
        std::atomic<bool> paused = false;
        std::jthread thread = std::jthread([](const std::stop_token&) {
            //default empty thread
        });


    public:
        ParallelRenderState() = default;

        ~ParallelRenderState();

        template<typename T> requires std::is_invocable_r_v<void, T>
        void createThread(T &&func) {
            std::scoped_lock lock(mutex);

            cancelUnsafe();
            auto startGate = std::make_shared<std::latch>(1);
            thread = std::jthread([f = std::forward<T>(func), startGate]() mutable {
                // The worker must not call back into ParallelRenderState until
                // the jthread (and therefore its stop token) is fully assigned.
                startGate->wait();
                f();
            });
            startGate->count_down();
        }

        [[nodiscard]] std::stop_token stopToken() const;

        [[nodiscard]] bool interruptRequested() const;

        void cancel();

        void interrupt();

        void pause();

        void resume();

        [[nodiscard]] bool isPaused() const { return paused.load(); }

    private:
        void cancelUnsafe();
    };


}
