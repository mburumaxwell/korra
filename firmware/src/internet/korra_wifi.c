#ifdef CONFIG_BOARD_HAS_WIFI
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(korra_wifi, LOG_LEVEL_INF);

#include <errno.h>

#include <korra_credentials.h>

#include "korra_utils.h"
#include "korra_wifi.h"

#define get_wifi_iface net_if_get_wifi_sta
#define WIFI_MGMT_EVENTS (             \
    NET_EVENT_WIFI_CONNECT_RESULT |    \
    NET_EVENT_WIFI_DISCONNECT_RESULT | \
    NET_EVENT_WIFI_SIGNAL_CHANGE |     \
    NET_EVENT_WIFI_NEIGHBOR_REP_COMP)

static struct k_work_delayable wifi_reconnect_work;
static void wifi_reconnect_work_handler(struct k_work *work);

static struct net_mgmt_event_callback wifi_mgmt_cb;
static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface);
static void wifi_status_print(struct wifi_iface_status *status);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
struct wifi_cert_data
{
    enum tls_credential_type type;
    uint32_t tag;
    uint8_t **data;
    size_t *len;
};
static void set_enterprise_creds_params(struct wifi_enterprise_creds_params *params);
static int process_certificates(struct wifi_cert_data *certs, size_t count);
// Have own heap so that we do not steal from others.
// Numbers: 2 CA, 2 Public, and 2 Keys. Assume @2KB = 12KB, plus 25%
static K_HEAP_DEFINE(eap_certs_heap, KB(15));
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE

int korra_wifi_init()
{
    LOG_DBG("Initializing");

    // Print mac address
    struct net_if *iface = get_wifi_iface();
    struct net_linkaddr *linkaddr = net_if_get_link_addr(iface);
    if (linkaddr && linkaddr->len == WIFI_MAC_ADDR_LEN)
    {
        LOG_INF("Mac Address: " FMT_LL_ADDR_6, PRINT_LL_ADDR_6(linkaddr->addr));
    }

    // Print WiFi version
    struct wifi_version version = {0};
    int ret = net_mgmt(NET_REQUEST_WIFI_VERSION, iface, &version, sizeof(version));
    if (ret)
    {
        LOG_WRN("Failed to get Wi-Fi versions");
        return ret;
    }
    LOG_INF("Driver Version: %s", version.drv_version);
    LOG_INF("Firmware Version: %s", version.fw_version);

    // Initialize and add event callbacks for management
    net_mgmt_init_event_callback(&wifi_mgmt_cb, wifi_mgmt_event_handler, WIFI_MGMT_EVENTS);
    net_mgmt_add_event_callback(&wifi_mgmt_cb);

    // Initialize work item for reconnection
    k_work_init_delayable(&wifi_reconnect_work, wifi_reconnect_work_handler);

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
    struct wifi_enterprise_creds_params ent_params = {0};
    set_enterprise_creds_params(&ent_params);
    ret = net_mgmt(NET_REQUEST_WIFI_ENTERPRISE_CREDS, iface, &ent_params, sizeof(ent_params));
    if (ret)
    {
        LOG_WRN("Set enterprise credentials failed: %d", ret);
        return ret;
    }
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE

    return 0;
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

#ifdef WIFI_NM_WPA_SUPPLICANT_DPP
int korra_wifi_provisioning()
{
    struct net_if *iface = get_wifi_iface();
    struct net_linkaddr *linkaddr = net_if_get_link_addr(iface);
    struct wifi_dpp_params params = {0};

    // TODO: check if we have wifi credentials stored already
    // https://github.com/zephyrproject-rtos/zephyr/pull/88653
    // https://github.com/zephyrproject-rtos/zephyr/issues/91790

    // Perform bootstrap gen
    params.action = WIFI_DPP_BOOTSTRAP_GEN;
    params.bootstrap_gen.type = WIFI_DPP_BOOTSTRAP_TYPE_QRCODE;
    params.bootstrap_gen.chan = 1;
    params.bootstrap_gen.op_class = 81; // for channel 1 and only for QR Code?
    memcpy(params.bootstrap_gen.mac, linkaddr->addr, WIFI_MAC_ADDR_LEN);
    int ret = net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params));
    if (ret)
    {
        LOG_WRN("Failed to request DPP bootstrap gen");
        return ret;
    }

    // Get the URI
    memset(&params, 0, sizeof(params));
    params.action = WIFI_DPP_BOOTSTRAP_GET_URI;
    params.id = net_if_get_by_iface(iface); // needs to be the index of the interface
    ret = net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params));
    if (ret)
    {
        LOG_WRN("Failed to request DPP bootstrap get URI");
        return ret;
    }

    LOG_INF("DPP QR Code (without quotes) \"%s\"", params.dpp_qr_code);

    // Listen
    memset(&params, 0, sizeof(params));
    params.action = WIFI_DPP_LISTEN;
    params.listen.role = WIFI_DPP_ROLE_ENROLLEE;
    params.listen.freq = 2412; // center frequency for channel 1
    ret = net_mgmt(NET_REQUEST_WIFI_DPP, iface, &params, sizeof(params));
    if (ret)
    {
        LOG_WRN("Failed to request DPP listen");
        return ret;
    }

    return ret;
}
#endif // WIFI_NM_WPA_SUPPLICANT_DPP

