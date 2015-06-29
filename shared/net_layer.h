#ifndef NET_LAYER_H__
#define NET_LAYER_H__

#define SOCK_UNDEF (-1)

enum {
    SOCK_RAW,
    SOCK_DATA
};

typedef struct __attribute__((packed)) {
    uint16_t src_port;                  ///< source port
    uint16_t dst_port;                  ///< destination port
    uint16_t length;                    ///< payload length
    /**
     * internet checksum
     *
     * @see <a href="http://tools.ietf.org/html/rfc1071">RFC 1071</a>
     */
    uint16_t checksum;
} udp_hdr_t;


void netl_init(void);

/**
 * Initialize a new socket.
 * `type` only influences calls to receive, not sends.
 * @param  type SOCK_RAW or SOCK_DATA
 * @return      socket id
 */
int netl_socket(uint type);

/**
 * Bind to a UDP port
 * @param  s    socket
 * @param  port port number
 * @return      -1 on error
 */
uint8_t netl_bind(int s, uint port);

/**
 * Unbind from a UDP port
 * @param  s    socket
 * @return      -1 on error
 */
uint8_t netl_unbind(int s);

/**
 * Receive from socket. `dest` will be filled with raw UDP packet if
 * socket is of type SOCK_RAW, otherwise will be filled with UDP
 * payload.
 * If `srcport` is not NULL, it is filled with sender's UDP port.
 * If `s` is bound to a port, this function will return on data.
 * If `s` is not bound to a port but `s` is of `type` SOCK_RAW, it will
 * receive all packets.
 * @param  s       socket
 * @param  dest    your buffer
 * @param  maxlen  your buffer length
 * @param  srcport sender port
 * @return         num of bytes received or -1 on error
 */
int netl_recv(int s, uint8_t *dest, uint maxlen, uint *srcport);

/**
 * Send a raw UDP message
 * @param  s    socket
 * @param  data pointer to UDP packet
 * @param  size
 * @return      sent length or -1 on error
 */
int netl_send(int sock, uint8_t *data, uint8_t size);

/**
 * Send data to a UDP port
 * @param  s    socket
 * @param  data pointer to data
 * @param  size
 * @return      sent length or -1 on error
 */
int netl_send_to(int s, uint8_t *data, uint8_t size, uint dstport);

#endif /* end of include guard: NET_LAYER_H__ */