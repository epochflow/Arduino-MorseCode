#ifndef NETWORK_H
#define NETWORK_H

#include "Arduino.h"
#include "ESP8266.h"
#include <SoftwareSerial.h>

#define RECV_TIME 50

enum packet_type
{
  PT_QUIT = -1,
  PT_CONNECT = 0,
  PT_MORSE = 1
};
 
enum morse_type
{
  MT_EMPTY = 0,
  MT_SHORT = 1,
  MT_LONG = 2
};
 
enum lang_type
{
  LANG_ENGLISH = 0,
  LANG_KOREAN = 1
};
 
struct packet_data
{
  packet_type type;
  String name;
  morse_type morse;
  lang_type lang;
       
  String to_string()
  {
    String buffer = 
      String(type) + "\n" +
      name + "\n" +
      String(morse) + "\n" +
      String(lang);
               
    return buffer;
  }
       
  packet_data(char * packet)
  {
    String data(packet);
    String buffer[4];
    int lastIndex = 0;
    for(int i = 0; i < 4; i++)
    {
      int index = data.indexOf('\n', lastIndex);
      buffer[i] = data.substring(lastIndex, index);
      lastIndex = index + 1;
    }
                       
    type = static_cast<packet_type>(buffer[0].toInt());
    name = buffer[1];
    morse = static_cast<morse_type>(buffer[2].toInt());
    lang = static_cast<lang_type>(buffer[3].toInt());
  }
       
  packet_data(packet_type _type, String _name, morse_type _morse, lang_type _lang) :
    type(_type), name(_name), morse(_morse), lang(_lang) {}
};

class Network
{
  private:
    bool isconnect = false;

  public:    
    void connect_server(ESP8266 wifi, String host, int port) {
      Serial.print(F("connect begin\r\n"));      
      if (wifi.createTCP(host, port)) {
        Serial.println(F("create tcp ok"));
        isconnect = true;
      } else {
        Serial.println(F("create tcp err"));
        isconnect = false;
      }    
      Serial.print(F("connect end\r\n"));
    }
    
    bool send(ESP8266 wifi, String name, morse_type morse, lang_type lang) {
      packet_data packet(PT_MORSE, name, morse, lang);
      String s = packet.to_string();
      const char* content = s.c_str();
      if (wifi.send((const uint8_t*)content, (uint32_t)strlen(content)))
      {
        Serial.println(F("send ok"));
        return true;
      }
      else
      {
        Serial.println(F("send err"));
        //isconnect = false;
      }
      return false;
    }

    bool is_connect() {
      return isconnect;
    }

    void receive_loop(ESP8266 wifi, void(*receiveFunc)(packet_data)) {
      uint8_t buffer[64] = {0};

      uint32_t len = wifi.recv(buffer, sizeof(buffer), RECV_TIME);
      if (len > 0) {
        packet_data packet((char*)buffer);
        Serial.println(F("Received packet"));
        if (receiveFunc != nullptr)
          receiveFunc(packet);
      }
    }
};

#endif
