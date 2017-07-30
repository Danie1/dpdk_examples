#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_errno.h>
#include <definitions.h>

#define RX_RING_SIZE (128 * 30)
#define TX_RING_SIZE 512

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 350
#define BURST_SIZE (1300)
#define REFRESH_TIME (500) //in ms

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN }
};

void ParsePacket(struct rte_mbuf* buf);
void HandleBurst(struct rte_mbuf *bufs[BURST_SIZE], uint16_t nb_rx);
void Init(void);

struct ports_statistics stats;

/* basicfwd.c: Basic DPDK skeleton forwarding example. */
static void print(int received, int total)
{
	const char clr[] = { 27, '[', '2', 'J', '\0' };
	const char topLeft[] = { 27, '[', '1', ';', '1', 'H','\0' };

	/* Clear screen and move to top left */
	printf("%s%s", clr, topLeft);

	printf("\nPort statistics ====================================");
	printf("\nPackets received: %d", received);
	printf("\nAggregate statistics ===============================");
	printf("\nTotal packets received: %d", total);
	printf("\n====================================================\n");
}

/* basicfwd.c: Basic DPDK skeleton forwarding example. */
static void PrintStatistics(void)
{
	const char clr[] = { 27, '[', '2', 'J', '\0' };
	const char topLeft[] = { 27, '[', '1', ';', '1', 'H','\0' };
	uint32_t sum_received = 0;

	/* Clear screen and move to top left */
	printf("%s%s", clr, topLeft);

	int i = 0;

	printf("====================================================\n");
	printf("%-14s | %9s | %9s | %9s |\n", "Type #", "Current", "Total", "Dropped");
	printf("====================================================\n");

	for (i = 0; i < NUM_OF_MSG_TYPES; i++)
	{
		printf("Type #%-8d | %9d | %9d | %9d |\n", i, stats.current[i].value, stats.total[i].value, stats.dropped[i].value);
		sum_received += stats.total[i].value;
	}

	printf("====================================================\n");
	printf("Total packets received: %d\n", sum_received);
	printf("====================================================\n");
}

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
	{
		printf("%d\n", retval);
		return retval;
	}

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
	struct ether_addr addr;
	rte_eth_macaddr_get(port, &addr);
	printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
			   " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
			(unsigned)port,
			addr.addr_bytes[0], addr.addr_bytes[1],
			addr.addr_bytes[2], addr.addr_bytes[3],
			addr.addr_bytes[4], addr.addr_bytes[5]);

	/* Enable RX in promiscuous mode for the Ethernet device. */
	//rte_eth_promiscuous_enable(port);

	return 0;
}

void Init(void)
{
	int i = 0;

	for (i = 0; i < NUM_OF_MSG_TYPES; i++)
	{
		stats.total[i].value = 0;
		stats.dropped[i].value = 0;
		stats.current[i].value = 0;
	}	
}

void ParsePacket(struct rte_mbuf* buf)
{
	struct payload p = *(struct payload*)((char*)buf->buf_addr + 128 + 14);
	
	// Validate msg index
	if (p.type > NUM_OF_MSG_TYPES)
	{
		stats.dropped[p.index].value += 1;
		return;
	}

	stats.current[p.type].value += 1;
	stats.total[p.type].value +=  1;
}

void HandleBurst(struct rte_mbuf *bufs[BURST_SIZE], uint16_t nb_rx)
{
	int i = 0;
	
	for (i = 0; i < nb_rx; i++)
	{
		ParsePacket(bufs[i]);
	}
}

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */
static __attribute__((noreturn)) void
lcore_main(void)
{
	//uint32_t total = 0;
	clock_t curr_time, last_time;
	unsigned int time_passed;

	/*
	 * Check that the port is on the same NUMA node as the polling thread
	 * for best performance.
	 */
	
	if (rte_eth_dev_socket_id(PORT_RECV) > 0 &&
			rte_eth_dev_socket_id(PORT_RECV) !=
					(int)rte_socket_id())
		printf("WARNING, port %u is on remote NUMA node to "
				"polling thread.\n\tPerformance will "
				"not be optimal.\n", PORT_RECV);

	printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n",
			rte_lcore_id());

	/* Run until the application is quit or killed. */
	for (;;) {
		/* Get burst of RX packets, from first port of pair. */
		curr_time = clock();
		time_passed = ((curr_time - last_time) * 1000) / CLOCKS_PER_SEC;

		/* Get burst of RX packets, from first port of pair. */
		struct rte_mbuf *bufs[BURST_SIZE];
		const uint16_t nb_rx = rte_eth_rx_burst(PORT_RECV, 0, bufs, BURST_SIZE);

		HandleBurst(bufs, nb_rx);

		//total += nb_rx;

		if ((time_passed > REFRESH_TIME) && nb_rx > 0)
		{
			PrintStatistics();
			last_time = curr_time;
		}

		if (unlikely(nb_rx == 0))
			continue;

		/* Free any unsent packets. */
		uint16_t msg_index;
		for (msg_index = 0; msg_index < nb_rx; msg_index++)
			rte_pktmbuf_free(bufs[msg_index]);
		}

}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int
main(int argc, char *argv[])
{
	struct rte_mempool *mbuf_pool;

	/* Initialize the Environment Abstraction Layer (EAL). */
	int ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");

	argc -= ret;
	argv += ret;

	Init();

	printf("SocketID: %d, CoreID: %d\n", rte_socket_id(), rte_lcore_id());
	
	
	// Creates a new mempool in memory to hold the mbufs. 
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL_RECV", NUM_MBUFS * 3,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

	if (mbuf_pool == NULL)
	{
		printf("%s\n", rte_strerror(rte_errno));
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");
	}

	// Initialize all ports.
	if (port_init(PORT_RECV, mbuf_pool) != 0)
		rte_exit(EXIT_FAILURE, "Cannot init port %d\n", PORT_RECV);
 
	if (rte_lcore_count() > 1)
		printf("\nWARNING: Too many lcores enabled. Only 1 used.\n");

	/* Call lcore_main on the master core only. */
	lcore_main();

	return 0;
}
