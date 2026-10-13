#include <stdio.h>
#include "esp_log.h" //the tools of the prints
#include "freertos/FreeRTOS.h" //the tools of the FreeRTOS (-to create and manage task)
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"
#include "secrets.h"
#include "esp_netif_sntp.h"
#include "mqtt_client.h"
#include "esp_crt_bundle.h"
#include <math.h>


static const char *TAG = "pdm"; //the label in the print, every part has his label
 /**
  * I (310) wifi:   connected to network
  * I (315) sensor: read value 20.5
  * I (320) pdm:    sending to cloud...
  */

#define I2C_SDA 21 //Qwiic SDA 
#define I2C_SCL 22 //Qwiic SCL


static EventGroupHandle_t wifi_events;
#define WIFI_CONNECTED_BIT BIT0
static esp_mqtt_client_handle_t mqtt_client;

static void wifi_handler(void *args, esp_event_base_t base, int32_t id, void *data){
   if(base == WIFI_EVENT && id == WIFI_EVENT_STA_START){
      esp_wifi_connect();  //start to connect the wifi
   }
   else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
   {
      ESP_LOGW(TAG, "wifi disconnected, retrying...");
      esp_wifi_connect();  //wifi fall? try to reconnect
   }
   else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
   {
      ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
      ESP_LOGI(TAG, "got IP: "IPSTR, IP2STR(&e->ip_info.ip));
      xEventGroupSetBits(wifi_events, WIFI_CONNECTED_BIT); //mark as connect
   }
}

static void wifi_init_sta(void){
   esp_err_t ret = nvs_flash_init(); //nvs need WIFI (storage)
   if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
   {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
   }
   ESP_ERROR_CHECK(ret);

   ESP_ERROR_CHECK(esp_netif_init()); //network layer
   ESP_ERROR_CHECK(esp_event_loop_create_default()); //the event loop
   esp_netif_create_default_wifi_sta();

   wifi_events = xEventGroupCreate();
   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   ESP_ERROR_CHECK(esp_wifi_init(&cfg));

   esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler, NULL, NULL);
   esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_handler, NULL, NULL);

   wifi_config_t wc = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS} };
   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); //station mode (client)
   ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
   ESP_ERROR_CHECK(esp_wifi_start());

   ESP_LOGI(TAG, "connecting to WIFI...");
   xEventGroupWaitBits(wifi_events, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY); //wait to IP
}

static void sync_time(void){
   esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
   esp_netif_sntp_init(&config);
   if(esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK){
      ESP_LOGW(TAG, "time sync failed");
   }
   else
   {
      ESP_LOGI(TAG, "time synced");
   }
   
}

static void mqtt_event_handler(void *args, esp_event_base_t base, int32_t id, void *data){
   switch ((esp_mqtt_event_id_t)id)
   {
   case MQTT_EVENT_CONNECTED: ESP_LOGI(TAG, "MQTT connected to Azure!"); break;
   case MQTT_EVENT_DISCONNECTED: ESP_LOGW(TAG, "MQTT disconnected"); break;
   case MQTT_EVENT_ERROR:  ESP_LOGE(TAG, "MQTT error");  break;
   default: break;
   }
}

static void mqtt_init(void){
   static char uri[128], username[256];
   snprintf(uri, sizeof(uri), "mqtts://%s:8883", AZURE_HOST); //MQTT over TLS, port 8883
   snprintf(username, sizeof(username), "%s/%s/?api-version=2021-04-12", AZURE_HOST, AZURE_DEVICE_ID); //Azure username format: host/deviceId/?api-version

   esp_mqtt_client_config_t cfg = {
      .broker.address.uri = uri,
      .broker.verification.crt_bundle_attach = esp_crt_bundle_attach, //built-in root CA bundle - verfies Azure's cert
      .credentials.client_id = AZURE_DEVICE_ID,
      .credentials.username = username,
      .credentials.authentication.password = AZURE_SAS_TOKEN, //SAS = the password
   };
   mqtt_client = esp_mqtt_client_init(&cfg);
   esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
   esp_mqtt_client_start(mqtt_client);
}

