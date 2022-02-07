/*
 ESP8266WiFiGeneric.cpp - WiFi library for esp8266

 Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 Reworked on 28 Dec 2015 by Markus Sattler

 */

#include "WiFi.h"
#include "WiFiGeneric.h"

extern "C" {
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include "lwip/ip_addr.h"
#include "lwip/opt.h"
#include "lwip/err.h"
#include "lwip/dns.h"
#include "dhcpserver/dhcpserver_options.h"

} //extern "C"

#include "esp32-hal.h"
#include <vector>
#include "sdkconfig.h"

ESP_EVENT_DEFINE_BASE(ARDUINO_EVENTS);
/*
 * Private (exposable) methods
 * */
static esp_netif_t* esp_netifs[ESP_IF_MAX] = {NULL, NULL, NULL};
esp_interface_t get_esp_netif_interface(esp_netif_t* esp_netif){
	for(int i=0; i<ESP_IF_MAX; i++){
		if(esp_netifs[i] != NULL && esp_netifs[i] == esp_netif){
			return (esp_interface_t)i;
		}
	}
	return ESP_IF_MAX;
}

void add_esp_interface_netif(esp_interface_t interface, esp_netif_t* esp_netif){
	if(interface < ESP_IF_MAX){
		esp_netifs[interface] = esp_netif;
	}
}

esp_netif_t* get_esp_interface_netif(esp_interface_t interface){
	if(interface < ESP_IF_MAX){
		return esp_netifs[interface];
	}
	return NULL;
}

esp_err_t set_esp_interface_hostname(esp_interface_t interface, const char * hostname){
	if(interface < ESP_IF_MAX){
		return esp_netif_set_hostname(esp_netifs[interface], hostname);
	}
	return ESP_FAIL;
}

esp_err_t set_esp_interface_ip(esp_interface_t interface, IPAddress local_ip=IPAddress(), IPAddress gateway=IPAddress(), IPAddress subnet=IPAddress()){
	esp_netif_t *esp_netif = esp_netifs[interface];
	esp_netif_dhcp_status_t status = ESP_NETIF_DHCP_INIT;
	esp_netif_ip_info_t info;
    info.ip.addr = static_cast<uint32_t>(local_ip);
    info.gw.addr = static_cast<uint32_t>(gateway);
    info.netmask.addr = static_cast<uint32_t>(subnet);

    log_v("Configuring %s static IP: " IPSTR ", MASK: " IPSTR ", GW: " IPSTR,
          interface == ESP_IF_WIFI_STA ? "Station" :
          interface == ESP_IF_WIFI_AP ? "SoftAP" : "Ethernet",
          IP2STR(&info.ip), IP2STR(&info.netmask), IP2STR(&info.gw));

    esp_err_t err = ESP_OK;
    if(interface != ESP_IF_WIFI_AP){
    	err = esp_netif_dhcpc_get_status(esp_netif, &status);
        if(err){
        	log_e("DHCPC Get Status Failed! 0x%04x", err);
        	return err;
        }
		err = esp_netif_dhcpc_stop(esp_netif);
		if(err && err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED){
			log_e("DHCPC Stop Failed! 0x%04x", err);
			return err;
		}
        err = esp_netif_set_ip_info(esp_netif, &info);
        if(err){
        	log_e("Netif Set IP Failed! 0x%04x", err);
        	return err;
        }
    	if(info.ip.addr == 0){
    		err = esp_netif_dhcpc_start(esp_netif);
    		if(err){
            	log_e("DHCPC Start Failed! 0x%04x", err);
            	return err;
            }
    	}
    } else {
    	err = esp_netif_dhcps_get_status(esp_netif, &status);
        if(err){
        	log_e("DHCPS Get Status Failed! 0x%04x", err);
        	return err;
        }
		err = esp_netif_dhcps_stop(esp_netif);
		if(err && err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED){
			log_e("DHCPS Stop Failed! 0x%04x", err);
			return err;
		}
        err = esp_netif_set_ip_info(esp_netif, &info);
        if(err){
        	log_e("Netif Set IP Failed! 0x%04x", err);
        	return err;
        }

        dhcps_lease_t lease;
        lease.enable = true;
        lease.start_ip.addr = static_cast<uint32_t>(local_ip) + (1 << 24);
        lease.end_ip.addr = static_cast<uint32_t>(local_ip) + (11 << 24);

        err = tcpip_adapter_dhcps_option(
            (tcpip_adapter_dhcp_option_mode_t)TCPIP_ADAPTER_OP_SET,
            (tcpip_adapter_dhcp_option_id_t)REQUESTED_IP_ADDRESS,
            (void*)&lease, sizeof(dhcps_lease_t)
        );
		if(err){
        	log_e("DHCPS Set Lease Failed! 0x%04x", err);
        	return err;
        }

		err = esp_netif_dhcps_start(esp_netif);
		if(err){
        	log_e("DHCPS Start Failed! 0x%04x", err);
        	return err;
        }
    }
	return err;
}

esp_err_t set_esp_interface_dns(esp_interface_t interface, IPAddress main_dns=IPAddress(), IPAddress backup_dns=IPAddress(), IPAddress fallback_dns=IPAddress()){
	esp_netif_t *esp_netif = esp_netifs[interface];
	esp_netif_dns_info_t dns;
	dns.ip.type = ESP_IPADDR_TYPE_V4;
	dns.ip.u_addr.ip4.addr = static_cast<uint32_t>(main_dns);
	if(dns.ip.u_addr.ip4.addr && esp_netif_set_dns_info(esp_netif, ESP_NETIF_DNS_MAIN, &dns) != ESP_OK){
    	log_e("Set Main DNS Failed!");
    	return ESP_FAIL;
    }
	if(interface != ESP_IF_WIFI_AP){
		dns.ip.u_addr.ip4.addr = static_cast<uint32_t>(backup_dns);
		if(dns.ip.u_addr.ip4.addr && esp_netif_set_dns_info(esp_netif, ESP_NETIF_DNS_BACKUP, &dns) != ESP_OK){
	    	log_e("Set Backup DNS Failed!");
	    	return ESP_FAIL;
	    }
		dns.ip.u_addr.ip4.addr = static_cast<uint32_t>(fallback_dns);
		if(dns.ip.u_addr.ip4.addr && esp_netif_set_dns_info(esp_netif, ESP_NETIF_DNS_FALLBACK, &dns) != ESP_OK){
	    	log_e("Set Fallback DNS Failed!");
	    	return ESP_FAIL;
	    }
	}
	return ESP_OK;
}

