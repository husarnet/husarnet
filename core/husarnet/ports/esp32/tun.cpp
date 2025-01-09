// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

// Husarnet TUN adapter implementation for the ESP32 platform consists of three
// abstraction layers (from the bottom): LwIP, ESP-NETIF and TunTap.
//
// Most of the code is related to the LwIP layer, which is responsible for
// routing packets. The ESP-NETIF layer serves only as a compatibility layer
// between the LwIP and ESP-IDF framework.
// Some parts of ESP-IDF are not directly compatible with LwIP netif and expect
// every network interface to be registered within the ESP-NETIF framework.
// Outgoing packets are handled via ip_route hook and passed to the Husarnet
// stack via a pbuf queue.
// Incoming packets are processed by the Husarnet stack directly and are
// outputted to the network stack by the TunTap::onLowerLayerData() function.

#include "husarnet/ports/esp32/tun.h"

#include "husarnet/ports/port.h"

#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "esp_check.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_net_stack.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/esp_netif_net_stack.h"
#include "lwip/ip.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/netdb.h"
#include "lwip/netif.h"
#include "lwip/sys.h"

typedef struct {
  esp_netif_driver_base_t base;
  TunTap* tunTap;
} husarnet_esp_netif_driver_t;

// --- LwIP layer ---

extern "C" {
err_t husarnet_netif_init(struct netif* netif);
err_t husarnet_netif_input(struct pbuf* p, struct netif* inp);
err_t husarnet_netif_output(
    struct netif* netif,
    struct pbuf* p,
    const ip6_addr_t* ipaddr);
}

static struct netif* husarnet_netif;

err_t husarnet_netif_init(struct netif* netif)
{
  esp_err_t err;

  netif->name[0] = 'h';
  netif->name[1] = 'n';

  // Derrive local MAC address from the universally
  // administered Ethernet interface address
  esp_mac_type_t mac_type = ESP_MAC_ETH;
  uint8_t mac[8];

  err = esp_read_mac(mac, mac_type);
  if(err != ESP_OK) {
    LOG_ERROR("Failed to read MAC address");
    return err;
  }

  err = esp_derive_local_mac(netif->hwaddr, mac);
  if(err != ESP_OK) {
    LOG_ERROR("Failed to derive local MAC address");
    return err;
  }

  netif->hwaddr_len = 6;

  netif_create_ip6_linklocal_address(netif, 1);

  // Set MTU
  netif->mtu = 1500;

  // Set packet output function
  netif->output_ip6 = husarnet_netif_output;

  // Misc
  NETIF_SET_CHECKSUM_CTRL(netif, NETIF_CHECKSUM_ENABLE_ALL);

  LOG_INFO("Husarnet TUN interface initialized");

  return ERR_OK;
}

// Noop, Husarnet stack processes incoming packets directly
void husarnet_netif_input(void* netif, void* buffer, size_t len, void* eb)
{
  return;
}

err_t husarnet_netif_output(
    struct netif* netif,
    struct pbuf* p,
    const ip6_addr_t* ipaddr)
{
  if(p->next != NULL) {
    LOG_ERROR("Packet chain is not supported");
    return ESP_FAIL;
  }

  // Send packet to be processed by the Husarnet stack
  auto driver = static_cast<husarnet_esp_netif_driver_t*>(
      esp_netif_get_io_driver(esp_netif_get_handle_from_netif_impl(netif)));
  if(xQueueSend(driver->tunTap->tunTapMsgQueue, &p, 0) != pdPASS) {
    LOG_ERROR("Failed to send packet to the Husarnet stack");
    return ESP_FAIL;
  }

  // Increase packet's refcounter to prevent it from being freed
  pbuf_ref(p);

  return ESP_OK;
}

// External LwIP hook to route packets to the Husarnet interface
extern "C" struct netif* lwip_hook_ip6_route(
    const ip6_addr_t* src,
    const ip6_addr_t* dest)
{
  LWIP_UNUSED_ARG(src);

  // Check if the destination address is a Husarnet address
  if(PP_NTOHL(dest->addr[0]) >> 16 == 0xFC94)
    return husarnet_netif;

  return NULL;
}

// --- ESP-NETIF layer ---

const esp_netif_inherent_config_t husarnet_esp_netif_base_config = {
    .flags = ESP_NETIF_FLAG_AUTOUP,
    .if_key = "hn",
    .if_desc = "Husarnet",
    .route_prio = 0,  // Do not route packets through the Husarnet interface
};

const struct esp_netif_netstack_config husarnet_esp_netif_netstack_config = {
    .lwip = {
        .init_fn = husarnet_netif_init,
        .input_fn = husarnet_netif_input,
    }};

const esp_netif_config_t husarnet_netif_config = {
    .base = &husarnet_esp_netif_base_config,
    .driver = NULL,
    .stack = &husarnet_esp_netif_netstack_config,
};

// Following IO Driver functions are noops, as this is a virtual interface
// that does not output packets to the physical layer. They are implemented
// only to retain compatibility with the ESP-NETIF.

static void husarnet_esp_netif_free_rx_buffer(void* h, void* buffer)
{
  return;
}

static esp_err_t husarnet_esp_netif_transmit(void* h, void* buffer, size_t len)
{
  return ESP_OK;
}

