/*  WiFi_library.c
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h" 
#include "esp_system.h" 
#include "esp_wifi.h" 
#include "esp_log.h"
#include "mdns.h" 
//#include "lwip/api.h" 
//#include "lwip/err.h"
#include "lwip/netdb.h" 

//project specific libraries
#include "WebServer_library.h"
#include "esp_netif.h"  //replace tcpip_adapter.h
#include "WiFi_library.h"


//---------------- defines
#if CONFIG_SET_MODE_NO
	#define EXAMPLE_ESP_WIFI_SSID      "" //if not defined get compile error
	#define EXAMPLE_ESP_WIFI_PASS      ""
#else
	#define EXAMPLE_ESP_WIFI_SSID      CONFIG_EXAMPLE_WIFI_SSID
	#define EXAMPLE_ESP_WIFI_PASS      CONFIG_EXAMPLE_WIFI_PASSWORD
		
#endif

#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_WIFI_STA_MAXIMUM_RETRY

#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_WIFI_CHANNEL
#define EXAMPLE_MAXIMUM_CONN       CONFIG_WIFI_AP_MAXIMUM_CONN

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0 //STAtion Wifi
#define WIFI_FAIL_BIT      BIT1 //STAtion Wifi (used in case of limited number of connection trials)
//aaa//#define AP2STA_CONNECTED_BIT   BIT2 //when Esp32 is AP and at least one station connected to it


static const char *TAG = "WiFi_library";
static const char *TAG_2 = "WiFi_library (Tag2)";
static char Esp_IP[16]; //IP address 
static int s_retry_num = 0;

//EventGroupHandle_t is a FreeRTOS event group to signal when we are connected
static EventGroupHandle_t s_wifi_event_group; //used only with WiFi STA
static void my_Detect_IP_for_AP_mode(char*);

void wifi_init_sta();
void wifi_init_softap(void);
void composeUniqueSSIDname(char* retString);

static void WiFi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
	
//----------------function definitions

char* get_Esp_IP(void) { //to get IP (string) from other files
	return Esp_IP; //address copy
}

static void WiFi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){	
	ESP_LOGI(TAG_2, "New Event: base = %s and Id = %d",(char*)event_base,event_id); 	
	
	// A list of events is shown at: 
	// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/event-handling.html 
	
	//==================events for STA mode======================
	//event triggered by esp_wifi_start() as STA:
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();	
		ESP_LOGI(TAG_2, "Event detail: end of esp_wifi_connect() and WIFI_EVENT_STA_START\n"); 
    } 
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) { 
		//// OPTION 1: STA with limited number of connection trials 
		////(at end unregister event handler)
		/*
        //if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
        //    esp_wifi_connect();
        //    s_retry_num++;
        //    ESP_LOGI(TAG, "retry to connect to the STAtion");
        //} else {
        //    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		//	ESP_LOGI(TAG,"connect to the AP fail");
        //}
		*/
		
		//// OPTION 2: STA with unlimited number of connection trials
		esp_wifi_connect();
		ESP_LOGI(TAG, "retry to connect to the STAtion");  
		vTaskDelay(300); //wait 3 second before retrying to connect
		ESP_LOGI(TAG_2, "Event detail: end of esp_wifi_connect() and WIFI_EVENT_STA_DISCONNECTED\n"); 
    } 
	//this event arises when the DHCP client successfully gets the IPV4 address from the external DHCP server
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) { 
	/*
	https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#wi-fi-got-ip-phase:
	A very common mistake is to initialize the socket before IP_EVENT_STA_GOT_IP is received. 
	DO NOT start the socket-related work before the IP is received.
	*/
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "STAtion got ip:" IPSTR, IP2STR(&event->ip_info.ip)); 
		
		//save STA IP address to string
		sprintf(Esp_IP,"%d.%d.%d.%d",IP2STR(&event->ip_info.ip));
		
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
		ESP_LOGI(TAG_2, "Event detail: end of IP_EVENT_STA_GOT_IP and xEventGroupSetBits");  	
    }
	//===================events for AP mode==========================
	//event triggered by esp_wifi_start() as AP:
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
		ESP_LOGI(TAG, "Wifi adapter started (event WIFI_EVENT_AP_START)");
				
		/*
		////initialize mDNS service (for AP only)
		//initialize_mDNS("webesp32"); //(for AP only)
		//
		//// start the HTTP server task
		////xTaskCreate(&http_server_task, "http_server_task", 20000, NULL, 5, NULL);
		//xTaskCreate(&http_server_task, "http_server_task", 4096, NULL, 5, NULL); // TBD posso ridurre stack ??
		//
		//ESP_LOGI(TAG, "HTTP server task started");
		*/
		ESP_LOGI(TAG_2, "(AP1): end of WIFI_EVENT_AP_START\n"); 
	}
	//event triggered by external STAtion connecting to this AP:
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
		//xEventGroupSetBits(s_wifi_event_group, AP2STA_CONNECTED_BIT);  //aaa/
		
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "AccessPoint "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
		
		//save AP IP address to string
		my_Detect_IP_for_AP_mode(&Esp_IP[0]);
		ESP_LOGI(TAG_2, "Event detail (AP2): end of WIFI_EVENT_AP_STACONNECTED and my_Detect_IP_for_AP_mode()\n");  
	
    } 
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "AccessPoint "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
		ESP_LOGI(TAG_2, "Event detail (AP3): end of WIFI_EVENT_AP_STADISCONNECTED and nothing more\n");
    }
}

