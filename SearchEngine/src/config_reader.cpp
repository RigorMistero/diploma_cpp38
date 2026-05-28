#include "config_reader.h"
#include <boost/algorithm/string.hpp>
#include <stdexcept>

ConfigReader::ConfigReader(const std::string& config_path)
{
  try
  {
    boost::property_tree::ini_parser::read_ini(config_path, pt_);
  }
  catch (const std::exception& e)
  {
    throw std::runtime_error("Failed to read config" + std::string(e.what()));
  }
}

std::string DatabaseConfig::connection_string() const
{
  return "host=" + host + " port=" + std::to_string(port) + " dbname=" + dbname +
  " user=" + user + " password=" + password;
}

DatabaseConfig ConfigReader::get_database_config() const
{
  DatabaseConfig cfg;
  cfg.host = pt_.get<std::string>("database.host", "localhost");
  cfg.port = pt_.get<int>("database.port", 5432);
  cfg.dbname = pt_.get<std::string>("database.dbname", "search_engine");
  cfg.user = pt_.get<std::string>("database.user", "postgres");
  cfg.password = pt_.get<std::string>("database.password", "");
  return cfg;
}

IndexerConfig ConfigReader::get_indexer_config() const
{
  IndexerConfig cfg;
  cfg.start_dir = pt_.get<std::string>("indexer.start_dir", "");
  cfg.thread_pool_size = pt_.get<int>("indexer.thread_pool_size",4);

  std::string exts = pt_.get<std::string>("indexer.extensions", "txt");
  boost::split(cfg.extensions, exts, boost::is_any_of(","));
  for (auto& ext : cfg.extensions)
  {
    boost::trim(ext);
  }
  return cfg;
}

SearcherConfig ConfigReader::get_searcher_config() const
{
  SearcherConfig cfg;
  cfg.max_results = pt_.get<int> ("searcher.max_results", 10);
  cfg.max_query_words = pt_.get<int>("searcher.max_query_words", 4);
  return cfg;
}
