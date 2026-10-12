#include <stdio.h>
#include "esp_log.h" //the tools of the prints
#include "freertos/FreeRTOS.h" //the tools of the FreeRTOS (-to create and manage task)
#include "freertos/task.h"
#include "driver/i2c_master.h"


static const char *TAG = "pdm"; //the label in the print, every part has his label
 /**
  * I (310) wifi:   connected to network
  * I (315) sensor: read value 20.5
  * I (320) pdm:    sending to cloud...
  */

#define I2C_SDA 21 //Qwiic SDA 
#define I2C_SCL 22 //Qwiic SCL

void app_main(void){
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

   for(;;){ //read accel forever, every 500ms
      i2c_master_transmit_receive(dev_handle, &accel_reg, 1, data, 6, 1000); //burst read, read reg that attch to each other

      //combine High+Low byte 16-bit signed-> +- int16
      int16_t ax = (int16_t)((data[0] << 8) | data[1]);
      int16_t ay = (int16_t)((data[2] << 8) | data[3]);
      int16_t az = (int16_t)((data[4] << 8) | data[5]);

      ESP_LOGI(TAG, "ax=%d ay=%d az=%d", ax, ay, az);

      vTaskDelay(pdMS_TO_TICKS(500)); //read every half sec
   }


   
}


