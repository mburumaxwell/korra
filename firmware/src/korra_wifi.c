#ifdef CONFIG_BOARD_HAS_WIFI
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_wifi, LOG_LEVEL_INF);

#include <errno.h>

#include "korra_utils.h"
#include "korra_wifi.h"

#ifdef CONFIG_NET_CONNECTION_MANAGER_CONNECTIVITY_WIFI_MGMT
#define USING_CONNECTION_MANAGER
#endif // CONFIG_NET_CONNECTION_MANAGER_CONNECTIVITY_WIFI_MGMT

static struct k_work_delayable wifi_reconnect_work;
static void wifi_reconnect_work_handler(struct k_work *work);

static struct net_mgmt_event_callback wifi_mgmt_cb;
static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);
static void wifi_status_print(struct wifi_iface_status *status);

#define get_wifi_iface net_if_get_wifi_sta

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
static const char ca_cert_test[] = {
#include <wifi_enterprise_test_certs/ca.pem.inc>
    '\0'};

static const char client_cert_test[] = {
#include <wifi_enterprise_test_certs/client.pem.inc>
    '\0'};

static const char client_key_test[] = {
#include <wifi_enterprise_test_certs/client-key.pem.inc>
    '\0'};

static const char ca_cert2_test[] = {
#include <wifi_enterprise_test_certs/ca2.pem.inc>
    '\0'};

static const char client_cert2_test[] = {
#include <wifi_enterprise_test_certs/client2.pem.inc>
    '\0'};

static const char client_key2_test[] = {
#include <wifi_enterprise_test_certs/client-key2.pem.inc>
    '\0'};
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE

void korra_wifi_init()
{
    LOG_DBG("Initializing");

    // Initialize and add event callbacks for management
    net_mgmt_init_event_callback(&wifi_mgmt_cb,
                                 wifi_mgmt_event_handler,
                                 NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_mgmt_cb);

    // Initialize work item for reconnection
    k_work_init_delayable(&wifi_reconnect_work, wifi_reconnect_work_handler);
}

int korra_wifi_status(struct wifi_iface_status *status)
{
    struct net_if *iface = get_wifi_iface();
    int ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, status, sizeof(struct wifi_iface_status));
    if (ret)
    {
        LOG_WRN("Status command failed: %d", ret);
        return ret;
    }

    return 0;
}

