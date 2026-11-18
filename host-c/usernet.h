/* usernet.h -- userspace TCP/IP over the CoreDevice tunnel (lwIP), replacing the kernel utun.
 *
 * The utun needs root on macOS and a TUN device on Linux; running the IP stack in-process removes
 * both. After the CoreDeviceProxy tunnel handshake, call usernet_start with the connection and the
 * negotiated addresses; from then on usernet_connect() opens TCP connections to the device over
 * lwIP instead of kernel sockets. The connection carries raw IPv6 packets (same framing the pump
 * used), which the reader thread feeds to lwIP and lwIP's output feeds back. */
#ifndef RP_USERNET_H
#define RP_USERNET_H

#include <stddef.h>

/* UDP over the userspace stack, for media.c's RTP/RTCP. The peer address is an opaque blob
 * because lwIP's struct sockaddr and the system's cannot share a translation unit (see tunio.c's
 * header comment) -- the caller records where packets came from and echoes it back on send,
 * never looking inside. */
typedef struct { unsigned char raw[32]; unsigned int len; } usernet_addr;

#ifndef HAVE_USERNET
/* Stubs when lwIP userspace networking is disabled (e.g. native Linux /dev/net/tun or macOS utun) */
static inline int usernet_start(void *idev_conn, const char *our_addr, const char *dev_addr) {
    (void)idev_conn; (void)our_addr; (void)dev_addr;
    return -1;
}
static inline int usernet_connect(const char *addr, int port) {
    (void)addr; (void)port;
    return -1;
}
static inline long usernet_read(int fd, void *buf, size_t n) {
    (void)fd; (void)buf; (void)n;
    return -1;
}
static inline long usernet_write(int fd, const void *buf, size_t n) {
    (void)fd; (void)buf; (void)n;
    return -1;
}
static inline void usernet_close(int fd) {
    (void)fd;
}
static inline int usernet_udp_socket(int *bound_port) {
    (void)bound_port;
    return -1;
}
static inline long usernet_recvfrom(int fd, void *buf, size_t n, usernet_addr *from) {
    (void)fd; (void)buf; (void)n; (void)from;
    return -1;
}
static inline long usernet_sendto(int fd, const void *buf, size_t n, const usernet_addr *to) {
    (void)fd; (void)buf; (void)n; (void)to;
    return -1;
}
static inline int usernet_set_recv_timeout_ms(int fd, int ms) {
    (void)fd; (void)ms;
    return -1;
}
#else
int  usernet_start(void *idev_conn, const char *our_addr, const char *dev_addr);
int  usernet_connect(const char *addr, int port);
long usernet_read(int fd, void *buf, size_t n);
long usernet_write(int fd, const void *buf, size_t n);
void usernet_close(int fd);
int  usernet_udp_socket(int *bound_port);
long usernet_recvfrom(int fd, void *buf, size_t n, usernet_addr *from);
long usernet_sendto(int fd, const void *buf, size_t n, const usernet_addr *to);
int  usernet_set_recv_timeout_ms(int fd, int ms);
#endif

/* Whether userspace mode is on (set by cdhost from RPLAY_USERSPACE_NET). */
void usernet_enable(int on);
int  usernet_is_on(void);

/* Unified tunnel I/O: use these everywhere a tunnel connection is opened/read/written, and the
 * kernel-vs-lwIP choice is made centrally. tun_connect blocks with a connect timeout. */
int  tun_connect(const char *addr, int port, int timeout_s);
long tun_read(int fd, void *buf, size_t n);
long tun_write(int fd, const void *buf, size_t n);
void tun_close(int fd);

/* lwIP fds are offset above kernel fds; a fd >= this is a usernet (lwIP) socket. */
#define USERNET_FD_BASE 768
static inline int usernet_owns(int fd) { return fd >= USERNET_FD_BASE; }

#endif