static const char * auth_mode_str(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
    	return ("OPEN");
        break;
    case WIFI_AUTH_WEP:
    	return ("WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
    	return ("PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
    	return ("WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
    	return ("WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
    	return ("WPA2_ENTERPRISE");
        break;
    default:
        break;
    }
	return ("UNKNOWN");
}

static char default_hostname[32] = {0,};
static const char * get_esp_netif_hostname(){
	if(default_hostname[0] == 0){
	    uint8_t eth_mac[6];
	    esp_wifi_get_mac((wifi_interface_t)WIFI_IF_STA, eth_mac);
	    snprintf(default_hostname, 32, "%s%02X%02X%02X", CONFIG_IDF_TARGET "-", eth_mac[3], eth_mac[4], eth_mac[5]);
	}
	return (const char *)default_hostname;
}
static void set_esp_netif_hostname(const char * name){
	if(name){
		snprintf(default_hostname, 32, "%s", name);
	}
}

static xQueueHandle _arduino_event_queue;
static TaskHandle_t _arduino_event_task_handle = NULL;
static EventGroupHandle_t _arduino_event_group = NULL;

static void _arduino_event_task(void * arg){
	arduino_event_t *data = NULL;
    for (;;) {
        if(xQueueReceive(_arduino_event_queue, &data, portMAX_DELAY) == pdTRUE){
            WiFiGenericClass::_eventCallback(data);
            free(data);
            data = NULL;
        }
    }
    vTaskDelete(NULL);
    _arduino_event_task_handle = NULL;
}

esp_err_t postArduinoEvent(arduino_event_t *data)
{
	if(data == NULL){
        return ESP_FAIL;
	}
	arduino_event_t * event = (arduino_event_t*)malloc(sizeof(arduino_event_t));
	if(event == NULL){
        log_e("Arduino Event Malloc Failed!");
        return ESP_FAIL;
	}
	memcpy(event, data, sizeof(arduino_event_t));
    if (xQueueSend(_arduino_event_queue, &event, portMAX_DELAY) != pdPASS) {
        log_e("Arduino Event Send Failed!");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static void _arduino_event_cb(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	arduino_event_t arduino_event;
	arduino_event.event_id = ARDUINO_EVENT_MAX;

	/*
	 * STA
	 * */
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    	log_v("STA Started");
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_STA_START;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {
    	log_v("STA Stopped");
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_STA_STOP;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_AUTHMODE_CHANGE) {
    	wifi_event_sta_authmode_change_t * event = (wifi_event_sta_authmode_change_t*)event_data;
    	log_v("STA Auth Mode Changed: From: %s, To: %s", auth_mode_str(event->old_mode), auth_mode_str(event->new_mode));
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE;
    	memcpy(&arduino_event.event_info.wifi_sta_authmode_change, event_data, sizeof(wifi_event_sta_authmode_change_t));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
    	wifi_event_sta_connected_t * event = (wifi_event_sta_connected_t*)event_data;
    	log_v("STA Connected: SSID: %s, BSSID: " MACSTR ", Channel: %u, Auth: %s", event->ssid, MAC2STR(event->bssid), event->channel, auth_mode_str(event->authmode));
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_STA_CONNECTED;
    	memcpy(&arduino_event.event_info.wifi_sta_connected, event_data, sizeof(wifi_event_sta_connected_t));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    	wifi_event_sta_disconnected_t * event = (wifi_event_sta_disconnected_t*)event_data;
    	log_v("STA Disconnected: SSID: %s, BSSID: " MACSTR ", Reason: %u", event->ssid, MAC2STR(event->bssid), event->reason);
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_STA_DISCONNECTED;
    	memcpy(&arduino_event.event_info.wifi_sta_disconnected, event_data, sizeof(wifi_event_sta_disconnected_t));
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        log_v("STA Got %sIP:" IPSTR, event->ip_changed?"New ":"Same ", IP2STR(&event->ip_info.ip));
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_STA_GOT_IP;
    	memcpy(&arduino_event.event_info.got_ip, event_data, sizeof(ip_event_got_ip_t));
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
    	log_v("STA IP Lost");
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_STA_LOST_IP;

	/*
	 * SCAN
	 * */
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
    	wifi_event_sta_scan_done_t * event = (wifi_event_sta_scan_done_t*)event_data;
    	log_v("SCAN Done: ID: %u, Status: %u, Results: %u", event->scan_id, event->status, event->number);
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_SCAN_DONE;
    	memcpy(&arduino_event.event_info.wifi_scan_done, event_data, sizeof(wifi_event_sta_scan_done_t));

	/*
	 * AP
	 * */
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
		log_v("AP Started");
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_AP_START;
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STOP) {
		log_v("AP Stopped");
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_AP_STOP;
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_PROBEREQRECVED) {
		wifi_event_ap_probe_req_rx_t * event = (wifi_event_ap_probe_req_rx_t*)event_data;
		log_v("AP Probe Request: RSSI: %d, MAC: " MACSTR, event->rssi, MAC2STR(event->mac));
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED;
    	memcpy(&arduino_event.event_info.wifi_ap_probereqrecved, event_data, sizeof(wifi_event_ap_probe_req_rx_t));
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
		wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
		log_v("AP Station Connected: MAC: " MACSTR ", AID: %d", MAC2STR(event->mac), event->aid);
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_AP_STACONNECTED;
    	memcpy(&arduino_event.event_info.wifi_ap_staconnected, event_data, sizeof(wifi_event_ap_staconnected_t));
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
		wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
		log_v("AP Station Disconnected: MAC: " MACSTR ", AID: %d", MAC2STR(event->mac), event->aid);
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_AP_STADISCONNECTED;
    	memcpy(&arduino_event.event_info.wifi_ap_stadisconnected, event_data, sizeof(wifi_event_ap_stadisconnected_t));
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED) {
    	ip_event_ap_staipassigned_t * event = (ip_event_ap_staipassigned_t*)event_data;
    	log_v("AP Station IP Assigned:" IPSTR, IP2STR(&event->ip));
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED;
    	memcpy(&arduino_event.event_info.wifi_ap_staipassigned, event_data, sizeof(ip_event_ap_staipassigned_t));

	/*
	 * ETH
	 * */
	} else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_CONNECTED) {
		log_v("Ethernet Link Up");
	    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
    	arduino_event.event_id = ARDUINO_EVENT_ETH_CONNECTED;
    	memcpy(&arduino_event.event_info.eth_connected, event_data, sizeof(esp_eth_handle_t));
	} else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_DISCONNECTED) {
		log_v("Ethernet Link Down");
    	arduino_event.event_id = ARDUINO_EVENT_ETH_DISCONNECTED;
	} else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_START) {
		log_v("Ethernet Started");
    	arduino_event.event_id = ARDUINO_EVENT_ETH_START;
	} else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_STOP) {
		log_v("Ethernet Stopped");
    	arduino_event.event_id = ARDUINO_EVENT_ETH_STOP;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_ETH_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        log_v("Ethernet got %sip:" IPSTR, event->ip_changed?"new":"", IP2STR(&event->ip_info.ip));
    	arduino_event.event_id = ARDUINO_EVENT_ETH_GOT_IP;
    	memcpy(&arduino_event.event_info.got_ip, event_data, sizeof(ip_event_got_ip_t));

	/*
	 * IPv6
	 * */
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_GOT_IP6) {
    	ip_event_got_ip6_t * event = (ip_event_got_ip6_t*)event_data;
    	esp_interface_t iface = get_esp_netif_interface(event->esp_netif);
    	log_v("IF[%d] Got IPv6: IP Index: %d, Zone: %d, " IPV6STR, iface, event->ip_index, event->ip6_info.ip.zone, IPV62STR(event->ip6_info.ip));
    	memcpy(&arduino_event.event_info.got_ip6, event_data, sizeof(ip_event_got_ip6_t));
    	if(iface == ESP_IF_WIFI_STA){
        	arduino_event.event_id = ARDUINO_EVENT_WIFI_STA_GOT_IP6;
    	} else if(iface == ESP_IF_WIFI_AP){
        	arduino_event.event_id = ARDUINO_EVENT_WIFI_AP_GOT_IP6;
    	} else if(iface == ESP_IF_ETH){
        	arduino_event.event_id = ARDUINO_EVENT_ETH_GOT_IP6;
    	}

	/*
	 * WPS
	 * */
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_SUCCESS) {
    	arduino_event.event_id = ARDUINO_EVENT_WPS_ER_SUCCESS;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_FAILED) {
    	wifi_event_sta_wps_fail_reason_t * event = (wifi_event_sta_wps_fail_reason_t*)event_data;
    	arduino_event.event_id = ARDUINO_EVENT_WPS_ER_FAILED;
    	memcpy(&arduino_event.event_info.wps_fail_reason, event_data, sizeof(wifi_event_sta_wps_fail_reason_t));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_TIMEOUT) {
    	arduino_event.event_id = ARDUINO_EVENT_WPS_ER_TIMEOUT;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_PIN) {
    	wifi_event_sta_wps_er_pin_t * event = (wifi_event_sta_wps_er_pin_t*)event_data;
    	arduino_event.event_id = ARDUINO_EVENT_WPS_ER_PIN;
    	memcpy(&arduino_event.event_info.wps_er_pin, event_data, sizeof(wifi_event_sta_wps_er_pin_t));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP) {
    	arduino_event.event_id = ARDUINO_EVENT_WPS_ER_PBC_OVERLAP;

	/*
	 * FTM
	 * */
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_FTM_REPORT) {
    	wifi_event_ftm_report_t * event = (wifi_event_ftm_report_t*)event_data;
    	arduino_event.event_id = ARDUINO_EVENT_WIFI_FTM_REPORT;
    	memcpy(&arduino_event.event_info.wifi_ftm_report, event_data, sizeof(wifi_event_ftm_report_t));


	/*
	 * SMART CONFIG
	 * */
	} else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
		log_v("SC Scan Done");
    	arduino_event.event_id = ARDUINO_EVENT_SC_SCAN_DONE;
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
    	log_v("SC Found Channel");
    	arduino_event.event_id = ARDUINO_EVENT_SC_FOUND_CHANNEL;
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        smartconfig_event_got_ssid_pswd_t *event = (smartconfig_event_got_ssid_pswd_t *)event_data;
        log_v("SC: SSID: %s, Password: %s", (const char *)event->ssid, (const char *)event->password);
    	arduino_event.event_id = ARDUINO_EVENT_SC_GOT_SSID_PSWD;
    	memcpy(&arduino_event.event_info.sc_got_ssid_pswd, event_data, sizeof(smartconfig_event_got_ssid_pswd_t));

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
    	log_v("SC Send Ack Done");
    	arduino_event.event_id = ARDUINO_EVENT_SC_SEND_ACK_DONE;

	/*
	 * Provisioning
	 * */
	} else if (event_base == WIFI_PROV_EVENT && event_id == WIFI_PROV_INIT) {
		log_v("Provisioning Initialized!");
    	arduino_event.event_id = ARDUINO_EVENT_PROV_INIT;
	} else if (event_base == WIFI_PROV_EVENT && event_id == WIFI_PROV_DEINIT) {
		log_v("Provisioning Uninitialized!");
    	arduino_event.event_id = ARDUINO_EVENT_PROV_DEINIT;
	} else if (event_base == WIFI_PROV_EVENT && event_id == WIFI_PROV_START) {
		log_v("Provisioning Start!");
    	arduino_event.event_id = ARDUINO_EVENT_PROV_START;
	} else if (event_base == WIFI_PROV_EVENT && event_id == WIFI_PROV_END) {
		log_v("Provisioning End!");
		wifi_prov_mgr_deinit();
    	arduino_event.event_id = ARDUINO_EVENT_PROV_END;
	} else if (event_base == WIFI_PROV_EVENT && event_id == WIFI_PROV_CRED_RECV) {
        wifi_sta_config_t *event = (wifi_sta_config_t *)event_data;
        log_v("Provisioned Credentials: SSID: %s, Password: %s", (const char *) event->ssid, (const char *) event->password);
    	arduino_event.event_id = ARDUINO_EVENT_PROV_CRED_RECV;
    	memcpy(&arduino_event.event_info.prov_cred_recv, event_data, sizeof(wifi_sta_config_t));
	} else if (event_base == WIFI_PROV_EVENT && event_id == WIFI_PROV_CRED_FAIL) {
        wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
        log_e("Provisioning Failed: Reason : %s", (*reason == WIFI_PROV_STA_AUTH_ERROR)?"Authentication Failed":"AP Not Found");
    	arduino_event.event_id = ARDUINO_EVENT_PROV_CRED_FAIL;
    	memcpy(&arduino_event.event_info.prov_fail_reason, event_data, sizeof(wifi_prov_sta_fail_reason_t));
	} else if (event_base == WIFI_PROV_EVENT && event_id == WIFI_PROV_CRED_SUCCESS) {
		log_v("Provisioning Success!");
    	arduino_event.event_id = ARDUINO_EVENT_PROV_CRED_SUCCESS;
    }
    
	if(arduino_event.event_id < ARDUINO_EVENT_MAX){
		postArduinoEvent(&arduino_event);
	}
}

