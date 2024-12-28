#include "ism43362.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../Inc/main.h"

volatile int data_ready = 0;
void ism43362_drdy_exti_callback() { data_ready = 1; }

#define ISM_DATA_RDY() (HAL_GPIO_ReadPin(ISM43362_DRDY_EXTI1_GPIO_Port, ISM43362_DRDY_EXTI1_Pin) == GPIO_PIN_SET)
#define ISM_ENABLE_CSN() HAL_GPIO_WritePin(ISM43362_SPI3_CSN_GPIO_Port, ISM43362_SPI3_CSN_Pin, GPIO_PIN_RESET)
#define ISM_DISABLE_CSN() HAL_GPIO_WritePin(ISM43362_SPI3_CSN_GPIO_Port, ISM43362_SPI3_CSN_Pin, GPIO_PIN_SET)
#define OK_MSG "\r\nOK\r\n"
#define RET_IF_NOT_OK(ret)                                                                                             \
    if (ret != Ok) {                                                                                                   \
        return ret;                                                                                                    \
    }
// #define USART1_LOG

extern SPI_HandleTypeDef hspi3;
#ifdef USART1_LOG
extern UART_HandleTypeDef huart1;
#endif

void ism43362_ret_to_str(ISM43362_RET ret, char *s, size_t len) {
    switch (ret) {
        case Ok:
            snprintf(s, len, "OK\r\n");
            break;
        case Error:
            snprintf(s, len, "Generic error\r\n");
            break;
        case RespBufferTooSmall:
            snprintf(s, len, "Response buffer is too small\r\n");
            break;
        case BadResponse:
            snprintf(s, len, "Response didn't contain OK string or badly formatted\r\n");
            break;
        case WrongInitMsg:
            snprintf(s, len, "Didn't get initial cursor\r\n");
            break;
        case PacketBufferTooSmall:
            snprintf(s, len, "Packet received too large for buffer\r\n");
            break;
        default:
            snprintf(s, len, "Unrecognized value %d\r\n", ret);
    }
}

