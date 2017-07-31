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
    struct port_info currentDropped[NUM_OF_MSG_TYPES];
} ports_statistics;

/***********************************************
* Enumerations
************************************************/
enum ETitles
{
    eTitles_currentPassed,
    eTitles_currentDropped,
    eTitles_overallPassed,
    eTitles_overallDropped,
    NUM_OF_TITLES
} ETitles;