static esp_err_t husarnet_esp_netif_post_attach(
    esp_netif_t* esp_netif,
    void* args)
{
  husarnet_esp_netif_driver_t* driver =
      static_cast<husarnet_esp_netif_driver_t*>(args);

  const esp_netif_driver_ifconfig_t driver_ifconfig = {
      .handle = driver,
      .transmit = husarnet_esp_netif_transmit,
      .driver_free_rx_buffer = husarnet_esp_netif_free_rx_buffer,
  };

  driver->base.netif = esp_netif;

  esp_err_t err = esp_netif_set_driver_config(esp_netif, &driver_ifconfig);
  esp_netif_action_start(driver->base.netif, 0, 0, NULL);

  return err;
}

static husarnet_esp_netif_driver_t* husarnet_esp_netif_driver_init()
{
  husarnet_esp_netif_driver_t* driver =
      static_cast<husarnet_esp_netif_driver_t*>(
          calloc(1, sizeof(husarnet_esp_netif_driver_t)));
  return driver;
}

// --- TunTap layer ---

TunTap::TunTap(ip6_addr_t ipAddr, size_t queueSize) : ipAddr(ipAddr)
{
  // Create message queue for pbuf packets
  tunTapMsgQueue = xQueueCreate(queueSize, sizeof(pbuf*));
  if(tunTapMsgQueue == NULL) {
    LOG_ERROR("Failed to create message queue");
    abort();
  }

  // Create and setup Husarnet network interface
  esp_netif_t* husarnet_esp_netif = esp_netif_new(&husarnet_netif_config);
  if(husarnet_esp_netif == NULL) {
    LOG_ERROR("Failed to create Husarnet ESP-NETIF interface");
    abort();
  }

  husarnet_netif =
      static_cast<netif*>(esp_netif_get_netif_impl(husarnet_esp_netif));

  husarnet_esp_netif_driver_t* driver = husarnet_esp_netif_driver_init();
  driver->tunTap = this;
  if(driver == NULL) {
    LOG_ERROR("Failed to create Husarnet ESP-NETIF driver");
    abort();
  }

  driver->base.post_attach = husarnet_esp_netif_post_attach;
  if(esp_netif_attach(husarnet_esp_netif, driver) != ESP_OK) {
    LOG_ERROR("Failed to attach Husarnet ESP-NETIF interface");
    abort();
  }

  // Add IPv6 address
  static_assert(
      sizeof(esp_ip6_addr_t) == sizeof(ip6_addr_t),
      "ESP-NETIF and LwIP IPv6 address size mismatch");

  esp_ip6_addr_t ip6_addr_esp;
  memcpy(&ip6_addr_esp, &(this->ipAddr), sizeof(esp_ip6_addr_t));
  if(esp_netif_add_ip6_address(husarnet_esp_netif, ip6_addr_esp, true) !=
     ESP_OK) {
    LOG_ERROR("Failed to add IPv6 address");
    abort();
  }

  // Create a netconn connection used to send packets from the Husarnet stack
  this->conn = netconn_new(NETCONN_RAW_IPV6);

  ip_addr_t ipSrc = IPADDR6_INIT(0, 0, 0, 0);
  memcpy(ipSrc.u_addr.ip6.addr, this->getIp6Addr().addr, 16);
  if(netconn_bind(this->conn, &ipSrc, 0) != ERR_OK) {
    LOG_ERROR("Failed to bind netconn");
    abort();
  }

  int netifIdx = esp_netif_get_netif_impl_index(husarnet_esp_netif);
  if(netconn_bind_if(this->conn, static_cast<u8_t>(netifIdx)) != ERR_OK) {
    LOG_ERROR("Failed to bind netconn");
    abort();
  }

  // Mark the connection as raw with IP headers included in the packet payload
  this->conn->pcb.raw->flags = RAW_FLAGS_HDRINCL;
}

void TunTap::onLowerLayerData(DeviceId source, string_view data)
{
  // Input packet should contain IPv4/IPv6 header
  if(data.size() < 40) {
    LOG_ERROR("Packet too short");
    return;
  }

  // Easiest way to sent a packet with IP headers included (format used
  // internally) would be to use BSD style sockets with IPV6_HDRINCL option.
  // This is not supported by LwIP, so we need to use netconn or raw API.
  struct netbuf* buf = netbuf_new();

  if(netbuf_ref(buf, data.data(), data.size()) != ERR_OK) {
    LOG_ERROR("Failed to create netbuf");
    return;
  }

  ip_addr_t ipDest = IPADDR6_INIT(0, 0, 0, 0);
  memcpy(ipDest.u_addr.ip6.addr, data.data() + 8, 16);
  if(netconn_sendto(this->conn, buf, &ipDest, 0) != ERR_OK) {
    LOG_ERROR("Failed to send packet");
    return;
  }

  netbuf_delete(buf);
}

void TunTap::close()
{
  // TODO: implement
}

void TunTap::processQueuedPackets()
{
  pbuf* p;

  while(xQueueReceive(tunTapMsgQueue, &p, 0) == pdPASS) {
    // Send packet to the Husarnet stack
    string_view packet((char*)p->payload, p->len);
    sendToLowerLayer(BadDeviceId, packet);

    pbuf_free(p);
  }
}

ip6_addr_t TunTap::getIp6Addr()
{
  return ipAddr;
}