ISM43362_RET ism43362_reset_module() {
    HAL_GPIO_WritePin(ISM43362_RST_GPIO_Port, ISM43362_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(ISM43362_RST_GPIO_Port, ISM43362_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(500);

    uint8_t init_cursor[6] = {0};
    size_t b_read = 0;
    bool read_too_much = false;
    ISM_ENABLE_CSN();
    while (ISM_DATA_RDY()) {
        if (b_read >= 6) {
            read_too_much = true;
            break;
        }
        HAL_SPI_Receive(&hspi3, init_cursor + b_read, 1, 0xFFFF);
        b_read += 2;
    }
    ISM_DISABLE_CSN();

    data_ready = 0;
    if (init_cursor[0] != 0x15 || init_cursor[1] != 0x15 || init_cursor[2] != '\r' || init_cursor[3] != '\n' ||
        init_cursor[4] != '>' || init_cursor[5] != ' ' || read_too_much) {
        return WrongInitMsg;
    }
    return Ok;
}

static void wait_data_ready() {
    while (data_ready == 0) {
        __NOP();
    }
    data_ready = 0;
}

ISM43362_RET ism43362_transmit_buffer(const uint8_t *tr_buffer, size_t tr_size, uint8_t *resp, size_t resp_buff_len,
                                      size_t *resp_len) {
    size_t cmd_len = tr_size;
    size_t words_len = cmd_len / 2;
    const uint8_t padding[2] = {'\n', tr_buffer[cmd_len - 1]};
    *resp = 0;

#ifdef USART1_LOG
    HAL_UART_Transmit(&huart1, tr_buffer, cmd_len, 1000);
#endif

    ISM_ENABLE_CSN();
    if (words_len > 0) {
        HAL_SPI_Transmit(&hspi3, tr_buffer, words_len, 1);
    }
    if (cmd_len % 2 == 1) {
        HAL_SPI_Transmit(&hspi3, padding, 1, 1);
    }
    data_ready = 0;
    ISM_DISABLE_CSN();

    wait_data_ready();

    size_t b_read = 0;
    bool resp_buff_full = false;
    ISM_ENABLE_CSN();
    while (ISM_DATA_RDY()) {
        if (b_read >= resp_buff_len - 1) {
            resp_buff_full = true;
            break;
        }
        HAL_SPI_Receive(&hspi3, resp + b_read, 1, 1);
        b_read += 2;
    }
    ISM_DISABLE_CSN();
    *resp_len = b_read;
    resp[b_read] = 0;
#ifdef USART1_LOG
    HAL_UART_Transmit(&huart1, (const uint8_t *) resp, *resp_len, 1000);
#endif

    HAL_Delay(1);

    if (resp_buff_full) {
        return RespBufferTooSmall;
    }

    if (strstr((char *) resp, OK_MSG) == NULL) {
        return BadResponse;
    }

    return Ok;
}

ISM43362_RET ism43362_execute_cmd(const char *cmd, uint8_t *resp, size_t resp_buff_len, size_t *resp_len) {
    return ism43362_transmit_buffer((const uint8_t *) cmd, strlen(cmd), resp, resp_buff_len, resp_len);
}

ISM43362_RET ism43362_enter_cmd_mode() {
    uint8_t resp[100] = {0};
    size_t resp_size = 0;
    return ism43362_execute_cmd("$$$\r\n", resp, sizeof(resp), &resp_size);
}

ISM43362_RET ism43362_enter_machine_mode() {
    uint8_t resp[100] = {0};
    size_t resp_size = 0;
    return ism43362_execute_cmd("---\r\n", resp, sizeof(resp), &resp_size);
}

JoinWifiConfig ism43362_get_default_wifi_config() {
    JoinWifiConfig conf = {.ssid = {0},
                           .password = {0},
                           .security = OPEN,
                           .dhcp = true,
                           .ip = {0, 0, 0, 0},
                           .netmask = {0, 0, 0, 0},
                           .gateway = {255, 255, 255, 255},
                           .primary_dns = {255, 255, 255, 255},
                           .secondary_dns = {255, 255, 255, 255},
                           .join_retry_count = 5,
                           .wep_auth = WEP_OPEN,
                           .country_code = US_0,
                           .is_connected = false};
    return conf;
}

ISM43362_RET ism43362_join_network(const JoinWifiConfig *conf) {
    if (conf == NULL || strlen(conf->ssid) == 0) {
        return Error;
    }

    ISM43362_RET ret;
    uint8_t buff[500] = {0};
    size_t buff_size = sizeof(buff);
    size_t resp_len = 0;
    char cmd[100] = {0};
    size_t cmd_size = sizeof(cmd);


    snprintf(cmd, cmd_size, "C1=%s\r\n", conf->ssid);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    if (strlen(conf->password) > 0) {
        snprintf(cmd, cmd_size, "C2=%s\r\n", conf->password);
        ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
        RET_IF_NOT_OK(ret);
    }

    snprintf(cmd, cmd_size, "C3=%d\r\n", conf->security);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, cmd_size, "C4=%d\r\n", conf->dhcp ? 1 : 0);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    if (!conf->dhcp) {
        snprintf(cmd, cmd_size, "C6=%d.%d.%d.%d\r\n", conf->ip[0], conf->ip[1], conf->ip[2], conf->ip[3]);
        ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
        RET_IF_NOT_OK(ret);

        snprintf(cmd, cmd_size, "C7=%d.%d.%d.%d\r\n", conf->netmask[0], conf->netmask[1], conf->netmask[2],
                 conf->netmask[3]);
        ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
        RET_IF_NOT_OK(ret);
    }


    snprintf(cmd, cmd_size, "C8=%d.%d.%d.%d\r\n", conf->gateway[0], conf->gateway[1], conf->gateway[2],
             conf->gateway[3]);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, cmd_size, "C9=%d.%d.%d.%d\r\n", conf->primary_dns[0], conf->primary_dns[1], conf->primary_dns[2],
             conf->primary_dns[3]);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, cmd_size, "CA=%d.%d.%d.%d\r\n", conf->secondary_dns[0], conf->secondary_dns[1],
             conf->secondary_dns[2], conf->secondary_dns[3]);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, cmd_size, "CB=%d\r\n", conf->join_retry_count);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    if (conf->security == WEP) {
        snprintf(cmd, cmd_size, "CE=%d\r\n", conf->wep_auth);
        ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
        RET_IF_NOT_OK(ret);
    }

    switch (conf->country_code) {
        case US_0:
            snprintf(cmd, cmd_size, "CN=US/0\r\n");
            break;
        case CA_0:
            snprintf(cmd, cmd_size, "CN=CA/0\r\n");
            break;
        case FR_0:
            snprintf(cmd, cmd_size, "CN=FR/0\r\n");
            break;
        case JP_0:
            snprintf(cmd, cmd_size, "CN=JP/0\r\n");
            break;
        default:
            return Error;
    }
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    return ism43362_execute_cmd("C0\r\n", buff, buff_size, &resp_len);
}

