#include "GradientLoader.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <utility>
#include <glm/common.hpp>

namespace merutilm::rff2 {
    namespace {
        constexpr int PALETTE_SIZE = 400;
        constexpr size_t MAX_FILE_BYTES = 64 * 1024 * 1024;
        constexpr size_t MAX_GRADIENTS = 16384;

        enum class Kind { END, WORD, STRING, OPEN, CLOSE, COLON, EQUALS };
        struct Token {
            Kind kind = Kind::END;
            std::string text;
        };

        class Lexer {
            std::string_view input;
            size_t offset = 0;
            size_t line = 1;

        public:
            explicit Lexer(std::string_view input) : input(input) {
                if (input.starts_with("\xef\xbb\xbf"))
                    offset = 3;
            }

            [[noreturn]] void fail(const std::string &message) const {
                throw std::runtime_error("Line " + std::to_string(line) + ": " + message);
            }

            Token next() {
                while (offset < input.size()) {
                    const char c = input[offset];
                    if (std::isspace(static_cast<unsigned char>(c)) || c == '\\') {
                        if (c == '\n') ++line;
                        ++offset;
                    } else if (c == ';' || (c == '/' && offset + 1 < input.size() && input[offset + 1] == '/')) {
                        while (offset < input.size() && input[offset] != '\n') ++offset;
                    } else {
                        break;
                    }
                }
                if (offset == input.size()) return {};
                const char c = input[offset++];
                switch (c) {
                    case '{': return {Kind::OPEN, {}};
                    case '}': return {Kind::CLOSE, {}};
                    case ':': return {Kind::COLON, {}};
                    case '=': return {Kind::EQUALS, {}};
                    case '"': {
                        std::string value;
                        while (offset < input.size()) {
                            char next = input[offset++];
                            if (next == '"') return {Kind::STRING, std::move(value)};
                            if (next == '\\' && offset < input.size()) {
                                next = input[offset++];
                                if (next == '\n') { ++line; continue; }
                                if (next == '\r') continue;
                            }
                            if (next == '\n') ++line;
                            value.push_back(next);
                        }
                        fail("Unterminated quoted title.");
                    }
                    default: {
                        const size_t begin = offset - 1;
                        while (offset < input.size()) {
                            const char next = input[offset];
                            if (std::isspace(static_cast<unsigned char>(next)) || next == '{' || next == '}' ||
                                next == ':' || next == '=' || next == ';' || next == '"' || next == '\\')
                                break;
                            ++offset;
                        }
                        return {Kind::WORD, std::string(input.substr(begin, offset - begin))};
                    }
                }
            }
        };

        template<typename T>
        T integer(const Token &token, Lexer &lexer) {
            if (token.kind != Kind::WORD) lexer.fail("Expected an integer.");
            std::string_view value = token.text;
            int base = 10;
            if (value.starts_with('+')) value.remove_prefix(1);
            if (value.starts_with("0x") || value.starts_with("0X")) { base = 16; value.remove_prefix(2); }
            T result{};
            const auto [end, status] = std::from_chars(value.data(), value.data() + value.size(), result, base);
            if (value.empty() || status != std::errc{} || end != value.data() + value.size())
                lexer.fail("Invalid integer: " + token.text);
            return result;
        }

        struct Stop { int index; glm::vec4 color; };

        std::vector<glm::vec4> sample(const std::array<std::optional<glm::vec4>, PALETTE_SIZE> &nodes,
                                      const bool smooth, const int rotation) {
            std::vector<Stop> stops;
            for (int i = 0; i < PALETTE_SIZE; ++i) {
                if (nodes[i]) stops.push_back({i, *nodes[i]});
            }
            if (stops.empty()) return {};
            if (stops.size() == 1) return std::vector<glm::vec4>(PALETTE_SIZE, stops.front().color);
            std::vector<glm::vec4> colors(PALETTE_SIZE);
            for (int i = 0; i < PALETTE_SIZE; ++i) {
                const int position = (i - rotation + PALETTE_SIZE) % PALETTE_SIZE;
                const auto upper = std::upper_bound(stops.begin(), stops.end(), position,
                                                    [](int index, const Stop &stop) { return index < stop.index; });
                const size_t right = upper == stops.end() ? 0 : static_cast<size_t>(upper - stops.begin());
                const size_t left = (right + stops.size() - 1) % stops.size();
                const int x0 = stops[left].index > position ? stops[left].index - PALETTE_SIZE : stops[left].index;
                const int x1 = stops[right].index <= position ? stops[right].index + PALETTE_SIZE : stops[right].index;
                const float width = static_cast<float>(x1 - x0);
                const float t = static_cast<float>(position - x0) / width;
                glm::vec4 color = glm::mix(stops[left].color, stops[right].color, t);
                if (smooth) {
                    // Cyclic cubic Hermite interpolation, including unevenly spaced nodes.
                    const size_t before = (left + stops.size() - 1) % stops.size();
                    const size_t after = (right + 1) % stops.size();
                    int xp = stops[before].index;
                    while (xp >= x0) xp -= PALETTE_SIZE;
                    int xn = stops[after].index;
                    while (xn <= x1) xn += PALETTE_SIZE;
                    const glm::vec4 m0 = (stops[right].color - stops[before].color) / static_cast<float>(x1 - xp);
                    const glm::vec4 m1 = (stops[after].color - stops[left].color) / static_cast<float>(xn - x0);
                    const float t2 = t * t;
                    const float t3 = t2 * t;
                    color = (2 * t3 - 3 * t2 + 1) * stops[left].color +
                            (t3 - 2 * t2 + t) * width * m0 +
                            (-2 * t3 + 3 * t2) * stops[right].color + (t3 - t2) * width * m1;
                }
                colors[i] = glm::clamp(color, glm::vec4(0), glm::vec4(1));
            }
            return colors;
        }
    }

