#include <EEPROM.h>
// For the RFID module.
#include "rfid.h"
#include "color.h"

#define MAX_USERS 4
#define MAX_PLOTS 4
#define MAX_WAIT 300

#define BTN_ADD 25
#define BTN_RESET_WAIT 100

#define BTN_PIN 2
#define RED_PIN 3
#define GREEN_PIN 5
#define BLUE_PIN 6

size_t data_current_address = 0;
RFID_ID users[MAX_USERS] = { {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} };

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
  read_storage();
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
    Serial.print("Loaded "); Serial.print(MAX_USERS) + Serial.println(" users: "); 
    for (size_t i = 0; i < MAX_USERS; i++) {
      Serial.print(" - "); Serial.println(users[i].to_string());
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
 * BASE
 */

RFID rfid_card(7, 8);
RFID_ID test_id(0x98, 0xC0, 0x6D, 0xDE);
uint8_t state(0);
uint16_t wait_count(0);
uint16_t btn_repeat(0);

/*!
 * Setups the entire program.
 */
void setup() 
{
  Serial.begin(9600);
  Serial.println("Starting up 'garage_velo'...");

  init_storage();

  pinMode(BTN_PIN, INPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

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

  if (state == 1) {
    on_add();
    return;
  }

  if (!rfid_card.has_card_present()) {
    clear_color_led();
    delay(100);
    return;
  }
  Serial.println("A card is detected!");
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
  
  Serial.print("Has card: ");
  Serial.print(rfid_card.has_card_present());
  if (rfid_card.has_new_card()) {
    bool success = false;
    for (size_t i = 0; i < MAX_USERS; i++) {
      if (users[i] == rfid_card.get_current_id()) {
        display_color(GREEN);
        Serial.println(" Hello user #" + String(i));
        success = true;
        break;
      }
    }
    if (!success)
      display_color(RED);
  } 
  if (rfid_card.has_card_present()) {
    Serial.print(" New: ");
    Serial.print(rfid_card.has_new_card());
    Serial.print(" ID: ");
    Serial.print(rfid_card.get_current_id().to_string());
    Serial.println();
  }
  delay(100);
}

void on_add()
{
  if (rfid_card.has_card_present()) {
    auto i = get_first_null_user();
    auto id = rfid_card.get_current_id();
    
    // Cannot add an user because all slots are filled.
    if (i == MAX_USERS) {
      Serial.println("[P'n'Go][Error] Cannot add new user: all slots are filled.");
      display_color(RED);
      delay(500);
      display_color(BLUE);
      delay(500);
      display_color(BLUE);
      delay(500);
      clear_color_led();
      state = 0;
      return;
    } else if (has_user(id)) {
      Serial.println("[P'n'Go][Error] Cannot add new user: user already added.");
      display_color(GREEN);
      delay(500);
      display_color(RED);
      delay(500);
      display_color(BLUE);
      delay(500);
      clear_color_led();
      state = 0;
      return;
    }

    users[i] = id;
    Serial.println("[P'n'Go] Added user '" + id.to_string() + "'.");
    wait_count = 0;
    state = 0;
  } else {
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
      state = 0;
    }
  }
}

void reset()
{
  for (size_t i = 0; i < MAX_USERS; i++)
    users[i] = RFID_ID(0, 0, 0, 0);
  users[0] = RFID_ID(0,0,0,0);
  write_storage();
}

void handle_button()
{
  auto btn_value = digitalRead(BTN_PIN);
  if (btn_value == LOW) {
    if (btn_repeat > BTN_RESET_WAIT) {}
    else if (btn_repeat > BTN_ADD) {
      state = 1;
    }
    btn_repeat = 0;
  } else if (btn_value == HIGH) {
    if (btn_repeat == BTN_ADD) Serial.println("btn_helper: Ready to add user.");
    btn_repeat++;
    if (btn_repeat == BTN_RESET_WAIT + 1) {
      Serial.println("[P'n'Go][WARNING] Resetting...");
      reset();
      display_color(GREEN);
      delay(10);
      display_color(RED);
      delay(10);
      clear_color_led();
    }
  }
}