void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();

//TCP stack initialize
    ESP_ERROR_CHECK(esp_netif_init()); //replaces deprecated tcp_adapter_init() of v3.x!!!!
	
//wifi event handler
    ESP_ERROR_CHECK(esp_event_loop_create_default()); //replaces deprecated esp_event_loop_init() of v3.x!!!!
	
    esp_netif_create_default_wifi_sta();

//WiFi stack: configure
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();	
//WiFi stack: initialize
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. In case your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
	
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) ); 
    ESP_ERROR_CHECK(esp_wifi_start() ); //this triggers event WIFI_EVENT_STA_START

	ESP_LOGI(TAG, "STAtion mode with SSID_name = %s\n",EXAMPLE_ESP_WIFI_SSID);	

/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
* number of re-tries (WIFI_FAIL_BIT). The bits are set by WiFi_event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE, //previous BITs should be cleared before returning.
            pdFALSE, //// Don't wait for both bits, either bit will do.
            portMAX_DELAY);

/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
* happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "STAtion connected to ap SSID:%s", EXAMPLE_ESP_WIFI_SSID); // Router's password is not displayed
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "STAtion Failed to connect to SSID:%s",  EXAMPLE_ESP_WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

	//// in case of OPTION 1 (above): STA with limited number of connection trials
	////remove following lines allows STA reconnection, in case of external AP reboot
	/*
	////(this in case of limited number of connection retries, as chosen above)
    //ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFi_event_handler));
    //ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi_event_handler));
    //vEventGroupDelete(s_wifi_event_group);
	*/
}

void wifi_init_softap(void) { 

	char unique_SSID_name[32]; //unique SSID name (MAC address based)
	composeUniqueSSIDname(unique_SSID_name);

//TCP stack initialize	
    ESP_ERROR_CHECK(esp_netif_init());
	
	
	//***************** NON FUNZIONA: prova per cambiare IP address di AP (Stop / start DHCPS)********
	// stop DHCP server
	/*
	//ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	//esp_err_t     esp_netif_dhcpc_stop(esp_netif_t *esp_netif);
	////ESP_ERROR_CHECK(esp_netif_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	//printf("- DHCP server stopped\n");
	
	// assign a static IP to the network interface
	tcpip_adapter_ip_info_t info;
    memset(&info, 0, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 20, 1);
    IP4_ADDR(&info.gw, 192, 168, 20, 1);
    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	//////////////////ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
	printf("- TCP adapter configured with IP 192.168.20.1/24\n");
	
	// start the DHCP server   
    //ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
	////ESP_ERROR_CHECK(esp_netif_dhcps_start(TCPIP_ADAPTER_IF_AP));
	//printf("- DHCP server started\n");
	
	*/	
	//****************** end prova DHCPS ********************
	
//wifi event handler	
    ESP_ERROR_CHECK(esp_event_loop_create_default());
		
    esp_netif_create_default_wifi_ap();
 
//WiFi stack: configure
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//WiFi stack: initialize
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFi_event_handler, NULL));
	
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAXIMUM_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
	
	//Replace base SSID given in menuconfig, by appending an unique tail (based on MAC address)
	strcpy((char*)wifi_config.ap.ssid, (char*)unique_SSID_name);
	wifi_config.ap.ssid_len = strlen((char*)wifi_config.ap.ssid);
	ESP_LOGI(TAG, "Unique SSID is: %s (pwd = %s)",wifi_config.ap.ssid,(char*)wifi_config.ap.password); 
	
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config)); 
	
    ESP_ERROR_CHECK(esp_wifi_start()); //this triggers event WIFI_EVENT_AP_START

	//note: for AP wifi_config.ap.ssid (=unique SSID name) instead of EXAMPLE_ESP_WIFI_SSID
	if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0){
		//ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s (no password) channel:%d",
        //     wifi_config.ap.ssid, EXAMPLE_ESP_WIFI_CHANNEL);
		ESP_LOGI(TAG_2, "wifi_init_softap finished. Now waiting event EXAMPLE_ESP_WIFI_CHANNEL");		 
	}
	else {
		ESP_LOGI(TAG_2, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             wifi_config.ap.ssid, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
	}
}

