#pragma once

#include <string>
#include <memory>
#include <pqxx/pqxx>

class DbManager
{
public:
  explicit DbManager (const std::string& connection_string);

  //creating tables if missing
  void initialize_tables();

  //creating connections  (for threads - each creates its own)
  std::unique_ptr<pqxx::connection> create_connection() const;

  //creating connecion string
  const std::string& connection_string() const;

private:
  std::string conn_string_;

  void create_documents_table(pqxx::work& txn);
  void create_words_table(pqxx::work& txn);
  void create_word_frequencies_table(pqxx::work& txn);
};
