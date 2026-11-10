//
// Created by Merutilm on 2025-06-25.
//

#pragma once

#include <string>

#include "../calc/iteration_types.hpp"
#include "RFFBinary.hpp"

namespace merutilm::rff2 {

    struct RFFLocationBinary final : RFFBinary{
        double logZoom;
        std::string real;
        std::string imag;
        itt::ptb maxIteration;

        static const RFFLocationBinary DEFAULT;

        explicit RFFLocationBinary(double logZoom, std::string real, std::string imag, itt::ptb maxIteration);

        [[nodiscard]] static RFFLocationBinary read(std::ifstream &in);

        [[nodiscard]] static RFFLocationBinary read(const std::filesystem::path &path);

        [[nodiscard]] bool hasData() const;

        [[nodiscard]] double getLogZoom() const { return logZoom; }

        [[nodiscard]] const std::string &getReal() const { return real; }

        [[nodiscard]] const std::string &getImag() const { return imag; }

        [[nodiscard]] itt::ptb getMaxIteration() const { return maxIteration; }

        void exportAsKeyframe(const std::filesystem::path &dir) const;

        void exportAsKeyframe(const std::filesystem::path &dir, uint32_t id) const;

        void exportFile(const std::filesystem::path &path) const;

        void write(std::ofstream &out) const;
    };
}
