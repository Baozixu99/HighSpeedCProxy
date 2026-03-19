aarch64-linux-gnu-gcc -o bt_clientv1 bt_client.c \
                      -I../lib/bt/bluez/usr/include \
                      -L../lib/bt/bluez/usr/lib -lbluetooth

aarch64-linux-gnu-gcc -o bt_serverv1 bt_server.c \
                      -I../lib/bt/bluez/usr/include \
                      -L../lib/bt/bluez/usr/lib -lbluetooth
