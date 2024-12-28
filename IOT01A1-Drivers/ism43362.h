#ifndef ISM43362_H
#define ISM43362_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    Ok,
    Error,
    RespBufferTooSmall,
    BadResponse,
    WrongInitMsg,
    PacketBufferTooSmall,
} ISM43362_RET;

void ism43362_drdy_exti_callback();
void ism43362_ret_to_str(ISM43362_RET ret, char *s, size_t len);
ISM43362_RET ism43362_reset_module();
ISM43362_RET ism43362_transmit_buffer(const uint8_t *tr_buffer, size_t tr_size, uint8_t *resp, size_t resp_buff_len,
                                      size_t *resp_len);
ISM43362_RET ism43362_execute_cmd(const char *cmd, uint8_t *resp, size_t resp_buff_len, size_t *resp_len);
ISM43362_RET ism43362_enter_cmd_mode();
ISM43362_RET ism43362_enter_machine_mode();

typedef enum { OPEN = 0, WEP = 1, WPA = 2, WPA2 = 3, WPA_WPA2 = 4, WPA2_TKIP = 5 } WifiSecurity;

typedef enum { US_0, CA_0, FR_0, JP_0 } CountryCode;

typedef enum { WEP_OPEN = 0, WEP_SHARED_KEY = 1 } WEPAuthType;

typedef struct {
    char ssid[32];
    char password[65];
    WifiSecurity security;
    bool dhcp;
    uint8_t ip[4];
    uint8_t netmask[4];
    uint8_t gateway[4];
    uint8_t primary_dns[4];
    uint8_t secondary_dns[4];
    uint8_t join_retry_count; // between 0 and 10
    WEPAuthType wep_auth;
    CountryCode country_code;
    bool is_connected;
} JoinWifiConfig;

JoinWifiConfig ism43362_get_default_wifi_config();
ISM43362_RET ism43362_join_network(const JoinWifiConfig *conf);
ISM43362_RET ism43362_read_wifi_config(JoinWifiConfig *conf);

typedef enum { SOCKET_0 = 0, SOCKET_1 = 1, SOCKET_2 = 2, SOCKET_3 = 3 } Socket;

ISM43362_RET ism43362_set_socket(Socket s);

typedef enum { TCP = 0, UDP = 1, UDP_LITE = 2 } TransportProtocol;

typedef struct {
    uint8_t ip[4];
    uint16_t port;
} WifiRemote;

ISM43362_RET ism43362_get_remote(WifiRemote *remote);

typedef struct {
    Socket s;
    TransportProtocol protocol;
    WifiRemote remote;
    uint16_t read_packet_size; // 1 to 1460
    uint16_t read_timeout_ms; // 0 to 30000
    uint16_t write_timeout_ms; // 0 to 30000
} WifiClientConfig;

WifiClientConfig ism43362_get_default_client_config();
ISM43362_RET ism43362_start_wifi_client(const WifiClientConfig *client);
ISM43362_RET ism43362_send(uint8_t *packet, size_t size);
ISM43362_RET ism43362_read(uint8_t *packet_buff, size_t buff_size, size_t *packet_size);

typedef struct {
    Socket s;
    uint16_t local_port;
    uint16_t read_packet_size; // 0 to 1460
    uint16_t read_timeout_ms; // 0 to 30000
    uint16_t write_timeout_ms; // 0 to 30000
} WifiBaseServerConfig;

WifiBaseServerConfig ism43362_get_default_base_server_config();
ISM43362_RET ism43362_start_udp_server(WifiBaseServerConfig *conf);

typedef struct {
    WifiBaseServerConfig base_conf;
    uint8_t listen_backlogs; // 1 to 6
    bool keep_alive_enabled;
    uint32_t keep_alive_timeout_ms; // 250 to 7200000
} WifiTcpServerConfig;

WifiTcpServerConfig ism43362_get_default_tcp_server_config();

typedef struct {
    bool connected;
    WifiRemote remote;
} RemoteTcpConnection;

ISM43362_RET ism43362_start_tcp_server(WifiTcpServerConfig *conf);
ISM43362_RET ism43362_check_tcp_server_connection(RemoteTcpConnection *conn);
ISM43362_RET ism43362_tcp_server_close_curr_conn();

#endif
