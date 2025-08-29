# clang -O2 -g -Wall -target bpf -c xdp_redirect_sock_kern.c -o xdp_redirect.o
aarch64-linux-gnu-gcc -o xdp_redirect_user xdp_redirect_sock_user.c /home/h/libbpf-arm64/usr/lib/aarch64-linux-gnu/libbpf.a \
                      -I/home/h/libbpfdev-arm64/usr/include \
                      -L/home/h/libbpf-arm64/usr/lib/aarch64-linux-gnu -lelf \
                      -L/home/h/libbpf-arm64/usr/lib/aarch64-linux-gnu -lz \
                      

# gcc -o xdp_redirect_user xdp_redirect_sock_user.c -lbpf