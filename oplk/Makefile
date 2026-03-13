all: 
	aarch64-linux-gnu-gcc -static -o mn /home/h/HighSpeedCProxy/oplk/common/src/obdcreate/obdcreate.c mn.c \
		-DCONFIG_INCLUDE_CFM -DCONFIG_INCLUDE_PDO -DCONFIG_INCLUDE_SDO_ASND -DCONFIG_KERNELSTACK_DIRECTLINK -DNMT_MAX_NODE_ID=254 \
		-I../lib/oplk/stack/include/ \
		-I./common/src \
		-I./common/objdicts/CiA302-4_MN/ \
		-I./common/objdicts/CiA401_CN/ \
		-L../lib/oplk/stack/linux/arm/ \
		-L../lib/oplk/libpcap/ \
		-loplkmn -lpcap -lrt -lpthread
	
	aarch64-linux-gnu-gcc -static -o cn /home/h/HighSpeedCProxy/oplk/common/src/obdcreate/obdcreate.c cn.c \
		-DCONFIG_INCLUDE_MASND -DCONFIG_INCLUDE_PDO -DCONFIG_INCLUDE_SDO_ASND -DCONFIG_KERNELSTACK_DIRECTLINK -DNMT_MAX_NODE_ID=0 \
		-I../lib/oplk/stack/include/ \
		-I./common/src \
		-I./common/objdicts/CiA302-4_MN/ \
		-I./common/objdicts/CiA401_CN/ \
		-L../lib/oplk/stack/linux/arm/ \
		-L../lib/oplk/libpcap/ \
		-loplkcn -lpcap -lrt -lpthread 
