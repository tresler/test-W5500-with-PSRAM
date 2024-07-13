//LED strip 
#define NUM_LEDS 300
#define NUM_STRIPS 16
byte brightness = 255;
bool newData = false;
unsigned long lastReload = millis();
unsigned long nowReload = millis();
CRGB leds[NUM_STRIPS][NUM_LEDS];
//constexpr const int led_out[16] = {18, 17, 16, 15, 7, 6, 5, 4, 41, 40, 39, 38, 21, 47, 48, 45};

#ifdef fastled
constexpr const int led_out[16] = {45, 48, 47, 42, 38, 39, 40, 41, 4, 5, 6, 7, 15, 16, 17, 18};
#endif
#ifdef ledriver
#include <I2SClockLessLedDriveresp32s3.h>
int led_out[16] = {45, 48, 47, 42, 38, 39, 40, 41, 4, 5, 6, 7, 15, 16, 17, 18};
I2SClocklessLedDriveresp32S3 driver;
#endif

//SD karta - čtečka
#define S_CS 21
uint8_t cardType;
uint64_t cardSize;

//sACN
int d_adr = 200;
int d_unv = 7;
int d_unv_count = 2;
const int d_adrs = 30;
byte dmxData[d_adrs];
#ifdef asyncACN
ESPAsyncE131 e131(d_unv_count);
e131_packet_t packet;
#else
WiFiUDP sacn;
Receiver recv(sacn, 7);
#endif
Preferences preferences;

//WiFi
IPAddress WIFI_IP_ADDR(192,168,0,1);
IPAddress AP_GATEWAY_IP(192,168,0,1);
IPAddress AP_NETWORK_MASK(255,255,255,0);
SPIClass * fspi = NULL;
AsyncWebServer webserver(80);
DNSServer dnsServer;
const char* ssid_ap = "ESP32Test";
const char* password_ap = "passwordTest";

// Ethernet nastaveni
uint8_t mac[] = {0x92, 0x10, 0x19, 0x38, 0x06, 0x03};
#define E_MOSI 11
#define E_CLK 12
#define ETH_PHY_TYPE     ETH_PHY_W5500
#define ETH_PHY_ADDR        1
#define ETH_PHY_CS          10
#define ETH_PHY_IRQ         9
#define ETH_PHY_RST         14
#define ETH_PHY_SPI_HOST   SPI2_HOST
#define ETH_PHY_SPI_SCK     12
#define ETH_PHY_SPI_MISO    13
#define ETH_PHY_SPI_MOSI    11
#define E_MISO 13
#define E_SCS 10
#define E_RST 14
#define E_INIT 9
#define USE_W5100 false
#define USING_W6100 true
#define USING_ENC28J60 false
bool eth_link = false;
static bool eth_connected = false;

void onEvent(arduino_event_id_t event, arduino_event_info_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("WiFi Started");
      break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED: 
      Serial.println("ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED");   
      break;
    #ifdef useETH
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("logo_alzbetinka");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED: 
      Serial.println("ETH Connected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:    
      Serial.printf("ETH Got IP: '%s'\n", esp_netif_get_desc(info.got_ip.esp_netif)); 
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    #endif
    default: break;
  }
}

#ifndef asyncACN
void sAcnDmxReceived() { 
  brightness = recv.dmx(d_adr);
  Serial.print("DMX data: ");
  for (int i=0; i < 30; i++ ) {
    if (d_adr+i <= 512) {
      Serial.print(recv.dmx(d_adr+i));
      Serial.print(" : ");
    } else {
      Serial.println("TODO next universe dmx in");
    }
  }
  //TODO only new data
  Serial.println("");
  newData = true;
}

// Nový zdroj sACN
void newSource() {
  Serial.print("new soure name: ");
  Serial.println(recv.name());
}

void framerate() {
  //Serial.print("sACN framerate fps: ");
  //Serial.println(recv.framerate());
}

void timeOut() {
  Serial.println("Timeout!");
}
#endif