int korra_wifi_connect()
{
    int ret;
    struct net_if *iface = get_wifi_iface();
    struct wifi_connect_req_params con_req_params = {0};

    /* Defaults */
    con_req_params.band = WIFI_FREQ_BAND_UNKNOWN;
    con_req_params.channel = WIFI_CHANNEL_ANY;
    con_req_params.security = WIFI_SECURITY_TYPE_NONE;
    con_req_params.mfp = WIFI_MFP_OPTIONAL;
    con_req_params.eap_ver = 1;
    con_req_params.verify_peer_cert = false;

    con_req_params.ssid = CONFIG_WIFI_SSID;
    con_req_params.ssid_length = strlen(con_req_params.ssid);

#ifdef CONFIG_WIFI_ENTERPRISE
    con_req_params.security = WIFI_SECURITY_TYPE_EAP_TTLS_MSCHAPV2;
    con_req_params.anon_id = CONFIG_WIFI_ENTERPRISE_ANON_ID;
    con_req_params.aid_length = strlen(con_req_params.anon_id);
    con_req_params.eap_identity = CONFIG_WIFI_ENTERPRISE_IDENTITY;
    con_req_params.eap_id_length = strlen(con_req_params.eap_identity);
    con_req_params.eap_password = CONFIG_WIFI_ENTERPRISE_PASSWORD;
    con_req_params.eap_passwd_length = strlen(con_req_params.eap_password);

    con_req_params.identities[0] = con_req_params.eap_identity;
    con_req_params.nusers = 1;
    con_req_params.passwords[0] = con_req_params.eap_password;
    con_req_params.passwds = 1;
#else
    if (strlen(CONFIG_WIFI_PSK) > 0)
    {
        con_req_params.security = WIFI_SECURITY_TYPE_PSK;
        con_req_params.psk = CONFIG_WIFI_PSK;
        con_req_params.psk_length = strlen(CONFIG_WIFI_PSK);
    }
#endif

    if (con_req_params.security == WIFI_SECURITY_TYPE_EAP_TLS ||
        con_req_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_MSCHAPV2 ||
        con_req_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_GTC ||
        con_req_params.security == WIFI_SECURITY_TYPE_EAP_TTLS_MSCHAPV2 ||
        con_req_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_TLS)
    {

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
        struct wifi_enterprise_creds_params ent_params = {0};
        ent_params.ca_cert = (uint8_t *)ca_cert_test;
        ent_params.ca_cert_len = ARRAY_SIZE(ca_cert_test);
        ent_params.client_cert = (uint8_t *)client_cert_test;
        ent_params.client_cert_len = ARRAY_SIZE(client_cert_test);
        ent_params.client_key = (uint8_t *)client_key_test;
        ent_params.client_key_len = ARRAY_SIZE(client_key_test);
        ent_params.ca_cert2 = (uint8_t *)ca_cert2_test;
        ent_params.ca_cert2_len = ARRAY_SIZE(ca_cert2_test);
        ent_params.client_cert2 = (uint8_t *)client_cert2_test;
        ent_params.client_cert2_len = ARRAY_SIZE(client_cert2_test);
        ent_params.client_key2 = (uint8_t *)client_key2_test;
        ent_params.client_key2_len = ARRAY_SIZE(client_key2_test);

        ret = net_mgmt(NET_REQUEST_WIFI_ENTERPRISE_CREDS, iface, &ent_params, sizeof(ent_params));
        if (ret)
        {
            LOG_WRN("Set enterprise credentials failed: %d", ret);
            return ret;
        }
#else
        LOG_ERR("Security configured to enterprise but CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE is not set");
        return -ENOTSUP;
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
    }

    // Connect to the WiFi network
    LOG_INF("Connecting to \"%s\"", con_req_params.ssid);
    ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &con_req_params, sizeof(con_req_params));
    if (ret)
    {
        LOG_ERR("Connect command failed: %d", ret);

        // // for some reason this hangs on ESP32 as of 2025-June-19
        // if (ret == -EALREADY)
        // {
        //     LOG_INF("Disconnecting ...");
        //     ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
        //     if (ret)
        //     {
        //         LOG_WRN("Disconnect command failed: %d", ret);
        //     }
        //     else
        //     {
        //         LOG_INF("Disconnected command issued");
        //     }
        // }
    }

    return ret;
}

static void wifi_status_print(struct wifi_iface_status *status)
{
    LOG_DBG("Link Mode: %s", wifi_link_mode_txt(status->link_mode));
    LOG_DBG("SSID: %.32s", status->ssid);
    LOG_DBG("BSSID: " FMT_LL_ADDR_6, PRINT_LL_ADDR_6(status->bssid));
    LOG_DBG("Band: %s", wifi_band_txt(status->band));
    LOG_DBG("Channel: %d", status->channel);
    LOG_DBG("Security: %s", wifi_security_txt(status->security));
    LOG_DBG("RSSI: %d", status->rssi);
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT)
    {
        if (status->status)
        {
#if !defined(USING_CONNECTION_MANAGER) && defined(CONFIG_INTERNET_RECONNECTION)
            // Retry in 5 seconds
            LOG_ERR("Connection request failed: %d. Retrying in 5 sec ...", status->status);
            k_work_schedule(&wifi_reconnect_work, K_SECONDS(5));
#else
            LOG_ERR("Connection request failed: %d", status->status);
#endif // !USING_CONNECTION_MANAGER && CONFIG_INTERNET_RECONNECTION
        }
        else
        {
            LOG_INF("Connected! conn_status: %d", status->conn_status);
            struct wifi_iface_status wstatus = {0};
            korra_wifi_status(&wstatus);
            wifi_status_print(&wstatus);
        }
    }
    else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT)
    {
        if (status->status)
        {
            LOG_ERR("Disconnection request failed: %d", status->status);
        }
        else
        {
#if !defined(USING_CONNECTION_MANAGER) && defined(CONFIG_INTERNET_RECONNECTION)
            // Retry in 5 seconds
            LOG_INF("Disconnected! disconn_reason: %d. Retrying in 5 sec ...", status->disconn_reason);
            k_work_schedule(&wifi_reconnect_work, K_SECONDS(5));
#else
            LOG_INF("Disconnected! disconn_reason: %d", status->disconn_reason);
#endif // !USING_CONNECTION_MANAGER && CONFIG_INTERNET_RECONNECTION
        }
    }
}

