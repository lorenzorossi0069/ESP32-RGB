/* WiFi_library.h
*/

#ifndef _WIFI_LIBRARY_H_
#define _WIFI_LIBRARY_H_

//like-public
void composeUniqueSSIDname(char* retString);
void wifi_init_sta(void);
void wifi_init_softap(void);
void initialize_mDNS(char *dns_name);

char* get_Esp_IP(void);

#endif





