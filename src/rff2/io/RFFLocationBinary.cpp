//
// Created by Merutilm on 2025-06-25.
//

#include "RFFLocationBinary.hpp"

#include <bit>
#include <cmath>
#include <utility>

#include "../app/IOUtilities.h"
#include "../constants/FileConstants.hpp"
#include "vulkan_helper/base/logger.hpp"

namespace merutilm::rff2 {
    namespace {
        constexpr uint64_t MAX_COORDINATE_TEXT_LENGTH = 1u << 20;
    }
    inline const RFFLocationBinary RFFLocationBinary::DEFAULT = RFFLocationBinary(0, "", "", 0);

    RFFLocationBinary::RFFLocationBinary(const double logZoom, std::string real, std::string imag,
                                         const itt::ptb maxIteration) :
        logZoom(logZoom), real(std::move(real)), imag(std::move(imag)), maxIteration(maxIteration) {
        static_assert(RFFBinaryRequirements<RFFLocationBinary>);
    }


    RFFLocationBinary RFFLocationBinary::read(std::ifstream &in) {
        float v = 0;
        const uint32_t version = readVersion(in, reinterpret_cast<std::byte *>(&v));

        double lz;
        if (!in || version > RFFBinary::VERSION)
            return DEFAULT;
        if (version == 0) {
            lz = v;
        } else {
            IOUtilities::readAndDecode(in, &lz);
        }

        const auto counterPosition = in.tellg();
        const auto readPayload = [&](const bool sized) {
            itt::ptb max = 0;
            uint64_t first = 0;
            IOUtilities::readAndDecode(in, &first);
            if (sized) {
                if (!in || first == 0 || first > sizeof(max))
                    return DEFAULT;
                in.read(reinterpret_cast<char *>(&max), static_cast<std::streamsize>(first));
            } else {
                max = first;
            }
            uint64_t len = 0;
            IOUtilities::readAndDecode(in, &len);
            if (!in || !std::isfinite(lz) || max == 0 || len == 0 || len > MAX_COORDINATE_TEXT_LENGTH)
                return DEFAULT;
            std::string re(len, '\0');
            IOUtilities::readAndDecode(in, len, re.data());
            IOUtilities::readAndDecode(in, &len);
            if (!in || len == 0 || len > MAX_COORDINATE_TEXT_LENGTH)
                return DEFAULT;
            std::string im(len, '\0');
            IOUtilities::readAndDecode(in, len, im.data());
            if (!in)
                return DEFAULT;
            return RFFLocationBinary{lz, std::move(re), std::move(im), max};
        };
        if (version >= 5) {
            auto result = readPayload(true);
            if (result.hasData() || version >= 6)
                return result;
            // RFF-EXP v5 predates upstream's sized counters.
            in.clear();
            in.seekg(counterPosition);
        }
        return readPayload(false);
    }


    RFFLocationBinary RFFLocationBinary::read(const std::filesystem::path &path) {
        return RFFBinary::importFile<RFFLocationBinary>(path);
    }

    bool RFFLocationBinary::hasData() const {
        return !real.empty() && !imag.empty() && maxIteration > 0 && std::isfinite(logZoom);
    }

    void RFFLocationBinary::exportAsKeyframe(const std::filesystem::path &dir) const {
        RFFBinary::exportFile(*this, IOUtilities::generateFilename(dir, Constants::File::EXT_LOCATION, nullptr));
    }

    void RFFLocationBinary::exportAsKeyframe(const std::filesystem::path &dir, const uint32_t id) const {
        exportFile(dir / IOUtilities::fileNameFormat(id, Constants::File::EXT_LOCATION));
    }

    void RFFLocationBinary::exportFile(const std::filesystem::path &path) const {
        RFFBinary::exportFile(*this, path);
    }


    void RFFLocationBinary::write(std::ofstream &out) const {
        uint64_t len = 0;
        IOUtilities::encodeAndWrite(out, logZoom);
        IOUtilities::encodeAndWrite(out, sizeof(maxIteration));
        IOUtilities::encodeAndWrite(out, maxIteration);
        len = real.length();
        IOUtilities::encodeAndWrite(out, len);
        IOUtilities::encodeAndWrite(out, real.data(), real.length());
        len = imag.length();
        IOUtilities::encodeAndWrite(out, len);
        IOUtilities::encodeAndWrite(out, imag.data(), imag.length());
    }


} // namespace merutilm::rff2
