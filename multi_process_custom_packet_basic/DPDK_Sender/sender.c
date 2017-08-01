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
#include <definitions.h>

#define RX_RING_SIZE 128
#define TX_RING_SIZE 512

#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 4096

static struct ether_addr l2fwd_ports_eth_addr[1];

static const struct rte_eth_conf port_conf_default = {
	.rxmode = { .max_rx_pkt_len = ETHER_MAX_LEN }
};

#define MAX_PKT_COUNT 0xffff
#define GET_RANDOM_BETWEEN(start, end) ((rand() % (end - start)) + start)
#define REFRESH_TIME (10000) //in ms
#define NUM_OF_TYPES (16)
#define PACKET_LIMIT (1000000000)


struct ports_statistics tx_stats;
struct ports_statistics creation_stats;

static float time_passed_overall;


/* basicfwd.c: Basic DPDK skeleton forwarding example. */
static void print(void)
{
	const char clr[] = { 27, '[', '2', 'J', '\0' };
	const char topLeft[] = { 27, '[', '1', ';', '1', 'H','\0' };
	unsigned int type_index;
	unsigned int totalTx[4/*ETitles.NUM_OF_TITLES*/];
	unsigned int totalCreation[4/*ETitles.NUM_OF_TITLES*/];

	for (type_index = 0; type_index < NUM_OF_TYPES; type_index++)
	{
		//Puts zeroes before first use
		if (type_index == 0)
		{
			totalTx[eTitles_currentPassed] = 0;
			totalTx[eTitles_currentDropped] = 0;
			totalTx[eTitles_overallPassed] = 0;
			totalTx[eTitles_overallDropped] = 0;

			totalCreation[eTitles_currentPassed] = 0;
			totalCreation[eTitles_currentDropped] = 0;
			totalCreation[eTitles_overallPassed] = 0;
			totalCreation[eTitles_overallDropped] = 0;
		}

		totalTx[eTitles_currentPassed] += tx_stats.current[type_index].value;
		totalTx[eTitles_currentDropped] += tx_stats.currentDropped[type_index].value;
		totalTx[eTitles_overallPassed] += tx_stats.total[type_index].value;
		totalTx[eTitles_overallDropped] += tx_stats.dropped[type_index].value;

		totalCreation[eTitles_currentPassed] += creation_stats.current[type_index].value;
		totalCreation[eTitles_currentDropped] += creation_stats.currentDropped[type_index].value;
		totalCreation[eTitles_overallPassed] += creation_stats.total[type_index].value;
		totalCreation[eTitles_overallDropped] += creation_stats.dropped[type_index].value;
	}

	/* Clear screen and move to top left */
	printf("%s%s", clr, topLeft);

	printf("\n=================================================================================");
	printf("\n|Pkts counted since last check : %d %-41s|", totalTx[eTitles_currentPassed], "pkts");
	printf("\n|Check time : %.2f%-62s|", (double)REFRESH_TIME/1000, "s");
	printf("\n|Pkts/s since last check : %d %-47s|",
				(totalTx[eTitles_currentPassed] * 1000) / REFRESH_TIME, "pkts/s");
	printf("\n|Running time : %.2f\t\t\t\t\t\t\t\t|", time_passed_overall/1000);

	printf("\n=================================================================================");
	printf("\n|%-15s|%23s%-8s|%19s%-12s|", "", "current session", "", "overall", "");
	printf("\n|%-15s=================================================================", "Type #");
	printf("\n|%-15s|%15s|%15s|%15s|%15s|", "", "pkts sent", "pkts dropped", "pkts sent", "pkts dropped");
	printf("\n=================================================================================");
	for (type_index = 0; type_index < NUM_OF_TYPES; type_index++)
	{
		printf("\n|%s%-9u|%15u|%15u|%15u|%15u|",
					"Type #",
					type_index,
					tx_stats.current[type_index].value,
					tx_stats.currentDropped[type_index].value,
					tx_stats.total[type_index].value,
					tx_stats.dropped[type_index].value
					);
	}
	/*printf("\n=================================================================================");
	printf("\n|  Total by types\t|\t%u\t|\t%u\t|\t%u\t|\t%u\t|",
					totalTx[eTitles_currentPassed],
					pkts_dropped_counted,
					total_sent,
					total_dropped
					);*/
	printf("\n=================================================================================");
	printf("\n|%-15s|%15u|%15u|%15u|%15u|",
					"Total tx",
					totalTx[eTitles_currentPassed],
					totalTx[eTitles_currentDropped],
					totalTx[eTitles_overallPassed],
					totalTx[eTitles_overallDropped]
					);
	printf("\n=================================================================================");
	printf("\n|%-15s|%15u|%15u|%15u|%15u|",
					"Total created",
					totalCreation[eTitles_currentPassed],
					totalCreation[eTitles_currentDropped],
					totalCreation[eTitles_overallPassed],
					totalCreation[eTitles_overallDropped]
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
	header.dst[0] = 	((0xff0000000000 & dst) >> 40)	| ((0x00ff00000000 & dst) >> 24);
	header.dst[1] = 	((0x0000ff000000 & dst) >> 24)	| ((0x000000ff0000 & dst) >> 8);
	header.dst[2] = 	((0x00000000ff00 & dst) >> 8)	| ((0x0000000000ff & dst) << 8);
	header.src[0] = 	((0xff0000000000 & src) >> 40)	| ((0x00ff00000000 & src) >> 24);
	header.src[1] = 	((0x0000ff000000 & src) >> 24)	| ((0x000000ff0000 & src) >> 8);
	header.src[2] = 	((0x00000000ff00 & src) >> 8)	| ((0x0000000000ff & src) << 8);
	header.protocol = 	((0xff00 & protocol) >> 8)		| ((0x00ff & protocol) >> 0);
	return header;
}

/**
* Creates the pointer to the rte_mbuf with the content of the packet.
* bufs - the pointer of the rte_mbuf which we want the packet to be in
* ready_eth_header - the eth header of the packet
* ready_payload - the payload of the packet
*/
static inline int create_packet(struct rte_mbuf** buf, struct rte_mempool* mbuf_pool, struct payload ready_payload)
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

	if (*buf == NULL)
	{
		return 0;
	}

	eth_header_loc = rte_pktmbuf_append(*buf, sizeof(struct eth_header));
	payload_loc = rte_pktmbuf_append(*buf, sizeof(struct payload));
	rte_memcpy(eth_header_loc, eth_header_ref , sizeof(struct eth_header));
	rte_memcpy(payload_loc, payload_ref, sizeof(struct payload));
	return 1;
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
	unsigned int  pkt_index = 0, pkts_counter = 0;
	struct payload ready_payload;
	clock_t curr_time, last_pkt_counter_reset;
	float time_passed;

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
	last_pkt_counter_reset = 0;
	
	for (i = 0; i < NUM_OF_TYPES; i++)
	{
		tx_stats.current[i].value = 0;
		tx_stats.currentDropped[i].value = 0;
		tx_stats.total[i].value = 0;
		tx_stats.dropped[i].value = 0;


		creation_stats.current[i].value = 0;
		creation_stats.currentDropped[i].value = 0;
		creation_stats.total[i].value = 0;
		creation_stats.dropped[i].value = 0;
	}

	time_passed_overall = 0;

	struct rte_mbuf *bufs[BURST_SIZE];

	/* Run until the application is quit or killed. */
	for (;;) {
		/* Get burst of RX packets, from first port of pair. */
		curr_time = clock();

		uint16_t nb_tx = 0;
		uint16_t nb_created = 0;
		
		time_passed = ((curr_time - last_pkt_counter_reset) * 1000) / CLOCKS_PER_SEC;
		if (time_passed > REFRESH_TIME)
		{
			
			time_passed_overall += time_passed;
			print();

			for (i = 0; i < NUM_OF_TYPES; i++)
			{
				tx_stats.current[i].value = 0;
				tx_stats.currentDropped[i].value = 0;

				creation_stats.current[i].value = 0;
				creation_stats.currentDropped[i].value = 0;
			}

			pkts_counter = 0;
			last_pkt_counter_reset = curr_time;
		}

		if (pkts_counter > PACKET_LIMIT)
		{
			continue;
		}

		//TODO free buf_addr if needed
		memset(bufs, 0x0, BURST_SIZE * sizeof(struct rte_mbuf *));

		for (i = 0; i < BURST_SIZE; i++)
		{
			ready_payload = create_payload(GET_RANDOM_BETWEEN(0, NUM_OF_TYPES), pkt_index + i, "Read Me\n\0", 9);
			if(create_packet(&bufs[i], mbuf_pool, ready_payload) == 0)
			{
				printf("faileed to create\n");
				creation_stats.currentDropped[ready_payload.type].value += 1;
				creation_stats.dropped[ready_payload.type].value += 1;
				continue;
			}
			
			nb_created += 1;
			creation_stats.current[ready_payload.type].value += 1;
			creation_stats.total[ready_payload.type].value += 1;

			//Assumes everything was sent, and then subtracts by the number of the dropped packets
			tx_stats.current[ready_payload.type].value += 1;
			tx_stats.total[ready_payload.type].value += 1;
		}

		
		// Send burst of TX packets, to second port of pair.
		nb_tx += rte_eth_tx_burst(PORT_SEND, 0,	bufs, nb_created);

		pkt_index += nb_tx;
		pkts_counter += nb_tx;

		/* Free any unsent packets. */
		if (unlikely(nb_tx < BURST_SIZE))
		{
			uint16_t buf = 0;
			unsigned int type = 0;
			unsigned int data_off = 0;
			for (buf = nb_tx; buf < BURST_SIZE; buf++)
			{
				if(bufs[buf] == NULL)
				{
					continue;
				}
				data_off = bufs[buf]->data_off;
				type = ((struct payload*)(bufs[buf]->buf_addr + data_off + sizeof(struct eth_header)))->type;
				//printf("start = %u, type = %u, index = %u\n", nb_tx, type, buf);
				tx_stats.currentDropped[type].value += 1;
				tx_stats.dropped[type].value += 1;

				//subtracts the dropped ones after assumed everything was sent
				tx_stats.current[type].value -= 1;
				tx_stats.total[type].value -= 1;

				rte_pktmbuf_free(bufs[buf]);
			}
		}
	}
}