    std::vector<ImportedGradient> GradientLoader::load(const std::filesystem::path &path, std::string &error) {
        error.clear();
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        if (!stream) { error = "Cannot open the gradient file."; return {}; }
        const auto length = stream.tellg();
        if (length <= 0 || static_cast<uint64_t>(length) > MAX_FILE_BYTES) {
            error = "The gradient file is empty or exceeds 64 MiB.";
            return {};
        }
        std::string text(static_cast<size_t>(length), '\0');
        stream.seekg(0);
        stream.read(text.data(), static_cast<std::streamsize>(text.size()));
        if (!stream) { error = "Cannot read the complete gradient file."; return {}; }
        return parse(text, error);
    }

    std::vector<ImportedGradient> GradientLoader::parse(const std::string_view text, std::string &error) {
        error.clear();
        if (text.size() > MAX_FILE_BYTES) { error = "The gradient file exceeds 64 MiB."; return {}; }
        try {
            Lexer lexer(text);
            std::vector<ImportedGradient> gradients;
            std::array<std::optional<glm::vec4>, PALETTE_SIZE> nodes{};
            std::string name;
            std::string blockName;
            bool inBlock = false;
            bool inColors = false;
            bool hasGradient = false;
            bool smooth = false;
            int rotation = 0;
            int pendingIndex = -1;
            Token token = lexer.next();
            while (token.kind != Kind::END) {
                if (token.kind == Kind::OPEN) {
                    if (inBlock) lexer.fail("Nested gradient entries are not supported.");
                    inBlock = true;
                    inColors = hasGradient = smooth = false;
                    rotation = 0;
                    pendingIndex = -1;
                    nodes.fill(std::nullopt);
                    name = blockName;
                } else if (token.kind == Kind::CLOSE) {
                    if (!inBlock) lexer.fail("Unexpected closing brace.");
                    if (pendingIndex >= 0) lexer.fail("An indexed stop has no color.");
                    if (hasGradient) {
                        auto colors = sample(nodes, smooth, rotation);
                        if (colors.empty()) lexer.fail("Gradient has no color stops.");
                        if (gradients.size() >= MAX_GRADIENTS) lexer.fail("Too many gradients in one file.");
                        gradients.push_back({name.empty() ? "Untitled" : name, std::move(colors)});
                    }
                    inBlock = false;
                    blockName.clear();
                } else if (!inBlock) {
                    if (token.kind == Kind::WORD || token.kind == Kind::STRING) blockName = token.text;
                } else if (token.kind == Kind::WORD) {
                    const std::string key = token.text;
                    token = lexer.next();
                    if (token.kind == Kind::COLON) {
                        if (pendingIndex >= 0) lexer.fail("An indexed stop has no color.");
                        inColors = key == "gradient";
                        hasGradient |= inColors;
                    } else if (token.kind == Kind::EQUALS) {
                        const Token value = lexer.next();
                        if (value.kind != Kind::WORD && value.kind != Kind::STRING)
                            lexer.fail("Missing value for " + key + ".");
                        if (inColors) {
                            if (key == "title") name = value.text;
                            else if (key == "smooth") {
                                if (value.text != "yes" && value.text != "no") lexer.fail("Expected smooth=yes or smooth=no.");
                                smooth = value.text == "yes";
                            } else if (key == "rotation") {
                                rotation = integer<int>(value, lexer) % PALETTE_SIZE;
                                if (rotation < 0) rotation += PALETTE_SIZE;
                            } else if (key == "index") {
                                if (pendingIndex >= 0) lexer.fail("An indexed stop has no color.");
                                pendingIndex = integer<int>(value, lexer);
                                if (pendingIndex < 0 || pendingIndex >= PALETTE_SIZE)
                                    lexer.fail("Gradient indices must be between 0 and 399.");
                            } else if (key == "color") {
                                if (pendingIndex < 0) lexer.fail("A color has no index.");
                                const uint32_t packed = integer<uint32_t>(value, lexer);
                                if (packed > 0xffffff) lexer.fail("Color must be a 24-bit RGB value.");
                                // Gradient files use Windows COLORREF order: 0xBBGGRR.
                                nodes[pendingIndex] = glm::vec4(static_cast<float>(packed & 255) / 255.0f,
                                                               static_cast<float>((packed >> 8) & 255) / 255.0f,
                                                               static_cast<float>((packed >> 16) & 255) / 255.0f, 1);
                                pendingIndex = -1;
                            }
                        }
                    } else {
                        continue; // Reconsider a brace or section instead of losing it.
                    }
                }
                token = lexer.next();
            }
            if (inBlock) lexer.fail("Unterminated gradient entry.");
            if (gradients.empty()) lexer.fail("No color gradients found in this file.");
            return gradients;
        } catch (const std::exception &exception) {
            error = exception.what();
            return {};
        }
    }
}
