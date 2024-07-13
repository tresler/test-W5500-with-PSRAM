//#define FASTLED_FORCE_SOFTWARE_PINS
#define ESP32_ARDUINO_NO_RGB_BUILTIN 
#define FASTLED_INTERNAL
//#define FASTLED_ESP32_I2S true
//#define FASTLED_ESP32_I2S_NUM_DMA_BUFFERS 16
#define asyncACN
//#define useETH 
#define ledriver //zapni PSRAM na OPI
//#define fastled
#include <Preferences.h>
#include <FastLED.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <DNSServer.h>
#include <AsyncTCP.h>
#ifdef useETH
#include <ETH.h>
#endif
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#ifdef asyncACN
#include <ESPAsyncE131.h>
#else
#include <sACN.h>
#endif
#include <SPI.h>
#include "alzbetinka_logo_test.h"
#include "sd_card.h"

void setup() {
  Serial.begin(115200);
 	delay(2000);

  //preferences
  preferences.begin("all", true); 
  d_adr = preferences.getInt("address", 1);
  d_adr = 500;
  d_unv = preferences.getInt("universe", 7);
  preferences.end();

  #ifdef fastled
  FastLED.addLeds<WS2813, led_out[0], GRB>(leds[0], NUM_LEDS);
  FastLED.addLeds<WS2812, led_out[1], GRB>(leds[1], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[2], GRB>(leds[2], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[3], GRB>(leds[3], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[4], GRB>(leds[4], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[5], GRB>(leds[5], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[6], GRB>(leds[6], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[7], GRB>(leds[7], NUM_LEDS);
  #ifdef FASTLED_ESP32_I2S
  FastLED.addLeds<WS2813, led_out[8], GRB>(leds[8], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[9], GRB>(leds[9], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[10], GRB>(leds[10], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[11], GRB>(leds[11], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[12], GRB>(leds[12], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[13], GRB>(leds[13], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[14], GRB>(leds[14], NUM_LEDS);
  FastLED.addLeds<WS2813, led_out[15], GRB>(leds[15], NUM_LEDS);
  #endif

  FastLED.setBrightness(60);

  delay(100);
  for (int i = 0; i < NUM_STRIPS; i++) {
    Serial.print("Cyklus: ");
    Serial.println(i);
    fill_solid(leds[i], NUM_LEDS, CRGB::Black);
  }
  FastLED.show();
  #endif

  // Pridavani jednotlivych vystupu LED pasku
  #ifdef ledriver
  Serial.println("Startuji LED driver (třeba PSRAM na OPI)");
  driver.initled((uint8_t *)leds, led_out, NUM_STRIPS, NUM_LEDS);  // Nezapomeň zapnout PSRAM na OPI
  delay(1000);
  driver.setBrightness(255);  
  for(int i = 0; i < NUM_STRIPS; i++) {  
    fill_solid(leds[i], NUM_LEDS, CRGB(0, 0, 0));
  }
  driver.show();
  #endif

  //WIFI setup
  Network.onEvent(onEvent);
  WiFi.begin(); 
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid_ap, password_ap);
  WiFi.softAPConfig(WIFI_IP_ADDR, AP_GATEWAY_IP, AP_NETWORK_MASK);  
  esp_err_t err = esp_wifi_set_mac(WIFI_IF_AP, &mac[0]);
  if (err == ESP_OK) {
    Serial.println("Success changing WiFi Mac Address");
  }
  // Setup the DNS server redirecting all the domains to the apIP
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", AP_GATEWAY_IP);
  // MDNS start
  if (!MDNS.begin("logo")) {
    Serial.println("Chyba vytváření MDNS responderu!");
  }
  Serial.println("mDNS responder nastartován");

  //Ethernet start
  fspi = new SPIClass(HSPI);  
  fspi->begin(E_CLK, E_MISO, E_MOSI, E_SCS);
  #ifdef useETH
  ETH.setTaskStackSize(8192); 
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, mac, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, *fspi);

  Serial.println("All network information: ");
  Serial.println(Network);
  #endif

  //Nacteni sd karty
  if(!SD.begin(S_CS, *fspi, 4000000, "/sd", 5, false)) {
    Serial.println("Card Mount Failed");
  } else {
    SDCardType();
    SDCardSize();
  }

  //sACN    
  Serial.println("Startuji sACN");
  #ifdef asyncACN
    if (e131.begin(E131_MULTICAST, d_unv, d_unv_count)) {
      Serial.println(F("Listening for data..."));
    } else { 
      Serial.println(F("*** e131.begin failed ***"));
    }
  #else
    recv.callbackDMX(sAcnDmxReceived);
    recv.callbackSource(newSource);
    recv.callbackFramerate(framerate);
    recv.callbackTimeout(timeOut);
    recv.begin(d_unv, false);  
  #endif 

  //Start asyncwebserver
  webserver.begin();

  
  for(int i = 0; i < NUM_STRIPS; i++) {       
    fill_gradient_RGB(leds[i], 0, CRGB::Blue, NUM_LEDS, CRGB::Red);
  }
  #ifdef fastled
  FastLED.setBrightness(brightness); 
  FastLED.show();
  #endif 
  #ifdef ledriver
  driver.setBrightness(brightness); 
  driver.show();
  #endif
  Serial.print("Gradient: ");

}

void loop() {
  #ifdef asyncACN
    if (!e131.isEmpty()) {
      e131.pull(&packet);
      if (htons(packet.universe) == d_unv) {
        brightness = packet.property_values[d_adr];
        for (int i = 0; i < d_adrs; i++ ) {
          if (d_adr+i <= 512) {
            dmxData[i] = packet.property_values[d_adr+i];
          }
        }
      } else if (htons(packet.universe) == d_unv+1 && d_adr+d_adrs > 512) {
        for (int i = 0; i < d_adr+d_adrs-512; i++ ) {
          dmxData[i+512-d_adr] = packet.property_values[i];
        }
      }
      Serial.print("Packet counter: ");
      Serial.println(e131.stats.num_packets);
      Serial.print("DMX data: ");
      for (int i = 0; i < d_adrs; i++) {
        Serial.print(dmxData[i]);
        Serial.print(" : ");
      }
      Serial.println("");
      newData = true;
    }
  #else
    recv.receive();
  #endif

  if (newData) {
    newData = false;
    for(int i = 0; i < NUM_STRIPS; i++) { 
      CRGB last = leds[i][NUM_LEDS-1];
      memmove( &leds[i][1], &leds[i][0], (NUM_LEDS) * (sizeof(CRGB)));
      leds[i][0] = last;
    }
    lastReload = millis();
    Serial.print("Doba zápisu LED pásků: ");
    #ifdef fastled
    FastLED.setBrightness(brightness); 
    FastLED.show();
    #endif
    #ifdef ledriver
    driver.setBrightness(brightness); 
    driver.show();
    #endif
    //Serial.println("");
    Serial.println(millis()-lastReload);
    lastReload = millis();
  } else {
    //Serial.print(".");
  }

}
