#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <future>
#include <thread>

#include "rff2/parallel/ParallelRenderState.h"

using namespace std::chrono_literals;
using merutilm::rff2::ParallelRenderState;

namespace {
    bool waitUntil(const std::function<bool()> &predicate, const std::chrono::milliseconds timeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (predicate())
                return true;
            std::this_thread::sleep_for(1ms);
        }
        return predicate();
    }
}

int main() {
    ParallelRenderState state;
    std::atomic<uint32_t> checkpoints = 0;

    state.createThread([&] {
        while (!state.interruptRequested()) {
            ++checkpoints;
            std::this_thread::sleep_for(1ms);
        }
    });

    if (!waitUntil([&] { return checkpoints.load() >= 5; }, 1s))
        return EXIT_FAILURE;

    state.pause();
    std::this_thread::sleep_for(20ms);
    const uint32_t pausedAt = checkpoints.load();
    std::this_thread::sleep_for(40ms);
    if (!state.isPaused() || checkpoints.load() != pausedAt)
        return EXIT_FAILURE;

    state.resume();
    if (!waitUntil([&] { return checkpoints.load() > pausedAt; }, 1s))
        return EXIT_FAILURE;

    state.pause();
    auto cancel = std::async(std::launch::async, [&] { state.cancel(); });
    if (cancel.wait_for(1s) != std::future_status::ready)
        return EXIT_FAILURE;
    cancel.get();
    state.resume();
    return EXIT_SUCCESS;
}
