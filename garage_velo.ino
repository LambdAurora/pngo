#include <ArduinoSTL.h>
#include <EEPROM.h>
// For the RFID module.
#include "rfid.h"
#include "options.h"

#define MAX_USERS 4
#define MAX_PLOTS 4
#define MAX_WAIT 300

#define BTN_ADD 25
#define BTN_RESET_WAIT 100

#define BTN_SELECT_PIN 2
#define BTN_MENU_PIN 4
#define RED_PIN 3
#define GREEN_PIN 5
#define BLUE_PIN 6

#define TO_STRING(input) (String(input).c_str())
#define ID_STR(id) (id.to_string().c_str())

size_t data_current_address = 0;
RFID_ID users[MAX_USERS] = { {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} };
uint8_t doors[MAX_USERS] = { 10, 9, 12, 13 };

struct Storage
{
  int u0_0 = 0x98, u0_1 = 0xC0, u0_2 = 0x6D, u0_3 = 0xDE;
  int u1_0 = 0x85, u1_1 = 0x53, u1_2 = 0xB0, u1_3 = 0x8;
  int u2_0 = 0, u2_1 = 0, u2_2 = 0, u2_3 = 0;
  int u3_0 = 0, u3_1 = 0, u3_2 = 0, u3_3 = 0;
};

typedef struct Storage Storage;

/*!
 * Initializes the storage.
 */
void init_storage()
{
  EEPROM.write(0x00, 0x4);
  if (EEPROM.read(0x00) != 0x42) {
    size_t addr = 0x00;
    // Write defaults.
    EEPROM.write(addr, 0x42);
    addr++;
    Storage storage{};
    EEPROM.put(addr, storage);
  }
}

/*!
 * Reads the storage.
 */
void read_storage()
{
  size_t addr = 0;
  // Structure identifier.
  if (EEPROM.read(addr) == 0x42) {
    Storage storage;
    addr++;
    EEPROM.get(addr, storage);
    users[0] = {storage.u0_0, storage.u0_1, storage.u0_2, storage.u0_3};
    users[1] = {storage.u1_0, storage.u1_1, storage.u1_2, storage.u1_3};
    users[2] = {storage.u2_0, storage.u2_1, storage.u2_2, storage.u2_3};
    users[3] = {storage.u3_0, storage.u3_1, storage.u3_2, storage.u3_3};
    std::cout << "Loaded " << MAX_USERS << " users: " << std::endl;
    for (size_t i = 0; i < MAX_USERS; i++) {
      std::cout << " - " << ID_STR(users[i]) << std::endl;
    }
  }
}

void write_storage()
{
  size_t addr = 0x00;
  EEPROM.write(addr, 0x42);
  addr++;
  Storage storage{};
  // User #0
  storage.u0_0 = users[0].get_bit_0();
  storage.u0_1 = users[0].get_bit_1();
  storage.u0_2 = users[0].get_bit_2();
  storage.u0_3 = users[0].get_bit_3();
  // User #1
  storage.u1_0 = users[1].get_bit_0();
  storage.u1_1 = users[1].get_bit_1();
  storage.u1_2 = users[1].get_bit_2();
  storage.u1_3 = users[1].get_bit_3();
  // User #0
  storage.u2_0 = users[2].get_bit_0();
  storage.u2_1 = users[2].get_bit_1();
  storage.u2_2 = users[2].get_bit_2();
  storage.u2_3 = users[2].get_bit_3();
  // User #0
  storage.u3_0 = users[3].get_bit_0();
  storage.u3_1 = users[3].get_bit_1();
  storage.u3_2 = users[3].get_bit_2();
  storage.u3_3 = users[3].get_bit_3();
  EEPROM.put(addr, storage);
}

bool has_user(const RFID_ID &id)
{
  for (size_t i = 0; i < MAX_USERS; i++)
    if (users[i] == id)
      return true;
  return false;
}

