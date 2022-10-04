// A standalone server that can be used to play Push Fight locally.
//
// It's basically a minimal HTTP/1.0 server that integrates static file serving
// with handling requests to /lookup/ similar to lookup-min-http-server.py
//
// Nothing about this is production ready. In particular, the server does not
// support multithreading so a single stalled request can block the server
// entirely. The server is intended for local use only.

#include "bytes.h"
#include "flags.h"
#include "minimized-lookup.h"
#include "minimized-accessor.h"
#include "parse-perm.h"
#include "position-value.h"

#if _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

static int CallWsaStartup() {
  WSADATA data;
  return WSAStartup(MAKEWORD(2, 2), &data);
}

static int wsa_startup_return_value = CallWsaStartup();

#else  // _WIN32

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#endif  // _WIN32

#include <unistd.h>

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <map>
#include <vector>

namespace {

const char *default_minimized_path = "minimized.bin";
const char *default_hostname = "localhost";
const char *default_portname = "8080";
const char *default_serve_dir = "static";
const char *default_index_file = "index.html";

std::string minimized_path = default_minimized_path;
std::string hostname = default_hostname;
std::string portname = default_portname;
std::string serve_dir = default_serve_dir;
std::string index_file = default_index_file;

const std::string lookup_path = "/lookup/";

std::optional<MinimizedAccessor> acc;

constexpr int max_header_size = 102400;  // 100 KiB

const std::string SPACE_STRING = " ";
const std::string AMPERSAND_STRING = "&";
const std::string NEWLINE_STRING = "\r\n";
const std::string SLASH_STRING = "/";

const std::map<std::string, std::string> empty_headers = {};

const std::map<std::string, std::string> content_type_by_extension = {
  {"bin", "application/octet-stream"},
  {"css", "text/css"},
  {"gif", "image/gif"},
  {"htm", "text/html"},
  {"html", "text/html"},
  {"jpg", "image/jpeg"},
  {"jpeg", "image/jpeg"},
  {"js", "application/javascript"},
  {"json", "application/json"},
  {"png", "image/png"},
  {"txt", "text/plain"},
};

int CreateListeningSocket(const char *hostname, const char *portname, int backlog=100) {
#ifdef _WIN32
  assert(wsa_startup_return_value == 0);
#endif

  struct addrinfo hints = {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  struct addrinfo *ai = nullptr;
  if (getaddrinfo(hostname, portname, &hints, &ai) != 0) {
    perror("getaddrinfo()");
    return -1;
  }

  for (struct addrinfo *p = ai; p != nullptr; p = p->ai_next) {
    int s = socket(p->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1) {
      perror("socket()");
    } else {
      int one = 1;
      if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&one), sizeof(one)) == -1) {
        perror("setsockopt()");
      } else if(bind(s, p->ai_addr, p->ai_addrlen) == -1) {
        perror("bind()");
      } else if (listen(s, backlog) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
      } else {
        // Success!
        freeaddrinfo(ai);
        return s;
      }
      close(s);
    }
  }
  freeaddrinfo(ai);
  return -1;
}

// Parses a string into parts separated by `delim` (which must be nonempty).
//
// The result always contains at least 1 element, even if `s` is empty.
std::vector<std::string> SplitString(const std::string &s, const std::string &delim) {
  assert(!delim.empty());
  std::vector<std::string> parts;
  size_t begin = 0, end = 0;
  while ((end = s.find(delim, begin)) != std::string::npos) {
    parts.push_back(s.substr(begin, end - begin));
    begin = end + delim.size();
  }
  parts.push_back(s.substr(begin));
  return parts;
}

void MakeLowercase(std::string &s) {
  for (char &ch : s) ch = tolower(ch);
}