static bool _start_network_event_task(){
    if(!_arduino_event_group){
        _arduino_event_group = xEventGroupCreate();
        if(!_arduino_event_group){
            log_e("Network Event Group Create Failed!");
            return false;
        }
        xEventGroupSetBits(_arduino_event_group, WIFI_DNS_IDLE_BIT);
    }
    if(!_arduino_event_queue){
    	_arduino_event_queue = xQueueCreate(32, sizeof(arduino_event_t*));
        if(!_arduino_event_queue){
            log_e("Network Event Queue Create Failed!");
            return false;
        }
    }

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    	log_e("esp_event_loop_create_default failed!");
        return err;
    }

    if(!_arduino_event_task_handle){
        xTaskCreateUniversal(_arduino_event_task, "arduino_events", 4096, NULL, ESP_TASKD_EVENT_PRIO - 1, &_arduino_event_task_handle, ARDUINO_EVENT_RUNNING_CORE);
        if(!_arduino_event_task_handle){
            log_e("Network Event Task Start Failed!");
            return false;
        }
    }

    if(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_arduino_event_cb, NULL, NULL)){
        log_e("event_handler_instance_register for WIFI_EVENT Failed!");
        return false;
    }

    if(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, &_arduino_event_cb, NULL, NULL)){
        log_e("event_handler_instance_register for IP_EVENT Failed!");
        return false;
    }

    if(esp_event_handler_instance_register(SC_EVENT, ESP_EVENT_ANY_ID, &_arduino_event_cb, NULL, NULL)){
        log_e("event_handler_instance_register for SC_EVENT Failed!");
        return false;
    }

    if(esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, &_arduino_event_cb, NULL, NULL)){
        log_e("event_handler_instance_register for ETH_EVENT Failed!");
        return false;
    }

    if(esp_event_handler_instance_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &_arduino_event_cb, NULL, NULL)){
        log_e("event_handler_instance_register for WIFI_PROV_EVENT Failed!");
        return false;
    }

    return true;
}

