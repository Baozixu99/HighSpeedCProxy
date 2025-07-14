#include "backend_proto.h"




int backend_proxy_msg_prosess(uint8_t *msg){
    return 0;
}
int backend_proxy_msg_response(uint8_t *msg){
    return 0;
}


int backend_proxy_dev_msg_prosess(uint8_t *msg){
    return 0;
}
int backend_proxy_dev_msg_response(uint8_t *msg){
    return 0;
}

int backend_proxy_strgy_msg_process(uint8_t *msg){
    return 0;
}
int backend_proxy_strgy_msg_response(uint8_t *msg){
    return 0;
}

int backend_proxy_sess_msg_process(uint8_t *msg){
    return 0;
}
int backend_proxy_sess_msg_response(uint8_t *msg){
    return 0;
}

int backend_proxy_data_msg_recv(struct BackendSession *sess, uint8_t *msg){
    return 0;
}
int backend_proxy_data_msg_send(struct BackendSession *sess, uint8_t *msg){
    return 0;
}