void composeUniqueSSIDname(char* retString) {
	char MACaddrSuffix[18]="\0";
	uint8_t mac[6];
	if (ESP_OK==esp_efuse_mac_get_default(&mac[0]))
	{
		sprintf(MACaddrSuffix,"%x:%x:%x:%x:%x:%x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
		ESP_LOGI(TAG, "esp_efuse_mac_get_default()= %s",MACaddrSuffix);
		sprintf(MACaddrSuffix,"%x%x",mac[4],mac[5]); //suffix: last 2 of MAC address
	}
	else {
		ESP_LOGI(TAG, "esp_efuse_mac_get_default()!= ESP_OK");	
	}

	sprintf(retString,"%s-%s",EXAMPLE_ESP_WIFI_SSID, MACaddrSuffix);
}

static void my_Detect_IP_for_AP_mode(char* Esp_IP) {
	//NOTES about this function:
	/*	
	//instead of using hardwired AP IP address
	//printf("\nHardwired Esp_IP %d.%d.%d.%d",192,168,4,1);
	//it is better to get it from system, using an API, in case it changes
	//---------------------------------------------------------------------------
	//it seems that in AP mode the struct containing IP address is different from STA case
	//I implemented my own function, but it can be replaced from proper API or macro, when found
	
	//---------------------------------------------------------------------------
	from ...\esp-idf\components\mdns\test_afl_fuzz_host\esp32_compat.h:
		
	typedef struct {
		ip4_addr_t ip;   		//it is a 4 byte field
		ip4_addr_t netmask;		//same as above
		ip4_addr_t gw;			//same as above
	} tcpip_adapter_ip_info_t;
	//-------------------------------------
	
	//following instructions works, and I used them
	printf("\n0- %d", (((uint8_t*)(&AP_ip_info.ip.addr))[0])); //ok
	printf("\n1- %d", (((uint8_t*)(&AP_ip_info.ip.addr))[1]));
	printf("\n2- %d", (((uint8_t*)(&AP_ip_info.ip.addr))[2]));
	printf("\n3- %d", (((const uint8_t*)(&raw_AP_IP_addr))[3]));
	//-------------------------------------
	
	//The 32 bit integere woth IP address is:
	raw_AP_IP_addr = AP_ip_info.ip.addr;
	//---------------------------------------------------------------------------
		
	//but I created following function	because the above things don't hold for AP	
	*/
	
	tcpip_adapter_ip_info_t 	AP_ip_info; 		//struct with all AP infos
	//uint32_t raw_AP_IP_addr	 =  AP_ip_info.ip.addr; //IP address as a 4 Bytes integer

	tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &AP_ip_info); 
	//printf( "\nIP tot = 0x%x", AP_ip_info.ip.addr); //to check raw IP address
			
	sprintf(Esp_IP,"%d.%d.%d.%d",(((uint8_t*)(&AP_ip_info.ip.addr))[0]),
									 (((uint8_t*)(&AP_ip_info.ip.addr))[1]), 
									 (((uint8_t*)(&AP_ip_info.ip.addr))[2]), 
									 (((uint8_t*)(&AP_ip_info.ip.addr))[3]));	
		
	//printf ("\ndebug detected AP IP is %s",Esp_IP); //just check
	
}
void initialize_mDNS(char *dns_name) {
	//----------------------------------------------
	//NOTE: mDNS is not supported in some OS (needs Avahi or Bonjour)
	//----------------------------------------------	
	//initialize mDNS service
	ESP_ERROR_CHECK(mdns_init());
	//set hostname
	ESP_ERROR_CHECK(mdns_hostname_set(dns_name));
	//set instance name (=friendly name)
	ESP_ERROR_CHECK(mdns_instance_name_set("HTTP Server"));
	
	ESP_LOGI(TAG, "mDNS started (note: http://%s does not work with some OS)",dns_name);	
}