bool tcpipInit(){
    static bool initialized = false;
    if(!initialized){
        initialized = true;
#if CONFIG_IDF_TARGET_ESP32
        uint8_t mac[8];
        if(esp_efuse_mac_get_default(mac) == ESP_OK){
            esp_base_mac_addr_set(mac);
        }
#endif
        initialized = esp_netif_init() == ESP_OK;
        if(initialized){
        	initialized = _start_network_event_task();
        } else {
        	log_e("esp_netif_init failed!");
        }
    }
    return initialized;
}

/*
 * WiFi INIT
 * */

static bool lowLevelInitDone = false;
bool WiFiGenericClass::_wifiUseStaticBuffers = false;

bool WiFiGenericClass::useStaticBuffers(){
    return _wifiUseStaticBuffers;
}

void WiFiGenericClass::useStaticBuffers(bool bufferMode){
    if (lowLevelInitDone) {
        log_w("WiFi already started. Call WiFi.mode(WIFI_MODE_NULL) before setting Static Buffer Mode.");
    } 
    _wifiUseStaticBuffers = bufferMode;
}


bool wifiLowLevelInit(bool persistent){
    if(!lowLevelInitDone){
        lowLevelInitDone = true;
        if(!tcpipInit()){
        	lowLevelInitDone = false;
        	return lowLevelInitDone;
        }
        if(esp_netifs[ESP_IF_WIFI_AP] == NULL){
            esp_netifs[ESP_IF_WIFI_AP] = esp_netif_create_default_wifi_ap();
        }
        if(esp_netifs[ESP_IF_WIFI_STA] == NULL){
            esp_netifs[ESP_IF_WIFI_STA] = esp_netif_create_default_wifi_sta();
        }

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	if(!WiFiGenericClass::useStaticBuffers()) {
	    cfg.static_tx_buf_num = 0;
            cfg.dynamic_tx_buf_num = 32;
	    cfg.tx_buf_type = 1;
            cfg.cache_tx_buf_num = 1;  // can't be zero!
	    cfg.static_rx_buf_num = 4;
            cfg.dynamic_rx_buf_num = 32;
        }

        esp_err_t err = esp_wifi_init(&cfg);
        if(err){
            log_e("esp_wifi_init %d", err);
        	lowLevelInitDone = false;
        	return lowLevelInitDone;
        }
        if(!persistent){
        	lowLevelInitDone = esp_wifi_set_storage(WIFI_STORAGE_RAM) == ESP_OK;
        }
        if(lowLevelInitDone){
			arduino_event_t arduino_event;
			arduino_event.event_id = ARDUINO_EVENT_WIFI_READY;
			postArduinoEvent(&arduino_event);
        }
    }
    return lowLevelInitDone;
}

static bool wifiLowLevelDeinit(){
    if(lowLevelInitDone){
    	lowLevelInitDone = !(esp_wifi_deinit() == ESP_OK);
    }
    return !lowLevelInitDone;
}

static bool _esp_wifi_started = false;

static bool espWiFiStart(){
    if(_esp_wifi_started){
        return true;
    }
    _esp_wifi_started = true;
    esp_err_t err = esp_wifi_start();
    if (err != ESP_OK) {
        _esp_wifi_started = false;
        log_e("esp_wifi_start %d", err);
        return _esp_wifi_started;
    }
    return _esp_wifi_started;
}

static bool espWiFiStop(){
    esp_err_t err;
    if(!_esp_wifi_started){
        return true;
    }
    _esp_wifi_started = false;
    err = esp_wifi_stop();
    if(err){
        log_e("Could not stop WiFi! %d", err);
        _esp_wifi_started = true;
        return false;
    }
    return wifiLowLevelDeinit();
}

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------- Generic WiFi function -----------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

