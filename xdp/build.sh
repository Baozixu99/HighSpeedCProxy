# clang -O2 -g -Wall -target bpf -c xdp_redirect_sock_kern.c -o xdp_redirect.o
gcc -o xdp_redirect_user xdp_redirect_sock_user.c -lbpf -lpthread