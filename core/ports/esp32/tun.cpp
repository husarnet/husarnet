// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
#include "husarnet/ports/esp32/tun.h"

#include "husarnet/ports/port.h"

#include "husarnet/logging.h"
#include "husarnet/util.h"

#include "esp_netif.h"
#include "sdkconfig.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/ip.h"
#include "lwip/ip6.h"
#include "lwip/ip6_addr.h"
#include "lwip/netdb.h"
#include "lwip/netif.h"

#include "esp_mac.h"
#include "esp_check.h"
#include "esp_netif.h"

extern "C" {
  err_t husarnet_netif_init(struct netif *netif);
  err_t husarnet_netif_input(struct pbuf *p, struct netif *inp);
  err_t husarnet_netif_output(struct netif *netif, struct pbuf *p, const ip6_addr_t *ipaddr);
}

static struct netif husarnet_netif = {0};

// TODO: switch to husarnet logger
static const char* TAG = "husarnet-tun";

err_t husarnet_netif_init(struct netif *netif) {
    esp_err_t err;

    netif->name[0] = 'h';
    netif->name[1] = 'n';

    // Derrive local MAC address from the universally
    // administered Ethernet interface address
    esp_mac_type_t mac_type = ESP_MAC_ETH;
    uint8_t mac[6];

    err = esp_read_mac(mac, mac_type);
    if (err != ESP_OK)
        ESP_RETURN_ON_ERROR(err, TAG, "Failed to read MAC address");

    err = esp_derive_local_mac(netif->hwaddr, mac);
    if (err != ESP_OK)
        ESP_RETURN_ON_ERROR(err, TAG, "Failed to derive local MAC address");

    netif->hwaddr_len = esp_mac_addr_len_get(mac_type);

    // Add IPv6 addresses
    ip6_addr_t ip6_addr = ((TunTap*)netif->state)->getIp6Addr();
    s8_t ip_idx; 
    
    netif_create_ip6_linklocal_address(netif, 1);
    err = netif_add_ip6_address(netif, &ip6_addr, &ip_idx);
    if (err != ESP_OK)
        ESP_RETURN_ON_ERROR(err, TAG, "Failed to add IPv6 address");

    netif_ip6_addr_set_state(netif, ip_idx, IP6_ADDR_PREFERRED);

    // Set MTU
    netif->mtu = 1500;

    // Set packet output function
    netif->output_ip6 = husarnet_netif_output;

    // Misc
    NETIF_SET_CHECKSUM_CTRL(netif, NETIF_CHECKSUM_ENABLE_ALL);

    ESP_LOGI(TAG, "Husarnet TUN interface initialized");

    return ERR_OK;
}

err_t husarnet_netif_input(struct pbuf *p, struct netif *inp) {
    ESP_LOGE(TAG, "Husarnet TUN interface received packet");
    
    return ESP_OK;
    //return ip_input(p, inp);
}

err_t husarnet_netif_output(struct netif *netif, struct pbuf *p, const ip6_addr_t *ipaddr) {
    ESP_LOGE(TAG, "Husarnet TUN interface sending packet");

    if (p->next != NULL) {
        ESP_LOGE(TAG, "Packet chain is not supported");
        return ESP_FAIL;
    }

    // Send packet to be processed by the Husarnet stack
    if (xQueueSend(((TunTap*)netif->state)->tunTapMsgQueue, &p, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send packet to the Husarnet stack");
        return ESP_FAIL;
    }

    // Increase packet's refcounter to prevent it from being freed
    pbuf_ref(p);

    return ESP_OK;
}

// External LwIP hook to route packets to the Husarnet interface
extern "C" struct netif* lwip_hook_ip6_route(const ip6_addr_t *src, const ip6_addr_t *dest)
{
    LWIP_UNUSED_ARG(src);

    // Check if the destination address is a Husarnet address
    if (PP_NTOHL(dest->addr[0]) >> 16 == 0xFC94)
        return &husarnet_netif;

    return NULL;
}

TunTap::TunTap(ip6_addr_t ipAddr, size_t queueSize): ipAddr(ipAddr)
{
    // Create message queue for pbuf packets
    tunTapMsgQueue = xQueueCreate(queueSize, sizeof(pbuf*));
    if (tunTapMsgQueue == NULL) {
        ESP_LOGE(TAG, "Failed to create message queue");
        abort();
    }

    // Create and setup Husarnet network interface
    if (netif_add_noaddr(&husarnet_netif, (void*) this, husarnet_netif_init, husarnet_netif_input) == NULL) {
        ESP_LOGE(TAG, "Failed to add Husarnet TUN interface");
        abort();
    }
    
    // Bring up the interface
    netif_set_up(&husarnet_netif);

    // Create raw protocol control block used to send packets from the Husarnet stack
    pcb = raw_new(0);
    pcb->flags = RAW_FLAGS_HDRINCL;
    memcpy(pcb->local_ip.u_addr.ip6.addr, ipAddr.addr, 16);
    pcb->local_ip.type = IPADDR_TYPE_V6;
    raw_bind_netif(pcb, &husarnet_netif);
}

void TunTap::onLowerLayerData(DeviceId source, string_view data)
{
    ESP_LOGI(TAG, "Received %d bytes from %s", data.size(), ((std::string)source).c_str());

    // Input packet should contain IPv4/IPv6 header
    if (data.size() < 40) {
        ESP_LOGE(TAG, "Packet too short");
        return;
    }
    
    // Allocate pbuf to pass packet to the LwIP thread
    pbuf* p = pbuf_alloc(PBUF_RAW, data.str().length(), PBUF_RAM);
    if (p == NULL) {
        ESP_LOGE(TAG, "Failed to allocate pbuf");
        return;
    }

    memcpy(p->payload, data.data(), data.size());

    ip_addr_t ipDest = IPADDR6_INIT(0, 0, 0, 0);
    ip_addr_t ipSrc = IPADDR6_INIT(0, 0, 0, 0);
    memcpy(ipDest.u_addr.ip6.addr, data.m_data + 8, 16);
    memcpy(ipSrc.u_addr.ip6.addr, source.data(), 16);

    err_t res;
    if ((res = raw_sendto_if_src(pcb, p, &ipDest, &husarnet_netif, &ipSrc)) != 0)
        ESP_LOGE(TAG, "Failed to send packet, error: %d", res);

    pbuf_free(p);
}

// bool TunTap::isRunning() {

void TunTap::close() {
    // TODO: implement
}

void TunTap::processQueuedPackets() {
    pbuf* p;

    while (xQueueReceive(tunTapMsgQueue, &p, 0) == pdPASS) {
        // Send packet to the Husarnet stack
        string_view packet((char*)p->payload, p->len);
        LOG_WARNING("packet: %d", p->len);
        sendToLowerLayer(BadDeviceId, packet);
        LOG_WARNING("packet sent");

        pbuf_free(p);
    }
}

ip6_addr_t TunTap::getIp6Addr() {
    return ipAddr;
}
