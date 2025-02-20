// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ipaddress.h"

#include "husarnet/ports/port.h"

bool IpAddress::isLinkLocal() const
{
  if(isMappedV4())
    return data[12] == 169 && data[13] == 254;

  return data[0] == 0xfe && data[1] == 0x80;
}

bool IpAddress::isMappedV4() const
{
  return memcmp(data.data(), "\0\0\0\0\0\0\0\0\0\0\xFF\xFF", 12) == 0;
}

bool IpAddress::isPrivateV4() const
{
  if(!isMappedV4())
    return false;
  if(data[12] == 127)
    return true;
  if(data[12] == 10)
    return true;
  if(data[12] == 192 && data[13] == 168)
    return true;
  if(data[12] == 172 && (data[13] & 0b11110000) == 16)
    return true;
  return false;
}

// Husarnet IPv6 address range
bool IpAddress::isFC94() const
{
  return data[0] == 0xfc && data[1] == 0x94;
}

bool IpAddress::isWildcard() const
{
  if(isMappedV4())
    return data[12] == 0;
  else
    return memcmp(data.data(), "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == 0;
}

bool IpAddress::isLoopback() const
{
  if(isMappedV4())
    return data[12] == 127;
  return memcmp(data.data(), "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01", 16) == 0;
}

bool IpAddress::isMulticast() const
{
  if(isMappedV4())
    return (data[12] & 0xf0) == 224;
  return data[0] == 0xff;
}

bool IpAddress::isPrivateNetworkV4() const
{
  if(!isMappedV4())
    return false;

  if(data[12] == 10)
    return true;
  if(data[12] == 172 && (data[13] & 0xf0) == 16)
    return true;
  if(data[12] == 192 && data[13] == 168)
    return true;

  return false;
}

bool IpAddress::isReservedNotPrivate() const
{
  if(isPrivateNetworkV4())
    return false;

  if(isWildcard() || isLoopback() || isMulticast() || isLinkLocal() || isFC94())
    return true;

  if(isMappedV4()) {
    if(data[12] == 100 && (data[13] & 0b11000000) == 64)  // CGNAT
      return true;
    if(data[12] == 192 && data[13] == 0 &&
       data[14] == 0)  // IETF Protocol Assignments
      return true;
    if(data[12] == 192 && data[13] == 0 && data[14] == 2)  // TEST-NET-1
      return true;
    if(data[12] == 198 && (data[13] & 0xfe) == 18)  // Benchmarking
      return true;
    if(data[12] == 198 && data[13] == 88 && data[14] == 99)  // 6to4 Relay
      return true;
    if(data[12] == 198 && data[13] == 51 && data[14] == 100)  // TEST-NET-2
      return true;
    if(data[12] == 203 && data[13] == 0 && data[14] == 113)  // TEST-NET-3
      return true;
    if(data[12] == 233 && data[13] == 252 && data[15] == 0)  // MCAST-TEST-NET
      return true;
    if((data[12] & 0xf0) == 240)  // Future use
      return true;
    if(data[12] == 255 && data[13] == 255 && data[14] == 255 &&
       data[15] == 255)  // Broadcast
      return true;
  } else {  // IPv6
    if(memcmp(data.data(), "\x00\x64\xff\x9b\0\0\0\0\0\0\0\0", 12) ==
       0)  // Global translation
      return true;
    if(memcmp(data.data(), "\x00\x64\xff\x9b\x00\x01", 6) ==
       0)  // Private translation
      return true;
    if(memcmp(data.data(), "\x01\x00\0\0\0\0\0\0", 8) == 0)  // Discard prefix
      return true;
    if(memcmp(data.data(), "\x20\x01\x00\x00", 4) == 0)  // Teredo
      return true;
    if(data[0] == 0x20 && data[1] == 0x01 && data[2] == 0 &&
       (data[3] & 0b11110000) == 0x20)  // ORCHIDv2
      return true;
    if(memcmp(data.data(), "\x20\x01\x0d\xb8", 4) == 0)  // Documentation
      return true;
    if(memcmp(data.data(), "\x20\x02", 2) == 0)  // 6to4
      return true;
  }

  return false;
}

// Copied from LwIP core/ipv6/ip6_addr.c
/**
 * Check whether "cp" is a valid ascii representation
 * of an IPv6 address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 *
 * @param cp IPv6 address in ascii representation (e.g. "FF01::1")
 * @param addr pointer to which to save the ip address in network order
 * @return 1 if cp could be converted to addr, 0 on failure
 */
int husarnet_ip6addr_aton(const char* cp, uint8_t* result)
{
  uint32_t addr[4];
  uint32_t addr_index, zero_blocks, current_block_index, current_block_value;
  const char* s;

  /* Count the number of colons, to count the number of blocks in a "::"
     sequence zero_blocks may be 1 even if there are no :: sequences */
  zero_blocks = 8;
  for(s = cp; *s != 0; s++) {
    if(*s == ':') {
      zero_blocks--;
    } else if(!isxdigit(*s)) {
      break;
    }
  }

  /* parse each block */
  addr_index = 0;
  current_block_index = 0;
  current_block_value = 0;
  for(s = cp; *s != 0; s++) {
    if(*s == ':') {
      if(current_block_index & 0x1) {
        addr[addr_index++] |= current_block_value;
      } else {
        addr[addr_index] = current_block_value << 16;
      }
      current_block_index++;
      current_block_value = 0;
      if(current_block_index > 7) {
        /* address too long! */
        return 0;
      }
      if(s[1] == ':') {
        if(s[2] == ':') {
          /* invalid format: three successive colons */
          return 0;
        }
        s++;
        /* "::" found, set zeros */
        while(zero_blocks > 0) {
          zero_blocks--;
          if(current_block_index & 0x1) {
            addr_index++;
          } else {
            addr[addr_index] = 0;
          }
          current_block_index++;
          if(current_block_index > 7) {
            /* address too long! */
            return 0;
          }
        }
      }
    } else if(isxdigit(*s)) {
      /* add current digit */
      current_block_value =
          (current_block_value << 4) +
          (isdigit(*s) ? (uint32_t)(*s - '0')
                       : (uint32_t)(10 + (islower(*s) ? *s - 'a' : *s - 'A')));
    } else {
      /* unexpected digit, space? CRLF? */
      break;
    }
  }

  if(current_block_index & 0x1) {
    addr[addr_index++] |= current_block_value;
  } else {
    addr[addr_index] = current_block_value << 16;
  }

  /* convert to network byte order. */
  for(addr_index = 0; addr_index < 4; addr_index++) {
    addr[addr_index] = htonl(addr[addr_index]);
  }

  if(current_block_index != 7) {
    return 0;
  }

  memcpy(result, addr, 16);
  return 1;
}

/**
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 *
 * @param cp IP address in ascii representation (e.g. "127.0.0.1")
 * @param addr pointer to which to save the ip address in network order
 * @return 1 if cp could be converted to addr, 0 on failure
 */
int husarnet_ip4addr_aton(const char* cp, uint8_t* result)
{
  uint32_t val;
  char c;
  uint32_t parts[4];
  uint32_t* pp = parts;

  c = *cp;
  for(;;) {
    /*
     * Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, 1-9=decimal.
     */
    if(!isdigit(c)) {
      return 0;
    }
    val = 0;
    uint8_t base = 10;
    if(c == '0') {
      c = *++cp;
      if(c == 'x' || c == 'X') {
        base = 16;
        c = *++cp;
      } else {
        base = 8;
      }
    }
    for(;;) {
      if(isdigit(c)) {
        val = (val * base) + (uint32_t)(c - '0');
        c = *++cp;
      } else if(base == 16 && isxdigit(c)) {
        val = (val << 4) | (uint32_t)(c + 10 - (islower(c) ? 'a' : 'A'));
        c = *++cp;
      } else {
        break;
      }
    }
    if(c == '.') {
      /*
       * Internet format:
       *  a.b.c.d
       *  a.b.c   (with c treated as 16 bits)
       *  a.b (with b treated as 24 bits)
       */
      if(pp >= parts + 3) {
        return 0;
      }
      *pp++ = val;
      c = *++cp;
    } else {
      break;
    }
  }
  /*
   * Check for trailing characters.
   */
  if(c != '\0' && !isspace(c)) {
    return 0;
  }
  /*
   * Concoct the address according to
   * the number of parts specified.
   */
  switch(pp - parts + 1) {
    case 0:
      return 0; /* initial nondigit */

    case 1: /* a -- 32 bits */
      break;

    case 2: /* a.b -- 8.24 bits */
      if(val > 0xffffffUL) {
        return 0;
      }
      if(parts[0] > 0xff) {
        return 0;
      }
      val |= parts[0] << 24;
      break;

    case 3: /* a.b.c -- 8.8.16 bits */
      if(val > 0xffff) {
        return 0;
      }
      if((parts[0] > 0xff) || (parts[1] > 0xff)) {
        return 0;
      }
      val |= (parts[0] << 24) | (parts[1] << 16);
      break;

    case 4: /* a.b.c.d -- 8.8.8.8 bits */
      if(val > 0xff) {
        return 0;
      }
      if((parts[0] > 0xff) || (parts[1] > 0xff) || (parts[2] > 0xff)) {
        return 0;
      }
      val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
      break;
    default:
      assert(false);
      break;
  }
  val = htonl(val);
  memcpy(result, &val, 4);
  return 1;
}

IpAddress IpAddress::parse(const char* s)
{
  assert(s != nullptr);
  // DBG_ASSERT(s[0] >= 0 && s[0] < 128);
  IpAddress r{};

  if(husarnet_ip4addr_aton(s, (uint8_t*)(r.data.data() + 12)) == 1) {
    r.data[10] = 0xFF;
    r.data[11] = 0xFF;
    return r;
  }

  if(husarnet_ip6addr_aton(s, (uint8_t*)r.data.data()) == 1) {
    return r;
  }

  return IpAddress{};
}

std::string IpAddress::str() const
{
  if(isMappedV4()) {
    return std::to_string(data[12]) + "." + std::to_string(data[13]) + "." +
           std::to_string(data[14]) + "." + std::to_string(data[15]);
  }

  std::string res;

  for(int i = 0; i < 16; i += 2) {
    size_t buf_size = 20;
    char buf[buf_size];
    snprintf(buf, buf_size, "%02x%02x", data.data()[i], data.data()[i + 1]);
    if(i != 0)
      res += ":";
    res += buf;
  }

  return res;
}