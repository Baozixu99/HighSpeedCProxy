#include "engine.h"
#include "iniparser.h"


BackendEngine *p_g_bk_eng;
BackendEngine g_bk_eng;


BackendEngine *get_global_backend_engine(){
    return p_g_bk_eng;
}


 /*
  * The high-speed network device configuration item in the INI file must comply with the following format:
  * [Device Name]
  * ip_addr = IP address
  * dev_id = device ID
  * dev_type = device type
  * dev_status = device status
  * ns_id = namespace ID
  *
  * Example:
  * [ens01]
  * ip_addr =        192.168.10.10
  * dev_id  =        0
  * dev_type =       0
  * dev_status =     1
  * ns_id =          100
  */

int engine_init_hs_net_dev(BackendEngine *eng){
    struct HighSpeedNetDeviceSet *set = NULL;
    struct HighSpeedNetDevice *hs_dev;
    dictionary *ini;
    int dev_num, dev_name_len, cnt = 0;
    const char *dev_name, *ip_addr;
    int ip_type, dev_id, dev_type, dev_status, ns_id;
    struct in_addr in4_addr;
    struct in6_addr in6_addr;
    union IPAddress *ip_data;

    char dev_pro_item[MAX_DEV_NAME + MAX_DEVICE_PROPERTY_NAME_LENGTH];

    set = malloc(sizeof(struct HighSpeedNetDeviceSet));
    if(NULL == set){
        error_print("engine_init_hs_net_dev() returns an error, because of allocating memory for device set failed!\n");
        goto hs_net_error;
    }

//    ini = iniparser_load(HS_NET_DEV_CFG);
    ini = iniparser_load("hs_net_dev.ini");
    if (NULL == ini) {
        error_print("engine_init_hs_net_dev() returns an error, because of opening INI file failed!\n");
        goto hs_net_error;
    }

/*
 * Get the number of [Device Name] sections.
 */
    dev_num = iniparser_getnsec(ini);
    if(0 == dev_num){
        error_print("engine_init_hs_net_dev() returns an error because no high-speed network device information exists in the INI file!\n");
        goto hs_net_error;
    }


    if(dev_num > MAX_HS_DEV_NUM){
        error_print("engine_init_hs_net_dev() returns an error because the number of high-speed network device exceeds MAX_HS_DEV_NUM!\n");
        goto hs_net_error;
    }

/*
 * Initialize high speed network device one by one.
 */
    for(; cnt < dev_num; cnt++){
        dev_name = iniparser_getsecname(ini, cnt);
        dev_name_len = strlen(dev_name);
/*
 * Make sure the length of the device name in the INI file does not exceed MAX_DEV_NAME.
 */
        if(dev_name_len > MAX_DEV_NAME){
            error_print("engine_init_hs_net_dev() returns an error because the high-speed network device names in the INI file are not valid!\n");
            goto hs_net_error;
        }

        hs_dev = &set->hs_net_dev[cnt];
        snprintf(hs_dev->name, dev_name_len, "%s", dev_name);

        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("ip_addr"), "%s:ip_addr", dev_name);
        ip_addr = iniparser_getstring(ini, dev_pro_item, NULL);
        if(NULL == ip_addr){
            error_print("engine_init_hs_net_dev() returns an error because there is at least one high-speed network device without an IP address configured in the INI file!\n");
            goto hs_net_error;
        }


/*
 *Check if the string in the ip_addr item is valid.
 * If valid, convert it to an IPv4 or IPv6 address.
 */
        ip_type = DEV_IP_TYPE(ip_addr);
        if(SESS_NON_IP_PROTO == ip_type){
            error_print("engine_init_hs_net_dev() returns an error because there is at least one high-speed network device with an invalid IP address string configured in the INI file!\n");
            goto hs_net_error;
        }else if(SESS_IPV4_PROTO == ip_type){
            inet_pton(AF_INET, ip_addr, &in4_addr);
            ip_data = &hs_dev->address;
            COPY_IN_TO_IPV4(&ip_data->ipv4_addr, &in4_addr);
        }else{
            inet_pton(AF_INET6, ip_addr, &in6_addr);
            ip_data = &hs_dev->address;
            COPY_IN6_TO_IPV6(&ip_data->ipv6_addr, &in6_addr);
        }

/*
 * Check if the content of the dev_id item is valid.
 * If valid, convert it to an integer.
 */
        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("dev_id"), "%s:dev_id", dev_name);
        dev_id = iniparser_getint(ini, dev_pro_item, -1);

        if(-1 == dev_id){
            error_print("engine_init_hs_net_dev() returns an error because there is at least one high-speed network device for which the dev ID is either incorrectly configured \
            or not configured in the INI file.\n");
            goto hs_net_error;
        }

        hs_dev->dev_id = dev_id;

/*
 * Check if the content of the dev_type item is valid.
 * If valid, convert it to an integer.
 */
        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("dev_type"), "%s:dev_type", dev_name);
        dev_type = iniparser_getint(ini, dev_pro_item, -1);

        if(!IS_VALID_HS_NET_DEV_TYPE(dev_type)){
            error_print("engine_init_hs_net_dev() returns an error because there is at least one high-speed network device for which the dev type is either incorrectly configured \
            or not configured in the INI file.\n");
            goto hs_net_error;
        }

        hs_dev->dev_type = dev_type;
