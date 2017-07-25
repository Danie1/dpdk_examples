#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_errno.h>

#define PORT_SEND 0

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

static struct ether_addr l2fwd_ports_eth_addr[1];

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN }
};

/* basicfwd.c: Basic DPDK skeleton forwarding example. */
static void print(int received, int total_recv, int sent, int total_sent)
{
	const char clr[] = { 27, '[', '2', 'J', '\0' };
	const char topLeft[] = { 27, '[', '1', ';', '1', 'H','\0' };

	/* Clear screen and move to top left */
	printf("%s%s", clr, topLeft);

	printf("\nPort statistics ====================================");
	printf("\nPackets received: %d", received);
	printf("\nPackets sent: %d", sent);
	printf("\nAggregate statistics ===============================");
	printf("\nTotal packets received: %d", total_recv);
	printf("\nTotal packets sent: %d", total_sent);
	printf("\n====================================================\n");
}


/* basicfwd.c: Basic DPDK skeleton forwarding example. */

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static inline int
port_init(uint8_t port, struct rte_mempool *mbuf_pool)
{
	struct rte_eth_conf port_conf = port_conf_default;
	const uint16_t rx_rings = 1, tx_rings = 1;
	int retval;
	uint16_t q;

	if (port >= rte_eth_dev_count())
		return -1;

	/* Configure the Ethernet device. */
	retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
	if (retval != 0)
		return retval;

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (q = 0; q < rx_rings; q++) {
		retval = rte_eth_rx_queue_setup(port, q, RX_RING_SIZE,
				rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (q = 0; q < tx_rings; q++) {
		retval = rte_eth_tx_queue_setup(port, q, TX_RING_SIZE,
				rte_eth_dev_socket_id(port), NULL);
		if (retval < 0)
			return retval;
	}

	/* Start the Ethernet port. */
	retval = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	/* Display the port MAC address. */
	rte_eth_macaddr_get(port,&l2fwd_ports_eth_addr[port]);

	printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
		(unsigned) port,
		l2fwd_ports_eth_addr[port].addr_bytes[0],
		l2fwd_ports_eth_addr[port].addr_bytes[1],
		l2fwd_ports_eth_addr[port].addr_bytes[2],
		l2fwd_ports_eth_addr[port].addr_bytes[3],
		l2fwd_ports_eth_addr[port].addr_bytes[4],
		l2fwd_ports_eth_addr[port].addr_bytes[5]);

	/* Enable RX in promiscuous mode for the Ethernet device. */
	//rte_eth_promiscuous_enable(port);

	return 0;
}


static void mac_updating(struct rte_mbuf *m, unsigned dest_portid)
{
	struct ether_hdr *eth;
	void *tmp;

	eth = rte_pktmbuf_mtod(m, struct ether_hdr *);

	/* 02:00:00:00:00:xx */
	tmp = &eth->d_addr.addr_bytes[0];
	//*((uint64_t *)tmp) = 0xb93e10290c00;
	//*((uint64_t *)tmp) = 0xc33e10290c00;

	*((uint64_t *)tmp) = 0x000000000002;

	/* src addr */
	ether_addr_copy(&l2fwd_ports_eth_addr[0], &eth->s_addr);
}

 
struct payload
{
    int type;
    int index;
    char msg[100];
};
 
struct eth_header
{
    char dst[6];
    char src[6];
    char protocol[2];
};

static void get_packet(struct rte_mbuf **m, struct rte_mempool* mbuf_pool)
{
    unsigned int /*ch_index, */data_len;
    unsigned int ready_pkts = 0;
    //unsigned int pkt_index;
    struct rte_mbuf *bufs = NULL;
    struct payload ready_payload;
    struct payload* payload_ref;
    struct payload* payload_loc;
    struct eth_header ready_eth_header;
    struct eth_header* eth_header_ref;
    struct eth_header* eth_header_loc;

    ready_payload.type = 4;
    ready_payload.index = 546;
    rte_memcpy(ready_payload.msg, "Read Me\n\0", 9);
 
 	ready_eth_header.dst[0] = 0x00;
 	ready_eth_header.dst[1] = 0x0c;
 	ready_eth_header.dst[2] = 0x29;
 	ready_eth_header.dst[3] = 0x10;
 	ready_eth_header.dst[4] = 0x3e;
    ready_eth_header.dst[5] = 0xc3;
    
    ready_eth_header.src[0] = 0x00;
    ready_eth_header.src[1] = 0x0c;
    ready_eth_header.src[2] = 0x29;
    ready_eth_header.src[3] = 0x10;
    ready_eth_header.src[4] = 0x3e;
    ready_eth_header.src[5] = 0xb9;

    ready_eth_header.protocol[0] = 8;
    ready_eth_header.protocol[1] = 0;

	bufs = rte_pktmbuf_alloc(mbuf_pool);

    payload_ref = &ready_payload;
    eth_header_ref = &ready_eth_header;

    if (bufs != NULL)
    {
        eth_header_loc = rte_pktmbuf_append(bufs, sizeof(struct eth_header) + sizeof(struct payload));
        payload_loc = eth_header_loc + sizeof(struct eth_header);//rte_pktmbuf_append(bufs, sizeof(struct eth_header) + sizeof(struct payload));
    }

    rte_memcpy(eth_header_loc, eth_header_ref, sizeof(struct eth_header));
    rte_memcpy(payload_loc, payload_ref, sizeof(struct payload));

    *m = bufs;
}

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */
static __attribute__((noreturn)) void lcore_main(struct rte_mempool * mbuf_pool)
{
	const uint8_t nb_ports = rte_eth_dev_count();
	uint8_t port;
	int total_recv = 0, total_sent = 0;
	int i = 0;

	/*
	 * Check that the port is on the same NUMA node as the polling thread
	 * for best performance.
	 */
	for (port = 0; port < nb_ports; port++)
		if (rte_eth_dev_socket_id(port) > 0 &&
				rte_eth_dev_socket_id(port) !=
						(int)rte_socket_id())
			printf("WARNING, port %u is on remote NUMA node to "
					"polling thread.\n\tPerformance will "
					"not be optimal.\n", port);

	printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
			rte_lcore_id());

	/* Run until the application is quit or killed. */
	for (;;) {
		/* Get burst of RX packets, from first port of pair. */
		uint16_t nb_rx = 0;
		struct rte_mbuf *bufs[BURST_SIZE];
		memset(bufs, 0x0, BURST_SIZE * sizeof(struct rte_mbuf *));

		get_packet(&bufs[0], mbuf_pool);
		nb_rx += 1;
		
		uint16_t nb_tx = 0;
		// Send burst of TX packets, to second port of pair.

		for (i = 0; i < 5; i++)
		{
			nb_tx += rte_eth_tx_burst(PORT_SEND, 0,	bufs, 1);
		}

		total_sent += nb_tx;

		if (nb_tx > 0)
		{
			print(nb_rx, total_recv, nb_tx, total_sent);
		}

		/* Free any unsent packets. */
		if (unlikely(nb_tx < nb_rx)) {
			uint16_t buf = 0;
			for (buf = nb_tx; buf < nb_rx; buf++)
				rte_pktmbuf_free(bufs[buf]);
		}
	}
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int
main(int argc, char *argv[])
{
	struct rte_mempool *mbuf_pool1;
	struct rte_mempool *mbuf_pool2;

	/* Initialize the Environment Abstraction Layer (EAL). */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	argc -= ret;
	argv += ret;

	printf("SocketID: %d, CoreID: %d\n", rte_socket_id(), rte_lcore_id());

	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool1 = rte_pktmbuf_pool_create("MBUF_POOL1", NUM_MBUFS * 1,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, 0);

	if (mbuf_pool1 == NULL)// || mbuf_pool2 == NULL)
	{
		printf("%s\n", rte_strerror(rte_errno));
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	}

	/* Initialize all ports. */
	if (port_init(PORT_SEND, mbuf_pool1) != 0)
		rte_exit(EXIT_FAILURE, "Cannot init port %"PRIu8 "\n", PORT_SEND);
 
	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	/* Call lcore_main on the master core only. */
	lcore_main(mbuf_pool1);

	return 0;
}