typedef struct WiFiEventCbList {
    static wifi_event_id_t current_id;
    wifi_event_id_t id;
    WiFiEventCb cb;
    WiFiEventFuncCb fcb;
    WiFiEventSysCb scb;
    arduino_event_id_t event;

    WiFiEventCbList() : id(current_id++), cb(NULL), fcb(NULL), scb(NULL), event(ARDUINO_EVENT_WIFI_READY) {}
} WiFiEventCbList_t;
wifi_event_id_t WiFiEventCbList::current_id = 1;


// arduino dont like std::vectors move static here
static std::vector<WiFiEventCbList_t> cbEventList;

bool WiFiGenericClass::_persistent = true;
bool WiFiGenericClass::_long_range = false;
wifi_mode_t WiFiGenericClass::_forceSleepLastMode = WIFI_MODE_NULL;
#if CONFIG_IDF_TARGET_ESP32S2
wifi_ps_type_t WiFiGenericClass::_sleepEnabled = WIFI_PS_NONE;
#else
wifi_ps_type_t WiFiGenericClass::_sleepEnabled = WIFI_PS_MIN_MODEM;
#endif

WiFiGenericClass::WiFiGenericClass() 
{
}

const char * WiFiGenericClass::getHostname()
{
    return get_esp_netif_hostname();
}

bool WiFiGenericClass::setHostname(const char * hostname)
{
    set_esp_netif_hostname(hostname);
    return true;
}

int WiFiGenericClass::setStatusBits(int bits){
    if(!_arduino_event_group){
        return 0;
    }
    return xEventGroupSetBits(_arduino_event_group, bits);
}

int WiFiGenericClass::clearStatusBits(int bits){
    if(!_arduino_event_group){
        return 0;
    }
    return xEventGroupClearBits(_arduino_event_group, bits);
}

int WiFiGenericClass::getStatusBits(){
    if(!_arduino_event_group){
        return 0;
    }
    return xEventGroupGetBits(_arduino_event_group);
}

int WiFiGenericClass::waitStatusBits(int bits, uint32_t timeout_ms){
    if(!_arduino_event_group){
        return 0;
    }
    return xEventGroupWaitBits(
        _arduino_event_group,    // The event group being tested.
        bits,  // The bits within the event group to wait for.
        pdFALSE,         // BIT_0 and BIT_4 should be cleared before returning.
        pdTRUE,        // Don't wait for both bits, either bit will do.
        timeout_ms / portTICK_PERIOD_MS ) & bits; // Wait a maximum of 100ms for either bit to be set.
}

/**
 * set callback function
 * @param cbEvent WiFiEventCb
 * @param event optional filter (WIFI_EVENT_MAX is all events)
 */
wifi_event_id_t WiFiGenericClass::onEvent(WiFiEventCb cbEvent, arduino_event_id_t event)
{
    if(!cbEvent) {
        return 0;
    }
    WiFiEventCbList_t newEventHandler;
    newEventHandler.cb = cbEvent;
    newEventHandler.fcb = NULL;
    newEventHandler.scb = NULL;
    newEventHandler.event = event;
    cbEventList.push_back(newEventHandler);
    return newEventHandler.id;
}

wifi_event_id_t WiFiGenericClass::onEvent(WiFiEventFuncCb cbEvent, arduino_event_id_t event)
{
    if(!cbEvent) {
        return 0;
    }
    WiFiEventCbList_t newEventHandler;
    newEventHandler.cb = NULL;
    newEventHandler.fcb = cbEvent;
    newEventHandler.scb = NULL;
    newEventHandler.event = event;
    cbEventList.push_back(newEventHandler);
    return newEventHandler.id;
}

wifi_event_id_t WiFiGenericClass::onEvent(WiFiEventSysCb cbEvent, arduino_event_id_t event)
{
    if(!cbEvent) {
        return 0;
    }
    WiFiEventCbList_t newEventHandler;
    newEventHandler.cb = NULL;
    newEventHandler.fcb = NULL;
    newEventHandler.scb = cbEvent;
    newEventHandler.event = event;
    cbEventList.push_back(newEventHandler);
    return newEventHandler.id;
}

/**
 * removes a callback form event handler
 * @param cbEvent WiFiEventCb
 * @param event optional filter (WIFI_EVENT_MAX is all events)
 */
void WiFiGenericClass::removeEvent(WiFiEventCb cbEvent, arduino_event_id_t event)
{
    if(!cbEvent) {
        return;
    }

    for(uint32_t i = 0; i < cbEventList.size(); i++) {
        WiFiEventCbList_t entry = cbEventList[i];
        if(entry.cb == cbEvent && entry.event == event) {
            cbEventList.erase(cbEventList.begin() + i);
        }
    }
}

void WiFiGenericClass::removeEvent(WiFiEventSysCb cbEvent, arduino_event_id_t event)
{
    if(!cbEvent) {
        return;
    }

    for(uint32_t i = 0; i < cbEventList.size(); i++) {
        WiFiEventCbList_t entry = cbEventList[i];
        if(entry.scb == cbEvent && entry.event == event) {
            cbEventList.erase(cbEventList.begin() + i);
        }
    }
}

void WiFiGenericClass::removeEvent(wifi_event_id_t id)
{
    for(uint32_t i = 0; i < cbEventList.size(); i++) {
        WiFiEventCbList_t entry = cbEventList[i];
        if(entry.id == id) {
            cbEventList.erase(cbEventList.begin() + i);
        }
    }
}

