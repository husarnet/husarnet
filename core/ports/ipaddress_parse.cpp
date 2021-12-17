#include "port.h"
// Copied from LwIP core/ipv6/ip6_addr.c

using u32_t = uint32_t;

/**
 * Check whether "cp" is a valid ascii representation
 * of an IPv6 address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 *
 * @param cp IPv6 address in ascii representation (e.g. "FF01::1")
 * @param addr pointer to which to save the ip address in network order
 * @return 1 if cp could be converted to addr, 0 on failure
 */
int husarnet_ip6addr_aton(const char* cp, uint8_t* result) {
  uint32_t addr[4];
  uint32_t addr_index, zero_blocks, current_block_index, current_block_value;
  const char* s;

  /* Count the number of colons, to count the number of blocks in a "::"
     sequence zero_blocks may be 1 even if there are no :: sequences */
  zero_blocks = 8;
  for (s = cp; *s != 0; s++) {
    if (*s == ':') {
      zero_blocks--;
    } else if (!isxdigit(*s)) {
      break;
    }
  }

  /* parse each block */
  addr_index = 0;
  current_block_index = 0;
  current_block_value = 0;
  for (s = cp; *s != 0; s++) {
    if (*s == ':') {
      if (current_block_index & 0x1) {
        addr[addr_index++] |= current_block_value;
      } else {
        addr[addr_index] = current_block_value << 16;
      }
      current_block_index++;
      current_block_value = 0;
      if (current_block_index > 7) {
        /* address too long! */
        return 0;
      }
      if (s[1] == ':') {
        if (s[2] == ':') {
          /* invalid format: three successive colons */
          return 0;
        }
        s++;
        /* "::" found, set zeros */
        while (zero_blocks > 0) {
          zero_blocks--;
          if (current_block_index & 0x1) {
            addr_index++;
          } else {
            addr[addr_index] = 0;
          }
          current_block_index++;
          if (current_block_index > 7) {
            /* address too long! */
            return 0;
          }
        }
      }
    } else if (isxdigit(*s)) {
      /* add current digit */
      current_block_value =
          (current_block_value << 4) +
          (isdigit(*s) ? (u32_t)(*s - '0')
                       : (u32_t)(10 + (islower(*s) ? *s - 'a' : *s - 'A')));
    } else {
      /* unexpected digit, space? CRLF? */
      break;
    }
  }

  if (current_block_index & 0x1) {
    addr[addr_index++] |= current_block_value;
  } else {
    addr[addr_index] = current_block_value << 16;
  }

  /* convert to network byte order. */
  for (addr_index = 0; addr_index < 4; addr_index++) {
    addr[addr_index] = htonl(addr[addr_index]);
  }

  if (current_block_index != 7) {
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
int husarnet_ip4addr_aton(const char* cp, uint8_t* result) {
  uint32_t val;
  char c;
  uint32_t parts[4];
  uint32_t* pp = parts;

  c = *cp;
  for (;;) {
    /*
     * Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, 1-9=decimal.
     */
    if (!isdigit(c)) {
      return 0;
    }
    val = 0;
    uint8_t base = 10;
    if (c == '0') {
      c = *++cp;
      if (c == 'x' || c == 'X') {
        base = 16;
        c = *++cp;
      } else {
        base = 8;
      }
    }
    for (;;) {
      if (isdigit(c)) {
        val = (val * base) + (u32_t)(c - '0');
        c = *++cp;
      } else if (base == 16 && isxdigit(c)) {
        val = (val << 4) | (u32_t)(c + 10 - (islower(c) ? 'a' : 'A'));
        c = *++cp;
      } else {
        break;
      }
    }
    if (c == '.') {
      /*
       * Internet format:
       *  a.b.c.d
       *  a.b.c   (with c treated as 16 bits)
       *  a.b (with b treated as 24 bits)
       */
      if (pp >= parts + 3) {
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
  if (c != '\0' && !isspace(c)) {
    return 0;
  }
  /*
   * Concoct the address according to
   * the number of parts specified.
   */
  switch (pp - parts + 1) {
    case 0:
      return 0; /* initial nondigit */

    case 1: /* a -- 32 bits */
      break;

    case 2: /* a.b -- 8.24 bits */
      if (val > 0xffffffUL) {
        return 0;
      }
      if (parts[0] > 0xff) {
        return 0;
      }
      val |= parts[0] << 24;
      break;

    case 3: /* a.b.c -- 8.8.16 bits */
      if (val > 0xffff) {
        return 0;
      }
      if ((parts[0] > 0xff) || (parts[1] > 0xff)) {
        return 0;
      }
      val |= (parts[0] << 24) | (parts[1] << 16);
      break;

    case 4: /* a.b.c.d -- 8.8.8.8 bits */
      if (val > 0xff) {
        return 0;
      }
      if ((parts[0] > 0xff) || (parts[1] > 0xff) || (parts[2] > 0xff)) {
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
