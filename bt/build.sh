aarch64-linux-gnu-gcc -o bt_client bt_client.c \
                      -I/home/h/bluez/usr/include \
                      -L/home/h/bluez/usr/lib -lbluetooth

aarch64-linux-gnu-gcc -o bt_server bt_server.c \
                      -I/home/h/bluez/usr/include \
                      -L/home/h/bluez/usr/lib -lbluetooth