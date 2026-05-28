#include "config_reader.h"
#include "db_manager.h"
#include "file_scanner.h"
#include "text_processor.h"
#include "thread_pool.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <pqxx/pqxx>

struct FileTask 
{
    std::string file_path;
    std::string file_name;
};

struct FileResult 
{
    std::string file_path;
    std::string file_name;
    std::unordered_map<std::string, int> frequencies;
    bool success = false;
    std::string error_msg;
};

// Reading and tokenizing
FileResult process_file(const FileTask& task)
{
    FileResult result;
    result.file_path = task.file_path;
    result.file_name = task.file_name;

    try 
    {
        std::ifstream file(task.file_path);
        if (!file.is_open()) 
        {
            result.error_msg = "Cannot open file";
            return result;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();

        auto words = TextProcessor::tokenize(content);
        result.frequencies = TextProcessor::count_frequencies(words);
        result.success = true;

    }
    catch (const std::exception& e) 
    {
        result.error_msg = e.what();
    }

    return result;
}

// single-threaded database write for no deadlocks
void write_to_database(DbManager& db_manager, const std::vector<FileResult>& results,
    size_t& files_processed, size_t& files_failed)
{
    auto conn = db_manager.create_connection();
    pqxx::work txn(*conn);

    for (const auto& res : results) 
    {
        if (!res.success || res.frequencies.empty()) 
        {
            if (!res.success) 
            {
                std::cerr << "[Indexer] File processing ERROR "
                    << res.file_path << ": " << res.error_msg << '\n';
                ++files_failed;
            }
            else 
            {
                ++files_processed;
            }
            continue;
        }

        try 
        {
            pqxx::result doc_result = txn.exec_params(
                "INSERT INTO documents (file_path, file_name) VALUES ($1, $2) "
                "ON CONFLICT (file_path) DO UPDATE SET file_name = $2, indexed_at = NOW() "
                "RETURNING id",
                res.file_path, res.file_name);

            std::string doc_id_str = doc_result[0][0].c_str();
            long doc_id = std::stol(doc_id_str);

            for (const auto& pair : res.frequencies) 
            {
                const std::string& word = pair.first;
                long freq = pair.second;

                pqxx::result word_result = txn.exec_params(
                    "INSERT INTO words (word) VALUES ($1) "
                    "ON CONFLICT (word) DO UPDATE SET word = EXCLUDED.word "
                    "RETURNING id",
                    word);

                std::string word_id_str = word_result[0][0].c_str();
                long word_id = std::stol(word_id_str);

                txn.exec_params(
                    "INSERT INTO word_frequencies (document_id, word_id, frequency) "
                    "VALUES ($1, $2, $3) "
                    "ON CONFLICT (document_id, word_id) DO UPDATE SET frequency = $3",
                    doc_id, word_id, freq);
            }
            ++files_processed;
        }
        catch (const std::exception& e) 
        {
            std::cerr << "[Indexer] File processing ERROR "
                << res.file_path << ": " << e.what() << '\n';
            ++files_failed;
        }
    }

    txn.commit();
    std::cout << "[Indexer] All data written to database.\n";
}

int main(int argc, char** argv)
{
    try 
    {
        std::string config_path = "config.ini";
        if (argc > 1) 
        {
            config_path = argv[1];
        }

        std::cout << "[Indexer] Reading config: " << config_path << '\n';
        ConfigReader config(config_path);

        auto db_cfg = config.get_database_config();
        auto idx_cfg = config.get_indexer_config();

        if (idx_cfg.start_dir.empty()) 
        {
            std::cerr << "[Indexer] Error: no directory for indexation.\n";
            return 1;
        }

        std::cout << "[Indexer] Connection to the database...\n";
        auto db_ptr = std::make_shared<DbManager>(db_cfg.connection_string());
        db_ptr->initialize_tables();

        std::cout << "[Indexer] Scanning directory: " << idx_cfg.start_dir << '\n';
        auto files = FileScanner::scan_directory(idx_cfg.start_dir, idx_cfg.extensions);

        if (files.empty()) 
        {
            std::cout << "[Indexer] No files for indexation found.\n";
            return 0;
        }

        // step 1. multithread reading and tokenizing 
        size_t thread_count = idx_cfg.thread_pool_size;
        if (thread_count == 0) 
        {
            thread_count = std::thread::hardware_concurrency();
            if (thread_count == 0) thread_count = 4;
        }

        std::cout << "[Indexer] Starting thread pool: " << thread_count << " threads\n";
        ThreadPool pool(thread_count);

        std::vector<FileResult> all_results(files.size());
        std::mutex results_mutex;

        for (size_t i = 0; i < files.size(); ++i) 
        {
            FileTask task;
            task.file_path = files[i];

            size_t slash_pos = files[i].rfind('\\');
            if (slash_pos != std::string::npos) 
            {
                task.file_name = files[i].substr(slash_pos + 1);
            }
            else 
            {
                task.file_name = files[i];
            }

            pool.enqueue([i, task, &all_results, &results_mutex]() 
                {
                FileResult result = process_file(task);
                std::lock_guard<std::mutex> lock(results_mutex);
                all_results[i] = std::move(result);
                });
        }

        std::cout << "[Indexer] Processing files (reading + tokenizing)...\n";
        pool.wait_all();

        // step 2, single-thread write to DB
        std::cout << "[Indexer] Writing results to database...\n";
        size_t files_processed = 0;
        size_t files_failed = 0;
        write_to_database(*db_ptr, all_results, files_processed, files_failed);

        std::cout << "\n[Indexer] ========== Indexation over ==========\n";
        std::cout << "[Indexer] Files processed: " << files_processed << '\n';
        std::cout << "[Indexer] Errors: " << files_failed << '\n';

    }
    catch (const std::exception& e) 
    {
        std::cerr << "[Indexer] CRITICAL ERROR: " << e.what() << '\n';
        return 1;
    }
    return 0;
}