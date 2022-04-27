#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include <esp_netif.h>
#include <esp_wifi.h>
#include "wifi_manager.h"

// #include "protocol_examples_common.h"
// #include "addr_from_stdin.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"

#include "ssd1306.h"
#include "font8x8_basic.h"


#include "cJSON.h"

/*
 You have to set this config value with menuconfig
 CONFIG_INTERFACE

 for i2c
 CONFIG_MODEL
 CONFIG_SDA_GPIO
 CONFIG_SCL_GPIO
 CONFIG_RESET_GPIO

 for SPI
 CONFIG_CS_GPIO
 CONFIG_DC_GPIO
 CONFIG_RESET_GPIO
*/

#define tag "SSD1306"
static const char TAG[] = "main";


//HTTP配置参数
static const char *HTTP_TAG = "httpTask";
#define MAX_HTTP_OUTPUT_BUFFER 2048
#define HOST "api.seniverse.com"
#define UserKey "SEsXJUQ-6J3l4H3Fc"
#define Location "suzhou"
#define Language "zh-Hans"
#define Strat "0"
#define Days "5"


/**
 * @brief RTOS task that periodically prints the heap memory available.
 * @note Pure debug information, should not be ever started on production code! This is an example on how you can integrate your code with wifi-manager
 */
void monitoring_task(void *pvParameter)
{
	for (;;)
	{
		ESP_LOGI(TAG, "free heap: %d", esp_get_free_heap_size());
		vTaskDelay(pdMS_TO_TICKS(10000));
	}
}

/**
 * @brief this is an exemple of a callback that you can setup in your own app to get notified of wifi manager event.
 */
void cb_connection_ok(void *pvParameter)
{
	ip_event_got_ip_t *param = (ip_event_got_ip_t *)pvParameter;

	/* transform IP to human readable string */
	char str_ip[16];
	esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

	ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);
}

/** HTTP functions **/
static void http_client_task(void *pvParameters)
{
	char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0}; // Buffer to store response of http request
	int content_length = 0;
	
	esp_http_client_config_t config = {
		// .url = "http://api.seniverse.com/v3/weather/daily.json?key=SEsXJUQ-6J3l4H3Fc&location=suzhou&language=zh-Hans&unit=c&start=0&days=5",
		.url = "https://api.seniverse.com/v3/weather/daily.json?key=SiJ6Yd1LFtd3-gHzr&location=suzhou&language=zh-Hans&unit=c&start=0&days=3",
		};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET Request
	esp_http_client_set_method(client, HTTP_METHOD_GET); //示例代码无此行;
	esp_err_t err = esp_http_client_open(client, 0);
	content_length = esp_http_client_fetch_headers(client);
	if (content_length < 0)
	{
		ESP_LOGE(HTTP_TAG, "HTTP client fetch headers failed");
		}
		else
		{
			int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
			if (data_read >= 0)
			{
				ESP_LOGI(HTTP_TAG, "HTTP GET Status = %d, content_length = %d",
						 esp_http_client_get_status_code(client),
						 esp_http_client_get_content_length(client));
				printf("data:%s", output_buffer);
				// json_request_parser(output_buffer);
			}
			else
			{
				ESP_LOGE(HTTP_TAG, "Failed to read response");
			}
		}
	esp_http_client_close(client);
	vTaskDelete(NULL);
}

