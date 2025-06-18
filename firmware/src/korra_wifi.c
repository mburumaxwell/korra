#if CONFIG_WIFI
#include <errno.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>

#include "korra_utils.h"
#include "korra_wifi.h"

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(korra_wifi);

static struct net_mgmt_event_callback wifi_cb;
static struct k_work_delayable wifi_work;

static void wifi_work_handler(struct k_work *work);
static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface);

#if CONFIG_WIFI_SCAN_NETWORKS
static uint32_t scan_result;
static struct net_mgmt_event_callback wifi_scan_cb;
static K_SEM_DEFINE(sem_wifi_scan, 0, 1);
static void wifi_scan_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface);
#endif // CONFIG_WIFI_SCAN_NETWORKS

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
static const char ca_cert_test[] = {
	#include <wifi_enterprise_test_certs/ca.pem.inc>
	'\0'
};

static const char client_cert_test[] = {
	#include <wifi_enterprise_test_certs/client.pem.inc>
	'\0'
};

static const char client_key_test[] = {
	#include <wifi_enterprise_test_certs/client-key.pem.inc>
	'\0'
};

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
    LOG_INF("Initializing");

    // Initialize and add event callbacks for WiFi
    net_mgmt_init_event_callback(&wifi_cb,
                                 wifi_event_handler,
                                 NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);

#if CONFIG_WIFI_SCAN_NETWORKS
    net_mgmt_init_event_callback(&wifi_scan_cb,
                                 wifi_scan_event_handler,
                                 NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE);
    net_mgmt_add_event_callback(&wifi_scan_cb);
#endif // CONFIG_WIFI_SCAN_NETWORKS

    // Initialize work item
    k_work_init_delayable(&wifi_work, wifi_work_handler);
}

int korra_wifi_connect()
{
    int ret;
    struct net_if *iface;
    struct wifi_connect_req_params con_req_params = { 0 };
#if CONFIG_WIFI_SCAN_NETWORKS
	struct wifi_scan_params scan_params = { 0 };
#endif // CONFIG_WIFI_SCAN_NETWORKS

    iface = net_if_get_wifi_sta();

#if CONFIG_WIFI_SCAN_NETWORKS
    scan_result = 0U;
    ret = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &scan_params, sizeof(scan_params));
    if (ret) {
        LOG_WRN("Scan request failed: %d", ret);
    }
    k_sem_take(&sem_wifi_scan, K_FOREVER); // wait for scanning to complete
#endif // CONFIG_WIFI_SCAN_NETWORKS

	/* Defaults */
    con_req_params.band = WIFI_FREQ_BAND_UNKNOWN;
    con_req_params.channel = WIFI_CHANNEL_ANY;
	con_req_params.security = WIFI_SECURITY_TYPE_NONE;
    con_req_params.mfp = WIFI_MFP_OPTIONAL;
    con_req_params.eap_ver = 1;
    con_req_params.verify_peer_cert = false;

    con_req_params.ssid = CONFIG_WIFI_SSID;
    con_req_params.ssid_length = strlen(con_req_params.ssid);

#if CONFIG_WIFI_ENTERPRISE
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
    if (strlen(CONFIG_WIFI_PSK) > 0) {
        con_req_params.security = WIFI_SECURITY_TYPE_PSK;
        con_req_params.psk = CONFIG_WIFI_PSK;
        con_req_params.psk_length = strlen(CONFIG_WIFI_PSK);
    }