static void parse_ip_str(const char *str, uint8_t *ip_arr) {
    sscanf(str, "%d.%d.%d.%d", ip_arr, ip_arr + 1, ip_arr + 2, ip_arr + 3);
}

ISM43362_RET ism43362_read_wifi_config(JoinWifiConfig *conf) {
    if (conf == NULL) {
        return Error;
    }

    uint8_t buff[500] = {0};
    size_t buff_size = sizeof(buff);
    size_t resp_len = 0;
    ISM43362_RET ret = ism43362_execute_cmd("C?\r\n", buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    int dhcp;
    int ip_v;
    char ip_addr[50];
    char mask[50];
    char gw[50];
    char dns1[50];
    char dns2[50];
    int auto_conn;
    char cc[10];
    int status;
    sscanf(buff, "\r\n%[^,],%[^,],%d,%d,%d,%[^,],%[^,],%[^,],%[^,],%[^,],%d,%d,%d,%[^,],%d\r\nOK\r\n", conf->ssid,
           conf->password, conf->security, &dhcp, &ip_v, ip_addr, mask, gw, dns1, dns2, &conf->join_retry_count,
           &auto_conn, &conf->wep_auth, cc, &status);

    conf->dhcp = dhcp == 1;
    parse_ip_str(ip_addr, conf->ip);
    parse_ip_str(mask, conf->netmask);
    parse_ip_str(gw, conf->gateway);
    parse_ip_str(dns1, conf->primary_dns);
    parse_ip_str(dns2, conf->secondary_dns);
    switch (cc[0]) {
        case 'U':
            conf->country_code = US_0;
            break;
        case 'C':
            conf->country_code = CA_0;
            break;
        case 'F':
            conf->country_code = FR_0;
            break;
        case 'J':
            conf->country_code = JP_0;
            break;
        default:
            return BadResponse;
    }
    conf->is_connected = status == 1;

#ifdef USART1_LOG
    char m[200];
    snprintf(m, sizeof(m), "ssid: '%s'\r\n", conf->ssid);
    HAL_UART_Transmit(&huart1, m, strlen(m), 1000);
    snprintf(m, sizeof(m), "ip: '%s', [%d %d %d %d]\r\n", ip_addr, conf->ip[0], conf->ip[1], conf->ip[2], conf->ip[3]);
    HAL_UART_Transmit(&huart1, m, strlen(m), 1000);
    snprintf(m, sizeof(m), "ssid: '%s'\r\n", cc);
    HAL_UART_Transmit(&huart1, m, strlen(m), 1000);
#endif

    return ret;
}

ISM43362_RET ism43362_set_socket(Socket s) {
    uint8_t buff[200] = {0};
    size_t buff_size = sizeof(buff);
    size_t resp_size;
    char cmd[7] = {0};
    snprintf(cmd, sizeof(cmd), "P0=%d\r\n", s);
    return ism43362_execute_cmd(cmd, buff, buff_size, &resp_size);
}

ISM43362_RET ism43362_get_remote(WifiRemote *remote) {
    uint8_t buff[1000];
    size_t read_len;
    ISM43362_RET ret = ism43362_execute_cmd("P?\r\n", buff, sizeof(buff), &read_len);
    RET_IF_NOT_OK(ret);
    int proto;
    int local_port;
    uint8_t host_ip[4];
    sscanf(buff, "\r\n%d,%d.%d.%d.%d,%d,%d.%d.%d.%d,%d,", &proto, remote->ip, remote->ip + 1, remote->ip + 2,
           remote->ip + 3, &local_port, host_ip, host_ip + 1, host_ip + 2, host_ip + 3, &remote->port);
    return Ok;
}

WifiClientConfig ism43362_get_default_client_config() {
    WifiClientConfig conf = {.s = SOCKET_0,
                             .protocol = TCP,
                             .remote = {.ip = {0, 0, 0, 0}, .port = 5025},
                             .read_packet_size = 1460,
                             .read_timeout_ms = 5000,
                             .write_timeout_ms = 5000};
    return conf;
}

ISM43362_RET ism43362_start_wifi_client(const WifiClientConfig *client) {
    if (client == NULL) {
        return Error;
    }

    char cmd[100];
    uint8_t buff[200] = {0};
    size_t buff_size = sizeof(buff);
    size_t resp_len;

    ISM43362_RET ret = ism43362_set_socket(client->s);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, sizeof(cmd), "P1=%d\r\n", client->protocol);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, sizeof(cmd), "P3=%d.%d.%d.%d\r\n", client->remote.ip[0], client->remote.ip[1], client->remote.ip[2],
             client->remote.ip[3]);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, sizeof(cmd), "P4=%d\r\n", client->remote.port);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, sizeof(cmd), "R1=%d\r\n", client->read_packet_size);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, sizeof(cmd), "R2=%d\r\n", client->read_timeout_ms);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, sizeof(cmd), "S2=%d\r\n", client->write_timeout_ms);
    ret = ism43362_execute_cmd(cmd, buff, buff_size, &resp_len);
    RET_IF_NOT_OK(ret);

    return ism43362_execute_cmd("P6=1\r\n", buff, buff_size, &resp_len);
}

