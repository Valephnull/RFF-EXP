//
// Created by Merutilm on 7/15/26.
//

#pragma once
#include <mutex>
#include <string>
namespace merutilm::rff2 {
    struct VideoProgressInfo {
        std::mutex mutex;
        double ratio = 0;
        std::string remainedTimeStr;
        bool active = false;
    };
}
