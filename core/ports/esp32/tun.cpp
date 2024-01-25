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
    const ip6_addr_t ipv6 = {
        .addr = {
            PP_HTONL(0xfc947248),
            PP_HTONL(0x150c4dd8),
            PP_HTONL(0x5bd5639a),
            PP_HTONL(0x898b818f)
        }
    };
    s8_t ip_idx; 
    
    netif_create_ip6_linklocal_address(netif, 1);
    err = netif_add_ip6_address(netif, &ipv6, &ip_idx);
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

TunTap::TunTap(size_t queueSize)
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
}

void TunTap::onLowerLayerData(DeviceId source, string_view data)
{
    ESP_LOGI(TAG, "Received %d bytes from %s", data.str().length(), ((std::string)source).c_str());
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

        pbuf_free(p);
    }
}