void json_request_parser(char *json_data)
{
	// uint8_t
	cJSON *root = NULL;

	printf("Version: %s\n", cJSON_Version()); //打印版本号;
	root = cJSON_Parse(json_data);			  // json_data 为心知天气的原始数据;
	if (!root)
	{
		printf("Error before: [%s]\n", cJSON_GetErrorPtr());
		return -1;
	}
	printf("%s\n\n", cJSON_Print(root)); /*将完整的数据以JSON格式打印出来*/
	

	uint8_t json[2048] = {0};
	cJSON *root = cJSON_CreateObject();
	// cJSON *sensors = cJSON_CreateArray();
	// cJSON *id1 = cJSON_CreateObject();
	// cJSON *id2 = cJSON_CreateObject();
	// cJSON *iNumber = cJSON_CreateNumber(10);

	// cJSON_AddItemToObject(id1, "id", cJSON_CreateString("1"));
	// cJSON_AddItemToObject(id1, "temperature1", cJSON_CreateString("23"));
	// cJSON_AddItemToObject(id1, "temperature2", cJSON_CreateString("23"));
	// cJSON_AddItemToObject(id1, "humidity", cJSON_CreateString("55"));
	// cJSON_AddItemToObject(id1, "occupancy", cJSON_CreateString("1"));
	// cJSON_AddItemToObject(id1, "illumination", cJSON_CreateString("23"));

	// cJSON_AddItemToObject(id2, "id", cJSON_CreateString("2"));
	// cJSON_AddItemToObject(id2, "temperature1", cJSON_CreateString("23"));
	// cJSON_AddItemToObject(id2, "temperature2", cJSON_CreateString("23"));
	// cJSON_AddItemToObject(id2, "humidity", cJSON_CreateString("55"));
	// cJSON_AddItemToObject(id2, "occupancy", cJSON_CreateString("1"));
	// cJSON_AddItemToObject(id2, "illumination", cJSON_CreateString("23"));

	// cJSON_AddItemToObject(id2, "value", iNumber);

	// cJSON_AddItemToArray(sensors, id1);
	// cJSON_AddItemToArray(sensors, id2);

	// cJSON_AddItemToObject(root, "sensors", sensors);
	// char *str = cJSON_Print(root);

	// uint32_t jslen = strlen(str);
	// memcpy(json, str, jslen);
	// printf("%s\n", json);

	// cJSON_Delete(root);
	// free(str);
	// str = NULL;
	
}

