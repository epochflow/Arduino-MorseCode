#ifndef CHANNEL_H
#define CHANNEL_H

#include "Arduino.h"
#include "network.h"

enum channel_type
{
  CT_SELF,
  CT_OTHER
};

class Channel
{
  private:
    String other_buffer;
    String other_text;
    String other_name;
    String self_buffer;
    String self_text;
  
    String morseToChar(String morse)
    {
      if(morse == ".-") return F("a");
      else if(morse == "-...") return F("b");
      else if(morse == "-.-.") return F("c");
      else if(morse == "-..") return F("d");
      else if(morse == ".") return F("e");
      else if(morse == "..-.") return F("f");
      else if(morse == "--.") return F("g");
      else if(morse == "....") return F("h");
      else if(morse == "..") return F("i");
      else if(morse == ".---") return F("j");
      else if(morse == "-.-") return F("k");
      else if(morse == ".-..") return F("l");
      else if(morse == "--") return F("m");
      else if(morse == "-.") return F("n");
      else if(morse == "---") return F("o");
      else if(morse == ".--.") return F("p");
      else if(morse == "--.-") return F("q");
      else if(morse == ".-.") return F("r");
      else if(morse == "...") return F("s");
      else if(morse == "-") return F("t");
      else if(morse == "..-") return F("u");
      else if(morse == "...-") return F("v");
      else if(morse == ".--") return F("w");
      else if(morse == "-..-") return F("x");
      else if(morse == "-.--") return F("y");
      else if(morse == "--..") return F("z");
	    else if(morse == ".----") return F("1");
      else if(morse == "..---") return F("2");
      else if(morse == "...--") return F("3");
      else if(morse == "....-") return F("4");
      else if(morse == ".....") return F("5");
      else if(morse == "-....") return F("6");
      else if(morse == "--...") return F("7");
	    else if(morse == "---..") return F("8");
      else if(morse == "----.") return F("9");
      else if(morse == "-----") return F("0");
	    else if(morse == ".-.-") return F(" ");
      else return "";
    }

  public:    
    void write_buffer(String selfName, packet_data packet)
    {
      String morse = packet.morse == MT_SHORT ? "." : packet.morse == MT_LONG ? "-" : " ";
      if (selfName == packet.name)
      {
        if (packet.morse == MT_EMPTY)
        {
          self_text += morseToChar(self_buffer);
          if (self_text.length() > 11)
            self_text = self_text.substring(1, self_text.length());
          self_buffer = "";
        }
        else
          self_buffer += morse;
      }
      else
      {
        other_name = packet.name;
        if (packet.morse == MT_EMPTY)
        {
          other_text += morseToChar(other_buffer);
          if (other_text.length() > 11)
            other_text = other_text.substring(1, other_text.length());
          other_buffer = "";
        }
        else
          other_buffer += morse;
      }
    }
    
    String read_buffer(channel_type type)
    {
      return type == CT_SELF ? self_buffer : other_buffer;
    }

    String read_text(channel_type type)
    {
      return type == CT_SELF ? self_text : other_text;
    }

    String read_other_name()
    {
      return other_name;
    }

    void clean_buffer()
    {
      self_buffer = "";
      other_buffer = "";
    }

    void clean_text()
    {
      self_text = "";
      other_text = "";
    }
};

#endif
