#define DISTPD_PORT         9000
#define BEE_PORT          9999
#define NUM_QUEUES        16
#define NUM_FLOWS         500
#define NUM_PREFIXES         500
#define SAMPLE_COUNT      1
#define TABLE_SIZE        1024
#define JUNK              1375
// #define C                 25000     // QUEUE CAPACITY
#define C                 1024
#define EQUATION_MULT_SHIFT     3   // multilying two sides of the equation

#define CTR_WIDTH   64
#define INGRESS_CTR_WIDTH 2
#define INGRESS_CTR_SIZE        1<<INGRESS_CTR_WIDTH
