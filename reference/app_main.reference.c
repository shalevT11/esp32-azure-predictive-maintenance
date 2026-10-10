/*
 * Predictive Maintenance — ESP32 firmware skeleton
 * ------------------------------------------------
 * Two FreeRTOS tasks joined by a queue:
 *   sensor_read_task  — periodically reads the sensor, pushes a sample to the queue
 *   cloud_tx_task     — drains the queue, publishes each sample to Azure IoT Hub
 *
 * This skeleton COMPILES AND RUNS with DUMMY data so you have a working base.
 * The two cores that matter in interviews are left for YOU to implement — see the
 * `TODO (YOU)` markers. Do not let a generator write these; that bit-level and
 * memory-management code is exactly what gets probed.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "pdm";

/* One telemetry sample. Extend with the fields your sensor actually provides. */
typedef struct {
    uint32_t seq;        /* monotonic sample counter            */
    int64_t  ts_ms;      /* timestamp (ms since boot for now)   */
    float    value;      /* e.g. vibration magnitude / temp C   */
} sample_t;

static QueueHandle_t s_sample_queue;

#define SENSOR_PERIOD_MS   1000
#define QUEUE_DEPTH        16

/* ------------------------------------------------------------------ */
/* Task 1: read the sensor                                            */
/* ------------------------------------------------------------------ */
static void sensor_read_task(void *arg)
{
    uint32_t seq = 0;
    TickType_t last = xTaskGetTickCount();

    /* TODO (YOU): initialise I2C and configure the sensor at register level.
     * - i2c_param_config() / i2c_driver_install()
     * - write the sensor's config registers (read the datasheet; use bit masks)
     * For now we emit a dummy ramp so the pipeline runs end-to-end. */

    for (;;) {
        sample_t s = {
            .seq   = seq++,
            .ts_ms = esp_log_timestamp(),
            /* TODO (YOU): replace with a real register read + conversion.
             * e.g. read ACCEL_XOUT_H/L from an MPU6050 over I2C and compute magnitude. */
            .value = 20.0f + (float)(seq % 10),
        };

        if (xQueueSend(s_sample_queue, &s, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "queue full, dropped sample %u", (unsigned)s.seq);
        }
        vTaskDelayUntil(&last, pdMS_TO_TICKS(SENSOR_PERIOD_MS));
    }
}

/* ------------------------------------------------------------------ */
/* Task 2: publish to the cloud                                       */
/* ------------------------------------------------------------------ */
static void cloud_tx_task(void *arg)
{
    /* TODO (YOU): bring up WiFi, then the Azure connection.
     * - connect WiFi (esp_wifi / example_connect)
     * - provision via DPS, then connect to IoT Hub
     * - init azure-iot-middleware-freertos: it does NO dynamic allocation, so
     *   you own the buffers; implement TLS setup and retry/reconnect yourself. */

    sample_t s;
    for (;;) {
        if (xQueueReceive(s_sample_queue, &s, portMAX_DELAY) == pdTRUE) {
            /* For now: just log. Replace with an MQTT publish. */
            ESP_LOGI(TAG, "sample seq=%u ts=%lld value=%.2f",
                     (unsigned)s.seq, s.ts_ms, s.value);

            /* TODO (YOU): build the JSON payload and publish over MQTT/TLS,
             * with retry + exponential backoff on failure. */
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "predictive-maintenance firmware starting");

    s_sample_queue = xQueueCreate(QUEUE_DEPTH, sizeof(sample_t));
    configASSERT(s_sample_queue != NULL);

    xTaskCreate(sensor_read_task, "sensor_read", 4096, NULL, 5, NULL);
    xTaskCreate(cloud_tx_task,    "cloud_tx",    8192, NULL, 4, NULL);
}
