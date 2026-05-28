#pragma once

#include <string>
#include <vector>

class FileScanner {
public:
    static std::vector<std::string> scan_directory(
        const std::string& directory,
        const std::vector<std::string>& extensions);

private:
    static bool has_valid_extension(const std::string& file_path,
        const std::vector<std::string>& extensions);
};