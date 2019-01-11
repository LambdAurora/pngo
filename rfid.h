#include <SoftwareSerial.h>

class RFID_ID
{
private:
  int bit0 = 0;
  int bit1 = 1;
  int bit2 = 2;
  int bit3 = 3;

public:
  RFID_ID(int bit0, int bit1, int bit2, int bit3) : bit0(bit0), bit1(bit1), bit2(bit2), bit3(bit3)
  {}

  int get_bit_0() const
  {
    return bit0;  
  }

  int get_bit_1() const
  {
    return bit1;
  }

  int get_bit_2() const
  {
    return bit2;
  }

  int get_bit_3() const
  {
    return bit3;
  }

  /*!
   * Transforms the ID into a string value.
   * @return The ID as a string.
   */
  String to_string() const
  {
    return String(bit0, HEX) + String(bit1, HEX) + String(bit2, HEX) + String(bit3, HEX);
  }

  bool operator==(const RFID_ID &other) const
  {
    return bit0 == other.bit0 && bit1 == other.bit1 && bit2 == other.bit2 && bit3 == other.bit3;
  }

  bool operator!=(const RFID_ID &other) const
  {
    return !(*this == other);
  }
};

class RFID
{
private:
  SoftwareSerial _handle;
  int last_data[11];
  RFID_ID current_id;
  int last_state = 0;
  
public:
  RFID(int pin1, int pin2) : _handle(pin1, pin2), current_id(0, 0, 0, 0)
  {}

  const SoftwareSerial &get_handle() const
  {
    return _handle;
  }

  const int *get_last_data() const
  {
    return last_data;
  }

  const RFID_ID &get_current_id() const
  {
    return current_id;
  }

  /*!
   * Initializes the RFID module.
   */
  void init()
  {
    _handle.begin(19200);
  }

  void halt()
  {
    // Halt tag.
    _handle.write((uint8_t) 255);
    _handle.write((uint8_t) 0);
    _handle.write((uint8_t) 1);
    _handle.write((uint8_t) 147);
    _handle.write((uint8_t) 148);
  }

  void seek()
  {
    // Search for RFID tag.
    _handle.write((uint8_t) 255);
    _handle.write((uint8_t) 0);
    _handle.write((uint8_t) 1);
    _handle.write((uint8_t) 130);
    _handle.write((uint8_t) 131);
    delay(10);
  }

  void parse()
  {
    while (_handle.available()) {
      if (_handle.read() == 255)
        for (size_t i = 1; i < 11; i++)
          last_data[i] = _handle.read();
    }
  }

  void update()
  {
    seek();
    delay(10);
    parse();
    if (last_state == 0 && has_card_present()) {
      current_id = { last_data[8], last_data[7], last_data[6], last_data[5] };
      last_state = 1;
    } else if (last_state == 1) {
      if (has_card_present())
        last_state = 2;
    }
    if (last_state != 0 && !has_card_present()) last_state = 0;
  }

  bool has_new_card() const
  {
    return last_state == 1;
  }

  bool has_card_present() const
  {
    if (last_data[2] == 2)
    {
      if (last_data[6] == 0xFF && last_data[7] == 0 && last_data[8] == 6 && last_data[9] == 0x82 && last_data[10] == 2)
        return true;
      return false;
    }
    else return true;  
  }
};