static void wifi_reconnect_work_handler(struct k_work *work)
{
    int ret = korra_wifi_connect();
    if (ret)
    {
#ifndef USING_CONNECTION_MANAGER
        // Schedule reconnection in 30 sec (longer to skip errors)
        k_work_schedule(&wifi_reconnect_work, K_SECONDS(30));
#endif // USING_CONNECTION_MANAGER
    }
}

#ifdef CONFIG_WIFI_SCAN_NETWORKS
static uint32_t scan_result;
static struct net_mgmt_event_callback wifi_scan_cb;
static K_SEM_DEFINE(sem_wifi_scan, 0, 1);
static void wifi_scan_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);

int korra_wifi_scan(k_timeout_t timeout)
{
    struct net_if *iface = get_wifi_iface();
    struct wifi_scan_params scan_params = {0};

    net_mgmt_init_event_callback(&wifi_scan_cb,
                                 wifi_scan_event_handler,
                                 NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE);
    net_mgmt_add_event_callback(&wifi_scan_cb);

    scan_result = 0U;
    int ret = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &scan_params, sizeof(scan_params));
    if (ret)
    {
        LOG_WRN("Scan request failed: %d", ret);
        return ret;
    }

    // wait for scanning to complete
    return k_sem_take(&sem_wifi_scan, timeout);
}

static void wifi_scan_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_WIFI_SCAN_RESULT)
    {
        const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;
        uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];
        uint8_t ssid_print[WIFI_SSID_MAX_LEN + 1];

        scan_result++;

        if (scan_result == 1U)
        {
            printk("%-4s | %-32s | %-13s | %-4s | %-20s | %-17s | %-8s\n",
                   "Num", "SSID", "Chan (Band)", "RSSI", "Security", "BSSID", "MFP");
        }

        strncpy(ssid_print, entry->ssid, sizeof(ssid_print) - 1);
        ssid_print[sizeof(ssid_print) - 1] = '\0';

        if (entry->mac_length)
            snprintf(mac_string_buf, sizeof(mac_string_buf), FMT_LL_ADDR_6, PRINT_LL_ADDR_6(entry->mac));

        printk("%-4d | %-32s | %-4u (%-6s) | %-4d | %-20s | %-17s | %-8s\n",
               scan_result,
               ssid_print,
               entry->channel,
               wifi_band_txt(entry->band),
               entry->rssi,
               ((entry->wpa3_ent_type) ? wifi_wpa3_enterprise_txt(entry->wpa3_ent_type) : wifi_security_txt(entry->security)),
               ((entry->mac_length) ? mac_string_buf : (const uint8_t *)""),
               wifi_mfp_txt(entry->mfp));
    }
    else if (mgmt_event == NET_EVENT_WIFI_SCAN_DONE)
    {
        const struct wifi_status *status = (const struct wifi_status *)cb->info;
        if (status->status)
        {
            LOG_WRN("Scan failed: (%d)", status->status);
        }
        else
        {
            LOG_DBG("Scan done");
        }

        k_sem_give(&sem_wifi_scan);                 // signal scan complete
        net_mgmt_del_event_callback(&wifi_scan_cb); // unregister because we do not need it till next scan
    }
}
#endif // CONFIG_WIFI_SCAN_NETWORKS

#endif // CONFIG_BOARD_HAS_WIFI
