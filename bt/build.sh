aarch64-none-linux-gnu-gcc -o bt_client bt_client.c \
                      -I/home/h/HighSpeedCProxy/lib/bt/bluez/usr/include \
                      -L/home/h/HighSpeedCProxy/lib/bt/bluez/usr/lib -lbluetooth

aarch64-none-linux-gnu-gcc -o bt_server bt_server.c \
                      -I/home/h/HighSpeedCProxy/lib/bt/bluez/usr/include \
                      -L/home/h/HighSpeedCProxy/lib/bt/bluez/usr/lib -lbluetooth