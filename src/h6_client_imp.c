#include "h6_client_imp.h"
#include "h6_client.h"
#include "h6_msg.h"

static uint32_t
h6_impl_recognize(h6_client *rc, msg_t *req)
{
    h6_msg_t *rm;
    RTSP_MSG_TYPE mt;
    RTSP_METHOD method;
    uint32_t rtsp = TR_RTSP_UNKNOWN;

    rm = msg_to_rtsp_message(req);
    mt = rtsp_message_get_type(rm);

    if (mt == RTSP_MESSAGE_DATA)
        return TR_RTSP_DATA;

    if (mt != RTSP_MESSAGE_REQUEST)
        return TR_RTSP_RESPONSE;

    rtsp_message_parse_request(rm, &method, NULL, NULL, NULL);
    switch (method)
    {
    case RTSP_DESCRIBE:
        rtsp = TR_RTSP_DESC;
        break;

    case RTSP_OPTIONS:
        rtsp = TR_RTSP_OPTION;
        break;

    case RTSP_SETUP:
        rtsp = TR_RTSP_SETUP;
        break;
    
    case RTSP_PLAY:
        rtsp = TR_RTSP_PLAY;
        break;

    case RTSP_PAUSE:
        rtsp = TR_RTSP_PAUSE;
        break;
    
    case RTSP_TEARDOWN:
        rtsp = TR_RTSP_TEARDOWN;
        break;

    case RTSP_SET_PARAMETER:
        rtsp = TR_RTSP_SETPARM;
        break;

    case RTSP_GET_PARAMETER:
        rtsp = TR_RTSP_GETPARM;
        break;

    default:
        break;
    }
    return rtsp;
}


static h6_client_ops h6_impl_client_ops = 
{
    .recognize              = h6_impl_recognize,
    .on_setup_channel       = h6_impl_on_setupchannel
    .on_response            = h6_impl_on_response,
    .on_data                = h6_impl_on_data,
    .on_register            = h6_impl_on_register,
    .on_heart_beat          = h6_impl_on_heart_beat,    
};

client_t *
h6_impl_client_new(uint32_t factory, void *sock)
{
    return (client_t*)h6_client_new(sizeof(h6_client),
        &h6_impl_client_ops, factory, sock);

}


static h6_client_ops h6_impl_ts_client_ops = 
{
    .init                   = h6_impl_ts_init,
    .fin                    = h6_impl_ts_fin,
    .kill                   = h6_impl_ts_kill,
    .recognize              = h6_impl_recognize,
    .on_setup_channel       = h6_impl_ts_on_setupchannel
    .on_response            = h6_impl_ts_on_response,
    .on_data                = h6_impl_ts_on_data,
    .on_register            = h6_impl_ts_on_register,
    .on_heart_beat          = h6_impl_ts_on_heart_beat,
};


client_t *
h6_impl_ts_client_new(uint32_t factory, void *sock)
{
    return (client_t*)h6_client_new(sizeof(h6_ts_client),
        &h6_impl_ts_client_ops, factory, sock);
}


int32_t
h6_impl_ts_client_killed(h6_ts_client *htc)
{
    int32_t killed;

    pthread_mutex_lock(rtc->lock);
    killed = htc->killed;
    pthread_mutex_unlock(rtc->lock);

    return killed;

}

