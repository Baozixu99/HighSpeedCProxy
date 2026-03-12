aarch64-none-linux-gnu-gcc -o bt_client bt_client.c \
                      -I../lib/bt/bluez/usr/include \
                      -L../lib/bt/bluez/usr/lib -lbluetooth

aarch64-none-linux-gnu-gcc -o bt_server bt_server.c \
                      -I../lib/bt/bluez/usr/include \
                      -L../lib/bt/bluez/usr/lib -lbluetooth