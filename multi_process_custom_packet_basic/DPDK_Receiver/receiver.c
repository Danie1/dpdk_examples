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
#define LCORE_MASTER (0)
#define LCORE_SLAVE (1)
#define NUM_OF_LCORES (2)

unsigned int lcore_index;

#define MULTIPLE_PRINTF_WITH_LCORE_DATA(str, ...)													\
								for(lcore_index = 0; lcore_index < NUM_OF_LCORES; lcore_index++)	\
								{																	\
									printf(str, ##__VA_ARGS__[lcore_index]);						\
								}																	\
								printf("\n")

#define MULTIPLE_PRINTF_WITHOUT_LCORE_DATA(str, ...)												\
								for(lcore_index = 0; lcore_index < NUM_OF_LCORES; lcore_index++)	\
								{																	\
									printf(str, ##__VA_ARGS__);										\
								}																	\
								printf("\n")

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN }
};

void ParsePacket(struct rte_mbuf* buf);
void HandleBurst(struct rte_mbuf *bufs[BURST_SIZE], uint16_t nb_rx);
void Init(void);

static struct ports_statistics stats[NUM_OF_LCORES];
static struct ports_statistics slave_stats[NUM_OF_LCORES];

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
	unsigned int lcore_ids[] = {0, 1, 2};
	unsigned lcore_id = rte_lcore_id();
	struct ports_statistics stable_copy_of_stats[NUM_OF_LCORES];

	for (lcore_index = 0; lcore_index < NUM_OF_LCORES; lcore_index++)
	{
		stable_copy_of_stats[lcore_index] = CopyPortsStatics(stats[lcore_index]);
	}

	/* Clear screen and move to top left */
	printf("%s%s", clr, topLeft);

	int i = 0;

	MULTIPLE_PRINTF_WITHOUT_LCORE_DATA("====================================================");
	MULTIPLE_PRINTF_WITH_LCORE_DATA("|%30s%-20u|", "lcore #", lcore_ids);
	MULTIPLE_PRINTF_WITHOUT_LCORE_DATA("====================================================");
	MULTIPLE_PRINTF_WITHOUT_LCORE_DATA("|%-13s | %9s | %9s | %9s |", "Type #", "Current", "Total", "Dropped");
	MULTIPLE_PRINTF_WITHOUT_LCORE_DATA("====================================================");

	for (i = 0; i < NUM_OF_MSG_TYPES; i++)
	{
		for (lcore_index = 0; lcore_index < NUM_OF_LCORES; lcore_index++)
		{
			printf("|Type #%-7d | %9d | %9d | %9d |",	i,
														stable_copy_of_stats[lcore_index].current[i].value,
														stable_copy_of_stats[lcore_index].total[i].value,
														stable_copy_of_stats[lcore_index].dropped[i].value);
			
			sum_received += stable_copy_of_stats[lcore_index].total[i].value;
		}
		printf("\n");
	}

	MULTIPLE_PRINTF_WITHOUT_LCORE_DATA("====================================================");
	printf("|Total packets received: %d\n", sum_received);
	MULTIPLE_PRINTF_WITHOUT_LCORE_DATA("====================================================");
	for (lcore_index = 0; lcore_index < NUM_OF_LCORES; lcore_index++)
	{
		printf("|lcore #%u with stable_copy_of_stats.total[%u] = %u\n", lcore_index, lcore_id, stable_copy_of_stats[lcore_index].total[0].value);
	}

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
	unsigned lcore_id = rte_lcore_id();

	for (i = 0; i < NUM_OF_MSG_TYPES; i++)
	{
		stats[lcore_id].total[i].value = 0;
		stats[lcore_id].dropped[i].value = 0;
		stats[lcore_id].current[i].value = 0;
	}	
}

void ParsePacket(struct rte_mbuf* buf)
{
	struct payload p = *(struct payload*)((char*)buf->buf_addr + 128 + 14);
	unsigned lcore_id = rte_lcore_id();

	// Validate msg index
	if (p.type > NUM_OF_MSG_TYPES)
	{
		stats[lcore_id].dropped[p.index].value += 1;
		return;
	}

	stats[lcore_id].current[p.type].value += 1;
	stats[lcore_id].total[p.type].value +=  1;
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
	unsigned lcore_id = rte_lcore_id();

	printf("started lcore #%u", lcore_id);

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
			lcore_id);

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

		stats[lcore_id].total[0].value += 1;

		if (lcore_id == LCORE_MASTER && (time_passed > REFRESH_TIME) && nb_rx > 0)
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
	unsigned lcore_id, lcore_slave_id;

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

	RTE_LCORE_FOREACH_SLAVE(lcore_slave_id)
	{
		rte_eal_remote_launch(lcore_main, NULL, lcore_slave_id);
	}

	/* Call lcore_main on the master core only. */
	lcore_main();

	return 0;
}
