#include "transceiver.h"
#include "thread.h"
#include "limits.h"
#include "mutex.h"
#include "byteorder.h"
#include "crc.h"
#include "net_layer.h"
#include "string.h"
#include "portail.h"

/* should probaby bust out a dedicated thread for this */

/**
 * UDP checksums are not calculated. It's not easy as IP packets are
 * terminated in the xport and the checksum is based on IP headers.
 * Checksum is checked at IP termination, then all other transports
 * (UART and radio) have their own CRC checks.
 */

#define TRANCSCEIVER TRANSCEIVER_DEFAULT
#define MAX_SOCKETS (3)

typedef struct {
    int id;
    uint type;
    uint port;
    kernel_pid_t dest_pid;
} sock_t;

static sock_t sockets[MAX_SOCKETS];
static uint socket_nextid = 0;
static uint next_rand_port = UINT_MAX;

static struct {
  uint radio_buffer_full;
  uint radio_send_fail;
  uint unhandled_packet;
} stats;

kernel_pid_t net_layer_pid = KERNEL_PID_UNDEF;


#define ENABLE_DEBUG    (1)
#include "debug.h"

static sock_t *new_socket(void)
{
    for (int i = 0; i < MAX_SOCKETS; ++i)
    {
        if (sockets[i].id == SOCK_UNDEF)
        {
            sock_t *sock = &sockets[i];
            sock->id = socket_nextid++;
            return sock;
        }
    }
    return NULL;
}

static sock_t *get_socket(int id)
{
    for (int i = 0; i < MAX_SOCKETS; ++i)
    {
        if (sockets[i].id == id)
        {
            return &sockets[i];
        }
    }
    return NULL;
}

static sock_t *get_socket_port(uint port)
{
    for (int i = 0; i < MAX_SOCKETS; ++i)
    {
        if (sockets[i].port == port)
        {
            return &sockets[i];
        }
    }
    return NULL;
}

static uint get_rand_port(void)
{
    if (next_rand_port < UINT_MAX/2)
    {
        next_rand_port = UINT_MAX;
    }
    return next_rand_port--;
}

bool sock_is_bound(sock_t *sock)
{
    return sock->port && sock->dest_pid != KERNEL_PID_UNDEF;
}


int netl_socket(uint type)
{
    sock_t *sock = new_socket();
    if (sock != NULL)
    {
        sock->type = type;
        return sock->id;
    }
    return -1;
}

/* for now capture all udp ports to caller */
uint8_t netl_bind(int s, uint port)
{
    sock_t *sock = get_socket(s);

    if (sock == NULL) {
        return -1;
    }

    sock_t *existing = get_socket_port(port);

    if (existing != NULL)
    {
        if (sock->id != existing->id)
        {
            return -1;
        }
    }
    else {
        /* bind! */
        sock->port = port;
        sock->dest_pid = thread_getpid();
    }
    return 0;
}

uint8_t netl_unbind(int s)
{
    sock_t *sock = get_socket(s);

    if (sock == NULL) {
        return -1;
    }

    if (sock_is_bound(sock))
    {
        sock->port = 0;
        sock->dest_pid = KERNEL_PID_UNDEF;
    }
    return 0;
}

int netl_close(int s)
{
    sock_t *sock = get_socket(s);

    if (sock == NULL) {
        return -1;
    }

    sock->id = SOCK_UNDEF;
    netl_unbind(s);

    return 0;
}

int netl_recv(int s, uint8_t *dest, uint maxlen, uint *srcport)
{
    sock_t *sock = get_socket(s);

    if (sock == NULL) {
        return -1;
    }

    msg_t m;

    while (1) {
        msg_receive(&m);

        if (m.sender_pid == net_layer_pid)
        {
            radio_packet_t *pkt = (radio_packet_t *) m.content.ptr;
            udp_hdr_t *udph = (udp_hdr_t *) pkt->data;
            char *data;
            uint length;

            if (sock->type == SOCK_RAW)
            {
                length = pkt->length;
                data = (char *) pkt->data;
            }
            else {
                length = udph->length;
                data = (char *) udph + sizeof(udp_hdr_t);
            }

            if (length <= maxlen)
            {
                memcpy(dest, data, length);
                if (srcport != NULL) {
                    *srcport = udph->src_port;
                }
                return length;
            }
            pkt->processing--;
        }
        /* else drop your msg, not ideal */
    }

    UNREACHABLE();
}