static void lcore_printer1(void* arg)
{
	unsigned lcore_id = rte_lcore_id();
	unsigned int index = 0;
	do
	{
		printf("1.lcore_id = %d index = %u\n", lcore_id, index);
		index++;
		if (lcore_id != 1)
		{
			printf("lcore_id != %d\n", lcore_id);
			break;
		}
		//sleep(1);
	} while(1);
}
static void lcore_printer2(void* arg)
{
	unsigned lcore_id = rte_lcore_id();
	unsigned int index = 0;
	do
	{
		printf("2.lcore_id = %d index = %u\n", lcore_id, index);
		index++;
		if (lcore_id != 0)
		{
			printf("lcore_id != %d\n", lcore_id);
			break;
		}
		//sleep(1);
	} while(1);
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
	unsigned lcore_id1, lcore_id2;
	int val;

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


	/*RTE_LCORE_FOREACH_SLAVE(lcore_id1)
	{
		val = rte_eal_remote_launch(lcore_printer1, NULL, lcore_id1);
	}
	lcore_printer2(NULL);*/
	//printf("here after 1 - %d\n", val);
	/*RTE_LCORE_FOREACH_SLAVE(lcore_id2)
	{
		val = rte_eal_remote_launch(lcore_printer2, NULL, lcore_id2);
	}*/
	//printf("here after 2 - %d\n", val);


	/* Call lcore_main on the master core only. */
	//lcore_main(mbuf_pool1);

	return 0;
}
