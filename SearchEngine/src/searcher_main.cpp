
#include "config_reader.h"
#include "db_manager.h"
#include "text_processor.h"

#include <iostream>
#include <vector>
#include <string>
#include <libpq-fe.h>
#include <cstdlib>

struct SearchResult 
{
    std::string file_name;
    std::string file_path;
    int relevance = 0;
};

static void exit_nicely(PGconn* conn) 
{
    PQfinish(conn);
}

std::vector<SearchResult> search_documents(
    PGconn* conn,
    const std::vector<std::string>& words,
    int max_results)
{
    std::vector<SearchResult> results;
    if (words.empty()) return results;

    // array for для ANY($1)
    std::string word_array = "{";
    for (size_t i = 0; i < words.size(); ++i) 
    {
        if (i > 0) word_array += ",";
        word_array += "\"" + words[i] + "\"";
    }
    word_array += "}";

    // query params
    const char* paramValues[3];
    paramValues[0] = word_array.c_str();

    std::string count_str = std::to_string(words.size());
    paramValues[1] = count_str.c_str();

    std::string limit_str = std::to_string(max_results);
    paramValues[2] = limit_str.c_str();

    const char* query = R"(
        SELECT d.file_name, d.file_path, SUM(wf.frequency) AS relevance
        FROM word_frequencies wf
        JOIN documents d ON wf.document_id = d.id
        JOIN words w ON wf.word_id = w.id
        WHERE w.word = ANY($1::text[])
        GROUP BY d.id, d.file_name, d.file_path
        HAVING COUNT(DISTINCT w.word) = $2::int
        ORDER BY relevance DESC
        LIMIT $3::int
    )";

    PGresult* res = PQexecParams(conn, query, 3, nullptr, paramValues, nullptr, nullptr, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "[Searcher] Query failed: " << PQerrorMessage(conn) << '\n';
        PQclear(res);
        return results;
    }

    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) 
    {
        SearchResult sr;
        sr.file_name = PQgetvalue(res, i, 0);
        sr.file_path = PQgetvalue(res, i, 1);
        sr.relevance = std::atoi(PQgetvalue(res, i, 2));
        results.push_back(sr);
    }

    PQclear(res);
    return results;
}

std::vector<std::string> parse_query(const std::string& input, int max_words) 
{
    if (input.empty()) return {};
    auto words = TextProcessor::tokenize(input);
    if (static_cast<int>(words.size()) > max_words) 
    {
        words.resize(max_words);
    }
    return words;
}

void print_results(const std::vector<SearchResult>& results) 
{
    if (results.empty()) {
        std::cout << "\n> NONE found. Try to change prompt.\n";
        return;
    }

    std::cout << "\n> Results found: " << results.size() << "\n";
    std::cout << "---------------------------------------\n";

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        std::cout << i + 1 << ". " << r.file_name << "\n";
        std::cout << "   Relevance: " << r.relevance << "\n";
        std::cout << "   Path: " << r.file_path << "\n\n";
    }
    std::cout << "---------------------------------------\n";
}

void print_help() {
    std::cout << "\nCommands:\n";
    std::cout << "  <user enters: word or words>    - search (up to 4 words separated by space)\n";
    std::cout << "  :help      - show this help\n";
    std::cout << "  :exit      - exit Searcher\n\n";
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

        std::cout << "[Searcher] Reading configuration: " << config_path << '\n';
        ConfigReader config(config_path);

        auto db_cfg = config.get_database_config();

        std::cout << "[Searcher] Connecting to database...\n";

        //Forming a connection string for libpq
        std::string conn_str = db_cfg.connection_string();
        PGconn* conn = PQconnectdb(conn_str.c_str());

        if (PQstatus(conn) != CONNECTION_OK) 
        {
            std::cerr << "[Searcher] Connection failed: " << PQerrorMessage(conn) << '\n';
            PQfinish(conn);
            return 1;
        }

        // client encoding setting
        PQsetClientEncoding(conn, "UTF8");

        std::cout << "\n";
        std::cout << "=============================\n";
        std::cout << "           Search Engine v1.0\n";
        std::cout << "=============================\n";
        print_help();

        auto search_cfg = config.get_searcher_config();

        std::string input;
        while (true) 
        {
            std::cout << "Searching> ";
            std::getline(std::cin, input);

            if (input.empty()) continue;

            if (input == ":exit" || input == ":quit") 
            {
                std::cout << "[Searcher] Shutdown.\n";
                break;
            }

            if (input == ":help") 
            {
                print_help();
                continue;
            }

            auto words = parse_query(input, search_cfg.max_query_words);

            if (words.empty()) {
                std::cout << "\n> Empty prompt or unacceptable words in prompt.\n";
                continue;
            }

            auto results = search_documents(conn, words, search_cfg.max_results);
            print_results(results);
        }

        PQfinish(conn);

    }
    catch (const std::exception& e) 
    {
        std::cerr << "[Searcher] Critical error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}