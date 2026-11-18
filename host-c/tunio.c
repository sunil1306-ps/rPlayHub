/* tunio.c -- the kernel-vs-lwIP routing for tunnel connections. Kept apart from usernet.c because
 * lwIP's <lwip/sockets.h> and the system <sys/socket.h> both define struct sockaddr et al. and
 * cannot share a translation unit. This file uses the system headers and calls into usernet.c
 * (lwIP) only through the plain functions in usernet.h. */
#include "usernet.h"

#include "compat.h"
#include <string.h>
#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

static int g_on;
void usernet_enable(int on) { g_on = on; }
int usernet_is_on(void) { return g_on; }

int tun_connect(const char *addr, int port, int timeout_s)
{
#ifdef HAVE_USERNET
    if (g_on) return usernet_connect(addr, port);
#else
    (void)g_on;
#endif
    struct sockaddr_in6 sa;
    memset(&sa, 0, sizeof sa);
    sa.sin6_family = AF_INET6;
    sa.sin6_port = htons((uint16_t)port);
    if (inet_pton(AF_INET6, addr, &sa.sin6_addr) != 1) return -1;
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (fd < 0) return -1;
#ifdef _WIN32
    DWORD ms = (timeout_s > 0 ? timeout_s : 10) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&ms, sizeof ms);
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&ms, sizeof ms);
#else
    struct timeval tv = { .tv_sec = timeout_s > 0 ? timeout_s : 10, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
#endif
#if defined(__linux__) && defined(SO_BINDTODEVICE)
    extern char g_ifname[64];
    char bind_target[64] = {0};
    if (g_ifname[0]) {
        char mpath[128], mtarget[256];
        snprintf(mpath, sizeof(mpath), "/sys/class/net/%s/master", g_ifname);
        ssize_t mlen = readlink(mpath, mtarget, sizeof(mtarget) - 1);
        if (mlen > 0) {
            mtarget[mlen] = '\0';
            char *slash = strrchr(mtarget, '/');
            const char *master = slash ? slash + 1 : mtarget;
            strncpy(bind_target, master, sizeof(bind_target) - 1);
        } else {
            strncpy(bind_target, g_ifname, sizeof(bind_target) - 1);
        }
        setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, bind_target, strlen(bind_target));
    }
#endif
    if (connect(fd, (struct sockaddr *)&sa, sizeof sa) != 0) { close(fd); return -1; }
    return fd;
}

long tun_read(int fd, void *buf, size_t n)
{
#ifdef HAVE_USERNET
    if (usernet_owns(fd)) return usernet_read(fd, buf, n);
#endif
    return (long)recv(fd, buf, n, 0);
}

long tun_write(int fd, const void *buf, size_t n)
{
#ifdef HAVE_USERNET
    if (usernet_owns(fd)) return usernet_write(fd, buf, n);
#endif
    return (long)send(fd, buf, n, 0);
}

void tun_close(int fd)
{
#ifdef HAVE_USERNET
    if (usernet_owns(fd)) { usernet_close(fd); return; }
#endif
    close(fd);
}
