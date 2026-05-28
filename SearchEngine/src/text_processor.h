#pragma once

#include <string>
#include <vector>
#include <unordered_map>

class TextProcessor {
public:
    static std::vector<std::string> tokenize(const std::string& text);
    static std::unordered_map<std::string, int> count_frequencies(
        const std::vector<std::string>& words);

    static const size_t MIN_WORD_LENGTH = 3;
    static const size_t MAX_WORD_LENGTH = 32;
};