static uint8_t _netl_send(sock_t *sock, uint8_t *data, uint8_t size)
{

    static radio_packet_t radio_pkt;
    static transceiver_command_t tcmd;

    radio_pkt.dst = TRANSCEIVER_BROADCAST;
    radio_pkt.data = data;
    radio_pkt.length = size;

    tcmd.transceivers = TRANCSCEIVER;
    tcmd.data = &radio_pkt;

    msg_t m;
    m.type = SND_PKT;
    m.content.ptr = (char *) &tcmd;
    msg_send_receive(&m, &m, transceiver_pid);

    // cc110x_send() returns int8_t
    int8_t sent_len = (int8_t) m.content.value;
    if (sent_len <= 0){
        stats.radio_send_fail += 1;
    }

    return sent_len;
}

static char send_to_buf[PORTAIL_MAX_DATA_SIZE];
static mutex_t send_to_buf_mutex = MUTEX_INIT;

int netl_send_to(int s, uint8_t *data, uint8_t size, uint dstport)
{
    sock_t *sock = get_socket(s);

    if (sock == NULL) {
        return -1;
    }

    if (sizeof(send_to_buf) - sizeof(udp_hdr_t) < size)
    {
        DEBUG("netl_send_to(): data too large\n");
        return -1;
    }

    /* concurrent call should be rare, rather save space with just one buf slot */
    mutex_lock(&send_to_buf_mutex);
    udp_hdr_t *udph = (udp_hdr_t *) send_to_buf;
    udph->dst_port = HTONS(dstport);
    if (sock_is_bound(sock)) {
        udph->src_port = HTONS(sock->port);
    } else {
        udph->src_port = HTONS(get_rand_port());
    }
    // write crc
    udph->length = HTONS(size);
    memcpy((char *) udph + sizeof(udp_hdr_t), data, size);

    int ret = _netl_send(sock, (uint8_t *) udph, size + sizeof(udp_hdr_t));

    mutex_unlock(&send_to_buf_mutex);

    return ret;
}

int netl_send(int s, uint8_t *data, uint8_t size)
{
    sock_t *sock = get_socket(s);

    if (sock == NULL) {
        return -1;
    }

    if (PORTAIL_MAX_DATA_SIZE < size)
    {
        DEBUG("netl_send_to(): data too large\n");
        return -1;
    }

    return _netl_send(sock, data, size);
}


static void *rx_thread(void *arg)
{
    transceiver_init(TRANCSCEIVER);
    transceiver_start();
    transceiver_register(TRANCSCEIVER, thread_getpid());

    msg_t m, target_m;

    while(1)
    {
        msg_receive(&m);

        if (m.type == PKT_PENDING)
        {
            bool pkt_handled = false;
            radio_packet_t *pkt = (radio_packet_t *) m.content.ptr;
            udp_hdr_t *udph = (udp_hdr_t *) pkt->data;

            /* for all sockets */
            for (int i = 0; i < MAX_SOCKETS; ++i)
            {
                sock_t *sock = &sockets[i];

                /* if has a pid registered */
                if (sock->dest_pid != KERNEL_PID_UNDEF)
                {
                    bool has_matching_port = sock->port && sock->port == NTOHS(udph->dst_port);
                    bool has_no_port_but_is_raw = !sock->port && sock->type == SOCK_RAW;

                    /* if should receive UDP msg */
                    if (has_matching_port || has_no_port_but_is_raw)
                    {
                        target_m = m;
                        int ret = msg_try_send(&target_m, sock->dest_pid);
                        if (ret == 1) {
                            /* successfully delivered, netl_recv will have to decr this */
                            pkt->processing++;
                            pkt_handled = true;
                        }
                    }
                }
            }
            if (!pkt_handled)
            {
                DEBUG("net_layer: radio packet dropped, lacked listening socket");
                stats.unhandled_packet += 1;
            }
            pkt->processing--;
        }
        else if (m.type == ENOBUFFER)
        {
            stats.radio_buffer_full += 1;
        }
    }
    UNREACHABLE();
}

static char stack_buffer[THREAD_STACKSIZE_DEFAULT];
static bool did_init = false;

void netl_init(void)
{
    if (!did_init)
    {
        /* initialize all socket IDs to null value */
        for (int i = 0; i < MAX_SOCKETS; ++i)
        {
            sockets[i].id = SOCK_UNDEF;
        }
        did_init = true;
    }

    if (net_layer_pid == KERNEL_PID_UNDEF)
    {
        net_layer_pid = thread_create(
          stack_buffer,
          sizeof(stack_buffer),
          THREAD_PRIORITY_MAIN - 2,
          0,
          rx_thread,
          NULL,
          "net_layer"
        );
    }
}