int korra_wifi_connect()
{
#ifdef CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_EAP_PEAP_MSCHAPV2
    struct wifi_connect_req_params con_req_params = {0};

    /* Defaults */
    con_req_params.band = WIFI_FREQ_BAND_UNKNOWN;
    con_req_params.channel = WIFI_CHANNEL_ANY;
    con_req_params.mfp = WIFI_MFP_OPTIONAL;
    con_req_params.eap_ver = 0;
    con_req_params.verify_peer_cert = true;

    con_req_params.ssid = CONFIG_WIFI_CREDENTIALS_STATIC_SSID;
    con_req_params.ssid_length = strlen(con_req_params.ssid);

    con_req_params.security = WIFI_SECURITY_TYPE_EAP_PEAP_MSCHAPV2;
    con_req_params.anon_id = CONFIG_WIFI_CREDENTIALS_STATIC_EAP_ANON_ID;
    con_req_params.aid_length = strlen(con_req_params.anon_id);
    con_req_params.eap_identity = CONFIG_WIFI_CREDENTIALS_STATIC_EAP_IDENTITY;
    con_req_params.eap_id_length = strlen(con_req_params.eap_identity);
    con_req_params.eap_password = CONFIG_WIFI_CREDENTIALS_STATIC_EAP_PASSWORD;
    con_req_params.eap_passwd_length = strlen(con_req_params.eap_password);

    con_req_params.identities[0] = con_req_params.eap_identity;
    con_req_params.nusers = 1;
    con_req_params.passwords[0] = con_req_params.eap_password;
    con_req_params.passwds = 1;

    // Connect to the WiFi network
    LOG_INF("Connecting to \"%s\"", con_req_params.ssid);
    struct net_if *iface = get_wifi_iface();
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &con_req_params, sizeof(con_req_params));
    ret = ret == -EALREADY ? 0 : ret; // skip if already connected
    if (ret)
    {
        LOG_ERR("Connect command failed: %d", ret);
    }
    return ret;
#else
    struct net_if *iface = get_wifi_iface();
	// struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
    LOG_INF("Connecting to stored networks");
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);
    if (ret)
    {
        LOG_ERR("Connect command failed: %d", ret);
    }
    return ret;
#endif
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
#ifndef CONFIG_WIFI_ESP32
            // Retry in 5 seconds
            LOG_ERR("Connection request failed: %d. Retrying in 5 sec ...", status->status);
            k_work_schedule(&wifi_reconnect_work, K_SECONDS(5));
#else
            LOG_ERR("Connection request failed: %d", status->status);
#endif // CONFIG_WIFI_ESP32
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
            // Retry in 5 seconds
            LOG_INF("Disconnected! disconn_reason: %d. Retrying in 5 sec ...", status->disconn_reason);
            k_work_schedule(&wifi_reconnect_work, K_SECONDS(5));
        }
    }
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_ROAMING
    // follow https://github.com/zephyrproject-rtos/zephyr/issues/87728
    else if (mgmt_event == NET_EVENT_WIFI_SIGNAL_CHANGE)
    {
        struct net_if *iface = get_wifi_iface();
        int ret = net_mgmt(NET_REQUEST_WIFI_START_ROAMING, iface, NULL, 0);
        if (ret)
        {
            LOG_WRN("Start roaming failed");
            return;
        }

        LOG_INF("Start roaming requested\n");
    }
    else if (mgmt_event == NET_EVENT_WIFI_NEIGHBOR_REP_COMP)
    {
        struct net_if *iface = get_wifi_iface();
        int ret = net_mgmt(NET_REQUEST_WIFI_NEIGHBOR_REP_COMPLETE, iface, NULL, 0);
        if (ret)
        {
            LOG_WRN("Neighbor report complete failed");
            return;
        }

        LOG_INF("Neighbor report complete requested");
    }
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_ROAMING
}

