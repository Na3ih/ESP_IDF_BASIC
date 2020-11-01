#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"


// set AP CONFIG values
#ifdef CONFIG_AP_HIDE_SSID
	#define CONFIG_AP_SSID_HIDDEN 1
#else
	#define CONFIG_AP_SSID_HIDDEN 0
#endif	
#ifdef CONFIG_WIFI_AUTH_OPEN
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_OPEN
#endif
#ifdef CONFIG_WIFI_AUTH_WEP
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WEP
#endif
#ifdef CONFIG_WIFI_AUTH_WPA_PSK
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA2_PSK
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA2_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA_WPA2_PSK
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA_WPA2_PSK
#endif
#ifdef CONFIG_WIFI_AUTH_WPA2_ENTERPRISE
	#define CONFIG_AP_AUTHMODE WIFI_AUTH_WPA2_ENTERPRISE
#endif

// Event group
static EventGroupHandle_t event_group;
const int STA_CONNECTED_BIT = BIT0;

// prototypes
static esp_err_t event_handler(void *ctx, system_event_t *event);
void ap_monitor_task(void *pvParameter);
static void http_server(void *pvParameters);
static void http_server_netconn_serve(struct netconn *conn);

#include "html.h"
// HTTP headers and web pages
const static char http_html_hdr[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
// const static char http_string[] = htmlExample;
// AP event handler
static esp_err_t event_handler(void *ctx, system_event_t *event) {
    switch(event->event_id) {
		
    case SYSTEM_EVENT_AP_START:
	
		printf("- Wifi adapter started\n\n");
		
		// start the HTTP server task
		xTaskCreate(&http_server, "http_server", 20000, NULL, 5, NULL);
		printf("- HTTP server started\n");

		break;
		
	case SYSTEM_EVENT_AP_STACONNECTED:
	
		xEventGroupSetBits(event_group, STA_CONNECTED_BIT);
		break;		
    
	default:
        break;
    }
   
	return ESP_OK;
}


// AP monitor task, receive Wifi AP events
void ap_monitor_task(void *pvParameter) {
	
	while(1) {
		
		xEventGroupWaitBits(event_group, STA_CONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
		printf("New station connected\n");
	}
}


// HTTP server task
static void http_server(void *pvParameters) {
	
	struct netconn *conn, *newconn;
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
	netconn_listen(conn);
	printf("* HTTP Server listening\n");
	do {
		err = netconn_accept(conn, &newconn);
		if (err == ERR_OK) {
			http_server_netconn_serve(newconn);
			netconn_delete(newconn);
		}
		vTaskDelay(10);
	} while(err == ERR_OK);
	netconn_close(conn);
	netconn_delete(conn);
}

static void http_server_netconn_serve(struct netconn *conn) {

	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;

	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
	  
		// get the request and terminate the string
		netbuf_data(inbuf, (void**)&buf, &buflen);
		buf[buflen] = '\0';
		
		// get the request body and the first line
		char* body = strstr(buf, "\r\n\r\n");
		char *request_line = strtok(buf, "\n");
		
		if(request_line) {
			
			// default page -> redirect to index.html
			if(strstr(request_line, "GET / ")) {
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
                netconn_write(conn, htmlExample, sizeof(htmlExample) - 1, NETCONN_NOCOPY);					
                printf("ASKED FOR INDEX.HTML \n\n");    ///<<< TODO: Dodac strone
			}
		}
	}
}

// Main application
void app_main()
{	
	// disable the default wifi logging
	esp_log_level_set("wifi", ESP_LOG_NONE);
	
	printf("ESP32 SoftAP HTTP Server\n\n");
	
	// create the event group to handle wifi events
	event_group = xEventGroupCreate();
	
	// initialize NVS
	ESP_ERROR_CHECK(nvs_flash_init());
	printf("- NVS initialized\n");
		
	// initialize the tcp stack
	tcpip_adapter_init();
	printf("- TCP adapter initialized\n");

	// stop DHCP server
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	printf("- DHCP server stopped\n");
	
	// assign a static IP to the network interface
	tcpip_adapter_ip_info_t info;
    memset(&info, 0, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 1, 1);
    IP4_ADDR(&info.gw, 192, 168, 1, 1);
    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
	printf("- TCP adapter configured with IP 192.168.1.1/24\n");
	
	// start the DHCP server   
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
	printf("- DHCP server started\n");
	
	// initialize the wifi event handler
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	printf("- Event loop initialized\n");
	
	// initialize the wifi stack in AccessPoint mode with config in RAM
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	printf("- Wifi adapter configured in SoftAP mode\n");

	// configure the wifi connection and start the interface
	wifi_config_t ap_config = {
        .ap = {
            .ssid = "ESP32",
            .password = "ESP32ESP32",
			.ssid_len = 5,
			.channel = 1,
			.authmode = WIFI_AUTH_WPA_PSK,
			.ssid_hidden = CONFIG_AP_SSID_HIDDEN,
			.max_connection = 10,
			.beacon_interval = 100,			
        },
    };
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	printf("- Wifi network settings applied\n");
	
    
	// start the wifi interface
	ESP_ERROR_CHECK(esp_wifi_start());
	printf("- Wifi adapter starting...\n");
	
	// start the tasks
    xTaskCreate(&ap_monitor_task, "ap_monitor_task", 2048, NULL, 5, NULL);
}