size_t get_first_null_user()
{
  for (size_t i = 0; i < MAX_USERS; i++) {
    if (users[i] == RFID_ID(0, 0, 0, 0))
      return i;
  }
  return MAX_USERS;
}

/*
 * LEDS
 */

const Color RED{1.f, 0.f, 0.f};
const Color GREEN{0.f, 1.f, 0.f};
const Color BLUE{0.f, 0.f, 1.f};
const Color YELLOW{.50f, .40f, 0.f};

void display_color(const Color &color)
{
  analogWrite(RED_PIN, pow(color.red(), 2.f) * 255);
  analogWrite(GREEN_PIN, pow(color.green(), 2.f) * 255);
  analogWrite(BLUE_PIN, pow(color.blue(), 2.f) * 255); 
}

void clear_color_led()
{
  analogWrite(RED_PIN, 0);
  analogWrite(GREEN_PIN, 0);
  analogWrite(BLUE_PIN, 0);
}

/* 
 * Base
 */

class Reflex {
private:
  std::vector<MenuOption *> _menu;
  size_t current_selected = 0;
  
public:
  uint8_t state = 0;

  Reflex() 
  {
    _menu.push_back(&BLANK_MENU_OPTION);
    _menu.push_back(new MenuOption("add user", color::from_int_rgba(0, 255, 0), [](Reflex &reflex) {
      reflex.state = 1;
    }));
    _menu.push_back(new MenuOption("reset", color::from_int_rgba(255, 0, 0), [](Reflex &reflex) { 
      reflex.reset(); 
    }));
  }

  void add_menu_option(MenuOption *option)
  {
    _menu.push_back(option);
  }

  MenuOption* get_current_option() const
  {
    return current_selected < _menu.size() ? _menu[current_selected] : nullptr;
  }

