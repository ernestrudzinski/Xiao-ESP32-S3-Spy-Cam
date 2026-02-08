#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// =========================
// USER SETTINGS
// =========================
const char* ssid     = "Necik";
const char* password = "pokolenie1978";

// Camera settings
static const framesize_t CAMERA_FRAMESIZE = FRAMESIZE_QVGA; // 320x240
static const int CAMERA_JPEG_QUALITY      = 15;             // lower = better quality/larger file (10-20 typical)
static const int CAMERA_FB_COUNT          = 2;

// =========================
// XIAO ESP32-S3 Sense camera pins (as you provided)
// =========================
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

// =========================
// Globals
// =========================
static httpd_handle_t httpd = NULL;

// =========================
// Web UI (HTML)
// =========================
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>XIAO ESP32-S3 Security Cam</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; margin:0; background:#111; color:#eee; text-align:center; }
    .wrap { padding:16px; max-width: 980px; margin: 0 auto; }
    h1 { margin: 12px 0 6px; font-size: 22px; }
    .card { background:#1b1b1b; border:1px solid #2a2a2a; border-radius:12px; padding:12px; margin:12px 0; }
    img { width:100%; max-width: 960px; border-radius:12px; border:1px solid #2a2a2a; }
    button {
      background:#2e7d32; color:#fff; border:0; border-radius:10px;
      padding:12px 14px; margin:6px; font-size:15px; cursor:pointer;
    }
    button.warn { background:#ef6c00; }
    button.stop { background:#c62828; }
    a { color:#90caf9; text-decoration:none; }
    .row { display:flex; flex-wrap:wrap; gap:8px; justify-content:center; }
    .status { font-size:14px; padding:10px; background:#0f0f0f; border-radius:10px; border:1px solid #2a2a2a; }
    code { color:#8bc34a; }
  </style>
</head>
<body>
  <div class="wrap">
    <h1>Security Camera</h1>

    <div class="card">
      <div style="margin-top:10px;">
        <img id="stream" src="/stream" alt="Live Stream">
      </div>
      <div style="margin-top:10px; font-size:13px;">
        Stream URL: <code>/stream</code>
      </div>
    </div>
  </div>

<script>
// Stream-only UI
</script>
</body>
</html>
)rawliteral";

// =========================
// HTTP handlers
// =========================
static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, INDEX_HTML, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t stream_handler(httpd_req_t *req) {
  static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
  static const char* _STREAM_BOUNDARY = "\r\n--frame\r\n";
  static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

  httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);

  char partBuf[64];

  while (true) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return ESP_FAIL;
    }

    size_t hlen = (size_t)snprintf(partBuf, sizeof(partBuf), _STREAM_PART, (unsigned)fb->len);

    if (httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY)) != ESP_OK) {
      esp_camera_fb_return(fb);
      return ESP_FAIL;
    }
    if (httpd_resp_send_chunk(req, partBuf, hlen) != ESP_OK) {
      esp_camera_fb_return(fb);
      return ESP_FAIL;
    }
    if (httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len) != ESP_OK) {
      esp_camera_fb_return(fb);
      return ESP_FAIL;
    }

    esp_camera_fb_return(fb);

    // Small delay to avoid starving other tasks
    vTaskDelay(1);
  }

  return ESP_OK;
}

static void startWebServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.ctrl_port = 32768;

  if (httpd_start(&httpd, &config) != ESP_OK) {
    Serial.println("ERROR: httpd_start failed");
    return;
  }

  httpd_uri_t uri_index = { .uri="/", .method=HTTP_GET, .handler=index_handler, .user_ctx=NULL };
  httpd_uri_t uri_stream = { .uri="/stream", .method=HTTP_GET, .handler=stream_handler, .user_ctx=NULL };

  httpd_register_uri_handler(httpd, &uri_index);
  httpd_register_uri_handler(httpd, &uri_stream);

  Serial.println("Web server started on port 80");
}

// =========================
// Camera init
// =========================
static bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;

  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size   = CAMERA_FRAMESIZE;
  config.jpeg_quality = CAMERA_JPEG_QUALITY;
  config.fb_count     = CAMERA_FB_COUNT;
  config.fb_location  = CAMERA_FB_IN_PSRAM;
  config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("ERROR: esp_camera_init failed: 0x%x\n", err);
    return false;
  }
  return true;
}

// =========================
// Arduino setup/loop
// =========================
void setup() {
  Serial.begin(115200);
  delay(250);

  Serial.println();
  Serial.println("XIAO ESP32-S3 Sense Security Cam Starting...");

  // Camera init
  if (!initCamera()) {
    Serial.println("ERROR: Camera init failed.");
    while (true) delay(1000);
  }
  Serial.println("Camera initialized.");

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());

  // Start web server
  startWebServer();

  Serial.println("Open:");
  Serial.print("  UI:     http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  Serial.print("  Stream: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/stream");
  Serial.println("  Stream-only mode (no SD or recording).");
}

void loop() {
  delay(50);
}