/**
 * callback for WiFi events
 * @param arg
 */
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
const char * arduino_event_names[] = {
		"WIFI_READY",
		"SCAN_DONE",
		"STA_START", "STA_STOP", "STA_CONNECTED", "STA_DISCONNECTED", "STA_AUTHMODE_CHANGE", "STA_GOT_IP", "STA_GOT_IP6", "STA_LOST_IP",
		"AP_START", "AP_STOP", "AP_STACONNECTED", "AP_STADISCONNECTED", "AP_STAIPASSIGNED", "AP_PROBEREQRECVED", "AP_GOT_IP6", 
		"FTM_REPORT",
		"ETH_START", "ETH_STOP", "ETH_CONNECTED", "ETH_DISCONNECTED", "ETH_GOT_IP", "ETH_GOT_IP6",
		"WPS_ER_SUCCESS", "WPS_ER_FAILED", "WPS_ER_TIMEOUT", "WPS_ER_PIN", "WPS_ER_PBC_OVERLAP",
		"SC_SCAN_DONE", "SC_FOUND_CHANNEL", "SC_GOT_SSID_PSWD", "SC_SEND_ACK_DONE",
		"PROV_INIT", "PROV_DEINIT", "PROV_START", "PROV_END", "PROV_CRED_RECV", "PROV_CRED_FAIL", "PROV_CRED_SUCCESS"
};
#endif
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_WARN
const char * system_event_reasons[] = { "UNSPECIFIED", "AUTH_EXPIRE", "AUTH_LEAVE", "ASSOC_EXPIRE", "ASSOC_TOOMANY", "NOT_AUTHED", "NOT_ASSOCED", "ASSOC_LEAVE", "ASSOC_NOT_AUTHED", "DISASSOC_PWRCAP_BAD", "DISASSOC_SUPCHAN_BAD", "UNSPECIFIED", "IE_INVALID", "MIC_FAILURE", "4WAY_HANDSHAKE_TIMEOUT", "GROUP_KEY_UPDATE_TIMEOUT", "IE_IN_4WAY_DIFFERS", "GROUP_CIPHER_INVALID", "PAIRWISE_CIPHER_INVALID", "AKMP_INVALID", "UNSUPP_RSN_IE_VERSION", "INVALID_RSN_IE_CAP", "802_1X_AUTH_FAILED", "CIPHER_SUITE_REJECTED", "BEACON_TIMEOUT", "NO_AP_FOUND", "AUTH_FAIL", "ASSOC_FAIL", "HANDSHAKE_TIMEOUT", "CONNECTION_FAIL" };
#define reason2str(r) ((r>176)?system_event_reasons[r-176]:system_event_reasons[r-1])
#endif
esp_err_t WiFiGenericClass::_eventCallback(arduino_event_t *event)
{
    if(event->event_id < ARDUINO_EVENT_MAX) {
        log_d("Arduino Event: %d - %s", event->event_id, arduino_event_names[event->event_id]);
    }
    if(event->event_id == ARDUINO_EVENT_WIFI_SCAN_DONE) {
        WiFiScanClass::_scanDone();

    } else if(event->event_id == ARDUINO_EVENT_WIFI_STA_START) {
        WiFiSTAClass::_setStatus(WL_DISCONNECTED);
        setStatusBits(STA_STARTED_BIT);
        if(esp_wifi_set_ps(_sleepEnabled) != ESP_OK){
            log_e("esp_wifi_set_ps failed");
        }
    } else if(event->event_id == ARDUINO_EVENT_WIFI_STA_STOP) {
        WiFiSTAClass::_setStatus(WL_NO_SHIELD);
        clearStatusBits(STA_STARTED_BIT | STA_CONNECTED_BIT | STA_HAS_IP_BIT | STA_HAS_IP6_BIT);
    } else if(event->event_id == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
        WiFiSTAClass::_setStatus(WL_IDLE_STATUS);
        setStatusBits(STA_CONNECTED_BIT);

        //esp_netif_create_ip6_linklocal(esp_netifs[ESP_IF_WIFI_STA]);
    } else if(event->event_id == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        uint8_t reason = event->event_info.wifi_sta_disconnected.reason;
        log_w("Reason: %u - %s", reason, reason2str(reason));
        if(reason == WIFI_REASON_NO_AP_FOUND) {
            WiFiSTAClass::_setStatus(WL_NO_SSID_AVAIL);
        } else if(reason == WIFI_REASON_AUTH_FAIL || reason == WIFI_REASON_ASSOC_FAIL) {
            WiFiSTAClass::_setStatus(WL_CONNECT_FAILED);
        } else if(reason == WIFI_REASON_BEACON_TIMEOUT || reason == WIFI_REASON_HANDSHAKE_TIMEOUT) {
            WiFiSTAClass::_setStatus(WL_CONNECTION_LOST);
        } else if(reason == WIFI_REASON_AUTH_EXPIRE) {

        } else {
            WiFiSTAClass::_setStatus(WL_DISCONNECTED);
        }
        clearStatusBits(STA_CONNECTED_BIT | STA_HAS_IP_BIT | STA_HAS_IP6_BIT);
        if(((reason == WIFI_REASON_AUTH_EXPIRE) ||
            (reason >= WIFI_REASON_BEACON_TIMEOUT && reason != WIFI_REASON_AUTH_FAIL)) &&
            WiFi.getAutoReconnect())
        {
            WiFi.disconnect();
            WiFi.begin();
        }
    } else if(event->event_id == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
        uint8_t * ip = (uint8_t *)&(event->event_info.got_ip.ip_info.ip.addr);
        uint8_t * mask = (uint8_t *)&(event->event_info.got_ip.ip_info.netmask.addr);
        uint8_t * gw = (uint8_t *)&(event->event_info.got_ip.ip_info.gw.addr);
        log_d("STA IP: %u.%u.%u.%u, MASK: %u.%u.%u.%u, GW: %u.%u.%u.%u",
            ip[0], ip[1], ip[2], ip[3],
            mask[0], mask[1], mask[2], mask[3],
            gw[0], gw[1], gw[2], gw[3]);
#endif
        WiFiSTAClass::_setStatus(WL_CONNECTED);
        setStatusBits(STA_HAS_IP_BIT | STA_CONNECTED_BIT);
    } else if(event->event_id == ARDUINO_EVENT_WIFI_STA_LOST_IP) {
        WiFiSTAClass::_setStatus(WL_IDLE_STATUS);
        clearStatusBits(STA_HAS_IP_BIT);

    } else if(event->event_id == ARDUINO_EVENT_WIFI_AP_START) {
        setStatusBits(AP_STARTED_BIT);
    } else if(event->event_id == ARDUINO_EVENT_WIFI_AP_STOP) {
        clearStatusBits(AP_STARTED_BIT | AP_HAS_CLIENT_BIT);
    } else if(event->event_id == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
        setStatusBits(AP_HAS_CLIENT_BIT);
    } else if(event->event_id == ARDUINO_EVENT_WIFI_AP_STADISCONNECTED) {
        wifi_sta_list_t clients;
        if(esp_wifi_ap_get_sta_list(&clients) != ESP_OK || !clients.num){
            clearStatusBits(AP_HAS_CLIENT_BIT);
        }

    } else if(event->event_id == ARDUINO_EVENT_ETH_START) {
        setStatusBits(ETH_STARTED_BIT);
    } else if(event->event_id == ARDUINO_EVENT_ETH_STOP) {
        clearStatusBits(ETH_STARTED_BIT | ETH_CONNECTED_BIT | ETH_HAS_IP_BIT | ETH_HAS_IP6_BIT);
    } else if(event->event_id == ARDUINO_EVENT_ETH_CONNECTED) {
        setStatusBits(ETH_CONNECTED_BIT);
    } else if(event->event_id == ARDUINO_EVENT_ETH_DISCONNECTED) {
        clearStatusBits(ETH_CONNECTED_BIT | ETH_HAS_IP_BIT | ETH_HAS_IP6_BIT);
    } else if(event->event_id == ARDUINO_EVENT_ETH_GOT_IP) {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
        uint8_t * ip = (uint8_t *)&(event->event_info.got_ip.ip_info.ip.addr);
        uint8_t * mask = (uint8_t *)&(event->event_info.got_ip.ip_info.netmask.addr);
        uint8_t * gw = (uint8_t *)&(event->event_info.got_ip.ip_info.gw.addr);
        log_d("ETH IP: %u.%u.%u.%u, MASK: %u.%u.%u.%u, GW: %u.%u.%u.%u",
            ip[0], ip[1], ip[2], ip[3],
            mask[0], mask[1], mask[2], mask[3],
            gw[0], gw[1], gw[2], gw[3]);
#endif
        setStatusBits(ETH_CONNECTED_BIT | ETH_HAS_IP_BIT);

    } else if(event->event_id == ARDUINO_EVENT_WIFI_STA_GOT_IP6) {
    	setStatusBits(STA_CONNECTED_BIT | STA_HAS_IP6_BIT);
    } else if(event->event_id == ARDUINO_EVENT_WIFI_AP_GOT_IP6) {
    	setStatusBits(AP_HAS_IP6_BIT);
    } else if(event->event_id == ARDUINO_EVENT_ETH_GOT_IP6) {
    	setStatusBits(ETH_CONNECTED_BIT | ETH_HAS_IP6_BIT);
    } else if(event->event_id == ARDUINO_EVENT_SC_GOT_SSID_PSWD) {
    	WiFi.begin(
			(const char *)event->event_info.sc_got_ssid_pswd.ssid,
			(const char *)event->event_info.sc_got_ssid_pswd.password,
			0,
			((event->event_info.sc_got_ssid_pswd.bssid_set == true)?event->event_info.sc_got_ssid_pswd.bssid:NULL)
		);
    } else if(event->event_id == ARDUINO_EVENT_SC_SEND_ACK_DONE) {
    	esp_smartconfig_stop();
    	WiFiSTAClass::_smartConfigDone = true;
    }

    for(uint32_t i = 0; i < cbEventList.size(); i++) {
        WiFiEventCbList_t entry = cbEventList[i];
        if(entry.cb || entry.fcb || entry.scb) {
            if(entry.event == (arduino_event_id_t) event->event_id || entry.event == ARDUINO_EVENT_MAX) {
                if(entry.cb) {
                    entry.cb((arduino_event_id_t) event->event_id);
                } else if(entry.fcb) {
                    entry.fcb((arduino_event_id_t) event->event_id, (arduino_event_info_t) event->event_info);
                } else {
                    entry.scb(event);
                }
            }
        }
    }
    return ESP_OK;
}