// This converts the successors to a JSON object in the same format as
// lookup-min-http-server.py, which is understood by analysis.js.
std::string ConvertSuccessors(
    const std::vector<EvaluatedSuccessor> &successors) {
  std::ostringstream oss;

  // Calculate overall status. Since successors are guaranteed to be sorted from
  // best to worst, the best status comes first. Special case: if there are no
  // successors, the position is lost-in-0.
  Value status = successors.empty() ? Value::LossIn(0) : successors.front().value;

  // Group successors by status. Again, the successors are already sorted.
  std::vector<std::pair<Value, std::vector<Moves>>> grouped_moves;
  for (const EvaluatedSuccessor &successor : successors) {
    if (grouped_moves.empty() || grouped_moves.back().first != successor.value) {
      grouped_moves.push_back({successor.value, {}});
    }
    grouped_moves.back().second.push_back(successor.moves);
  }

  // Convert to a JSON string.
  //
  // Note: we don't bother to escape anything here because we happen to know the
  // string versions of values and moves do not contain characters that are
  // forbidden in a JSON strings.
  oss << "{\"status\":\"" << status << "\",\"successors\":[";
  for (size_t i = 0; i < grouped_moves.size(); ++i) {
    if (i != 0) oss << ',';
    oss << "{\"status\":\"" << grouped_moves[i].first << "\",\"moves\":[";
    for (size_t j = 0; j < grouped_moves[i].second.size(); ++j) {
      if (j != 0) oss << ',';
      oss << '"' << grouped_moves[i].second[j] << '"';
    }
    oss << "]}";
  }
  oss << "]}";
  return oss.str();
}

/*
// Parses query parameters formatted as `key=value` separated by ampersands ('&'),
// without URL-decoding keys/values.
//
// e.g. a string 'foo=123&bar=baz' is parsed into {{"foo", "123"}, {"bar", "baz"}},
// and 'foo%20bar=bla+bla' is parsed into {"foo%20bar", "bla+bla"}.
//
// If duplicate keys occur, the last value is kept.
std::map<std::string, std::string> ParseQueryParameters(const std::string &query_string) {
  std::map<std::string, std::string> map;
  size_t pos = 0;
  for (;;) {
    std::string::size_type i = query_string.find('=', pos);
    std::string::size_type j = query_string.find('&', pos);
    if (i < j) {
      std::string key = query_string.substr(pos, i - pos);
      std::string value = query_string.substr(i + 1, j - i - 1);
      map[key] = value;
    }
    if (j == std::string::npos) break;
    pos = j + 1;
  }
  return map;
}
*/

void SendData(int s, const char *data, size_t len) {
  while (len > 0) {
    ssize_t n = send(s, data, len, 0);
    if (n < 0) {
      perror("send()");
      return;
    }
    assert(n <= len);
    data += n;
    len -= n;
  }
}

void SendData(int s, const uint8_t *data, size_t len) {
  return SendData(s, reinterpret_cast<const char*>(data), len);
}

void SendData(int s, const char *data) {
  SendData(s, data, strlen(data));
}

void SendData(int s, const std::string &data) {
  SendData(s, data.data(), data.size());
}

void SendResponse(
  int s, int status, std::string_view status_text,
  std::string_view content, std::string_view content_type = "text/plain",
  const std::map<std::string, std::string> &headers = empty_headers) {
  std::ostringstream oss;
  oss << "HTTP/1.0 " << status << ' ' << status_text << "\r\n"
      << "Content-Type: " << content_type << "\r\n"
      << "Content-Length: " << content.size() << "\r\n";
  for (const auto &elem : headers) {
    // Assumes all headers are already correctly formatted with no characters
    // that need to be escaped.
    oss << elem.first << ": " << elem.second << "\r\n";
  }
  oss << "\r\n" << content;

  SendData(s, oss.str());
}

void SendMethodNotAllowed(int s) {
  SendResponse(s, 405, "Method Not Allowed", "Method not allowed.");
}

void SendBadRequest(int s, std::string_view message = "Bad request.") {
  SendResponse(s, 400, "Bad Request", message);
}

void SendNotFound(int s) {
  SendResponse(s, 404, "Not Found", "Resource not found.");
}

bool StartsWith(const std::string &s, const std::string &prefix) {
  return s.rfind(prefix, 0) == 0;
}

bool StartsWith(const std::string &s, const char *prefix) {
  return s.rfind(prefix, 0) == 0;
}

