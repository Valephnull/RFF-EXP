//
// Created by Merutilm on 2025-05-09.
//

#include "ParallelRenderState.h"

#include <future>
#include <mutex>


namespace merutilm::rff2 {
    ParallelRenderState::~ParallelRenderState() {
        cancel();
    }

    std::stop_token ParallelRenderState::stopToken() const {
        return thread.get_stop_token();
    }

    bool ParallelRenderState::interruptRequested() const {
        std::unique_lock lock(pauseMutex);
        pauseCondition.wait(lock, [this] {
            return !paused.load() || thread.get_stop_token().stop_requested();
        });
        return thread.get_stop_token().stop_requested();
    }

    void ParallelRenderState::interrupt() {
        thread.request_stop();
        pauseCondition.notify_all();
    }

    void ParallelRenderState::pause() {
        paused = true;
    }

    void ParallelRenderState::resume() {
        paused = false;
        pauseCondition.notify_all();
    }

    void ParallelRenderState::cancel() {
        std::scoped_lock lock(mutex);
        cancelUnsafe();
    }

    void ParallelRenderState::cancelUnsafe() {
        if (thread.joinable()) {
            interrupt();
            thread.join();
        }
    }
}
