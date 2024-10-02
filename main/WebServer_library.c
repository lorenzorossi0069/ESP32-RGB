/* WebServer_library.c
*/
#include <stdlib.h> //for strtol()
#include "esp_log.h" 
#include "lwip/api.h" 
#include "lwip/netdb.h" 
#include "cJSON.h"
//no//#include "GPIO_library.h"
#include "WiFi_library.h" //IP address
//no//#include "timeCounters_library.h" 
#include "nvs_library.h"
#include "spiffs_lib.h"

//#define USE_3_RGB_SLIDERS

static const char *TAG =   "WebServer_library";
static const char *TAG_2 = "WebServer_library(Tag_2)";  //minor priority

//per GET [0-0xFF]
int lux_R_post;
int lux_G_post;
int lux_B_post;

//per POST
long lux_RGB_post; //for long strtol()
char RGB_value_str_get[10];

void http_server_task(void *pvParameters);
//static void fillStrings(void);
static void http_server_netconn_serve_RGB(struct netconn *conn);
static void spiffs_serve(char* resource, struct netconn *conn);


// static headers for HTTP responses
const static char http_html_hdr[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
const static char http_404_hdr[] =  "HTTP/1.1 404 NOT FOUND\n\n";

void strAsHex(int lux_R, int lux_G, int lux_B,  char* hexString) {
	//this function converts [0xRR,0xGG,0xBB] into string "#RRGGBB" for JSON		
	static const char nibble_tab[] = "0123456789ABCDEF";
	int nti; //nibble table index
	long lux_RGB_get = lux_R*0x10000 + lux_G*0x100 + lux_B;

	*hexString='#';
	for(int pi=5; pi>=0; pi--) {
		nti=(lux_RGB_get>>(pi*4))&0x0000000F; //nibble table index	
		*(++hexString)=nibble_tab[nti]; 		
	}
	*(hexString+7)='\0'; //sting terminator	
}

//serve static content from SPIFFS
void spiffs_serve(char* resource, struct netconn *conn) {		
	char full_path[100];
	sprintf(full_path, "/spiffs%s", resource);
	ESP_LOGI(TAG, "Serving static resource: %s", full_path);
	struct stat st;
	if (stat(full_path, &st) == 0) {
		ESP_LOGI(TAG_2, "%s",http_html_hdr); //"HTTP/1.1 200 OK\nContent-type: text/html\n\n";
		netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
		
		//open the file for reading
		FILE* f = fopen(full_path, "r");
		if(f == NULL) {
			printf("Unable to open the file %s\n", full_path);
			return;
		}
		
		//send the file content to the client
		char buffer[1024];
		while(fgets(buffer, 1024, f)) {
			//ESP_LOGI(TAG_2, "buffer= %s", buffer); //new 2021-02-12
			netconn_write(conn, buffer, strlen(buffer), NETCONN_NOCOPY);
		}
		fclose(f);
		fflush(stdout);
	}
	else {
		ESP_LOGI(TAG, "%s", http_404_hdr); //"err 404"
		netconn_write(conn, http_404_hdr, sizeof(http_404_hdr) - 1, NETCONN_NOCOPY);
	}
}


static void http_server_netconn_serve_RGB(struct netconn *conn) {
	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;

	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
	  
		// get the request and terminate the string
		netbuf_data(inbuf, (void**)&buf, &buflen);
		buf[buflen] = '\0';
		
		ESP_LOGI(TAG_2, "+++++++++++++++++++++");
		ESP_LOGI(TAG_2, "(line %d): buf= %s",__LINE__, buf); 
		ESP_LOGI(TAG_2, "---------------------");
		
		// get the request body and the first line
		char* body = strstr(buf, "\r\n\r\n");	 	
		char *request_line = strtok(buf, "\n");
		
		if(request_line) {
					
			//print useful info for diagnostic (eg weird HTML requests)
			ESP_LOGI(TAG_2, "(line %d): request_line= %s",__LINE__, request_line);
			
			//add another 2nd print useful info for diagnostic (eg weird HTML requests)
			ESP_LOGI(TAG_2, "(line %d): body= %s",__LINE__, body);
											
			// dynamic page: setConfig
			if(strstr(request_line, "POST /setConfig")) {		
				cJSON *root = cJSON_Parse(body);	
				
				//info	for POST  ----------------
				if (!root)
				{
					printf("leo Error before: [%s]\n",cJSON_GetErrorPtr()); //new 19-02-21
				}
				ESP_LOGI(TAG_2, "(line %d): cJSON (RAW) *root (Addr) = %d", __LINE__,(int)root); ////new 19-02-21
				char *debug_rendered = cJSON_Print(root);
				if (debug_rendered ==  NULL) {
					ESP_LOGI(TAG_2, "(line %d): rx POST->NULL ptr", __LINE__);
				}else{
					ESP_LOGI(TAG_2, "(line %d): rx POST %s", __LINE__, debug_rendered); 
				}
				//----------------

			//#ifndef USE_3_RGB_SLIDERS				
				cJSON *lux_RGB_item = cJSON_GetObjectItemCaseSensitive(root, "lux-RGB"); 	
				//NOTE 1: use strtol(..16) instead of atoi() to convert str to long hex 
				//NOTE 2: start string from index[1] to skip initial "#"
	
				lux_RGB_post = strtol(&(lux_RGB_item->valuestring)[1], NULL, 16); 
							
				setR_FF((lux_RGB_post&0x00FF0000)>>16);
				setG_FF((lux_RGB_post&0x0000FF00)>>8);
				setB_FF(lux_RGB_post&0x000000FF);
				
			//#else
			//	printf("\n...pause 2 sec per debug..\n"); //debug
			//	vTaskDelay(250); //pause 2.5 sec per debug
			//	//and hereafter set single colors R, G and B			
			//	//extra: in case of 3 independent sliders
			//	cJSON *lux_R_item = cJSON_GetObjectItemCaseSensitive(root, "lux-R");
			//	lux_R_post = atoi(lux_R_item->valuestring);
			//	
			//	cJSON *lux_G_item = cJSON_GetObjectItemCaseSensitive(root, "lux-G"); 
			//	lux_G_post = atoi(lux_G_item->valuestring);
			//	
			//	cJSON *lux_B_item = cJSON_GetObjectItemCaseSensitive(root, "lux-B"); 
			//	lux_B_post = atoi(lux_B_item->valuestring);
			//					
			//	setR_FF(lux_R_post);
			//	setG_FF(lux_G_post);
			//	setB_FF(lux_B_post);	
			//	
			//	//debug end
			//#endif
			
				cJSON_Delete(root); //new 19-02-21 cJSON_Print allocates memory
	
				ESP_LOGI(TAG, "POST => set [%d %d %d]\n", getR_FF(), getG_FF(), getB_FF());
			}
			
			else if(strstr(request_line, "GET /getConfig")) {
			
				cJSON *root = cJSON_CreateObject();
								
				strAsHex(getR_FF(), getG_FF(), getB_FF(), RGB_value_str_get);

				cJSON_AddStringToObject(root, "lux-RGB", RGB_value_str_get);
				
				char *rendered = cJSON_Print(root);
				
				ESP_LOGI(TAG_2, "(line %d): GET /getConfig: sending root: %s", __LINE__, rendered);
				
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, rendered, strlen(rendered), NETCONN_NOCOPY);
			}
						
			// default page: index.html
			else if(strstr(request_line, "GET / ")) {
				spiffs_serve("/index.html", conn);
			}
			// other content, get it from SPIFFS
			else {		
				// get other requested resource (css, etc)
				
				//NOTE: strtok() is called here just to break string (arg1) into a series of tokens, using arg2 as delimiter
				//NOTE there is no need to get returned results, so 'char* method' is not used here
				//char* method = strtok(request_line, " "); //Needed to load files (css, etc)
				strtok(request_line, " "); //Needed to load other files (eg css, etc)
				
				char* resource = strtok(NULL, " ");
				spiffs_serve(resource, conn);
			}
		}
	}
	
	// close the connection and free the buffer
	netconn_close(conn);
	netbuf_delete(inbuf);
}


void http_server_task(void *pvParameters) {	
	struct netconn *conn, *newconn;
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80); //NULL = IP_ADDR_ANY
	netconn_listen(conn);
	ESP_LOGI(TAG, "HTTP Server listening...");
	
	do { //task must never end
		err = netconn_accept(conn, &newconn); //blocking 
		//ESP_LOGI(TAG, "New client connected");
		if (err == ERR_OK) {
			http_server_netconn_serve_RGB(newconn);					
			netconn_delete(newconn);
		}
		vTaskDelay(1); //allows task to be pre-empted
	} while(err == ERR_OK);
	ESP_LOGI(TAG, "close connection");
	netconn_close(conn);
	netconn_delete(conn);
}