/**
 * Return the current channel associated with the network
 * @return channel (1-13)
 */
int32_t WiFiGenericClass::channel(void)
{
    uint8_t primaryChan = 0;
    wifi_second_chan_t secondChan = WIFI_SECOND_CHAN_NONE;
    if(!lowLevelInitDone){
        return primaryChan;
    }
    esp_wifi_get_channel(&primaryChan, &secondChan);
    return primaryChan;
}


/**
 * store WiFi config in SDK flash area
 * @param persistent
 */
void WiFiGenericClass::persistent(bool persistent)
{
    _persistent = persistent;
}


/**
 * enable WiFi long range mode
 * @param enable
 */
void WiFiGenericClass::enableLongRange(bool enable)
{
    _long_range = enable;
}


/**
 * set new mode
 * @param m WiFiMode_t
 */
bool WiFiGenericClass::mode(wifi_mode_t m)
{
    wifi_mode_t cm = getMode();
    if(cm == m) {
        return true;
    }
    if(!cm && m){
        if(!wifiLowLevelInit(_persistent)){
            return false;
        }
    } else if(cm && !m){
        return espWiFiStop();
    }

    esp_err_t err;
    if(m & WIFI_MODE_STA){
    	err = set_esp_interface_hostname(ESP_IF_WIFI_STA, get_esp_netif_hostname());
        if(err){
            log_e("Could not set hostname! %d", err);
            return false;
        }
    }
    err = esp_wifi_set_mode(m);
    if(err){
        log_e("Could not set mode! %d", err);
        return false;
    }
    if(_long_range){
        if(m & WIFI_MODE_STA){
            err = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
            if(err != ESP_OK){
                log_e("Could not enable long range on STA! %d", err);
                return false;
            }
        }
        if(m & WIFI_MODE_AP){
            err = esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_LR);
            if(err != ESP_OK){
                log_e("Could not enable long range on AP! %d", err);
                return false;
            }
        }
    }
    if(!espWiFiStart()){
        return false;
    }
    return true;
}

/**
 * get WiFi mode
 * @return WiFiMode
 */
wifi_mode_t WiFiGenericClass::getMode()
{
    if(!lowLevelInitDone || !_esp_wifi_started){
        return WIFI_MODE_NULL;
    }
    wifi_mode_t mode;
    if(esp_wifi_get_mode(&mode) != ESP_OK){
        log_w("WiFi not started");
        return WIFI_MODE_NULL;
    }
    return mode;
}

/**
 * control STA mode
 * @param enable bool
 * @return ok
 */
bool WiFiGenericClass::enableSTA(bool enable)
{

    wifi_mode_t currentMode = getMode();
    bool isEnabled = ((currentMode & WIFI_MODE_STA) != 0);

    if(isEnabled != enable) {
        if(enable) {
            return mode((wifi_mode_t)(currentMode | WIFI_MODE_STA));
        }
        return mode((wifi_mode_t)(currentMode & (~WIFI_MODE_STA)));
    }
    return true;
}

/**
 * control AP mode
 * @param enable bool
 * @return ok
 */
