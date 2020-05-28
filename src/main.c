#include "mgos.h"
#include "mgos_wifi.h"
#include "mgos_rpc.h"
#include "mgos_net.h"
#include "mg_rpc_channel_tcp_common.h"

static void sum_cb(struct mg_rpc_request_info *ri, void *cb_arg,
                   struct mg_rpc_frame_info *fi, struct mg_str args)
{
    LOG(LL_INFO, ("executing timer code"));
    double a = 0, b = 0;
    if (json_scanf(args.p, args.len, ri->args_fmt, &a, &b) == 2)
    {
        mg_rpc_send_responsef(ri, "%.2lf", a + b);
    }
    else
    {
        mg_rpc_send_errorf(ri, -1, "Bad request. Expected: {\"a\":N1,\"b\":N2}");
    }
    (void)cb_arg;
    (void)fi;
}

static void send_data_to_server_cb(void *arg)
{
    LOG(LL_INFO, ("Preparing to send data to server"));
    // struct mg_rpc_call_opts opts = {.dst = mg_mk_str("ws://192.168.0.104:5000")};
    // mg_rpc_callf(mgos_rpc_get_global(), mg_mk_str("ping"), NULL, NULL, &opts,
    //              NULL);

    // Looking for our websocket connection
    struct mg_mgr *mgr;
    struct mg_connection *c;
    mgr = mgos_get_mgr();

    for (c = mg_next(mgr, NULL); c != NULL; c = mg_next(mgr, c))
    {
        if (c->flags & MG_F_IS_WEBSOCKET)
        {
            char *buf = mg_rpc_channel_tcp_get_info(c);
            LOG(LL_INFO, ("Found a ws connection with destination: %s", buf));
            break;
        }
    }
    if (c != NULL)
    {
        LOG(LL_INFO, ("Sending data to server"));
        // create json string to send via websocket
        char data[] = "{\"jsonrpc\":\"2.0\", \"method\":\"test\"}";
        mg_send_websocket_frame(c, WEBSOCKET_OP_TEXT, data, sizeof(data)-1);
        LOG(LL_INFO, ("Data sent to server"));
    }
    (void)arg;
}

static void connect_to_ws_cb(int ev, void *ev_data, void *userdata)
{
    LOG(LL_INFO, ("We are connected to the internet."));
    struct mg_rpc_call_opts opts = {.dst = mg_mk_str("ws://68.183.90.153:5000")};
    mg_rpc_callf(mgos_rpc_get_global(), mg_mk_str("ping"), NULL, NULL, &opts,
                 NULL);
    LOG(LL_INFO, ("Connecting to WebSocket server."));
    (void)ev;
    (void)ev_data;
    (void)userdata;
}

enum mgos_app_init_result mgos_app_init(void)
{
    mgos_usleep(5000);

    // switching on the GSM modem
    mgos_gpio_set_mode(15, MGOS_GPIO_MODE_OUTPUT);
    mgos_gpio_write(15, false);
    mgos_msleep(4000); // sleeping for 4 seconds to turn on gsm modem
    mgos_gpio_write(15, true);

    // Register JSON RPC Callback
    mg_rpc_add_handler(mgos_rpc_get_global(), "Sum", "{a: %lf, b: %lf}", sum_cb, NULL);

    // To check when we are connected to the internet
    mgos_event_add_handler(MGOS_NET_EV_IP_ACQUIRED, connect_to_ws_cb, NULL);

    // Register callback for a timer to send information to the server
    mgos_set_timer(1000, MGOS_TIMER_REPEAT, send_data_to_server_cb, NULL);

    return MGOS_APP_INIT_SUCCESS;
}
