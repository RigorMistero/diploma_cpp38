#include "file_scanner.h"
#include <windows.h>
#include <algorithm>
#include <iostream>

std::vector<std::string> FileScanner::scan_directory(
    const std::string& directory,
    const std::vector<std::string>& extensions)
{
    std::vector<std::string> files;

    WIN32_FIND_DATAA find_data;
    std::string search_path = directory + "\\*";

    // Checking directory exists
    DWORD attrs = GetFileAttributesA(directory.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        std::cerr << "[FileScanner] ERROR: directory '"
            << directory << "' doesn't exist or is not a folder.\n";
        return files;
    }

    // recursive traversal of stack
    std::vector<std::string> dirs_to_scan;
    dirs_to_scan.push_back(directory);

    while (!dirs_to_scan.empty()) {
        std::string current_dir = dirs_to_scan.back();
        dirs_to_scan.pop_back();

        std::string current_search = current_dir + "\\*";
        HANDLE find_handle = FindFirstFileA(current_search.c_str(), &find_data);

        if (find_handle == INVALID_HANDLE_VALUE) {
            continue;
        }

        do {
            std::string name = find_data.cFileName;

            // missing "." ".."
            if (name == "." || name == "..") {
                continue;
            }

            std::string full_path = current_dir + "\\" + name;

            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                // add is_folder to stack for recursion
                dirs_to_scan.push_back(full_path);
            }
            else {
                // Check file extension  
                if (extensions.empty() || has_valid_extension(name, extensions)) {
                    files.push_back(full_path);
                }
            }

        } while (FindNextFileA(find_handle, &find_data) != 0);

        FindClose(find_handle);
    }

    std::cout << "[FileScanner] Files found: " << files.size() << '\n';
    return files;
}

bool FileScanner::has_valid_extension(const std::string& file_name,
    const std::vector<std::string>& extensions)
{
    // endpoint 
    size_t dot_pos = file_name.rfind('.');
    if (dot_pos == std::string::npos) {
        return false;
    }

    std::string ext = file_name.substr(dot_pos + 1);

    // to lower register
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (const auto& valid_ext : extensions) {
        std::string lower_ext = valid_ext;
        std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
        if (ext == lower_ext) {
            return true;
        }
    }

    return false;
}