void app_main(void){

      wifi_init_sta(); //first call to the WIFI
      sync_time();
      mqtt_init();

   //set the I2C master bus
   //send the start -> wait for ACK
   // if find the device(ACK), else-> NACK
   i2c_master_bus_config_t bus_config = {
      .i2c_port = I2C_NUM_0, //which peripheral has in I2C
      .sda_io_num = I2C_SDA,           //the pin of SDA
      .scl_io_num = I2C_SCL,           //the pin of SCL
      .clk_source = I2C_CLK_SRC_DEFAULT, //the clock source
      .glitch_ignore_cnt = 7,          //noise filtering
      .flags.enable_internal_pullup = true, //internal pull-up
   };
   i2c_master_bus_handle_t bus_handle;
   ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle)); //create the bus

   //who ans the bus
   ESP_LOGI(TAG, "Scanning I2C bus...");
   for (uint8_t addr = 0x08; addr < 0x78; addr++) //the range of adresses in I2C
   {
      if (i2c_master_probe(bus_handle, addr, 50) == ESP_OK)
      {
         ESP_LOGI(TAG, "Found device at 0x%02X", addr); //ACK-> found the device
      }
   }
   ESP_LOGI(TAG, "Scan done.");

   //attach the ICM-20948 to the bus(device handle)
   i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7, //address bit 7
      .device_address = 0x69,
      .scl_speed_hz = 100000,
   };
   i2c_master_dev_handle_t dev_handle;
   ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));

   //read the WHO_AM_I (write-then-read)
   uint8_t reg = 0x00;
   uint8_t who = 0;
   ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, &reg, 1, &who, 1, 1000));
   ESP_LOGI(TAG, "WHO_AM_I = 0x%02X (expected 0xEA)", who);

   //wake-up the ICM-20948 (clear SLEEP)
   uint8_t pwr_reg = 0x06; 
   uint8_t value = 0;

   ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, &pwr_reg, 1, &value, 1, 1000));
   ESP_LOGI(TAG, "PWR_MGMT = 0x%02X ", value); 

   value &= ~(1 << 6); //clear 6th bit (SLEEP) -> wakes him up

   uint8_t buf[2] = {pwr_reg, value}; //{reg addr, new value}
   ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, buf, 2, 1000)); //sends to the sensor
   ESP_LOGI(TAG, "PWR_MGMT = 0x%02X after", value); 

   uint8_t accel_reg = 0x2D; //start address (ACCEL_XOUT_H)
   uint8_t data[6]; //6 diff bytes into 1 arr ->X,Y,Z 
   int seq = 0;
   char topic[128];
   snprintf(topic, sizeof(topic), "devices/%s/messages/events/", AZURE_DEVICE_ID); //Azure D2C topic

   float peak = 0;   //we wanna catch the pick at a time
   int reads = 0; //nums of timr we make a read
   
   for(;;){ //read accel forever, every 200ms
      i2c_master_transmit_receive(dev_handle, &accel_reg, 1, data, 6, 1000); //burst read, read reg that attch to each other

      //combine High+Low byte 16-bit signed-> +- int16
      int16_t ax = (int16_t)((data[0] << 8) | data[1]);
      int16_t ay = (int16_t)((data[2] << 8) | data[3]);
      int16_t az = (int16_t)((data[4] << 8) | data[5]);

      float mag = sqrtf((float)ax*ax + (float)ay*ay + (float)az*az) / 16384.0f;

      if (mag > peak) //to follow the peak
      {
         peak = mag;
      }
      reads++; 

      if (reads >= 50) //calc the pick from a 50 reads in a window
      {
         char payload[64]; //build payload JSON
         snprintf(payload, sizeof(payload), "{\"seq\":%d,\"value\":%.3f}", seq++, peak); //sends the peak from the window
         esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0); //publish to Azure (API - plumbing)
         ESP_LOGI(TAG, "sent: %s", payload);
         peak = 0; //calc a new pick at the next window
         reads = 0;
      }
      
      ESP_LOGI(TAG, "ax=%d ay=%d az=%d", ax, ay, az);

      vTaskDelay(pdMS_TO_TICKS(200)); //read every 0.2 sec
   }


   
}


