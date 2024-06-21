#include "stm32f10x.h"
#include "esp8266_driver.h"
#include "esp8266_command_func.h"
#include "OLED.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

static ESP_VERSION_INFO_t Esp8266_version_infomation;
static ESP_STATION_INFO_t Esp8266_station_infomation;

static ESP_WIFI_STATE_t   Esp8266_Wifi_state;
static ESP_TCP_IP_STATE_t Esp8266_TCPIP_state;


static uint8_t string_startwith(const char* str, const char* str_start);
static uint8_t string_startmatch(const char* str, const char* str_start, char** str_output, uint16_t length);
static uint8_t string_join_command(char** str_dest, const char* start_str,...);


/// @brief if string is start with str_start, return 0, or return 1
/// @param str string to match 
/// @param str_start the string should start with this str.
/// @return 0: succeed;  1: failed
static uint8_t string_startwith(const char* str, const char* str_start){
    return  strncmp(str, str_start, strlen(str_start)) == 0 ? 0: 1;
}
/// @brief match the start of string, and from just after match place, colloect string of specified length 
/// @param str        string to match 
/// @param str_start  the string should start with this str.
/// @param str_output the address of the output string, it collect the parameter after the matched str_start
/// @param length     length to copy(if too large, will copy until string end)
/// @return 0: match string succeed and result is written into str_output. 
///         1: match failed
/// @warning str_output would be automatically freed in this function so init input as NULL
/// @warning free the str_output must  after use it.
static uint8_t string_startmatch(const char* str, const char* str_start, char** str_output, uint16_t length){
    if (*str_output != NULL) {
        free(*str_output);
        *str_output = NULL;
    }
    uint16_t s = strlen(str_start);
    if (s + length > strlen(str)) length = strlen(str) - s;  // for length too large 
    if (strncmp(str, str_start, s) != 0) {
        // failed to compare the start string:
        return 1;
    }
    (*str_output) = malloc((length + 1) * sizeof(char));
    uint8_t l = strlen(str);    
    strncpy((*str_output), str + s, length);
    return 0;
}

/// @brief concatnate each parameter of the command to the start command, 
///        default deliminator is ","; 
///        "\\r\\n" will be automatically add to the str.
/// @param str_dest  address of the destination string 
/// @param start_str start 
/// @param other parameters should all be of type char* to concatenate command. 
/// @warning init str_dest as NULL, and it must be freed after usage. 
/// @return 0: make command succeed 
///         1: command exceed ESP8266_TX_BUFFER_SIZE and str_dest would be NULL
static uint8_t string_join_command(char** str_dest, const char* start_str,...){
    if (str_dest!=NULL){
        free((*str_dest));
        (*str_dest) = NULL;
    }
    (*str_dest) = malloc(ESP8266_TX_BUFFER_SIZE * sizeof(char));
    strcpy((*str_dest), start_str);

    va_list arg;
    va_start (arg, start_str);  // begin param
    const char* str;            // use str for receive 
    
    uint16_t index = 0;
    while (1){
        str = va_arg(arg, const char*);   // receive the string
        if (str == NULL) break;
        if (strlen((*str_dest)) + strlen(str) + 1 > ESP8266_TX_BUFFER_SIZE){ 
            free((*str_dest)); (*str_dest) = NULL;
            va_end(arg);   // release the storge of parameter 
            return 1;
        }
        if (index!=0){
            (*str_dest) = strcat((*str_dest),",");
        }
        index ++;
        (*str_dest) = strcat((*str_dest), str);
    }
    va_end(arg);
    if (strlen((*str_dest)) + 2 > ESP8266_TX_BUFFER_SIZE) {
        free((*str_dest)); (*str_dest) = NULL;
        return 1;
    }
    (*str_dest) = strcat((*str_dest),"\r\n");
    return 0;
}

// -------- add self-defined esp8266 Command function here -----------------

ESP_Error_t esp8266_cmd_wifi_set_softAPconfig(){
    return ESP_RES_OK;
}

/// ESP_Error_t esp8266_cmd_tcp_cipsend 


/****** Command Call back function, which would be seprated later ******/
int main(){
	Delay_ms(1000);  // wait 1s for uart to start 
    OLED_Init();     
    // ESP_Error_t res1 = esp8266_sendcmd("AT\r\n", "OK", NULL);     // esp8266_Init();
    if (esp8266_Init() == ESP_RES_OK){
        OLED_ShowString(1,1, "Init Suc");
    } 
    else {
        OLED_ShowString(1,1, "Init Failed");
    }
    esp8266_cmd_wifi_set_mode(3);
    esp8266_cmd_tcp_set_cipmux(1);
    if (!esp8266_cmd_tcp_set_tcpServer(1, 8080)){
        OLED_ShowString(1,1,"listen at 8080");
    }
    
    // note : the length may be strlen - 1
    if (!esp8266_cmd_tcp_cipsend("0",12, NULL, NULL, "hello,world!")){
        OLED_ShowString(3,1,"string send suc");
         //  OLED_ShowNum(4,1,strlen("hello,world!"), 3);  strlen is 12 
    }else{
        OLED_ShowString(3,1,"str send fail");
    }

    while (1)
    {
        esp8266_waitmsg();  // this can be called in main loop    
    }
}
