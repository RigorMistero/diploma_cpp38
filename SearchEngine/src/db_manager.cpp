#include "db_manager.h"
#include <iostream>

DbManager::DbManager(const std::string& connection_string) : conn_string_(connection_string)
{
}

void DbManager::initialize_tables()
{
    auto conn = create_connection();
    pqxx::work txn(*conn);

    create_documents_table(txn);
    create_words_table(txn);
    create_word_frequencies_table(txn);

    txn.commit();
    std::cout << "Database tables created successfully (or been created earlier).\n";
}

std::unique_ptr<pqxx::connection> DbManager::create_connection() const
{
    auto conn = std::make_unique<pqxx::connection>(conn_string_);
    conn->set_client_encoding("UTF8");
    return conn;
}

const std::string& DbManager::connection_string() const
{
    return conn_string_;
}

void DbManager::create_documents_table(pqxx::work& txn)
{
    txn.exec(R"(
    CREATE TABLE IF NOT EXISTS documents(
      id SERIAL PRIMARY KEY,
      file_path TEXT UNIQUE NOT NULL,
      file_name TEXT NOT NULL,
      indexed_at TIMESTAMP DEFAULT NOW()
    );
  )"
    );
}

void DbManager::create_words_table(pqxx::work& txn)
{
    txn.exec(R"(
    CREATE TABLE IF NOT EXISTS words(
      id SERIAL PRIMARY KEY,
      word VARCHAR(32) UNIQUE NOT NULL
    );
  )");

    txn.exec(R"(
    CREATE INDEX IF NOT EXISTS idx_words_word ON words(word);
  )");
}

void DbManager::create_word_frequencies_table(pqxx::work& txn)
{
    txn.exec(R"(
    CREATE TABLE IF NOT EXISTS word_frequencies(
      document_id INT REFERENCES documents(id) ON DELETE CASCADE,
      word_id INT REFERENCES words(id) ON DELETE CASCADE,
      frequency INT NOT NULL DEFAULT 1,
      PRIMARY KEY (document_id, word_id)
    );
  )");

    txn.exec(R"(
    CREATE INDEX IF NOT EXISTS idx_wf_document_id ON word_frequencies(document_id);
  )");

    txn.exec(R"(
    CREATE INDEX IF NOT EXISTS idx_wf_word_id ON word_frequencies(word_id);
  )");
}