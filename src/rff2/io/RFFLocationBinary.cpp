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
                                         const uint64_t maxIteration) :
        logZoom(logZoom), real(std::move(real)), imag(std::move(imag)), maxIteration(maxIteration) {
        static_assert(RFFBinaryRequirements<RFFLocationBinary>);
    }


    RFFLocationBinary RFFLocationBinary::read(std::ifstream &in) {
        float v;
        const uint32_t version = readVersion(in, reinterpret_cast<std::byte *>(&v));

        double lz;
        if (!in || version > RFFBinary::VERSION)
            return DEFAULT;
        if (version == 0) {
            lz = v;
        } else {
            IOUtilities::readAndDecode(in, &lz);
        }

        uint64_t max;
        IOUtilities::readAndDecode(in, &max);
        uint64_t len;
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
        IOUtilities::encodeAndWrite(out, maxIteration);
        len = real.length();
        IOUtilities::encodeAndWrite(out, len);
        IOUtilities::encodeAndWrite(out, real.data(), real.length());
        len = imag.length();
        IOUtilities::encodeAndWrite(out, len);
        IOUtilities::encodeAndWrite(out, imag.data(), imag.length());
    }


} // namespace merutilm::rff2