void HandleHttpRequest(
    int s,
    const std::string &request_method,
    const std::string &request_uri,
    const std::map<std::string, std::string> &request_headers
) {
  (void) request_headers;  // unused

  if (request_method != "GET") {
    SendMethodNotAllowed(s);
    return;
  }

  // Maybe TODO: strip query string from path? Currently we will return 404.

  if (StartsWith(request_uri, lookup_path)) {
    std::vector<std::string> components = SplitString(request_uri.substr(lookup_path.size()), SLASH_STRING);
    // Match path of the form: perms/<permutation string/id>
    if (components.size() == 2 && components[0] == "perms") {
      std::optional<std::vector<EvaluatedSuccessor>> successors;
      std::string error;
      if (std::optional<Perm> perm = ParsePerm(components[1], &error); perm) {
        successors = LookupSuccessors(*acc, *perm, &error);
      }
      if (!successors) {
        SendResponse(s, 400, "Bad Request", error);
        return;
      }

      std::string content = ConvertSuccessors(*successors);
      SendResponse(s, 200, "OK", content, "application/json", {
        {"Cache-Control", "public, max-age=86400"},  // cache for 1 day
      });
      return;
    }

    // Pattern not matched.
    SendNotFound(s);
    return;

  } else {
    // Serve static files.

    // Maybe TODO: add support for Modification-Time, If-Modified-Since, HEAD requests, etc.

    // Forbid accessing files that start with a dot, and forbid relative paths like "/../bla".
    if (request_uri[0] != '/') {
      SendBadRequest(s);
      return;
    }
    // The main security feature: forbid absolute paths and double-dot components;
    std::filesystem::path request_path = request_uri.substr(1);
    request_path = request_path.lexically_normal();
    if (request_path.empty() || (request_path.native().size() == 1 && request_path.native()[0] == '.')) {
      request_path = index_file;
    }
    if (!request_path.is_relative()) {
      SendBadRequest(s);
      return;
    }
    for (const auto &component : request_path) {
      if (component.native()[0] == '.') {
        // Deny access to any files that start with dot.
        // Importantly, this includes ".." components!
        SendBadRequest(s);
        return;
      }
    }

    std::filesystem::path path = std::filesystem::path(serve_dir) / request_path;
    if (!std::filesystem::exists(path)) {
      SendNotFound(s);
      return;
    }

    std::string extension = path.extension().string();
    std::string content_type;
    if (!extension.empty()) {
      auto it = content_type_by_extension.find(extension);
      if (it != content_type_by_extension.end()) content_type = it->second;
    }

    // Note: this aborts on error, which is not ideal :/
    bytes_t bytes = ReadFromFile(path.string().c_str());

    std::ostringstream oss;
    oss << "HTTP/1.0 200 OK\r\n";
    oss << "Content-Length: " << bytes.size() << "\r\n";
    if (!content_type.empty()) oss << "Content-Type: " << content_type << "\r\n";
    oss << "\r\n";
    std::string header = oss.str();;
    SendData(s, header.data(), header.size());
    SendData(s, bytes.data(), bytes.size());
  }
}

void HandleRequest(int s) {
  std::string request_header;
  while (request_header.size() < max_header_size) {
    char buffer[8192];
    ssize_t n = recv(s, buffer, std::min(sizeof(buffer), max_header_size - request_header.size()), 0);
    if (n < 0) {
      perror("recv()");
      break;
    }
    if (n == 0) {
      std::cerr << "Premature EOF." << std::endl;
      break;
    }
    // Position to start searching for terminating sequence "\r\n\r\n". Start 3
    // bytes back to catch the sequence if it was split between multiple reads.
    size_t search_pos = request_header.size() >= 3 ? request_header.size() - 3 : 0;
    request_header.append(buffer, n);
    std::string::size_type end_pos = request_header.find("\r\n\r\n", search_pos, 4);
    if (end_pos != std::string::npos) {
      // Request complete.
      // Note: we don't support duplicate headers or multiline headers.
      request_header.resize(end_pos);
      std::vector<std::string> headers = SplitString(request_header, NEWLINE_STRING);
      // Parse request line (e.g. "GET /bla HTTP/1.0")
      std::vector<std::string> parts = SplitString(headers[0], SPACE_STRING);
      if (parts.size() != 3 || parts[0].empty() || parts[1].empty() || !StartsWith(parts[2], "HTTP/") != 0) {
        std::cerr << "Invalid request type.\n" << std::endl;
        break;
      }
      std::map<std::string, std::string> header_map;
      for (size_t i = 1; i < headers.size(); ++i) {
        size_t j = headers[i].find(": ");
        if (j == std::string::npos) {
          std::cerr << "Missing delimiter in header." << std::endl;
          break;
        }
        std::string key = headers[i].substr(0, j);
        MakeLowercase(key);
        if (header_map.count(key)) {
          std::cerr << "Duplicate header (not supported)." << std::endl;
          break;
        }
        if (key.empty()) {
          std::cerr << "Empty header name." << std::endl;
          break;
        }
        if (key[0] == ' ' || key[0] == '\t') {
          std::cerr << "Multiline header (not supported)." << std::endl;
          break;
        }
        header_map[key] = headers[i].substr(j + 2);
      }

      // Succesfully parsed request! Now handle it.
      HandleHttpRequest(s, parts[0], parts[1], header_map);
      break;
    }
  }
  // 2 == SHUT_RDWR on Linux == SD_BOTH on Windows
  shutdown(s, 2);
  close(s);
}

