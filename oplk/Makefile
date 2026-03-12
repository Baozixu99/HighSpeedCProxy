all: 
	aarch64-linux-gnu-gcc -static -o mn mn.c \
		-I../lib/oplk/stack/include/ \
		-L../lib/oplk/stack/linux/arm/ \
		-L../lib/oplk/libpcap/ \
		-loplkmn -lpcap -lrt -lpthread
	
	aarch64-linux-gnu-gcc -static -o cn cn.c \
		-I../lib/oplk/stack/include/ \
		-L../lib/oplk/stack/linux/arm/ \
		-L../lib/oplk/libpcap/ \
		-loplkcn -lpcap -lrt -lpthread 