  void init() {
    std::cout << " :: System -> Initializing...";
    pinMode(BTN_MENU_PIN, INPUT);
    pinMode(BTN_SELECT_PIN, INPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    for (size_t i = 0; i < MAX_USERS; i++)
      pinMode(doors[i], OUTPUT);
  }

  void reset() {
    std::cout << " :: System -> Resetting...";
    for (size_t i = 0; i < MAX_USERS; i++)
      users[i] = RFID_ID(0, 0, 0, 0);
    users[0] = RFID_ID(0,0,0,0);
    write_storage();
    current_selected = 0;
    std::cout << "done!" << std::endl;
  }

  Reflex& operator++()
  {
    current_selected++;
    if (current_selected >= _menu.size())
      current_selected = 0;
    return *this;
  }
};

Reflex reflex;
RFID rfid_card(7, 8);
RFID_ID test_id(0x98, 0xC0, 0x6D, 0xDE);
uint16_t wait_count(0);
uint16_t btn_repeat(0);

/*!
 * Setups the entire program.
 */
void setup() 
{
  Serial.begin(9600);
  Serial.println("Starting up 'garage_velo'...");
  reflex.init();

  init_storage();
  read_storage();

  // Initialize the serial communication to the RFID module.
  rfid_card.init();
  delay(10);
  rfid_card.halt();
}

/*!/
 * The main loop.885/.558887/
 */
void loop() 
{
  rfid_card.update();

  handle_button();

  if (reflex.state == 1) {
    on_add();
    return;
  }

  if (!rfid_card.has_card_present()) {
    clear_color_led();
    delay(100);
    return;
  }
  std::cout << "A card is detected!" << std::endl;
  RFID_ID current_id = rfid_card.get_current_id();

  size_t user = -1;
  for (size_t i = 0; i < MAX_USERS; i++) {
    if (users[i] == current_id) {
      user = i;
      break;
    }
  }

  if (user < 0) {
    delay(100);
    return;
  }
  
  std::cout << "Has card: " << rfid_card.has_card_present();
  if (rfid_card.has_new_card()) {
    bool success = false;
    for (size_t i = 0; i < MAX_USERS; i++) {
      if (users[i] == rfid_card.get_current_id()) {
        display_color(GREEN);
        std::cout << " Hello user #" << TO_STRING(i + 1) << std::endl;
        success = true;
        break;
      }
    }
    if (!success)
      display_color(RED);
  } else {
    if (rfid_card.get_current_id().to_string() != "0000")
      for (size_t i = 0; i < MAX_USERS; i++) {
        if (users[i] == rfid_card.get_current_id()) {
          std::cout << " :: LOCKER -> Unlock for user #" << TO_STRING(i + 1) << std::endl;
          display_color(color::ORANGE);
          digitalWrite(doors[i], HIGH);
          delay(15000);
          digitalWrite(doors[i], LOW);
          clear_color_led();
          break;
        }
      }
  }
  if (rfid_card.has_card_present())
    std::cout << " New: " << rfid_card.has_new_card() << " ID: " << ID_STR(rfid_card.get_current_id()) << std::endl;
  delay(100);
}

void on_add()
{
  if (rfid_card.has_new_card()) {
    auto i = get_first_null_user();
    auto id = rfid_card.get_current_id();
    
    // Cannot add an user because all slots are filled.
    if (i == MAX_USERS) {
      std::cout << "[P'n'Go][Error] Cannot add new user: all slots are filled." << std::endl;
      display_color(RED);
      delay(500);
      display_color(BLUE);
      delay(500);
      display_color(BLUE);
      delay(500);
      clear_color_led();
      reflex.state = 0;
      return;
    } else if (has_user(id)) {
      std::cout << "[P'n'Go][Error] Cannot add new user: user already added." << std::endl;
      display_color(RED);
      delay(500);
      display_color(color::from_int_rgba(255, 0, 255));
      delay(500);
      display_color(BLUE);
      delay(500);
      clear_color_led();
      reflex.state = 0;
      return;
    }

    if (id.to_string() == "60ffd0" || id.to_string() == "ffffffffffffffff")
      return;

    users[i] = id;
    std::cout << "[P'n'Go] Added user '" << ID_STR(id) << "'." << std::endl;
    wait_count = 0;
    reflex.state = 0;
  } else {
    wait_count++;
    if (wait_count >= MAX_WAIT) {
      display_color(BLUE);
      delay(500);
      clear_color_led();
      delay(500);
      display_color(BLUE);
      delay(500);
      display_color(RED);
      delay(500);
      clear_color_led();
      wait_count = 0;
      reflex.state = 0;
    }
  }
}


void handle_button()
{
  auto menu_value = digitalRead(BTN_MENU_PIN);
  if (menu_value == HIGH) {
    reflex++;
    auto option = reflex.get_current_option();
    std::cout << " :: Menu -> " << option->get_name() << std::endl;
    display_color(option->get_color());
    delay(500);
    clear_color_led();
  }

  auto select_value = digitalRead(BTN_SELECT_PIN);
  if (select_value == LOW) return;
  auto option = reflex.get_current_option();
  std::cout << " :: Menu -> Executing " << option->get_name() << "..." << std::endl;
  if (option == nullptr) return;
  for (size_t i = 0; i < 3; i++) {
    display_color(option->get_color());
    delay(50);
    clear_color_led();
    delay(50);
  }
  option->execute(reflex);
}
/*void handle_button()
{
  auto btn_value = digitalRead(BTN_MENU_PIN);
  if (btn_value == LOW) {
    if (btn_repeat > BTN_RESET_WAIT) {}
    else if (btn_repeat > BTN_ADD) {
      state = 1;
    }
    btn_repeat = 0;
  } else if (btn_value == HIGH) {
    if (btn_repeat == BTN_ADD) std::cout << "btn_helper: Ready to add user." << std::endl;
    btn_repeat++;
    if (btn_repeat == BTN_RESET_WAIT + 1) {
      std::cout << "[P'n'Go][WARNING] Resetting..." << std::endl;
      reset();
      display_color(GREEN);
      delay(10);
      display_color(RED);
      delay(10);
      clear_color_led();
    }
  }
}*/
