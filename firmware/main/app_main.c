 #include "esp_log.h" //the tools of the prints
#include "freertos/FreeRTOS.h" //the tools of the FreeRTOS (-to create and manage task)
#include "freertos/task.h"
#include "freertos/queue.h"


static const char *TAG = "pdm"; //the label in the print, every part has his label
 /**
  * I (310) wifi:   connected to network
  * I (315) sensor: read value 20.5
  * I (320) pdm:    sending to cloud...
  */

static QueueHandle_t counter_queue; //the handle for the queue
 




static void counter_task(void *args){ //the task function - counts up
   
   int count = 0;
   for(;;){ //forever loop, the task runs forever
      count++;
      vTaskDelay(pdMS_TO_TICKS(1000)); //sleep

      xQueueSend(counter_queue, &count, portMAX_DELAY); //sends the count value
   }
}

static void print_task(void *args){
   for(;;){ //forever loop, the task runs forever

      int received; //the variable that will hold the received value
      xQueueReceive(counter_queue, &received, portMAX_DELAY); //gets the value to received (got from xQueueSend)

      ESP_LOGI(TAG, "count = %d", received); //printing the num

   }
}

void app_main(void){

   ESP_LOGI(TAG, "starting..."); //print, the I is Info

   counter_queue = xQueueCreate(10, sizeof(int)); //creates the queue

    /*
    1. which func to run
    2. name for debug
    3. how many stack the task gets
    4. which parameter to send
    5. priority
    6. handle the task
    */
    xTaskCreate(counter_task, "counter", 4096, NULL, 5, NULL); //create and starts the task of counter
    xTaskCreate(print_task, "print", 4096, NULL, 5, NULL); //create and starts the task of print

 }