/*
 * Check if the content of the dev_status item is valid.
 * If valid, convert it to an integer.
 */
        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("dev_status"), "%s:dev_status", dev_name);
        dev_status = iniparser_getint(ini, dev_pro_item, -1);

        if(!IS_VALID_HS_NET_DEV_STATUS(dev_status)){
            error_print("engine_init_hs_net_dev() returns an error because there is at least one high-speed network device for which the dev status is either incorrectly configured \
            or not configured in the INI file.\n");
            goto hs_net_error;
        }

        hs_dev->dev_status = dev_status;

/*
 * Check if the content of the ns_id item is valid.
 * If valid, convert it to an integer.
 */
        memset(dev_pro_item, 0, sizeof(dev_pro_item));
        snprintf(dev_pro_item, dev_name_len + strlen("ns_id"), "%s:ns_id", dev_name);
        ns_id = iniparser_getint(ini, dev_pro_item, -1);
        if(-1 == ns_id){
            error_print("engine_init_hs_net_dev() returns an error because there is at least one high-speed network device for which the dev ID is either incorrectly configured \
            or not configured in the INI file.\n");
            goto hs_net_error;
        }

        hs_dev->ns_id = ns_id;

        TAILQ_INIT(&hs_dev->conn_q);

    }//  for(; cnt < dev_num; cnt++)

/*
 * Reclaim memory resources.
 */
    iniparser_freedict(ini);
    free(set);

    return BACKEND_PROXY_PROSESS_OK;
hs_net_error:
    if(NULL != ini)
        iniparser_freedict(ini);

    if(NULL != set)
        free(set);

    return BACKEND_PROXY_PROSESS_ERROR;
}

int engine_init_sess_pool(BackendEngine *eng){
    struct BackendSessionPool *sess_pool = NULL;

/*
 * Initialize the session pool.
 */
    sess_pool = (struct BackendSessionPool*)malloc(sizeof(struct BackendSessionPool));
    if(NULL == sess_pool){
        error_print("engine_init_sess_pool() returns an error because there is not enough memory to allocate the session pool.\n");
        return BACKEND_PROXY_PROSESS_ERROR;
    }

    high_speed_init_pool(sess_pool);

    eng->sess_pool = high_speed_pool = sess_pool;

    return BACKEND_PROXY_PROSESS_OK;
}


int engine_init_shared_mem_pool(BackendEngine *eng){
    struct SharedMemoryPool *mem_pool;
    int ret;
/*
 * Initialize the shared memory pool.
 */
    mem_pool = malloc(sizeof(struct SharedMemoryPool));
    if(NULL == mem_pool){
        error_print("engine_init_sess_pool() returns an error because there is not enough memory to allocate the shared memory pool.\n");
        return BACKEND_PROXY_PROSESS_ERROR;
    }

    ret = init_shared_mem_pool(mem_pool);
    if(BACKEND_PROXY_PROSESS_OK != ret){
        error_print("engine_init_sess_pool() returns an error because it cannot initialize the shared memory pool successfully.\n");
        free(mem_pool);
        return BACKEND_PROXY_PROSESS_ERROR;
    }

    return BACKEND_PROXY_PROSESS_OK;
}


int engine_init_shared_mem_pool_lock(BackendEngine *eng){
    struct SharedMemoryPoolLock *mem_pool_lock;
    int ret;
/*
 * Initialize the shared memory pool lock.
 */
    mem_pool_lock = malloc(sizeof(struct SharedMemoryPoolLock));
    if(NULL == mem_pool_lock){
        error_print("engine_init_sess_pool() returns an error because there is not enough memory to allocate the shared memory pool lock.\n");
        return BACKEND_PROXY_PROSESS_ERROR;
    }

    ret = init_shared_mem_pool_lock(mem_pool_lock);
    if(BACKEND_PROXY_PROSESS_OK != ret){
        error_print("engine_init_sess_pool() returns an error because it cannot initialize the shared memory pool lock successfully.\n");
        free(mem_pool_lock);
        return BACKEND_PROXY_PROSESS_ERROR;
    }

    return BACKEND_PROXY_PROSESS_OK;
}


void engine_init()
{
    int ret;

    p_g_bk_eng = &g_bk_eng;
    memset(p_g_bk_eng, 0, sizeof(BackendEngine));

    ret = engine_init_hs_net_dev(p_g_bk_eng);

    if(BACKEND_PROXY_PROSESS_OK != ret){
        error_print("engine_init_hs_net_dev() returns error!");
        return;
    }

    ret = engine_init_sess_pool(p_g_bk_eng);

    if(BACKEND_PROXY_PROSESS_OK != ret){
        error_print("engine_init_sess_pool() returns error!");
        return;
    }

    ret = engine_init_shared_mem_pool(p_g_bk_eng);

    if(BACKEND_PROXY_PROSESS_OK != ret){
        error_print("engine_init_shared_mem_pool() returns error!");
        return;
    }

    ret = engine_init_shared_mem_pool_lock(p_g_bk_eng);

    if(BACKEND_PROXY_PROSESS_OK != ret){
        error_print("engine_init_shared_mem_pool_lock() returns error!");
        return;
    }
}

void engine_run()
{
    
}