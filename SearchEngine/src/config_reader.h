#pragma once

#include <string>
#include <vector>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

struct DatabaseConfig
{
  std::string host = "localhost";
  int port = 5432;
  std::string dbname = "search_engine";
  std::string user = "postgres";
  std::string password = "";

  std::string connection_string() const;
};

struct IndexerConfig
{
  std::string start_dir;
  std::vector<std::string> extensions;
  int thread_pool_size = 4;
};

struct SearcherConfig
{
  int max_results = 10;
  int max_query_words = 4;
};

class ConfigReader
{
public:
  explicit ConfigReader (const std::string& config_path);

  DatabaseConfig get_database_config() const;
  IndexerConfig  get_indexer_config() const;
  SearcherConfig get_searcher_config() const;

private:
  boost::property_tree::ptree pt_;
};