ISM43362_RET ism43362_send(uint8_t *packet, size_t size) {
    uint8_t cmd[2000] = {0};
    snprintf(cmd, sizeof(cmd), "S3=%d\r", size);
    size_t start_data = strlen(cmd);
    memcpy((void *) (cmd + start_data), (void *) packet, size);

    size_t base_end = start_data + size;
    cmd[base_end] = '\r';
    cmd[base_end + 1] = '\n';
    size_t cmd_len = base_end + 2;

    uint8_t buff[200];
    size_t resp_size;
    return ism43362_transmit_buffer(cmd, cmd_len, buff, sizeof(buff), &resp_size);
}

ISM43362_RET ism43362_read(uint8_t *packet_buff, size_t buff_size, size_t *packet_size) {
    uint8_t resp[2000] = {0};
    size_t resp_len;
    ISM43362_RET ret = ism43362_execute_cmd("R0\r\n", resp, sizeof(resp), &resp_len);
    RET_IF_NOT_OK(ret);

    size_t rec_pckt_len = resp_len - 10; // "\r\nDATA\r\nOK\r\n> "
    size_t len;
    ISM43362_RET retval = Ok;
    if (rec_pckt_len > buff_size) {
        len = buff_size;
        retval = PacketBufferTooSmall;
    } else {
        len = rec_pckt_len;
    }
    memcpy(packet_buff, resp + 2, len);
    *packet_size = len;
    return retval;
}

WifiBaseServerConfig ism43362_get_default_base_server_config() {
    WifiBaseServerConfig conf = {.s = SOCKET_0,
                                 .local_port = 5024,
                                 .read_packet_size = 1460,
                                 .read_timeout_ms = 5000,
                                 .write_timeout_ms = 5000};
    return conf;
}

