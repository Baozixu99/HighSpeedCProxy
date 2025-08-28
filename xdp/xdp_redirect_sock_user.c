// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2017 - 2018 Intel Corporation. */
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>

#include <arpa/inet.h>
#include <bpf/libbpf.h>
#include <bpf/xsk.h>
#include <bpf/bpf.h>

#ifndef SOL_XDP
#define SOL_XDP 283
#endif

#ifndef AF_XDP
#define AF_XDP 44
#endif

#ifndef PF_XDP
#define PF_XDP AF_XDP
#endif

#define NUM_FRAMES (4 * 1024)
#define BATCH_SIZE 64

#define DEBUG_HEXDUMP 1
#define MAX_SOCKS 1

typedef __u64 u64;
typedef __u32 u32;

enum benchmark_type {
	BENCH_L2FWD = 0,
};

static enum benchmark_type opt_bench = BENCH_L2FWD;
static u32 opt_xdp_flags = XDP_FLAGS_UPDATE_IF_NOEXIST | XDP_FLAGS_SKB_MODE;
static const char *opt_if = "eth1";
static int opt_ifindex;
static int opt_queue = 0;
static int opt_poll = 1;
static u32 opt_xdp_bind_flags = XDP_USE_NEED_WAKEUP | XDP_COPY;
static u32 opt_umem_flags;
static int opt_unaligned_chunks;
static int opt_mmap_flags;
static u32 opt_xdp_bind_flags;
static int opt_xsk_frame_size = XSK_UMEM__DEFAULT_FRAME_SIZE;
static int opt_timeout = 1000;

/*
* This option adds support for a new flag called need_wakeup 
* that is present in the FILL ring and the TX ring, the rings
* for which user space is a producer. When this option is set
* in the bind call, the need_wakeup flag will be set if the 
* kernel needs to be explicitly woken up by a syscall to 
* continue processing packets. If the flag is zero, no syscall
* is needed.
*/
static bool opt_need_wakeup = true;

static __u32 prog_id;

struct xsk_umem_info {
	struct xsk_ring_prod fq;
	struct xsk_ring_cons cq;
	struct xsk_umem *umem;
	void *buffer;
};

struct xsk_socket_info {
	struct xsk_ring_cons rx;
	struct xsk_ring_prod tx;
	struct xsk_umem_info *umem;
	struct xsk_socket *xsk;
	unsigned long rx_npkts;
	unsigned long tx_npkts;
	unsigned long prev_rx_npkts;
	unsigned long prev_tx_npkts;
	u32 outstanding_tx;
};

static int num_socks = 1;
struct xsk_socket_info *xsks;
static void *bufs;
static struct xsk_umem_info *umem;
static const char pkt_data[] =
	"\x00\x50\x56\xC0\x00\x08"  // 目标MAC: 00-50-56-C0-00-08
	"\x00\x0C\x29\xF1\x8A\x0F"  // 源MAC: 00:0c:29:f1:8a:0f
	"\x08\x00"                  // 以太类型: IPv4 (0x0800)
	"\x45\x00"                  // IPv4版本和头部长度(20字节)
	"\x00\x2E"                  // 总长度: 46字节
	"\x00\x00\x00\x00"          // 标识/标志/片偏移
	"\x40\x11"                  // TTL:64, 协议:UDP(0x11)
	"\x7A\x8D"                  // [新]IP头部校验和: 0x7A8D (计算过程见下文)
	"\xC0\xA8\xB3\x89"          // 源IP: 192.168.179.137 (0xC0A8B389)
	"\xC0\xA8\xB3\x01"          // 目标IP: 192.168.179.1 (0xC0A8B301)
	"\x22\xB8"                  // 源端口: 8888 (0x22B8)
	"\x22\xB8"                  // 目标端口: 8888 (0x22B8)
	"\x00\x1A"                  // UDP长度: 26字节
	"\x1E\xA5"                  // [新]UDP校验和: 0x1EA5 (计算过程见下文)
	"\x34\x33\x1F\x69\x40\x6B"  // UDP载荷(保持不变)
	"\x54\x59\xB6\x14\x2D\x11"
	"\x44\xBF\xAF\xD9\xBE\xAA";