void PrintUsage() {
  std::cout << "pushfight-standalone-server" << "\n\n"
    << "Options:\n\n"
    << " --minimized=<path to minimized.bin> (default: " << default_minimized_path << ")\n"
    << " --host=<hostname to bind to> (default: " << default_hostname << ")\n"
    << " --port=<port to listen on> (default: " << default_portname << ")\n"
    << " --static=<directory with static content> (default: " << default_serve_dir << ")\n"
    << " --index=<directory index file> (default: " << default_index_file << ")\n"
    << std::endl;
}

}  // namespace

int main(int argc, char *argv[]) {
#ifdef _WIN32
  assert(wsa_startup_return_value == 0);
#endif

  std::map<std::string, Flag> flags = {
    {"minimized", Flag::optional(minimized_path)},
    {"host", Flag::optional(hostname)},
    {"port", Flag::optional(portname)},
    {"static", Flag::optional(serve_dir)},
    {"index", Flag::optional(index_file)},
  };

  if (!ParseFlags(argc, argv, flags)) {
    std::cout << "\n";
    PrintUsage();
    return 1;
  }

  std::cout << "Serving static content from directory: " << serve_dir << std::endl;
  if (!std::filesystem::is_directory(serve_dir)) {
    std::cerr << serve_dir << " is not a directory!" << std::endl;
    return 1;
  }

  if (!std::filesystem::exists(std::filesystem::path(serve_dir) / index_file)) {
    std::cerr << "Index file (" << index_file << ") does not exist!" << std::endl;
    return 1;
  }

  if (!std::filesystem::exists(std::filesystem::path(serve_dir) / "bundle.js")) {
    std::cerr << "bundle.js file does not exist!\n"
        "See html/README.txt for instructions how to rebuild it." << std::endl;
    return 1;
  }

  std::cout << "Using index file: " << index_file << std::endl;
  if (const auto index_path = std::filesystem::path(serve_dir) / index_file;
      !std::filesystem::is_regular_file(index_path)) {
    std::cerr << index_path << " is not a regular file!" << std::endl;
    return 1;
  }

  // Hack: automatically switch to the xz-compressed file if the uncompressed
  // file does not exist.
  if (!std::filesystem::exists(minimized_path) &&
      std::filesystem::exists(minimized_path + ".xz")) {
    minimized_path += ".xz";
  }

  std::cout << "Using minimized position data from: " << minimized_path << std::endl;
  acc.emplace(minimized_path.c_str());

  std::cout << "Creating a TCP socket to listen on host " << hostname << " port " << portname << "..." << std::endl;
  int server_socket = CreateListeningSocket(hostname.c_str(), portname.c_str());
  if (server_socket == -1) {
    // Error message has already been printed.
    return 1;
  }

  std::cout << "\nPush Fight standalone server now serving on "
      << "http://" << hostname << ":" << portname << "/" << std::endl;

  while(true) {
    struct sockaddr_storage client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addrlen);
    if (client_socket == -1) {
      perror("accept()");
      continue;
    }
    HandleRequest(client_socket);
  }
}