static ISM43362_RET ism43362_setup_server(WifiBaseServerConfig *conf) {
    if (conf == NULL) {
        return Error;
    }

    uint8_t buff[1000];
    size_t read_len;
    char cmd[100];

    snprintf(cmd, sizeof(cmd), "P0=%d\r\n", conf->s);
    ISM43362_RET ret = ism43362_execute_cmd(cmd, buff, sizeof(buff), &read_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, sizeof(cmd), "P2=%d\r\n", conf->local_port);
    ret = ism43362_execute_cmd(cmd, buff, sizeof(buff), &read_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, sizeof(cmd), "R1=%d\r\n", conf->read_packet_size);
    ret = ism43362_execute_cmd(cmd, buff, sizeof(buff), &read_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, sizeof(cmd), "R2=%d\r\n", conf->read_timeout_ms);
    ret = ism43362_execute_cmd(cmd, buff, sizeof(buff), &read_len);
    RET_IF_NOT_OK(ret);

    snprintf(cmd, sizeof(cmd), "S2=%d\r\n", conf->write_timeout_ms);
    return ism43362_execute_cmd(cmd, buff, sizeof(buff), &read_len);
}

ISM43362_RET ism43362_start_udp_server(WifiBaseServerConfig *conf) {
    ISM43362_RET ret = ism43362_setup_server(conf);
    RET_IF_NOT_OK(ret);

    uint8_t buff[1000];
    size_t read_len;

    ret = ism43362_execute_cmd("P1=1\r\n", buff, sizeof(buff), &read_len);
    RET_IF_NOT_OK(ret);

    return ism43362_execute_cmd("P5=1\r\n", buff, sizeof(buff), &read_len);
}

WifiTcpServerConfig ism43362_get_default_tcp_server_config() {
    WifiTcpServerConfig conf = {.base_conf = ism43362_get_default_base_server_config(),
                                .listen_backlogs = 1,
                                .keep_alive_enabled = false,
                                .keep_alive_timeout_ms = 7200000};
    return conf;
}

ISM43362_RET ism43362_start_tcp_server(WifiTcpServerConfig *conf) {
    ISM43362_RET ret = ism43362_setup_server(&conf->base_conf);
    RET_IF_NOT_OK(ret);

    uint8_t buff[500];
    size_t read_len;
    char cmd[100];

    snprintf(cmd, sizeof(cmd), "P8=%d\r\n", conf->listen_backlogs);
    ret = ism43362_execute_cmd(cmd, buff, sizeof(buff), &read_len);
    RET_IF_NOT_OK(ret);

    if (conf->keep_alive_enabled) {
        snprintf(cmd, sizeof(cmd), "PK=1,%d\r\n", conf->keep_alive_timeout_ms);
        ret = ism43362_execute_cmd(cmd, buff, sizeof(buff), &read_len);
        RET_IF_NOT_OK(ret);
    }

    return ism43362_execute_cmd("P5=11\r\n", buff, sizeof(buff), &read_len);
}

ISM43362_RET ism43362_check_tcp_server_connection(RemoteTcpConnection *conn) {
    if (conn == NULL) {
        return Error;
    }

    uint8_t buff[2000];
    size_t read_len;
    ISM43362_RET ret = ism43362_execute_cmd("MR\r\n", buff, sizeof(buff), &read_len);
    RET_IF_NOT_OK(ret);
    if (strstr(buff, "Accepted") == NULL) {
        conn->connected = false;
        return Ok;
    }

    conn->connected = true;
    sscanf(buff, "\r\n[SOMA][TCP SVR] Accepted %d.%d.%d.%d:%d", conn->remote.ip, conn->remote.ip + 1,
           conn->remote.ip + 2, conn->remote.ip + 3, &conn->remote.port);

    return Ok;
}

ISM43362_RET ism43362_tcp_server_close_curr_conn() {
    uint8_t buff[500];
    size_t read_len;
    return ism43362_execute_cmd("P5=10\r\n", buff, sizeof(buff), &read_len);
}
