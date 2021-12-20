// port.h has to be the first include, newline afterwards prevents autoformatter
// from sorting it
#include "port.h"

#include "husarnet_config.h"
#include "licensing.h"
#include "util.h"

std::string requestLicense(InetAddress address) {
  int sockfd = OsSocket::connectTcpSocket(address);
  if (sockfd < 0) {
    exit(1);
  }

  std::string readBuffer;
  readBuffer.resize(8192);

  std::string request =
      "GET /license.json HTTP/1.1\n"
      "Host: app.husarnet.com\n"
      "User-Agent: husarnet\n"
      "Accept: */*\n\n";

  SOCKFUNC(send)(sockfd, request.data(), request.size(), 0);
  size_t len =
      SOCKFUNC(recv)(sockfd, (char*)readBuffer.data(), readBuffer.size(), 0);
  size_t pos = readBuffer.find("\r\n\r\n");

  if (pos == std::string::npos) {
    LOG("invalid response from the server: %s", readBuffer.c_str());
    exit(1);
  }
  pos += 4;
  return readBuffer.substr(pos, len - pos);
}
