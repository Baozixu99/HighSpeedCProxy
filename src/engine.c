#include "engine.h"


BackendEngine *p_g_bk_eng;
BackendEngine g_bk_eng;


BackendEngine *get_global_backend_engine(){
    return p_g_bk_eng;
}

int engine_init_hs_net_dev(BackendEngine *eng){
    struct HighSpeedNetDeviceSet *set;


    return BACKEND_PROXY_PROSESS_OK;
}

int engine_init_sess_pool(BackendEngine *eng){
    return BACKEND_PROXY_PROSESS_OK;
}


int engine_init_mem_pool(BackendEngine *eng){
    return BACKEND_PROXY_PROSESS_OK;
}

int engine_init_mem_pool_lock(BackendEngine *eng){
    return BACKEND_PROXY_PROSESS_OK;
}


void engine_init()
{
    int ret;

    p_g_bk_eng = &g_bk_eng;
    memset(p_g_bk_eng, 0, sizeof(BackendEngine));

    ret = engine_init_hs_net_dev(p_g_bk_eng);

    if(BACKEND_PROXY_PROSESS_OK != ret){
        error_print("engine_init_hs_net_dev returns error!");
        return;
    }

    ret = engine_init_sess_pool(p_g_bk_eng);

    if(BACKEND_PROXY_PROSESS_OK != ret){
        error_print("engine_init_sess_pool returns error!");
        return;
    }

    ret = engine_init_mem_pool(p_g_bk_eng);

    if(BACKEND_PROXY_PROSESS_OK != ret){
        error_print("engine_init_mem_pool returns error!");
        return;
    }

    ret = engine_init_mem_pool_lock(p_g_bk_eng);

    if(BACKEND_PROXY_PROSESS_OK != ret){
        error_print("engine_init_mem_pool_lock returns error!");
        return;
    }
}

void engine_run()
{
    
}