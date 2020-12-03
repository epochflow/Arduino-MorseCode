#include "network.h"
#include "channel.h"
#include <ESP8266.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <SoftwareSerial.h>

#define SSID F("Umaru Net 2G")
#define PASSWORD F("")
#define HOST_NAME F("221.153.141.15")
#define HOST_PORT 5823
#define NAME F("GM AKE")

#define SHORT_PRESS_TIME 140
#define EMPTY_TIME 500
#define DISPLAY_TIME 10000

#define BT_RXD 2
#define BT_TXD 3

#define P_MORSE 8
#define P_BUZZER 7
#define P_LED_R 4
#define P_LED_G 5
#define P_LED_B 6

// Morse check
int lastState = LOW;
bool e_check = false;
bool d_check = false;
unsigned long p_time  = 0;
unsigned long r_time = 0;
unsigned long r_s_time = 0;
unsigned long r_p_time = 0;

SoftwareSerial mySerial(BT_RXD, BT_TXD);
ESP8266 wifi(mySerial);
Network net;
Channel channel;
U8G2_SH1106_128X64_NONAME_1_4W_SW_SPI u8g2(U8G2_R0, 13, 11, 10, 9);

void setup() {
  Serial.begin(9600);
  Serial.print(F("setup begin\r\n"));

  // Network Init
  {
    Serial.print(F("FW Version:"));
    Serial.println(wifi.getVersion().c_str());

    if (wifi.setOprToStationSoftAP())
      Serial.print(F("to station + softap ok\r\n"));
    else
      Serial.print(F("to station + softap err\r\n"));

    if (wifi.joinAP(SSID, PASSWORD)) {
      Serial.print(F(("Join AP success\r\n")));
      Serial.print(F("IP:"));
      Serial.println(wifi.getLocalIP().c_str());
    } else {
      Serial.print(F("Join AP failure\r\n"));
    }

    if (wifi.disableMUX())
      Serial.print(F("single ok\r\n"));
    else
      Serial.print(F("single err\r\n"));

    net.connect_server(wifi, HOST_NAME, HOST_PORT);
  }

  // Digital Init
  {
    pinMode(P_MORSE, INPUT);
    pinMode(P_BUZZER, OUTPUT);
    pinMode(P_LED_R, OUTPUT);
    pinMode(P_LED_G, OUTPUT);
    pinMode(P_LED_B, OUTPUT);

    u8g2.begin();
    u8g2.setFlipMode(1);
    //u8g.setRot180();
  }

  Serial.print(F("setup end\r\n"));
}

void led(int r, int g, int b)
{
  digitalWrite(P_LED_R, r);
  digitalWrite(P_LED_G, g);
  digitalWrite(P_LED_B, b);
}

void on_receive_set()
{
  if (r_s_time > 0) {
    if (millis() - r_p_time > r_s_time) {
      r_s_time = 0;
      digitalWrite(P_BUZZER, LOW);
    }
  }
}

void on_receive(packet_data packet)
{
  String morse = packet.morse == MT_SHORT ? "." : packet.morse == MT_LONG ? "-" : " ";
  d_check = true;

  r_p_time = millis();
  if (packet.name != NAME) {
    if (packet.morse == MT_SHORT) {
      r_s_time = 50;
      led(LOW, HIGH, LOW);
      digitalWrite(P_BUZZER, HIGH);
    }
    if (packet.morse == MT_LONG) {
      r_s_time = 150;
      led(LOW, LOW, HIGH);
      digitalWrite(P_BUZZER, HIGH);
    }
  }

  channel.write_buffer(NAME, packet);

  display_loop(
    channel.read_other_name(),
    channel.read_text(CT_OTHER) + channel.read_buffer(CT_OTHER),
    "",
    NAME,
    channel.read_text(CT_SELF) + channel.read_buffer(CT_SELF));

  Serial.println(packet.name + " : " + morse);
}

void display_loop(String line1, String line2, String line3, String line4, String line5) {
  char * c1 = (char*)line1.c_str();
  char * c2 = (char*)line2.c_str();
  char * c3 = (char*)line3.c_str();
  char * c4 = (char*)line4.c_str();
  char * c5 = (char*)line5.c_str();
  u8g2.setFont(u8g2_font_unifont_tf);
  u8g2.firstPage();
  do {
    u8g2.drawStr(0, 10, c1);
    u8g2.drawStr(0, 22, c2);
    u8g2.drawStr(0, 34, c3);
    u8g2.drawStr(0, 46, c4);
    u8g2.drawStr(0, 58, c5);
  } while (u8g2.nextPage());
}

void morse_check(boolean morseInput) {
  if (lastState == LOW && morseInput == HIGH) {
    p_time = millis();
    digitalWrite(P_BUZZER, morseInput);
  }

  if (lastState == HIGH && morseInput == LOW) {
    r_time = millis();
    e_check = true;
    digitalWrite(P_BUZZER, morseInput);
    if (r_time - p_time < SHORT_PRESS_TIME) {
      led(LOW, HIGH, LOW);
      net.send(wifi, NAME, MT_SHORT, LANG_ENGLISH);
    }
    else {
      led(LOW, LOW, HIGH);
      net.send(wifi, NAME, MT_LONG, LANG_ENGLISH);
    }
  }
  lastState = morseInput;
  if (e_check) {
    if (millis() - r_time > EMPTY_TIME) {
      e_check = false;
      led(HIGH, HIGH, HIGH);
      net.send(wifi, NAME, MT_EMPTY, LANG_ENGLISH);
    }
  }
  if (d_check) {
    if (millis() - r_p_time > DISPLAY_TIME) {
      display_loop("", "", "", "", "");
      channel.clean_buffer();
      channel.clean_text();
      d_check = false;
    }
  }
}

void loop() {
  if (net.is_connect()) {
    net.receive_loop(wifi, on_receive);
    
    int morseInput = digitalRead(P_MORSE) == LOW ? HIGH : LOW;

    morse_check(morseInput);
    on_receive_set();
  }
  else
    led(HIGH, LOW, LOW);
}