bool WiFiGenericClass::enableAP(bool enable)
{

    wifi_mode_t currentMode = getMode();
    bool isEnabled = ((currentMode & WIFI_MODE_AP) != 0);

    if(isEnabled != enable) {
        if(enable) {
            return mode((wifi_mode_t)(currentMode | WIFI_MODE_AP));
        }
        return mode((wifi_mode_t)(currentMode & (~WIFI_MODE_AP)));
    }
    return true;
}

/**
 * control modem sleep when only in STA mode
 * @param enable bool
 * @return ok
 */
bool WiFiGenericClass::setSleep(bool enabled){
    return setSleep(enabled?WIFI_PS_MIN_MODEM:WIFI_PS_NONE);
}

/**
 * control modem sleep when only in STA mode
 * @param mode wifi_ps_type_t
 * @return ok
 */
bool WiFiGenericClass::setSleep(wifi_ps_type_t sleepType)
{
    if(sleepType != _sleepEnabled){
        _sleepEnabled = sleepType;
        if((getMode() & WIFI_MODE_STA) != 0){
            if(esp_wifi_set_ps(_sleepEnabled) != ESP_OK){
                log_e("esp_wifi_set_ps failed!");
                return false;
            }
        }
        return true;
    }
    return false;
}

/**
 * get modem sleep enabled
 * @return true if modem sleep is enabled
 */
wifi_ps_type_t WiFiGenericClass::getSleep()
{
    return _sleepEnabled;
}

/**
 * control wifi tx power
 * @param power enum maximum wifi tx power
 * @return ok
 */
bool WiFiGenericClass::setTxPower(wifi_power_t power){
    if((getStatusBits() & (STA_STARTED_BIT | AP_STARTED_BIT)) == 0){
        log_w("Neither AP or STA has been started");
        return false;
    }
    return esp_wifi_set_max_tx_power(power) == ESP_OK;
}

wifi_power_t WiFiGenericClass::getTxPower(){
    int8_t power;
    if((getStatusBits() & (STA_STARTED_BIT | AP_STARTED_BIT)) == 0){
        log_w("Neither AP or STA has been started");
        return WIFI_POWER_19_5dBm;
    }
    if(esp_wifi_get_max_tx_power(&power)){
        return WIFI_POWER_19_5dBm;
    }
    return (wifi_power_t)power;
}

/**
 * Initiate FTM Session.
 * @param frm_count Number of FTM frames requested in terms of 4 or 8 bursts (allowed values - 0(No pref), 16, 24, 32, 64)
 * @param burst_period Requested time period between consecutive FTM bursts in 100's of milliseconds (allowed values - 0(No pref), 2 - 255)
 * @param channel Primary channel of the FTM Responder
 * @param mac MAC address of the FTM Responder
 * @return true on success
 */
bool WiFiGenericClass::initiateFTM(uint8_t frm_count, uint16_t burst_period, uint8_t channel, const uint8_t * mac) {
  wifi_ftm_initiator_cfg_t ftmi_cfg = {
    .resp_mac = {0,0,0,0,0,0},
    .channel = channel,
    .frm_count = frm_count,
    .burst_period = burst_period,
  };
  if(mac != NULL){
    memcpy(ftmi_cfg.resp_mac, mac, 6);
  }
  // Request FTM session with the Responder
  if (ESP_OK != esp_wifi_ftm_initiate_session(&ftmi_cfg)) {
    log_e("Failed to initiate FTM session");
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------ Generic Network function ---------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

/**
 * DNS callback
 * @param name
 * @param ipaddr
 * @param callback_arg
 */
static void wifi_dns_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    if(ipaddr) {
        (*reinterpret_cast<IPAddress*>(callback_arg)) = ipaddr->u_addr.ip4.addr;
    }
    xEventGroupSetBits(_arduino_event_group, WIFI_DNS_DONE_BIT);
}

/**
 * Resolve the given hostname to an IP address.
 * @param aHostname     Name to be resolved
 * @param aResult       IPAddress structure to store the returned IP address
 * @return 1 if aIPAddrString was successfully converted to an IP address,
 *          else error code
 */
int WiFiGenericClass::hostByName(const char* aHostname, IPAddress& aResult)
{
    ip_addr_t addr;
    aResult = static_cast<uint32_t>(0);
    waitStatusBits(WIFI_DNS_IDLE_BIT, 16000);
    clearStatusBits(WIFI_DNS_IDLE_BIT | WIFI_DNS_DONE_BIT);
    err_t err = dns_gethostbyname(aHostname, &addr, &wifi_dns_found_callback, &aResult);
    if(err == ERR_OK && addr.u_addr.ip4.addr) {
        aResult = addr.u_addr.ip4.addr;
    } else if(err == ERR_INPROGRESS) {
        waitStatusBits(WIFI_DNS_DONE_BIT, 15000);  //real internal timeout in lwip library is 14[s]
        clearStatusBits(WIFI_DNS_DONE_BIT);
    }
    setStatusBits(WIFI_DNS_IDLE_BIT);
    if((uint32_t)aResult == 0){
        log_e("DNS Failed for %s", aHostname);
    }
    return (uint32_t)aResult != 0;
}

IPAddress WiFiGenericClass::calculateNetworkID(IPAddress ip, IPAddress subnet) {
	IPAddress networkID;

	for (size_t i = 0; i < 4; i++)
		networkID[i] = subnet[i] & ip[i];

	return networkID;
}

IPAddress WiFiGenericClass::calculateBroadcast(IPAddress ip, IPAddress subnet) {
    IPAddress broadcastIp;
    
    for (int i = 0; i < 4; i++)
        broadcastIp[i] = ~subnet[i] | ip[i];

    return broadcastIp;
}

uint8_t WiFiGenericClass::calculateSubnetCIDR(IPAddress subnetMask) {
	uint8_t CIDR = 0;

	for (uint8_t i = 0; i < 4; i++) {
		if (subnetMask[i] == 0x80)  // 128
			CIDR += 1;
		else if (subnetMask[i] == 0xC0)  // 192
			CIDR += 2;
		else if (subnetMask[i] == 0xE0)  // 224
			CIDR += 3;
		else if (subnetMask[i] == 0xF0)  // 242
			CIDR += 4;
		else if (subnetMask[i] == 0xF8)  // 248
			CIDR += 5;
		else if (subnetMask[i] == 0xFC)  // 252
			CIDR += 6;
		else if (subnetMask[i] == 0xFE)  // 254
			CIDR += 7;
		else if (subnetMask[i] == 0xFF)  // 255
			CIDR += 8;
	}

	return CIDR;
}
