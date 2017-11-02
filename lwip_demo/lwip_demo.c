/**
 * Copyright (c) 2013-2017 by the author(s)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 *
 * Set up for receiving and transmitting ethernet packet with following
 * specification
 *
 * Link layer: Address Resolution Protocol (ARP) etharp
 * Internet layer: Internet Protocol (IP)
 * Transport layer: UDP, TCP
 * Application layer: Dynamic Host Configuration Protocol DHCP
 *
 * Author(s):
 *   Philipp Wagner <philipp.wagner@tum.de>
 *   Annika Fuchs   <annika.fuchs@tum.de>
 *
 */

#include <stdio.h>
#include <string.h>
#include <optimsoc-baremetal.h>
#include <optimsoc-runtime.h>

#include "lwip/init.h"

#include "lwip/debug.h"

#include "lwip/sys.h"
#include "lwip/timeouts.h"

#include "lwip/stats.h"

#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/etharp.h"

#include "lwip/pbuf.h"
#include "lwip/raw.h"
#include "netif/ethernet.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"

#include "lwip/inet_chksum.h"

/**
 *
 * Interface Options
 *
 */

#define ETH_INTERRUPT                   4
#define ETH_CTRL                        0xD0000000
#define ETH_DATA                        0xC0000000

#define ETH_DATA_ISR                    ETH_DATA + 0x00000000
#define ETH_DATA_IER                    ETH_DATA + 0x00000004
#define TDFR                            ETH_DATA + 0x00000008
#define TDFV                            ETH_DATA + 0x0000000C
#define TDFD                            ETH_DATA + 0x00000010
#define TLR                             ETH_DATA + 0x00000014
#define RDFR                            ETH_DATA + 0x00000018
#define RDFO                            ETH_DATA + 0x0000001C
#define RDFD                            ETH_DATA + 0x00000020
#define RLR                             ETH_DATA + 0x00000024
#define SRR                             ETH_DATA + 0x00000028
#define TDR                             ETH_DATA + 0x0000002C
#define RDR                             ETH_DATA + 0x00000030

#define BIT(nr)                         (1UL << (nr))
#define ETH_ISR_RECEIVE_COMPLETE_BIT    BIT(26)
#define ETH_ISR_TRANSMIT_COMPLETE_BIT   BIT(27)
#define ETH_ISR_TRANSMIT_SIZE_ERROR_BIT BIT(25)


/**
 * Ethernet Definitions
 */

int mymac[6] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc };
#define MYMACADDRESS         &mymac

/**
 * Debugging
 */

unsigned char debug_flags;
#define DEBUG                           0

/**
 * Incoming packet queue
 */
struct optimsoc_list_t *eth_rx_pbuf_queue = NULL;

#if !LWIP_DHCP
/**
 *  (manual) host IP configuration
 */
static ip4_addr_t ipaddr, netmask, gw;
static ip6_addr_t ip6addr;
#endif

/**
 * Function Instatiation/Definition
 */
void eth_mac_irq(void* arg);

