//
// Created by Merutilm on 2025-05-08.
//

#include "RFFDynamicMapBinary.hpp"

#include <bit>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>

#include "../app/IOUtilities.h"
#include "../constants/Constants.hpp"
#include "vulkan_helper/base/logger.hpp"
#include "vulkan_helper/util/BufferImageUtils.hpp"

namespace merutilm::rff2 {

    namespace {
        template<typename T>
        void appendValue(std::vector<std::byte> &bytes, const T &value) {
            const size_t offset = bytes.size();
            bytes.resize(offset + sizeof(T));
            std::memcpy(bytes.data() + offset, &value, sizeof(T));
        }

        template<typename T>
        bool readValue(const std::span<const std::byte> bytes, size_t &offset, T &value) {
            if (offset > bytes.size() || bytes.size() - offset < sizeof(T))
                return false;
            std::memcpy(&value, bytes.data() + offset, sizeof(T));
            offset += sizeof(T);
            return true;
        }

        bool isFiniteDouble(const double value) {
            constexpr uint64_t EXPONENT = uint64_t{0x7ff} << 52;
            return (std::bit_cast<uint64_t>(value) & EXPONENT) != EXPONENT;
        }

        bool isFiniteFloat(const float value) {
            constexpr uint32_t EXPONENT = uint32_t{0xff} << 23;
            return (std::bit_cast<uint32_t>(value) & EXPONENT) != EXPONENT;
        }
    }

    inline const RFFDynamicMapBinary RFFDynamicMapBinary::DEFAULT =
            RFFDynamicMapBinary(0, 0, 0, std::vector<double>(), 0, 0);

    RFFDynamicMapBinary::RFFDynamicMapBinary(const double logZoom, const uint64_t period, const uint64_t maxIteration,
                                             std::vector<double> iterations, const uint16_t width,
                                             const uint16_t height) :
        RFFMapBinary(logZoom), period(period), maxIteration(maxIteration), iterations(std::move(iterations)),
        width(width), height(height) {
        static_assert(RFFBinaryRequirements<RFFDynamicMapBinary>);
    }


    RFFDynamicMapBinary RFFDynamicMapBinary::read(std::ifstream &in) {

        uint32_t wh = 0;
        const uint32_t version = readVersion(in, reinterpret_cast<std::byte *>(&wh));
        uint16_t w;
        uint16_t h;
        double lz;
        if (version == 0) {
            w = static_cast<uint16_t>(wh & 0xffffU);
            h = static_cast<uint16_t>((wh >> 16u) & 0xffffU);
            float z;
            IOUtilities::readAndDecode(in, &z);
            lz = z;
        } else {
            IOUtilities::readAndDecode(in, &w);
            IOUtilities::readAndDecode(in, &h);
            IOUtilities::readAndDecode(in, &lz);
        }

        uint64_t p;
        IOUtilities::readAndDecode(in, &p);
        uint64_t m;
        IOUtilities::readAndDecode(in, &m);
        auto i = std::vector<double>(w * h);
        IOUtilities::readAndDecode(in, &i);
        return RFFDynamicMapBinary{lz, p, m, i, w, h};
    }

    RFFDynamicMapBinary RFFDynamicMapBinary::read(const std::filesystem::path &path) {
        std::ifstream in(path, std::ios::in | std::ios::binary | std::ios::ate);
        if (!in.is_open())
            return DEFAULT;
        const std::streamoff length = in.tellg();
        if (length <= 0 || static_cast<uint64_t>(length) > std::numeric_limits<size_t>::max())
            return DEFAULT;
        in.seekg(0, std::ios::beg);
        std::vector<std::byte> bytes(static_cast<size_t>(length));
        in.read(reinterpret_cast<char *>(bytes.data()), length);
        return in ? decode(bytes) : DEFAULT;
    }

