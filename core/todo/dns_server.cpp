// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "dns_server.h"

#include <unordered_map>
#include <vector>

#include "husarnet/ports/port.h"
#include "husarnet/ports/sockets.h"

#include "husarnet/util.h"

std::string read_label(const std::string& data, int& pos)
{
  if(pos >= data.size())
    return "";
  int len = data[pos];
  if(pos + len + 1 > data.size())
    return "";
  std::string label = data.substr(pos + 1, len);
  pos += len + 1;
  return label;
}

struct DnsRequest {
  bool ok;
  uint16_t id;
  struct Label {
    std::string name;
    uint16_t qtype;
    uint16_t qclass;
  };

  std::vector<Label> labels;
};

DnsRequest parseDnsRequest(std::string s)
{
  if(s.size() < 12) {
    LOG("DNS request too short");
    return {false};
  }

  auto id = htons(unpack<uint16_t>(substr<0, 2>(s)));
  // auto flags = unpack<uint16_t>(substr<2,2>(s));
  auto qcount = htons(unpack<uint16_t>(substr<4, 2>(s)));

  std::vector<DnsRequest::Label> labels;
  int pos = 12;
  for(int i = 0; i < qcount; i++) {
    std::string label = "";
    while(true) {
      std::string part = read_label(s, pos);
      if(part == "")
        break;
      label += part + ".";
    }
    if(label != "")
      label = label.substr(0, label.size() - 1);

    if(pos + 4 > s.size())
      break;

    uint16_t qtype = htons(unpack<uint16_t>(s.substr(pos, 2)));
    uint16_t qclass = htons(unpack<uint16_t>(s.substr(pos + 2, 2)));

    LOG("query %s (qtype = %d, qclass = %d)", label.c_str(), (int)qtype,
        (int)qclass);

    labels.push_back({label, qtype, qclass});
    pos += 4;
  }
  return DnsRequest{true, id, labels};
}

std::string mk_label(std::string label)
{
  std::string res = "";
  std::string curr = "";
  for(const char& ch : label + ".") {
    if(ch == '.') {
      res += pack(uint8_t(curr.size())) + curr;
      curr = "";
    } else {
      curr.push_back(ch);
    }
  }
  res += '\0';
  return res;
}

std::string
makeDnsResponse(int id, ResolveResponse resp, DnsRequest::Label query)
{
  uint16_t opcode = 0;
  uint16_t flags = (1 << 15) | (opcode << 12) | (int)resp.code |
                   /* rd|ra */ (1 << 7) | (1 << 8);
  uint16_t qcount = (query.name.size() > 0) ? 1 : 0;
  uint16_t acount = (resp.code == ResolveResponse::OK && query.qtype == 28 &&
                     query.qclass == 1)
                      ? 1
                      : 0;
  uint16_t nscount = 0;
  uint16_t arcount = 0;

  std::string body = pack(htons(id)) + pack(htons(flags)) +
                     pack(htons(qcount)) + pack(htons(acount)) +
                     pack(htons(nscount)) + pack(htons(arcount));

  if(qcount == 1) {
    body += mk_label(query.name);
    body += pack(htons(uint16_t(query.qtype))) +
            pack(htons(uint16_t(query.qclass)));
  }

  if(acount == 1) {
    body += mk_label(resp.label);
    std::string rdata = resp.address.toBinary();
    body += pack(htons(uint16_t(28))) + pack(htons(uint16_t(1))) +
            pack(htonl(uint32_t(10))) + pack(htons(uint16_t(rdata.size())));
    body += rdata;
  }

  return body;
}

std::string processDnsRequest(std::string s, ResolveFunc resolver)
{
  DnsRequest req = parseDnsRequest(s);
  if(!req.ok)
    return "";

  if(req.labels.size() > 0) {
    ResolveResponse resp = resolver(req.labels[0].name);
    if(resp.code == ResolveResponse::IGNORE)
      return "";

    return makeDnsResponse(req.id, resp, req.labels[0]);
  }

  return "";
}

uint16_t checksum(std::string pkt)
{
  if(pkt.size() % 2 == 1)
    pkt += '\0';

  int s = 0;
  for(int i = 0; i < pkt.size(); i += 2)
    s += unpack<uint16_t>(pkt.substr(i, 2));
  s = (s >> 16) + (s & 0xffff);
  s += s >> 16;
  s = ~s;
  return s & 0xffff;
}

std::string makeUdp6Packet(
    std::string src,
    std::string dst,
    int srcport,
    int dstport,
    std::string data)
{
  std::string pseudoheader =
      src + dst + pack(htons(data.size() + 8)) + std::string("\0\0\0\x11", 4);
  std::string header = pack(htons(srcport)) + pack(htons(dstport)) +
                       pack(htons(data.size() + 8)) + pack(htons(0));
  std::string chksum =
      pack(checksum(pseudoheader + header + data));  // native endian
  return header.substr(0, header.size() - 2) + chksum + data;
}

struct UdpPacket {
  uint16_t src_port;
  uint16_t dst_port;
  std::string data;
};