#endif

    if (con_req_params.security == WIFI_SECURITY_TYPE_EAP_TLS ||
	    con_req_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_MSCHAPV2 ||
	    con_req_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_GTC ||
	    con_req_params.security == WIFI_SECURITY_TYPE_EAP_TTLS_MSCHAPV2 ||
	    con_req_params.security == WIFI_SECURITY_TYPE_EAP_PEAP_TLS) {

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
        struct wifi_enterprise_creds_params ent_params = { 0 };
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
        if (ret) {
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
    if (ret) {
        LOG_ERR("Connect request failed: %d", ret);
    }

    return ret;
}

static void wifi_work_handler(struct k_work *work)
{
    int ret = korra_wifi_connect();
    if (ret) {
        LOG_ERR("Reconnection failed, retrying in 5 seconds...");
        k_work_schedule(&wifi_work, K_SECONDS(5)); // Schedule reconnection in 5 sec
    }
}

static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        if (status->status) {
            LOG_ERR("Connection request failed: %d", status->status);
        } else {
            LOG_INF("Connected! conn_status: %d", status->conn_status);
            struct net_if *iface = net_if_get_wifi_sta();
        	struct wifi_iface_status status = { 0 };
            int ret = net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status, sizeof(struct wifi_iface_status));
            if (!ret) {
                LOG_DBG("Link Mode: %s", wifi_link_mode_txt(status.link_mode));
                LOG_DBG("SSID: %.32s", status.ssid);
                LOG_DBG("BSSID: " FMT_LL_ADDR_6, PRINT_LL_ADDR_6(status.bssid));
                LOG_DBG("Band: %s", wifi_band_txt(status.band));
                LOG_DBG("Channel: %d", status.channel);
                LOG_DBG("Security: %s", wifi_security_txt(status.security));
                LOG_DBG("RSSI: %d", status.rssi);
            } else {
                LOG_WRN("Status request failed: %d", ret);
            }
        }
    } else if (mgmt_event == NET_EVENT_WIFI_DISCONNECT_RESULT) {
        if (status->status) {
            LOG_ERR("Disconnection request failed: %d", status->status);
        } else {
            LOG_INF("Disconnected! disconn_reason: %d", status->disconn_reason);
            k_work_schedule(&wifi_work, K_NO_WAIT); // Schedule reconnection immediately
        }
    }
}

#if CONFIG_WIFI_SCAN_NETWORKS
static void wifi_scan_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_WIFI_SCAN_RESULT) {
        const struct wifi_scan_result *entry = (const struct wifi_scan_result *)cb->info;
    	uint8_t mac_string_buf[sizeof("xx:xx:xx:xx:xx:xx")];
    	uint8_t ssid_print[WIFI_SSID_MAX_LEN + 1];

        scan_result++;

        if (scan_result == 1U) {
            printk("%-4s | %-32s | %-13s | %-4s | %-20s | %-17s | %-8s\n",
                   "Num", "SSID", "Chan (Band)", "RSSI", "Security", "BSSID", "MFP");
        }

        strncpy(ssid_print, entry->ssid, sizeof(ssid_print) - 1);
        ssid_print[sizeof(ssid_print) - 1] = '\0';

        if (entry->mac_length)
            net_sprint_ll_addr_buf(entry->mac, WIFI_MAC_ADDR_LEN, mac_string_buf, sizeof(mac_string_buf));

        printk("%-4d | %-32s | %-4u (%-6s) | %-4d | %-20s | %-17s | %-8s\n",
               scan_result,
               ssid_print,
               entry->channel,
               wifi_band_txt(entry->band),
               entry->rssi,
               ((entry->wpa3_ent_type) ? wifi_wpa3_enterprise_txt(entry->wpa3_ent_type) : wifi_security_txt(entry->security)),
               ((entry->mac_length) ? mac_string_buf : (const uint8_t *)""),
               wifi_mfp_txt(entry->mfp));
    } else if (mgmt_event == NET_EVENT_WIFI_SCAN_DONE) {
        const struct wifi_status *status = (const struct wifi_status *)cb->info;
        if (status->status) {
            LOG_WRN("Scan failed: (%d)", status->status);
        } else {
            LOG_DBG("Scan done");
        }

        scan_result = 0U;
        k_sem_give(&sem_wifi_scan); // signal scan complete
    }
}
#endif // CONFIG_WIFI_SCAN_NETWORKS

#endif // CONFIG_WIFI
