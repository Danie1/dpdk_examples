#include <stdint.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_errno.h>
#include <time.h>
#include <stdlib.h>

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

#define MAX_PKT_COUNT 0xffff
#define GET_RANDOM_BETWEEN(start, end) ((rand() % (end - start)) + start)
#define REFRESH_TIME (50) //in ms
#define PKT_COUNTER_RESET_TIME (50) //in ms
#define NUM_OF_TYPES (16)
#define PACKET_LIMIT (1000000000)









/* basicfwd.c: Basic DPDK skeleton forwarding example. */
static void print(	unsigned int pkts_counted, unsigned int pkts_dropped_counted,
					unsigned int total_sent, unsigned int total_dropped,
					unsigned int pkts_type_counter[], unsigned int pkts_type_counter_overall[],
					unsigned int pkts_dropped_type_counter[], unsigned int pkts_dropped_type_counter_overall[],
					float time_passed_overall)
{
	const char clr[] = { 27, '[', '2', 'J', '\0' };
	const char topLeft[] = { 27, '[', '1', ';', '1', 'H','\0' };
	unsigned int type_index;
	unsigned int total_by_types[4];

	for (type_index = 0; type_index < NUM_OF_TYPES; type_index++)
	{
		//Puts zeroes before first use
		if (type_index == 0)
		{
			total_by_types[0] = 0;
			total_by_types[1] = 0;
			total_by_types[2] = 0;
			total_by_types[3] = 0;
		}

		total_by_types[0] += pkts_type_counter[type_index];
		total_by_types[1] += pkts_dropped_type_counter[type_index];
		total_by_types[2] += pkts_type_counter_overall[type_index];
		total_by_types[3] += pkts_dropped_type_counter_overall[type_index];
	}

	/* Clear screen and move to top left */
	printf("%s%s", clr, topLeft);

	printf("\n=================================================================================");
	printf("\n|Pkts counted since last check : %d pkts", pkts_counted);
	printf("\n|check time : %.2fs", (double)PKT_COUNTER_RESET_TIME/1000);
	printf("\n|Pkts/s since last check : %d pkts/s",
				(pkts_counted * 1000) / PKT_COUNTER_RESET_TIME);
	printf("\n|running time : %.2f", time_passed_overall/1000);
	printf("\n=================================================================================");
	
	for (type_index = 0; type_index < NUM_OF_TYPES; type_index++)
	{
		printf("\n|type num %u\t|\t%u\t|\t%u\t|\t%u\t|\t%u\t|",
					type_index,
					pkts_type_counter[type_index],
					pkts_dropped_type_counter[type_index],
					pkts_type_counter_overall[type_index],
					pkts_dropped_type_counter_overall[type_index]
					);
	}
	printf("\n=================================================================================");
	printf("\n|Total by types\t|\t%u\t|\t%u\t|\t%u\t|\t%u\t|",
					pkts_counted,
					pkts_dropped_counted,
					total_sent,
					total_dropped
					);
	printf("\n=================================================================================");
	printf("\n|Total\t\t|\t%u\t|\t%u\t|\t%u\t|\t%u\t|",
					pkts_counted,
					pkts_dropped_counted,
					total_sent,
					total_dropped
					);
	printf("\n=================================================================================\n");
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

 
struct payload
{
    int type;
    int index;
    char msg[100];
};
 
struct eth_header
{
    unsigned short int dst[3];
    unsigned short int src[3];
    unsigned short int protocol;
};




/**
* Creates and returns a default payload
* return - payload with default values
*/
static struct payload create_payload(int type, int index, const char* msg, unsigned int msg_len)
{
	struct payload pl;
	pl.type = type;
	pl.index = index;
	rte_memcpy(pl.msg, msg, msg_len);
	return pl;
}

/**
* Creates and returns a default eth header 
* return - eth header with default values
*/
static struct eth_header create_eth_header(uint64_t src, uint64_t dst, uint16_t protocol) /*uint64_t src, uint64_t dst, uint16_t protocol*/
{
	//unsigned int byte_index;
	struct eth_header header;
	/*for (byte_index = 0; byte_index < 6; byte_index++)
	{
		header.dst[byte_index] = (dst & (0x11111111 << (byte_index * 2))) >> (byte_index * 2);
		header.src[byte_index] = (src & (0x11111111 << (byte_index * 2))) >> (byte_index * 2);
	}*/
	header.dst[0] = ((0xff0000000000 & dst) >> 40)	| ((0x00ff00000000 & dst) >> 24);
	header.dst[1] = ((0x0000ff000000 & dst) >> 24)	| ((0x000000ff0000 & dst) >> 8);
	header.dst[2] = ((0x00000000ff00 & dst) >> 8)	| ((0x0000000000ff & dst) << 8);
	header.src[0] = ((0xff0000000000 & src) >> 40)	| ((0x00ff00000000 & src) >> 24);
	header.src[1] = ((0x0000ff000000 & src) >> 24)	| ((0x000000ff0000 & src) >> 8);
	header.src[2] = ((0x00000000ff00 & src) >> 8)	| ((0x0000000000ff & src) << 8);
	header.protocol = ((0xff00 & protocol) >> 8) | ((0x00ff & protocol) >> 0);
	return header;
}

/**
* Creates the pointer to the rte_mbuf with the content of the packet.
* bufs - the pointer of the rte_mbuf which we want the packet to be in
* ready_eth_header - the eth header of the packet
* ready_payload - the payload of the packet
*/
static inline void create_packet(struct rte_mbuf** buf, struct rte_mempool* mbuf_pool, struct payload ready_payload)
{
	struct payload* payload_ref;
	void* payload_loc;
	struct eth_header* eth_header_ref;
	void* eth_header_loc;
	struct eth_header ready_eth_header;

	*buf = rte_pktmbuf_alloc(mbuf_pool);
	
	payload_ref = &ready_payload;
	ready_eth_header = create_eth_header(0x010203040506, 0x05060708090a, 0x0800);
	eth_header_ref = &ready_eth_header;

	if (buf != NULL)
	{
		eth_header_loc = rte_pktmbuf_append(*buf, sizeof(struct eth_header));
		payload_loc = rte_pktmbuf_append(*buf, sizeof(struct payload));
	}

	rte_memcpy(eth_header_loc, eth_header_ref , 14);
	rte_memcpy(payload_loc, payload_ref, sizeof(struct payload));
}


/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */
static __attribute__((noreturn)) void lcore_main(struct rte_mempool * mbuf_pool)
{
	const uint8_t nb_ports = rte_eth_dev_count();
	uint8_t port;
	//int total_sent = 0;
	int i = 0;
	unsigned int  pkt_index = 0;
	struct payload ready_payload;
	clock_t curr_time, last_table_show_time, last_pkt_counter_reset;
	float time_passed;

	unsigned int pkts_counter, pkts_counter_overall;
	unsigned int last_pkts_counter, last_pkts_counter_overall;
	unsigned int pkts_dropped_counter, pkts_dropped_counter_overall;
	unsigned int last_pkts_dropped_counter, last_pkts_dropped_counter_overall;
	unsigned int pkts_type_counter_overall[NUM_OF_TYPES];
	unsigned int last_pkts_type_counter_overall[NUM_OF_TYPES];
	unsigned int pkts_type_counter[NUM_OF_TYPES];
	unsigned int last_pkts_type_counter[NUM_OF_TYPES];
	unsigned int pkts_dropped_type_counter_overall[NUM_OF_TYPES];
	unsigned int last_pkts_dropped_type_counter_overall[NUM_OF_TYPES];
	unsigned int pkts_dropped_type_counter[NUM_OF_TYPES];
	unsigned int last_pkts_dropped_type_counter[NUM_OF_TYPES];
	float time_passed_overall;



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


	srand(time(NULL));
	last_table_show_time = 0;
	last_pkt_counter_reset = 0;

	pkts_counter = 0;
	last_pkts_counter = 0;
	pkts_counter_overall = 0;
	last_pkts_counter_overall = 0;

	pkts_dropped_counter = 0;
	last_pkts_dropped_counter = 0;
	pkts_dropped_counter_overall = 0;
	last_pkts_dropped_counter_overall = 0;
	
	for (i = 0; i < NUM_OF_TYPES; i++)
	{
		pkts_type_counter[i] = 0;
		pkts_dropped_type_counter[i] = 0;
		pkts_type_counter_overall[i] = 0;
		pkts_dropped_type_counter_overall[i] = 0;
	}

	time_passed_overall = 0;

	struct rte_mbuf *bufs[BURST_SIZE];

	/* Run until the application is quit or killed. */
	for (;;) {
		/* Get burst of RX packets, from first port of pair. */
		curr_time = clock();
		uint16_t nb_tx = 0;
		uint16_t nb_rx = 0;
		
		time_passed = ((curr_time - last_pkt_counter_reset) * 1000) / CLOCKS_PER_SEC;
		if (time_passed > PKT_COUNTER_RESET_TIME)
		{
			last_pkt_counter_reset = curr_time;
			last_pkts_counter = pkts_counter;
			last_pkts_dropped_counter = pkts_dropped_counter;
			last_pkts_counter_overall = pkts_counter_overall;
			last_pkts_dropped_counter_overall = pkts_dropped_counter_overall;
			pkts_counter = 0;
			pkts_dropped_counter = 0;
			for (i = 0; i < NUM_OF_TYPES; i++)
			{
				last_pkts_type_counter[i] = pkts_type_counter[i];
				last_pkts_dropped_type_counter[i] = pkts_dropped_type_counter[i];
				last_pkts_dropped_type_counter_overall[i] = pkts_dropped_type_counter_overall[i];
				last_pkts_type_counter_overall[i] = pkts_type_counter_overall[i];
				pkts_type_counter[i] = 0;
				pkts_dropped_type_counter[i] = 0;
			}

		}

		time_passed = ((curr_time - last_table_show_time) * 1000) / CLOCKS_PER_SEC;
		if (time_passed > REFRESH_TIME)
		{
			time_passed_overall += time_passed;
			print(last_pkts_counter, last_pkts_dropped_counter,
					last_pkts_counter_overall, last_pkts_dropped_counter_overall,
					last_pkts_type_counter, last_pkts_type_counter_overall,
					last_pkts_dropped_type_counter, last_pkts_dropped_type_counter_overall,
					time_passed_overall);

			last_table_show_time = curr_time;
		}

		if (pkts_counter > PACKET_LIMIT)
		{
			continue;
		}

		memset(bufs, 0x0, BURST_SIZE * sizeof(struct rte_mbuf *));

		for (i = 0; i < BURST_SIZE; i++)
		{
			ready_payload = create_payload(GET_RANDOM_BETWEEN(0, NUM_OF_TYPES), pkt_index + i, "Read Me\n\0", 9);
			pkts_type_counter[ready_payload.type] += 1;
			pkts_type_counter_overall[ready_payload.type] += 1;
			pkts_type_counter_overall[ready_payload.type] += 1;
			create_packet(&bufs[i], mbuf_pool, ready_payload);
		}
		nb_rx += BURST_SIZE;
		pkt_index += nb_tx;
		
		// Send burst of TX packets, to second port of pair.
		nb_tx += rte_eth_tx_burst(PORT_SEND, 0,	bufs, BURST_SIZE);
		pkts_counter += nb_tx;
		pkts_counter_overall += nb_tx;
		
		

		/* Free any unsent packets. */
		if (unlikely(nb_tx < BURST_SIZE))
		{
			uint16_t buf = 0;
			for (buf = nb_tx; buf < BURST_SIZE; buf++)
			{
				pkts_dropped_type_counter[buf]++;
				pkts_dropped_type_counter_overall[buf]++;
			}

			pkts_dropped_counter += (BURST_SIZE - nb_tx);
			pkts_dropped_counter_overall += (BURST_SIZE - nb_tx);
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
	//struct rte_mempool *mbuf_pool2;

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