uint32_t swap_uint32(uint32_t val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

/**
 * Initialisation of the FPGA
 *
 * Set up interrupt handler, timers and enables interrupts.
 *
 */

static void app_init()
{
    or1k_interrupt_handler_add(ETH_INTERRUPT, &eth_mac_irq, 0);
    or1k_interrupt_enable(ETH_INTERRUPT);

    REG32(ETH_DATA_ISR) = 0xFFFFFFFF;
    REG32(ETH_DATA_IER) = 0x0C000000;

    or1k_timer_init(1000); // Hz == 1ms Timer tickets
    or1k_timer_enable();
    or1k_interrupts_enable();

    eth_rx_pbuf_queue = optimsoc_list_init(NULL);
}

/**
 * ISR: Handle newly received Ethernet packet
 *
 * Check status bits RDFO, RLR, RDR if reception was successful, read the
 * incoming packet dtat into memory, and queue the packet in eth_rx_pbuf_queue.
 */
void eth_mac_irq(void* arg)
{
    uint32_t *eth_data = NULL;
    size_t eth_data_size = 0;

    if (!(REG32(ETH_DATA_ISR) & ETH_ISR_RECEIVE_COMPLETE_BIT)) {
        REG32(ETH_DATA_ISR) = 0xFFFFFFFF;
        return;
    }

    REG32(ETH_DATA_ISR) = 0xFFFFFFFF;

    if (REG32(RDFO) == 0) {
        return;
    }

    do {
        eth_data_size = REG32(RLR); // don't write u16_t in front!
        int des_adr = REG32(RDR);
        int i = 0;
        eth_data = calloc(eth_data_size / 4, sizeof(uint32_t));
        if (eth_data == NULL) {
            return;
        }
        for (i = 0; i < eth_data_size / 4; i++) {
            eth_data[i] = swap_uint32(REG32(RDFD));
        }

        struct pbuf* p = pbuf_alloc(PBUF_RAW, eth_data_size, PBUF_POOL);
        if (p == NULL) {
            printf("eth_mac_irq: Buffer Overflow by generating p.\n");
            free(eth_data);
            return;
        }
        err_t rv;
        rv = pbuf_take(p, (const void*) eth_data, eth_data_size);
        if (rv != ERR_OK) {
            printf("eth_mac_irq: pbuf_take() FAILED returned %d\n", rv);
            free(eth_data);
            continue;
        }
        free(eth_data);
        optimsoc_list_add_tail(eth_rx_pbuf_queue, p);
    } while (REG32(RDFO) > 0);
}

/**
 * netif_output: output handler/driver for FPGA (communicate with AXI Stream
 * FIFO
 *
 */
static err_t netif_output(struct netif *netif, struct pbuf *p)
{
    LINK_STATS_INC(link.xmit);
    MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
    int unicast = ((((uint16_t *) p->payload)[0] & 0x01) == 0);
    if (unicast) {
        MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
    } else {
        MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
    }
#if DEBUG
    printf("netif_output: Writing to Stream FIFO and start transmission.\n");
#endif
    uint32_t TDFV_before = REG32(TDFV);
    if (TDFV_before < p->tot_len) {
        printf("netif_output: Not enough memory in the AXI Stream FIFO.\n");
        return ERR_MEM;
    }

    uint32_t store = or1k_critical_begin();
    REG32(TDR) = (uint32_t) 0x00000002;
    uint32_t left, tmp_len;
    uint32_t buf_p = 0x0;
    struct pbuf *q;

    /**
     * To access q->payload we use a typcast to uint16_t, because of restricted
     * alignment options.
     */

    for (q = p; q != NULL; q = q->next) {
        for (left = 0;
                (left <= (int) ((q->len) / 2) - ((p->tot_len % 4) == 0 ? 1 : 0));
                left = left + 2) {
            buf_p = ((uint16_t *) q->payload)[left];
            buf_p = buf_p << 16;
            buf_p = buf_p | ((uint16_t *) q->payload)[left + 1];
            REG32(TDFD) = swap_uint32(buf_p);
        }
    }

    uint32_t TDFV_after = REG32(TDFV);
    REG32(TLR) = p->tot_len;

    uint32_t ISR_wait = REG32(ETH_DATA_ISR);

    while (1) {
        ISR_wait = REG32(ETH_DATA_ISR);
        if (ISR_wait & ETH_ISR_TRANSMIT_COMPLETE_BIT) {
            break;
        }
        if ((ISR_wait & ETH_ISR_TRANSMIT_SIZE_ERROR_BIT)) {
            printf("Transmit Size Error\n");
            return ERR_VAL;
        }
    };

    REG32(ETH_DATA_ISR) = 0xFFFFFFFF;
    uint32_t TDFV_V = REG32(TDFV);
    or1k_critical_end(store);
    return ERR_OK;
}

/**
 * netif_status_callback: callback function if netif status changes
 */

static void netif_status_callback(struct netif *netif)
{
#if LWIP_DHCP
    printf("dhcp_supplied_address: %i\n", dhcp_supplied_address(netif));
#endif
    printf("netif status changed %s\n", ip4addr_ntoa(netif_ip4_addr(netif)));
}

/**
 * lwip_stack_init: Initialization function for network interface netif
 */
static err_t lwip_stack_init(struct netif *netif)
{
    netif->linkoutput = netif_output;
    netif->output = etharp_output;
    netif->output_ip6 = ethip6_output;
    netif->mtu = ETHERNET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP
            | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 100000000);

    SMEMCPY(netif->hwaddr, MYMACADDRESS, sizeof(netif->hwaddr));
    netif->hwaddr_len = sizeof(netif->hwaddr);
    return ERR_OK;
}

