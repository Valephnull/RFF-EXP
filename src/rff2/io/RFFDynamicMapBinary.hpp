//
// Created by Merutilm on 2025-05-08.
//

#pragma once
#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

#include "../calc/iteration_types.hpp"
#include "RFFBinary.hpp"
#include "RFFMapBinary.hpp"

namespace merutilm::rff2 {
    struct RFFDynamicMapBinary final : RFFMapBinary{

        itt::ptb period;
        itt::ptb maxIteration;
        std::vector<double> iterations;
        uint16_t width;
        uint16_t height;
        static const RFFDynamicMapBinary DEFAULT;

        RFFDynamicMapBinary(double logZoom, itt::ptb period, itt::ptb maxIteration, std::vector<double> iterations,
                            uint16_t width, uint16_t height);

        [[nodiscard]] static RFFDynamicMapBinary read(std::ifstream &in);

        [[nodiscard]] static RFFDynamicMapBinary read(const std::filesystem::path &path);

        [[nodiscard]] static RFFDynamicMapBinary decode(std::span<const std::byte> bytes);

        [[nodiscard]] std::vector<std::byte> encode() const;

        [[nodiscard]] bool hasData() const;

        [[nodiscard]] bool hasValidIterations() const;

        [[nodiscard]] double getLogZoom() const { return logZoom; }

        [[nodiscard]] static RFFDynamicMapBinary readByID(const std::filesystem::path &dir, uint32_t id);

        void exportAsKeyframe(const std::filesystem::path &dir) const;

        void exportAsKeyframe(const std::filesystem::path &dir, uint32_t id) const;

        void exportFile(const std::filesystem::path &path) const;

        void write(std::ofstream &out) const;
    };

}