static void wifi_reconnect_work_handler(struct k_work *work)
{
    int ret = korra_wifi_connect();
    if (ret)
    {
        // Schedule reconnection in 30 sec (longer to skip errors)
        k_work_schedule(&wifi_reconnect_work, K_SECONDS(30));
    }
}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
static void set_enterprise_creds_params(struct wifi_enterprise_creds_params *params)
{
    struct wifi_cert_data certs[] = {
        {
            .type = TLS_CREDENTIAL_CA_CERTIFICATE,
            .tag = KORRA_CREDENTIAL_WIFI_CA_TAG,
            .data = &params->ca_cert,
            .len = &params->ca_cert_len,
        },
        {
            .type = TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
            .tag = KORRA_CREDENTIAL_WIFI_CLIENT_TAG,
            .data = &params->client_cert,
            .len = &params->client_cert_len,
        },
        {
            .type = TLS_CREDENTIAL_PRIVATE_KEY,
            .tag = KORRA_CREDENTIAL_WIFI_CLIENT_TAG,
            .data = &params->client_key,
            .len = &params->client_key_len,
        },
        {
            .type = TLS_CREDENTIAL_CA_CERTIFICATE,
            .tag = KORRA_CREDENTIAL_WIFI_CA_P2_TAG,
            .data = &params->ca_cert2,
            .len = &params->ca_cert2_len,
        },
        {
            .type = TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
            .tag = KORRA_CREDENTIAL_WIFI_CLIENT_P2_TAG,
            .data = &params->client_cert2,
            .len = &params->client_cert2_len,
        },
        {
            .type = TLS_CREDENTIAL_PRIVATE_KEY,
            .tag = KORRA_CREDENTIAL_WIFI_CLIENT_P2_TAG,
            .data = &params->client_key2,
            .len = &params->client_key2_len,
        },
    };

    // process certs
    if (process_certificates(certs, ARRAY_SIZE(certs)) != 0)
    {
        goto cleanup;
    }

    return;

cleanup:
    for (size_t i = 0; i < ARRAY_SIZE(certs); i++)
    {
        if (certs[i].data)
        {
            k_heap_free(&eap_certs_heap, *certs[i].data);
        }
    }
}
static int process_certificates(struct wifi_cert_data *certs, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        int ret;
        size_t len = 0;
        uint8_t *cert_tmp;

        ret = tls_credential_get(certs[i].tag, certs[i].type, NULL, &len);
        if (ret != -EFBIG)
        {
            LOG_ERR("Failed to get credential tag: %s, err: %d", korra_credential_tag_type_txt(certs[i].tag), ret);
            return ret;
        }
        LOG_DBG("Found credential type: %-18s -> tag: %-20s (len: %d)",
                tls_credential_type_txt(certs[i].type),
                korra_credential_tag_type_txt(certs[i].tag),
                len);

        cert_tmp = k_heap_alloc(&eap_certs_heap, len, K_FOREVER);
        if (!cert_tmp)
        {
            LOG_ERR("Failed to allocate memory for credential tag: %s", korra_credential_tag_type_txt(certs[i].tag));
            return -ENOMEM;
        }

        ret = tls_credential_get(certs[i].tag, certs[i].type, cert_tmp, &len);
        if (ret)
        {
            LOG_ERR("Failed to get credential tag: %s", korra_credential_tag_type_txt(certs[i].tag));
            k_heap_free(&eap_certs_heap, cert_tmp);
            return ret;
        }

        *certs[i].data = cert_tmp;
        *certs[i].len = len;
    }

    return 0;
}
#endif // CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE

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
    LOG_INF("Scan requested. Waiting for completion ...");
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
