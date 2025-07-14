// SPDX-License-Identifier: GPL-2.0
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

#define MAX_SOCKS 4
struct{
    __uint(type, BPF_MAP_TYPE_XSKMAP);
    __uint(key_size, sizeof(int));
    __uint(value_size, sizeof(int));
	__uint(max_entries, MAX_SOCKS);
}xsks_map SEC(".maps");


SEC("xdp_sock") int xdp_sock_prog(struct xdp_md *ctx)
{
    int index = ctx->rx_queue_index;

    // A set entry here means that the corresponding queue_id
    // has an active AF_XDP socket bound to it.
    if (bpf_map_lookup_elem(&xsks_map, &index))
        return bpf_redirect_map(&xsks_map, index, 0);

    return XDP_PASS;
}
char _license[] SEC("license") = "GPL";
//clang -g -c -O2 -target bpf -c xdpsock_kern.c -o xdpsock_kern.o