void init()
{

}

#if LWIP_DHCP
/**
 * DHCP Init
 */
void dhcp_init(struct netif *netif)
{
    err_t error_dhcp;
    /* Start DHCP and HTTPD */
    error_dhcp = dhcp_start(netif);
    if (error_dhcp != ERR_OK) {
        printf("DHCP Error occurred - out of memory.\n");
        return;
    }
    printf("DHCP started.\n");
    u8_t myip_dhcp;
    myip_dhcp = dhcp_supplied_address(netif);
}
#endif // LWIP_DHCP

void main(void)
{
    struct netif netif;
    struct netif *netif_control;
    err_t err_ipv6_netif;

    app_init();
    lwip_init();
#if !LWIP_DHCP
    // IP Address for example packets
    IP4_ADDR(&gw, 192, 168, 100, 1);
    IP4_ADDR(&ipaddr, 192, 168, 100, 114);
    IP6_ADDR(&ip6addr, 1234, 5666, 1914, 5111);
    IP4_ADDR(&netmask, 255, 255, 255, 0);

    netif_control = netif_add(&netif, &ipaddr, &netmask, &gw, NULL,
            lwip_stack_init, netif_input);

    if (netif_control == NULL) {
        printf("netif_add_ip6_address failed.\n");
    }

    err_ipv6_netif = netif_add_ip6_address(&netif, &ip6addr, NULL);
    if (err_ipv6_netif != ERR_OK) {
        printf("netif_add_ip6_address failed.\n");
    }
#else
    netif_add(&netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4, NULL,
              lwip_stack_init, netif_input);
#endif
    strncpy(netif.name, "e0", 2);

    netif_create_ip6_linklocal_address(&netif, 1);
    netif.ip6_autoconfig_enabled = 1;

    netif_set_status_callback(&netif, netif_status_callback);
    netif_set_default(&netif);
    netif_set_up(&netif);

#if LWIP_DHCP
    dhcp_init(&netif);
#endif // LWIP_DHCP

    // All initialization done, we're ready to receive data
    printf("main: Reset done, Init done, Interrupts enabled\n");

    optimsoc_list_iterator_t iter = 0;

#if LWIP_UDP
    static struct udp_pcb *udpecho_raw_pcb;
    udp_init();
    udpecho_raw_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    udp_recv(udpecho_raw_pcb, NULL, NULL);
    udp_bind_netif(udpecho_raw_pcb, &netif);
#endif // LWIP_UDP

#if LWIP_DEBUG
    debug_flags |= (LWIP_DBG_ON|LWIP_DBG_TRACE|LWIP_DBG_STATE|LWIP_DBG_FRESH|LWIP_DBG_HALT);
#endif //LWIP_DEBUG

#if LWIP_TCP
    static struct tcp_pcb *tcpecho_raw_pcb;
    tcpecho_raw_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (tcpecho_raw_pcb != NULL) {
        tcp_bind(tcpecho_raw_pcb, IP_ANY_TYPE, 2049);
        tcp_bind_netif(tcpecho_raw_pcb, &netif);
    }
#endif // LWIP_TCP

    int T_en = 0;
    while (1) {
        // TODO: Check link status

        if (optimsoc_list_length(eth_rx_pbuf_queue) != 0) {

            uint32_t restore = or1k_critical_begin();
            struct pbuf* p = (struct pbuf*) optimsoc_list_remove_head(
                    eth_rx_pbuf_queue);
            or1k_critical_end(restore);

            LINK_STATS_INC(link.recv);

            MIB2_STATS_NETIF_ADD(&netif, ifoutoctets, p->tot_len);
            int unicast = ((((uint16_t *) p->payload)[0] & 0x01) == 0);
            if (unicast) {
                MIB2_STATS_NETIF_INC(&netif, ifoutucastpkts);
            } else {
                MIB2_STATS_NETIF_INC(&netif, ifoutnucastpkts);
            }
            // TODO: activate the checksum test

            if (netif.input(p, &netif) != ERR_OK) {
                pbuf_free(p);
            }
        }

        /**
         * The busy for loop is only necassary to get a clean printf.
         */
#if DEBUG
        for (int i = 0; i <= 1000; i++)
            ; // For loop (busy waiting)
#endif

        /* Cyclic lwIP timers check */
        sys_check_timeouts();
    }
}