static size_t gen_eth_frame(struct xsk_umem_info *umem, u64 addr)
{
	memcpy(xsk_umem__get_data(umem->buffer, addr), pkt_data,
	       sizeof(pkt_data) - 1);
	return sizeof(pkt_data) - 1;
}

static void remove_xdp_program(void)
{
	__u32 curr_prog_id = 0;

	if (bpf_get_link_xdp_id(opt_ifindex, &curr_prog_id, opt_xdp_flags)) {
		printf("bpf_get_link_xdp_id failed\n");
		exit(EXIT_FAILURE);
	}
	if (prog_id == curr_prog_id)
		bpf_set_link_xdp_fd(opt_ifindex, -1, opt_xdp_flags);
	else if (!curr_prog_id)
		printf("couldn't find a prog id on a given interface\n");
	else
		printf("program on interface changed, not removing\n");
}

static void int_exit()
{
	struct xsk_umem *umem = xsks->umem->umem;

	xsk_socket__delete(xsks->xsk);
	(void)xsk_umem__delete(umem);
	remove_xdp_program();
	munmap(bufs, NUM_FRAMES * opt_xsk_frame_size);
	exit(EXIT_SUCCESS);
}

static void __exit_with_error(int error, const char *file, const char *func,
			      int line)
{
	fprintf(stderr, "%s:%s:%i: errno: %d/\"%s\"\n", file, func,
		line, error, strerror(error));
	remove_xdp_program();
	exit(EXIT_FAILURE);
}

#define exit_with_error(error) __exit_with_error(error, __FILE__, __func__, \
						 __LINE__)