    RFFDynamicMapBinary RFFDynamicMapBinary::decode(const std::span<const std::byte> bytes) {
        size_t offset = 0;
        uint32_t versionOrDimensions = 0;
        if (!readValue(bytes, offset, versionOrDimensions))
            return DEFAULT;

        uint16_t w = 0;
        uint16_t h = 0;
        double lz = 0;
        if (versionOrDimensions > (1u << 23u)) {
            w = static_cast<uint16_t>(versionOrDimensions & 0xffffu);
            h = static_cast<uint16_t>(versionOrDimensions >> 16u);
            float oldLogZoom = 0;
            if (!readValue(bytes, offset, oldLogZoom) || !isFiniteFloat(oldLogZoom))
                return DEFAULT;
            lz = oldLogZoom;
        } else {
            if (versionOrDimensions > RFFBinary::VERSION ||
                !readValue(bytes, offset, w) || !readValue(bytes, offset, h) ||
                !readValue(bytes, offset, lz) || !isFiniteDouble(lz))
                return DEFAULT;
        }

        uint64_t p = 0;
        uint64_t m = 0;
        if (!readValue(bytes, offset, p) || !readValue(bytes, offset, m) || w == 0 || h == 0 || m == 0)
            return DEFAULT;
        const size_t elementCount = static_cast<size_t>(w) * h;
        if (offset > bytes.size() || bytes.size() - offset != elementCount * sizeof(double))
            return DEFAULT;
        std::vector<double> values(elementCount);
        std::memcpy(values.data(), bytes.data() + offset, elementCount * sizeof(double));
        return RFFDynamicMapBinary(lz, p, m, std::move(values), w, h);
    }

    std::vector<std::byte> RFFDynamicMapBinary::encode() const {
        if (!hasData())
            return {};
        std::vector<std::byte> bytes;
        bytes.reserve(sizeof(uint32_t) + sizeof(width) + sizeof(height) + sizeof(logZoom) +
                      sizeof(period) + sizeof(maxIteration) + iterations.size() * sizeof(double));
        appendValue(bytes, RFFBinary::VERSION);
        appendValue(bytes, width);
        appendValue(bytes, height);
        appendValue(bytes, logZoom);
        appendValue(bytes, period);
        appendValue(bytes, maxIteration);
        const size_t offset = bytes.size();
        bytes.resize(offset + iterations.size() * sizeof(double));
        std::memcpy(bytes.data() + offset, iterations.data(), iterations.size() * sizeof(double));
        return bytes;
    }

    bool RFFDynamicMapBinary::hasData() const {
        return width > 0 && height > 0 && maxIteration > 0 &&
               iterations.size() == static_cast<size_t>(width) * height;
    }

    bool RFFDynamicMapBinary::hasValidIterations() const {
        if (!hasData())
            return false;
        constexpr uint64_t SIGN = uint64_t{1} << 63;
        constexpr uint64_t EXPONENT = uint64_t{0x7ff} << 52;
        for (const double iteration: iterations) {
            const uint64_t bits = std::bit_cast<uint64_t>(iteration);
            if ((bits & SIGN) != 0 || (bits & EXPONENT) == EXPONENT || (bits & ~SIGN) == 0 ||
                iteration > static_cast<double>(maxIteration))
                return false;
        }
        return true;
    }

    RFFDynamicMapBinary RFFDynamicMapBinary::readByID(const std::filesystem::path &dir, const uint32_t id) {
        return importFile<RFFDynamicMapBinary>(dir / IOUtilities::fileNameFormat(id, Constants::File::EXT_DYNAMIC_MAP));
    }


    void RFFDynamicMapBinary::exportAsKeyframe(const std::filesystem::path &dir) const {
        RFFBinary::exportFile(*this, IOUtilities::generateFilename(dir, Constants::File::EXT_DYNAMIC_MAP, nullptr));
    }

    void RFFDynamicMapBinary::exportAsKeyframe(const std::filesystem::path &dir, const uint32_t id) const {
        exportFile(dir / IOUtilities::fileNameFormat(id, Constants::File::EXT_DYNAMIC_MAP));
    }

    void RFFDynamicMapBinary::exportFile(const std::filesystem::path &path) const {
        const std::vector<std::byte> bytes = encode();
        if (bytes.empty())
            return;
        std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            vkh::logger::log_err("ERROR : Cannot save file");
            return;
        }
        out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    void RFFDynamicMapBinary::write(std::ofstream &out) const {
        IOUtilities::encodeAndWrite(out, width);
        IOUtilities::encodeAndWrite(out, height);
        IOUtilities::encodeAndWrite(out, logZoom);
        IOUtilities::encodeAndWrite(out, period);
        IOUtilities::encodeAndWrite(out, maxIteration);
        IOUtilities::encodeAndWrite(out, iterations);
    }

} // namespace merutilm::rff2