UdpPacket parseUdp6Packet(std::string data)
{
  UdpPacket res;
  res.src_port = htons(unpack<uint16_t>(data.substr(0, 2)));
  res.dst_port = htons(unpack<uint16_t>(data.substr(2, 2)));
  res.data = data.substr(8);
  return res;
}

std::string processUdpWithDnsRequest(
    std::string src,
    std::string dst,
    std::string data,
    ResolveFunc resolver)
{
  if(data[0] != 0x11)
    return "";  // not udp
  UdpPacket packet = parseUdp6Packet(data.substr(1));
  if(packet.dst_port != 53) {
    return "";
  }

  std::string dnsResp = processDnsRequest(packet.data, resolver);
  if(dnsResp == "")
    return "";
  return '\x11' + makeUdp6Packet(dst, src, 53, packet.src_port, dnsResp);
}

struct DnsSocketWrapper : NgSocket, NgSocketDelegate {
  NgSocket* socket;
  const DeviceId dnsServerAddress = IpAddress::parse("fc94::1").toBinary();
  ;  // IpAddress::parse("2001:4860:4860::8888").toBinary();
  ResolveFunc resolver;
  DeviceId deviceId;
  int dnsFd;

  const InetAddress recursiveDnsServer = InetAddress::parse("8.8.8.8:53");

  std::unordered_map<uint16_t, std::pair<uint16_t, uint16_t> > dnsIdMapping;

  DnsSocketWrapper()
  {
    dnsFd =
        OsSocket::bindUdpSocket(InetAddress{IpAddress(), (uint16_t)0}, false);

    OsSocket::bindCustomDgramFd(
        dnsFd, std::bind(
                   &DnsSocketWrapper::dnsReply, this, std::placeholders::_1,
                   std::placeholders::_2));
  }

  void dnsReply(InetAddress src, string_view data)
  {
    LOG_DEBUG("dns reply: %s", encodeHex(data).c_str());
    if(src != recursiveDnsServer)
      return;

    uint16_t id = unpack<uint16_t>(data.substr(0, 2));
    if(dnsIdMapping.find(id) == dnsIdMapping.end()) {
      LOG("unknown DNS response");
      return;
    }
    uint16_t orgId = dnsIdMapping[id].first;
    uint16_t orgPort = dnsIdMapping[id].second;
    dnsIdMapping.erase(dnsIdMapping.find(id));
    std::string newData = pack(orgId) + std::string(data).substr(2);

    delegate->onDataPacket(
        dnsServerAddress,
        '\x11' +
            makeUdp6Packet(dnsServerAddress, deviceId, 53, orgPort, newData));
  }

  uint16_t makeDnsId()
  {
    while(true) {
      uint16_t id = random() % (1 << 16);
      if(dnsIdMapping.find(id) == dnsIdMapping.end())
        return id;
    }
  }

  void forwardDnsPacket(string_view rawData)
  {
    if(dnsIdMapping.size() > 100) {
      LOG("too many pending DNS requests, clearing");
      dnsIdMapping.clear();
    }
    UdpPacket packet = parseUdp6Packet(rawData.substr(1));

    uint16_t newId = makeDnsId();
    uint16_t orgId = unpack<uint16_t>(packet.data.substr(0, 2));
    dnsIdMapping[newId] = std::make_pair(orgId, packet.src_port);

    std::string newData = pack(newId) + packet.data.substr(2);
    LOG("forward to recursive server");
    OsSocket::udpSend(recursiveDnsServer, newData, dnsFd);
  }

  void periodic() override
  {
    socket->periodic();
  }

  void onDataPacket(DeviceId source, string_view data) override
  {
    delegate->onDataPacket(source, data);
  }

  void sendDataPacket(DeviceId target, string_view packet) override
  {
    if(target == dnsServerAddress) {
      if(packet.size() >= 8 && packet[0] == 0x3A &&
         packet[1] == 0x80) {  // ICMP
        // reply to pings
        std::string response;
        response.push_back(0x3A);
        response.push_back(0x81);
        response.push_back(0x00);
        response += pack<uint16_t>(
            htons(htons(unpack<uint16_t>(packet.substr(3, 2))) - 256));
        response += packet.substr(5);

        LOG_DEBUG(
            "ping %s -> %s", encodeHex(packet).c_str(),
            encodeHex(response).c_str());
        delegate->onDataPacket(dnsServerAddress, response);
        return;
      }

      std::string resp =
          processUdpWithDnsRequest(deviceId, target, packet, resolver);
      if(resp == "") {
        // forward the packet to a recursive DNS server
        forwardDnsPacket(packet);
      } else {
        delegate->onDataPacket(dnsServerAddress, resp);
      }
    } else {
      socket->sendDataPacket(target, packet);
    }
  }
};

NgSocket* dnsServerWrap(DeviceId deviceId, NgSocket* sock, ResolveFunc resolver)
{
  auto dev = new DnsSocketWrapper;
  dev->deviceId = deviceId;
  dev->socket = sock;
  dev->resolver = resolver;
  sock->delegate = dev;
  return dev;
}
