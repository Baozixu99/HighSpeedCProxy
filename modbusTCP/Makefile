all: 
	aarch64-linux-gnu-gcc -static -o mbserver server.c \
		-I../lib/modbusTCP/include/ \
		-L../lib/modbusTCP/lib/ \
		-lmodbus
	aarch64-linux-gnu-gcc -static -o mbclient client.c \
		-I../lib/modbusTCP/include/ \
		-L../lib/modbusTCP/lib/ \
		-lmodbus	