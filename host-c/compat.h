#ifndef RP_COMPAT_H
#define RP_COMPAT_H

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <process.h>
#include <stdint.h>
#include <sys/stat.h>

typedef intptr_t ssize_t;
#define sleep(x) Sleep((DWORD)((x)*1000))
#define usleep(x) Sleep((DWORD)((x)/1000))
#define close(x) closesocket(x)
#define mkdir(p, m) _mkdir(p)

static inline int set_nonblocking(int fd) {
    u_long mode = 1;
    return ioctlsocket((SOCKET)fd, FIONBIO, &mode);
}

static inline int is_sock_wouldblock(void) {
    int err = WSAGetLastError();
    return err == WSAEWOULDBLOCK || err == WSAEINTR;
}

#else

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

static inline int set_nonblocking(int fd) {
    int fl = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}

static inline int is_sock_wouldblock(void) {
    return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR;
}

extern char g_bind_dev[64];

#endif

/* --- Libplist cross-version compatibility --- */
#include <plist/plist.h>

#ifndef plist_mem_free
#define plist_mem_free(ptr) free(ptr)
#endif

#ifdef HAVE_LIBPLIST_2_3
#define rp_plist_from_memory(data, len, pl) plist_from_memory((const char *)(data), (uint32_t)(len), (pl), NULL)
#else
#define rp_plist_from_memory(data, len, pl) plist_from_memory((const char *)(data), (uint32_t)(len), (pl))
#endif

static inline void compat_plist_get_unix_date_val(plist_t node, int64_t *sec) {
    int32_t s32 = 0, u32 = 0;
    plist_get_date_val(node, &s32, &u32);
    if (sec) *sec = (int64_t)s32 + 978307200LL;
}
#ifndef plist_get_unix_date_val
#define plist_get_unix_date_val compat_plist_get_unix_date_val
#endif

#endif /* RP_COMPAT_H */
