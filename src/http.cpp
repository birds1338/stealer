#include "http.hpp"

HTTPResponse HTTP::get(const std::string &url, const std::map<std::string, std::string> &headers) {
  return request(url, "GET", "", headers);
}

HTTPResponse HTTP::post(const std::string &url, const std::string &data, const std::map<std::string, std::string> &headers) {
  return request(url, "POST", data, headers);
}

HTTPResponse HTTP::put(const std::string &url, const std::string &data, const std::map<std::string, std::string> &headers) {
  return request(url, "PUT", data, headers);
}

HTTPResponse HTTP::request(const std::string &url, const std::string &method, const std::string &data,
                           const std::map<std::string, std::string> &headers) {
  std::wstring user_agent = L"curl/7.68.0";
  auto it = headers.find("User-Agent");
  if (it != headers.end())
    user_agent = s2ws(it->second);

  HINTERNET session = WinHttpOpen(user_agent.c_str(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session)
    throw HTTPException("Failed to open WinHTTP session.");

  URL_COMPONENTS components;
  memset(&components, 0, sizeof(components));
  components.dwStructSize = sizeof(components);

  std::wstring wUrl = s2ws(url);
  wchar_t hostName[256];
  wchar_t urlPath[1024];
  components.lpszHostName = hostName;
  components.dwHostNameLength = _countof(hostName);
  components.lpszUrlPath = urlPath;
  components.dwUrlPathLength = _countof(urlPath);

  if (!WinHttpCrackUrl(wUrl.c_str(), (DWORD)wUrl.length(), 0, &components)) {
    WinHttpCloseHandle(session);
    throw HTTPException("Failed to parse URL.");
  }

  HINTERNET connection = WinHttpConnect(session, components.lpszHostName, components.nPort, 0);
  if (!connection) {
    WinHttpCloseHandle(session);
    throw HTTPException("Failed to connect to server.");
  }

  HINTERNET request =
      WinHttpOpenRequest(connection, s2ws(method).c_str(), components.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                         (components.nPort == INTERNET_DEFAULT_HTTPS_PORT) ? WINHTTP_FLAG_SECURE : 0);
  if (!request) {
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    throw HTTPException("Failed to open HTTP request.");
  }

  for (const auto &header : headers) {
    std::wstring fullHeader = s2ws(header.first + ": " + header.second);
    WinHttpAddRequestHeaders(request, fullHeader.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);
  }

  if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)(data.empty() ? nullptr : data.c_str()), (DWORD)data.length(),
                          (DWORD)data.length(), 0)) {
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    throw HTTPException("Failed to send HTTP request.");
  }

  if (!WinHttpReceiveResponse(request, NULL)) {
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    throw HTTPException("Failed to receive HTTP response.");
  }

  DWORD status_code = 0;
  DWORD statusCodeSize = sizeof(status_code);
  WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status_code,
                      &statusCodeSize, WINHTTP_NO_HEADER_INDEX);

  DWORD size = 0;
  WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &size, WINHTTP_NO_HEADER_INDEX);
  std::wstring raw_headers(size / sizeof(wchar_t), 0);
  WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, &raw_headers[0], &size,
                      WINHTTP_NO_HEADER_INDEX);

  std::map<std::string, std::string> response_headers = parse_headers(raw_headers);

  std::vector<char> buffer;
  DWORD bytesRead = 0;
  do {
    buffer.resize(buffer.size() + 1024);
    if (!WinHttpReadData(request, &buffer[buffer.size() - 1024], 1024, &bytesRead)) {
      WinHttpCloseHandle(request);
      WinHttpCloseHandle(connection);
      WinHttpCloseHandle(session);
      throw HTTPException("Failed to read HTTP response.");
    }
  } while (bytesRead > 0);

  buffer.resize(buffer.size() - 1024 + bytesRead);

  WinHttpCloseHandle(request);
  WinHttpCloseHandle(connection);
  WinHttpCloseHandle(session);

  return HTTPResponse(status_code, std::string(buffer.begin(), buffer.end()), response_headers);
}

std::map<std::string, std::string> HTTP::parse_headers(const std::wstring &raw_headers) {
  std::map<std::string, std::string> headers;
  size_t start = 0;
  size_t end = 0;

  while ((end = raw_headers.find(L"\r\n", start)) != std::wstring::npos) {
    std::wstring headerLine = raw_headers.substr(start, end - start);
    size_t colon = headerLine.find(L':');
    if (colon != std::wstring::npos) {
      std::string name = ws2s(headerLine.substr(0, colon));
      std::string value = ws2s(headerLine.substr(colon + 2)); // Skip ": "
      headers[name] = value;
    }
    start = end + 2;
  }

  return headers;
}