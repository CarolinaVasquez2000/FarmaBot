// Librerías necesarias para la ESP32-CAM y servidor web
#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "camera_pins.h"

// -------------------------
// CONFIGURACIÓN WIFI
// -------------------------

// Nombre de la red WiFi (SSID) y contraseña
const char* ssid = "MOVISTAR WIFI2000";
const char* password = "87654321";

// -------------------------
// DEFINICIÓN DE PINES
// -------------------------

#define ARRANQUE 14   // Pin que indica inicio de búsqueda
#define TORRE_PIN 12  // Pin que indica detección de QR
#define ALTURA 13     // Pin que indica altura baja/alta
#define FLASH 4       // Pin del flash (LED)

// -------------------------
// SERVIDOR WEB
// -------------------------

WebServer server(80);  // Servidor HTTP en puerto 80

// -------------------------
// MANEJO DE PINS POR HTTP
// -------------------------

// Controla el pin ARRANQUE (14)
void control_pin14() {
  String state = server.arg("state");  // Lee parámetro ?state=1 o ?state=0

  if (state == "1") {
    digitalWrite(ARRANQUE, HIGH);
    server.send(200, "text/plain", "Arranque de busqueda");
  } else if (state == "0") {
    digitalWrite(ARRANQUE, LOW);
    server.send(200, "text/plain", "Deshabilitación de arranque");
  } else {
    server.send(400, "text/plain", "Parámetro 'state' no válido");
  }
}

// Controla el pin TORRE (12)
void control_pin12() {
  String state = server.arg("state");

  if (state == "1") {
    digitalWrite(TORRE_PIN, HIGH);
    server.send(200, "text/plain", "Pin 12 encendido - QR detectado");
  } else if (state == "0") {
    digitalWrite(TORRE_PIN, LOW);
    server.send(200, "text/plain", "Pin 12 apagado - QR no detectado");
  } else {
    server.send(400, "text/plain", "Parámetro 'state' no válido");
  }
}

// Controla el pin ALTURA (13)
void control_pin13() {
  String state = server.arg("state");

  if (state == "1") {
    digitalWrite(ALTURA, HIGH);
    server.send(200, "text/plain", "Pin 13 encendido - Altura baja");
  } else if (state == "0") {
    digitalWrite(ALTURA, LOW);
    server.send(200, "text/plain", "Pin 13 apagado - Altura alta");
  } else {
    server.send(400, "text/plain", "Parámetro 'state' no válido");
  }
}

// -------------------------
// CAPTURA Y ENVÍO DE FOTO JPG
// -------------------------

// Captura una imagen JPG de la cámara y la envía por HTTP
void handleJPGStream() {
  WiFiClient client = server.client();
  if (!client) return;

  camera_fb_t *fb = esp_camera_fb_get();  // Captura un frame
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Responde con la imagen en formato JPEG
  server.sendHeader("Content-Type", "image/jpeg");
  server.sendHeader("Content-Length", String(fb->len));
  server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);  // Libera el buffer
}

// -------------------------
// SETUP: CONFIGURACIÓN INICIAL
// -------------------------

void setup() {
  Serial.begin(115200);  // Inicia comunicación serial para debug

  // Configura los pines de salida y los apaga/inicializa
  pinMode(ARRANQUE, OUTPUT);   digitalWrite(ARRANQUE, LOW);
  pinMode(TORRE_PIN, OUTPUT);  digitalWrite(TORRE_PIN, LOW);
  pinMode(ALTURA, OUTPUT);     digitalWrite(ALTURA, LOW);
  pinMode(FLASH, OUTPUT);      digitalWrite(FLASH, HIGH);  // Mantiene flash encendido

  // -------------------------
  // CONFIGURACIÓN DE LA CÁMARA
  // -------------------------

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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;      // Frecuencia del reloj
  config.frame_size = FRAMESIZE_QVGA;  // Resolución QVGA
  config.pixel_format = PIXFORMAT_JPEG;  // Formato JPEG
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;  // Framebuffer en PSRAM
  config.jpeg_quality = 12;   // Calidad JPEG (más bajo, mejor calidad)
  config.fb_count = 1;        // Número de framebuffers

  // Inicializa la cámara con la configuración
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Error al inicializar la cámara");
    return;
  }

  // -------------------------
  // CONEXIÓN WIFI
  // -------------------------

  WiFi.begin(ssid, password);  // Intenta conectar

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Conectado a WiFi");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  // -------------------------
  // ENDPOINTS HTTP
  // -------------------------

  server.on("/jpg", HTTP_GET, handleJPGStream);   // Captura foto
  server.on("/arranque", HTTP_GET, control_pin14);  // Control pin ARRANQUE
  server.on("/torre", HTTP_GET, control_pin12);     // Control pin TORRE
  server.on("/baja", HTTP_GET, control_pin13);      // Control pin ALTURA

  server.begin();  // Inicia servidor web
}

// -------------------------
// LOOP PRINCIPAL
// -------------------------

void loop() {
  server.handleClient();  // Atiende las peticiones web
}
