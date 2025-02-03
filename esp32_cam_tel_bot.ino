#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "esp_camera.h"

// Данные Wi-Fi
const char* ssid = "MY_WI-FI";
const char* password = "MY_PASSWORD_WI-FI";

// Телеграм-бот токен
const char* botToken = "67237543:AAHa94NTJv7P3Knles3ORiVeypWBgDer541s";

// ID чата Telegram
const String chatId = "1222140543";

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

// Определение GPIO для камеры TY-OV2
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27

#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM    5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

void sendPhoto() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  Serial.println("Uploading photo...");
  
  String url = "https://api.telegram.org/bot" + String(botToken) + "/sendPhoto";
  WiFiClientSecure client;
  client.setInsecure();

  if (client.connect("api.telegram.org", 443)) {
    String boundary = "----ESP32Boundary";
    String header = "--" + boundary + "\r\n" +
                    "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + chatId + "\r\n" +
                    "--" + boundary + "\r\n" +
                    "Content-Disposition: form-data; name=\"photo\"; filename=\"photo.jpg\"\r\n" +
                    "Content-Type: image/jpeg\r\n\r\n";

    String footer = "\r\n--" + boundary + "--\r\n";

    client.print("POST " + url + " HTTP/1.1\r\n");
    client.print("Host: api.telegram.org\r\n");
    client.print("Content-Type: multipart/form-data; boundary=" + boundary + "\r\n");
    client.print("Content-Length: " + String(header.length() + fb->len + footer.length()) + "\r\n");
    client.print("Connection: close\r\n\r\n");

    client.print(header);
    client.write(fb->buf, fb->len);
    client.print(footer);
    
    Serial.println("Photo sent!");
  } else {
    Serial.println("Connection to Telegram failed");
  }
  
  esp_camera_fb_return(fb);
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    if (chat_id != chatId) continue;
    
    if (text == "/photo") {
      sendPhoto();
    }
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected!");
  
  client.setInsecure();
  startCamera();
}

void loop() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  if (numNewMessages) {
    handleNewMessages(numNewMessages);
  }
  delay(1000);
}