static void process_packet(void *packet, size_t len)
{
    
    struct ethhdr *eth = packet;
    
    printf("Packet received (%lu bytes):\n", len);
    printf("  Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
           eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
    printf("  Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth->h_source[0], eth->h_source[1], eth->h_source[2],
           eth->h_source[3], eth->h_source[4], eth->h_source[5]);
    printf("  Ethertype: 0x%04x\n", ntohs(eth->h_proto));
    printf("\n");
}


static struct xsk_umem_info *xsk_configure_umem(void *buffer, u64 size)
{
	struct xsk_umem_info *umem;
	struct xsk_umem_config cfg = {
		.fill_size = XSK_RING_PROD__DEFAULT_NUM_DESCS,
		.comp_size = XSK_RING_CONS__DEFAULT_NUM_DESCS,
		.frame_size = opt_xsk_frame_size,
		.frame_headroom = XSK_UMEM__DEFAULT_FRAME_HEADROOM,
		.flags = opt_umem_flags
	};

	int ret;

	umem = calloc(1, sizeof(*umem));
	if (!umem)
		exit_with_error(errno);

	ret = xsk_umem__create(&umem->umem, buffer, size, &umem->fq, &umem->cq,
			       &cfg);

	if (ret)
		exit_with_error(-ret);

	umem->buffer = buffer;
	return umem;
}

static struct xsk_socket_info *xsk_configure_socket(struct xsk_umem_info *umem)
{
	struct xsk_socket_config cfg;
	struct xsk_socket_info *xsk;
	int ret;
	u32 idx;
	int i;

	xsk = calloc(1, sizeof(*xsk));
	if (!xsk)
		exit_with_error(errno);

	xsk->umem = umem;
	cfg.rx_size = XSK_RING_CONS__DEFAULT_NUM_DESCS;
	cfg.tx_size = XSK_RING_PROD__DEFAULT_NUM_DESCS;
	cfg.libbpf_flags = 0;
	cfg.xdp_flags = opt_xdp_flags;
	cfg.bind_flags = opt_xdp_bind_flags;
	ret = xsk_socket__create(&xsk->xsk, opt_if, opt_queue, umem->umem,
				 &xsk->rx, &xsk->tx, &cfg);
	if (ret)
		exit_with_error(-ret);

	ret = bpf_get_link_xdp_id(opt_ifindex, &prog_id, opt_xdp_flags);
	if (ret)
		exit_with_error(-ret);

	ret = xsk_ring_prod__reserve(&xsk->umem->fq,
				     XSK_RING_PROD__DEFAULT_NUM_DESCS,
				     &idx);
	if (ret != XSK_RING_PROD__DEFAULT_NUM_DESCS)
		exit_with_error(-ret);
	for (i = 0; i < XSK_RING_PROD__DEFAULT_NUM_DESCS; i++)
		*xsk_ring_prod__fill_addr(&xsk->umem->fq, idx++) =
			i * opt_xsk_frame_size;
	xsk_ring_prod__submit(&xsk->umem->fq,
			      XSK_RING_PROD__DEFAULT_NUM_DESCS);

	return xsk;
}

static void parse_params()
{
	opt_ifindex = if_nametoindex(opt_if);
	if (!opt_ifindex) {
		fprintf(stderr, "ERROR: interface \"%s\" does not exist\n",
			opt_if);
	}

	if ((opt_xsk_frame_size & (opt_xsk_frame_size - 1)) &&
	    !opt_unaligned_chunks) {
		fprintf(stderr, "--frame-size=%d is not a power of two\n",
			opt_xsk_frame_size);
	}
}


static void rx_drop(struct xsk_socket_info *xsk, struct pollfd *fds)
{
	unsigned int rcvd, i;
	u32 idx_rx = 0, idx_fq = 0;
	int ret;

	rcvd = xsk_ring_cons__peek(&xsk->rx, BATCH_SIZE, &idx_rx);
	if (!rcvd) {
		if (xsk_ring_prod__needs_wakeup(&xsk->umem->fq))
			ret = poll(fds, num_socks, opt_timeout);
		return;
	}

	ret = xsk_ring_prod__reserve(&xsk->umem->fq, rcvd, &idx_fq);
	while (ret != rcvd) {
		if (ret < 0)
			exit_with_error(-ret);
		if (xsk_ring_prod__needs_wakeup(&xsk->umem->fq))
			ret = poll(fds, num_socks, opt_timeout);
		ret = xsk_ring_prod__reserve(&xsk->umem->fq, rcvd, &idx_fq);
	}

	for (i = 0; i < rcvd; i++) {
		u64 addr = xsk_ring_cons__rx_desc(&xsk->rx, idx_rx)->addr;
		u32 len = xsk_ring_cons__rx_desc(&xsk->rx, idx_rx++)->len;
		u64 orig = xsk_umem__extract_addr(addr);

		addr = xsk_umem__add_offset_to_addr(addr);
		char *pkt = xsk_umem__get_data(xsk->umem->buffer, addr);

		process_packet(pkt, len);
		*xsk_ring_prod__fill_addr(&xsk->umem->fq, idx_fq++) = orig;
	}

	xsk_ring_prod__submit(&xsk->umem->fq, rcvd);
	xsk_ring_cons__release(&xsk->rx, rcvd);
	xsk->rx_npkts += rcvd;
}

void rx(void)
{
	struct pollfd pfd;
	int i, ret;

	// TODO: modify to suit epoll for eventloop and run-to-complete mode
	pfd.fd = xsk_socket__fd(xsks->xsk);
	pfd.events = POLLIN;
	
	for (;;) {
		rx_drop(xsks, &pfd);
	}
}

static void kick_tx(struct xsk_socket_info *xsk)
{
	int ret;

	ret = sendto(xsk_socket__fd(xsk->xsk), NULL, 0, MSG_DONTWAIT, NULL, 0);
	if (ret >= 0 || errno == ENOBUFS || errno == EAGAIN || errno == EBUSY)
		return;
	exit_with_error(errno);
}

static inline void complete_tx_only(struct xsk_socket_info *xsk)
{
	unsigned int rcvd;
	u32 idx;

	if (!xsk->outstanding_tx)
		return;
	/*
		* it is the case that there is no data ready,when the kernel
		* has detected that there are no more buffers on the FILL ring
		* and no buffers left on the RX HW ring of the NIC. In this case,
		* interrupts are turned off as the NIC cannot receive any packets
		* (as there are no buffers to put them in), and the need_wakeup 
		* flag is set so that user space can put buffers on the FILL ring 
		* and then call poll() so that the kernel driver can put these buffers
		* on the HW ring and start to receive packets.
		*/ 
	if (!opt_need_wakeup || xsk_ring_prod__needs_wakeup(&xsk->tx))
		kick_tx(xsk);

	rcvd = xsk_ring_cons__peek(&xsk->umem->cq, BATCH_SIZE, &idx);
	if (rcvd > 0) {
		xsk_ring_cons__release(&xsk->umem->cq, rcvd);
		xsk->outstanding_tx -= rcvd;
		xsk->tx_npkts += rcvd;
	}

	/* re-add completed Tx buffers, put cq to fq, is need? */
	// rcvd = xsk_ring_cons__peek(&umem->cq, ndescs, &idx_cq);
	// if (rcvd > 0) {
	// 	unsigned int i;
	// 	int ret;

	// 	ret = xsk_ring_prod__reserve(&umem->fq, rcvd, &idx_fq);
	// 	while (ret != rcvd) {
	// 		if (ret < 0)
	// 			exit_with_error(-ret);
	// 		if (xsk_ring_prod__needs_wakeup(&umem->fq))
	// 			ret = poll(fds, num_socks, opt_timeout);
	// 		ret = xsk_ring_prod__reserve(&umem->fq, rcvd, &idx_fq);
	// 	}

	// 	for (i = 0; i < rcvd; i++)
	// 		*xsk_ring_prod__fill_addr(&umem->fq, idx_fq++) =
	// 			*xsk_ring_cons__comp_addr(&umem->cq, idx_cq++);

	// 	xsk_ring_prod__submit(&xsk->umem->fq, rcvd);
	// 	xsk_ring_cons__release(&xsk->umem->cq, rcvd);
}

static void tx_only(struct xsk_socket_info *xsk, u32 frame_nb)
{
	u32 idx;

	if (xsk_ring_prod__reserve(&xsk->tx, BATCH_SIZE, &idx) == BATCH_SIZE) {
		unsigned int i;

		for (i = 0; i < BATCH_SIZE; i++) {
			xsk_ring_prod__tx_desc(&xsk->tx, idx + i)->addr	=
				(frame_nb + i) << XSK_UMEM__DEFAULT_FRAME_SHIFT;
			xsk_ring_prod__tx_desc(&xsk->tx, idx + i)->len =
				sizeof(pkt_data) - 1;
		}

		xsk_ring_prod__submit(&xsk->tx, BATCH_SIZE);
		xsk->outstanding_tx += BATCH_SIZE;
		frame_nb += BATCH_SIZE;
		frame_nb %= NUM_FRAMES;
	}

	complete_tx_only(xsk);
}

void tx(void)
{
	u32 frame_nb = 0;
	int i;
	for (i = 0; i < NUM_FRAMES; i++){
		(void)gen_eth_frame(umem, i * opt_xsk_frame_size);
	}

	for (;;) {
		tx_only(xsks, frame_nb);
	}
}

/*
* 
* sudo ./xdp_redirect_user
*/
int main()
{
	parse_params();
	/* Reserve memory for the umem. Use hugepages if unaligned chunk mode */
	bufs = mmap(NULL, NUM_FRAMES * opt_xsk_frame_size,
		    PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS | opt_mmap_flags, -1, 0);
	if (bufs == MAP_FAILED) {
		printf("ERROR: mmap failed\n");
		exit(EXIT_FAILURE);
	}
       /* Create sockets... */
	umem = xsk_configure_umem(bufs, NUM_FRAMES * opt_xsk_frame_size);
	xsks = xsk_configure_socket(umem);

	// signal(SIGINT, int_exit);
	// signal(SIGTERM, int_exit);
	// signal(SIGABRT, int_exit);

	rx();
	// tx();
	int xdp_sock_fd = xsk_socket__fd(xsks->xsk);
	return xdp_sock_fd;
}
