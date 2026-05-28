#include "text_processor.h"
#include <cctype>

const size_t TextProcessor::MIN_WORD_LENGTH;
const size_t TextProcessor::MAX_WORD_LENGTH;

std::vector<std::string> TextProcessor::tokenize(const std::string& text)
{
    std::vector<std::string> words;
    std::string current;

    for (char c : text) {
        char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (std::isalpha(static_cast<unsigned char>(lower))) {
            current += lower;
        }
        else {
            if (!current.empty()) {
                if (current.length() >= MIN_WORD_LENGTH && current.length() <= MAX_WORD_LENGTH) {
                    words.push_back(current);
                }
                current.clear();
            }
        }
    }

    if (!current.empty()) {
        if (current.length() >= MIN_WORD_LENGTH && current.length() <= MAX_WORD_LENGTH) {
            words.push_back(current);
        }
    }

    return words;
}

std::unordered_map<std::string, int> TextProcessor::count_frequencies(
    const std::vector<std::string>& words)
{
    std::unordered_map<std::string, int> freq;
    for (const auto& word : words) {
        ++freq[word];
    }
    return freq;
}