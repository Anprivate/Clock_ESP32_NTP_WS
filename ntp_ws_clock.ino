#include <Arduino.h>
#include <GyverNTP.h>

#include "FastLED.h"

// Fixed definitions cannot change on the fly.
#define LED_DT 13        // Data pin to connect to the strip.
#define COLOR_ORDER GRB  // It's GRB for WS2812 and BGR for APA102.
#define LED_TYPE WS2812  // Using APA102, WS2812, WS2801. Don't forget to modify LEDS.addLeds to suit.
#define NUM_LEDS 58      // Number of LED's. 7*2*4 + 2

struct CRGB leds[NUM_LEDS];  // Initialize our LED array.
CRGB curr_color = CRGB::White;
byte curr_brightness = 8;

// order - 0abcdefg
const byte seg_font[16] = {
  0b1111110,  // 0
  0b0110000,  // 1
  0b1101101,  //2
  0b1111001,  //3
  0b0110011,  //4
  0b1011011,  //5
  0b1011111,  //6
  0b1110000,  //7
  0b1111111,  //8
  0b1111011,  //9
  0b1110111,  //a
  0b0011111,  //b
  0b1001110,  //c
  0b0111101,  //d
  0b1001111,  //e
  0b1000111   //f
};

// upper nibble - index of byte, lower nibble - position of bit in byte (a - 0, g - 6)
const byte seg_map[NUM_LEDS] = { 0x33, 0x33, 0x34, 0x34, 0x35, 0x35, 0x30, 0x30, 0x31, 0x31, 0x32, 0x32, 0x36, 0x36,
                                 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x26, 0x26,
                                 0x45, 0x46,
                                 0x13, 0x13, 0x14, 0x14, 0x15, 0x15, 0x10, 0x10, 0x11, 0x11, 0x12, 0x12, 0x16, 0x16,
                                 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x06, 0x06 };

// current data [0/1] - hours, [2/3] - minutes, [4] - dot points
byte curr_data[5] = { 0, 0, 0, 0, 0 };
byte prev_data[5] = { 1, 2, 3, 4, 5 };

int lastWIFIStatus;

void setup() {
  Serial.begin(115200);

  WiFi.begin("AP_NAME", "AP_PASSWORD");

  while (WiFi.status() != WL_CONNECTED) delay(100);
  Serial.println("Connected");

  // обработчик ошибок
  NTP.onError([]() {
    Serial.println(NTP.readError());
    Serial.print("online: ");
    Serial.println(NTP.online());
  });

  NTP.begin(3);                // запустить и указать часовой пояс
  NTP.setPeriod(60 * 60 * 6);  // период синхронизации в секундах
                               // NTP.setHost("ntp1.stratum2.ru");     // установить другой хост
                               // NTP.setHost(IPAddress(1, 2, 3, 4));  // установить другой хост
                               // NTP.asyncMode(false);                // выключить асинхронный режим
                               // NTP.ignorePing(true);                // не учитывать пинг до сервера
                               // NTP.updateNow();                     // обновить прямо сейчас

  LEDS.addLeds<LED_TYPE, LED_DT, COLOR_ORDER>(leds, NUM_LEDS);  // Use this for WS2812

  FastLED.setBrightness(curr_brightness);
}

void loop() {
  curr_data[4] = NTP.online() ? 0x03 : 0x00;

  // тикер вернёт true каждую секунду в 0 мс секунды, если время синхронизировано
  if (NTP.tick()) {
    // вывод в Datime
    Datime dt = NTP;
    curr_data[0] = dt.hour / 10;
    curr_data[1] = dt.hour % 10;
    curr_data[2] = dt.minute / 10;
    curr_data[3] = dt.minute % 10;
  }

  // изменился онлайн-статус
  if (NTP.statusChanged()) {
    Serial.print("STATUS: ");
    Serial.println(NTP.online());
  }

  bool was_change = false;

  for (int i = 0; i < 5; i++)
    if (curr_data[i] != prev_data[i])
      was_change = true;

  if (was_change) {
    byte raw_data[5];
    for (byte i = 0; i < 4; i++) raw_data[i] = seg_font[curr_data[i]];
    raw_data[4] = curr_data[4];

    for (int8_t i = 0; i < NUM_LEDS; i++) {
      byte idx = seg_map[i] >> 4;
      byte position = seg_map[i] & 0x07;

      byte tmp1 = raw_data[idx];

      if (tmp1 & (0b01000000 >> position)) {
        leds[i] = curr_color;
      } else {
        leds[i] = CRGB::Black;
      }
    }

    FastLED.show();

    for (int i = 0; i < 5; i++) prev_data[i] = curr_data[i];
  }
}