void app_main(void)
{
	SSD1306_t dev;
	uint8_t center, top, bottom;
	char lineChar[20];
	/* start the wifi manager */
	wifi_manager_start();

	/* register a callback as an example to how you can integrate your code with the wifi manager */
	wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);

	/* your code should go here. Here we simply create a task on core 2 that monitors free heap memory */
	xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1);
	printf("正常进入0\n");
	vTaskDelay(1000);
	printf("正常进入1\n");
	xTaskCreate(http_client_task, "http_client", 5120, NULL, 3, NULL);
	vTaskDelay(1000);
	/************************************************
#if CONFIG_I2C_INTERFACE
	ESP_LOGI(tag, "INTERFACE is i2c");
	ESP_LOGI(tag, "CONFIG_SDA_GPIO=%d", CONFIG_SDA_GPIO);
	ESP_LOGI(tag, "CONFIG_SCL_GPIO=%d", CONFIG_SCL_GPIO);
	ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d", CONFIG_RESET_GPIO);
	i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
#endif // CONFIG_I2C_INTERFACE

#if CONFIG_SPI_INTERFACE
	ESP_LOGI(tag, "INTERFACE is SPI");
	ESP_LOGI(tag, "CONFIG_MOSI_GPIO=%d", CONFIG_MOSI_GPIO);
	ESP_LOGI(tag, "CONFIG_SCLK_GPIO=%d", CONFIG_SCLK_GPIO);
	ESP_LOGI(tag, "CONFIG_CS_GPIO=%d", CONFIG_CS_GPIO);
	ESP_LOGI(tag, "CONFIG_DC_GPIO=%d", CONFIG_DC_GPIO);
	ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d", CONFIG_RESET_GPIO);
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO);
#endif // CONFIG_SPI_INTERFACE

#if CONFIG_FLIP
	dev._flip = true;
	ESP_LOGW(tag, "Flip upside down");
#endif

#if CONFIG_SSD1306_128x64
	ESP_LOGI(tag, "Panel is 128x64");
	ssd1306_init(&dev, 128, 64);
#endif // CONFIG_SSD1306_128x64
#if CONFIG_SSD1306_128x32
	ESP_LOGI(tag, "Panel is 128x32");
	ssd1306_init(&dev, 128, 32);
#endif // CONFIG_SSD1306_128x32

	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);

#if CONFIG_SSD1306_128x64
	top = 2;
	center = 3;
	bottom = 8;
	ssd1306_display_text(&dev, 0, "SSD1306 128x64", 14, false);
	ssd1306_display_text(&dev, 1, "ABCDEFGHIJKLMNOP", 16, false);
	ssd1306_display_text(&dev, 2, "abcdefghijklmnop", 16, false);
	ssd1306_display_text(&dev, 3, "Hello World!!", 13, false);
	ssd1306_clear_line(&dev, 4, true);
	ssd1306_clear_line(&dev, 5, true);
	ssd1306_clear_line(&dev, 6, true);
	ssd1306_clear_line(&dev, 7, true);
	ssd1306_display_text(&dev, 4, "SSD1306 128x64", 14, true);
	ssd1306_display_text(&dev, 5, "ABCDEFGHIJKLMNOP", 16, true);
	ssd1306_display_text(&dev, 6, "abcdefghijklmnop", 16, true);
	ssd1306_display_text(&dev, 7, "Hello World!!", 13, true);
#endif // CONFIG_SSD1306_128x64

#if CONFIG_SSD1306_128x32
	top = 1;
	center = 1;
	bottom = 4;
	ssd1306_display_text(&dev, 0, "SSD1306 128x32", 14, false);
	ssd1306_display_text(&dev, 1, "Hello World!!", 13, false);
	ssd1306_clear_line(&dev, 2, true);
	ssd1306_clear_line(&dev, 3, true);
	ssd1306_display_text(&dev, 2, "SSD1306 128x32", 14, true);
	ssd1306_display_text(&dev, 3, "Hello World!!", 13, true);
#endif // CONFIG_SSD1306_128x32
	vTaskDelay(3000 / portTICK_PERIOD_MS);

	// Display Count Down
	uint8_t image[24];
	memset(image, 0, sizeof(image));
	ssd1306_display_image(&dev, top, (6 * 8 - 1), image, sizeof(image));
	ssd1306_display_image(&dev, top + 1, (6 * 8 - 1), image, sizeof(image));
	ssd1306_display_image(&dev, top + 2, (6 * 8 - 1), image, sizeof(image));
	for (int font = 0x39; font > 0x30; font--)
	{
		memset(image, 0, sizeof(image));
		ssd1306_display_image(&dev, top + 1, (7 * 8 - 1), image, 8);
		memcpy(image, font8x8_basic_tr[font], 8);
		if (dev._flip)
			ssd1306_flip(image, 8);
		ssd1306_display_image(&dev, top + 1, (7 * 8 - 1), image, 8);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	// Scroll Up
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, 0, "---Scroll  UP---", 16, true);
	// ssd1306_software_scroll(&dev, 7, 1);
	ssd1306_software_scroll(&dev, (dev._pages - 1), 1);
	for (int line = 0; line < bottom + 10; line++)
	{
		lineChar[0] = 0x01;
		sprintf(&lineChar[1], " Line %02d", line);
		ssd1306_scroll_text(&dev, lineChar, strlen(lineChar), false);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
	vTaskDelay(3000 / portTICK_PERIOD_MS);

	// Scroll Down
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, 0, "--Scroll  DOWN--", 16, true);
	// ssd1306_software_scroll(&dev, 1, 7);
	ssd1306_software_scroll(&dev, 1, (dev._pages - 1));
	for (int line = 0; line < bottom + 10; line++)
	{
		lineChar[0] = 0x02;
		sprintf(&lineChar[1], " Line %02d", line);
		ssd1306_scroll_text(&dev, lineChar, strlen(lineChar), false);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
	vTaskDelay(3000 / portTICK_PERIOD_MS);

	// Page Down
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, 0, "---Page	DOWN---", 16, true);
	ssd1306_software_scroll(&dev, 1, (dev._pages - 1));
	for (int line = 0; line < bottom + 10; line++)
	{
		// if ( (line % 7) == 0) ssd1306_scroll_clear(&dev);
		if ((line % (dev._pages - 1)) == 0)
			ssd1306_scroll_clear(&dev);
		lineChar[0] = 0x02;
		sprintf(&lineChar[1], " Line %02d", line);
		ssd1306_scroll_text(&dev, lineChar, strlen(lineChar), false);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
	vTaskDelay(3000 / portTICK_PERIOD_MS);

	// Horizontal Scroll
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, center, "Horizontal", 10, false);
	ssd1306_hardware_scroll(&dev, SCROLL_RIGHT);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_hardware_scroll(&dev, SCROLL_LEFT);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_hardware_scroll(&dev, SCROLL_STOP);

	// Vertical Scroll
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, center, "Vertical", 8, false);
	ssd1306_hardware_scroll(&dev, SCROLL_DOWN);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_hardware_scroll(&dev, SCROLL_UP);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
	ssd1306_hardware_scroll(&dev, SCROLL_STOP);

	// Invert
	ssd1306_clear_screen(&dev, true);
	ssd1306_contrast(&dev, 0xff);
	ssd1306_display_text(&dev, center, "  Good Bye!!", 12, true);
	vTaskDelay(5000 / portTICK_PERIOD_MS);

	// Fade Out
	ssd1306_fadeout(&dev);
************************************************/

#if 0
	// Fade Out
	for(int contrast=0xff;contrast>0;contrast=contrast-0x20) {
		ssd1306_contrast(&dev, contrast);
		vTaskDelay(40);
	}
#endif

	// Restart module
	// esp_restart();
}
