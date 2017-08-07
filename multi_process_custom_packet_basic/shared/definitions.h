/***********************************************
* General Definitions
************************************************/
#define PORT_SEND (0)
#define PORT_RECV (1)

// Number of message types that exist
#define NUM_OF_MSG_TYPES (16)
/***********************************************
* Message Payload Definitions
************************************************/
struct payload
{
    int type;
    int index;
    char msg[100];
} payload;

struct eth_header
{
    unsigned short int dst[3];
    unsigned short int src[3];
    unsigned short int protocol;
} eth_header;

/***********************************************
* Port Statistics Definitions
************************************************/
struct port_info
{
    uint32_t value;
} port_info;

struct ports_statistics
{
	struct port_info total[NUM_OF_MSG_TYPES];
	struct port_info dropped[NUM_OF_MSG_TYPES];
	struct port_info current[NUM_OF_MSG_TYPES];
} ports_statistics;

/***********************************************
* Port Statistics functions
************************************************/
//TODO check if dangerous
static struct ports_statistics CopyPortsStatics(struct ports_statistics original)
{
    unsigned int type_index;
    struct ports_statistics copy;
    for (type_index = 0; type_index < NUM_OF_MSG_TYPES; type_index++)
    {
        copy.total[type_index] = original.total[type_index];
        copy.dropped[type_index] = original.dropped[type_index];
        copy.current[type_index] = original.current[type_index];
    }
    return copy;
}