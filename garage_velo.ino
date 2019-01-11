#include <EEPROM.h>
// For the RFID module.
#include "rfid.h"
#include "color.h"

#define MAX_USERS 4
#define MAX_PLOTS 4

#define RED_PIN 2
#define GREEN_PIN 3
#define BLUE_PIN 4

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
 * Definition of a plot.
 */
struct Plot
{
  bool valid = false;
  uint16_t lock_pin;
  uint16_t indicator_pin;
  int user;
};

Plot plots[4];

/*!
 * Creates a plot instance.
 */
Plot make_plot(uint16_t lock_pin, uint64_t indicator_pin, int user)
{
  Plot plot{};
  plot.valid = true;
  plot.lock_pin = lock_pin;
  plot.indicator_pin = indicator_pin;
  plot.user = user;
  return plot; 
}

bool Plot_has_user(const Plot &plot)
{
  return plot.user > -1;
}

int get_plot_from_user(size_t user)
{
  for (size_t i = 0; i < MAX_PLOTS; i++)
    if (plots[i].user == user) return i;
  return -1;
}

int get_first_empty_plot()
{
  for (size_t i = 0; i < MAX_PLOTS; i++)
    if (!Plot_has_user(plots[i])) return i; 
  return -1;
}

bool has_empty_plot()
{
  return get_first_empty_plot() != -1;
}

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
    for (size_t i = 0; i < MAX_PLOTS; i++) {
      EEPROM.write(addr, 0);
      addr++;
      EEPROM.write(addr, 0);
      addr++;
      EEPROM.write(addr, -1);
      addr++;
    }
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
    for (size_t i = 0; i < MAX_PLOTS; i++) {
      uint16_t lock_pin = EEPROM.read(addr);
      addr++;
      uint16_t indicator_pin = EEPROM.read(addr);
      addr++;
      int user = EEPROM.read(addr);
      plots[i] = make_plot(lock_pin, indicator_pin, user);
      Serial.println("Loaded plot #" + String(i) + "...");
      addr++;
    }
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
  for (size_t i = 0; i < MAX_PLOTS; i++) {
    Plot plot = plots[i];
    EEPROM.write(addr, plot.lock_pin);
    addr++;
    EEPROM.write(addr, plot.indicator_pin);
    addr++;
    EEPROM.write(addr, plot.user);
    Serial.println("Saved plot #" + String(i) + "...");
    addr++;
  }
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

/*
 * LEDS
 */

const Color RED{1.f, 0.f, 0.f};
const Color GREEN{0.f, 1.f, 0.f};
const Color YELLOW{1.f, .50f, 0.f};

void display_color(const Color &color)
{
  analogWrite(RED_PIN, color.red() * 255);
  analogWrite(GREEN_PIN, color.green() * 255);
  analogWrite(BLUE_PIN, color.blue() * 255); 
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

/*!
 * Setups the entire program.
 */
void setup() 
{
  Serial.begin(9600);
  Serial.println("Starting up 'garage_velo'...");

  init_storage();

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

  auto plot = get_plot_from_user(user);
  if (plot < 0) {
    // Allocate a plot for the user.
  } else {
    // Free the user's plot.
  }
  
  Serial.print("Has card: ");
  Serial.print(rfid_card.has_card_present());
  if (rfid_card.has_new_card()) {
    for (size_t i = 0; i < MAX_USERS; i++) {
      if (users[i] == rfid_card.get_current_id()) {
        Serial.println(" Hello user #" + String(i));
      }
    }
    if (rfid_card.get_current_id() == test_id) {
      Serial.println(" Hello Jean-Pierre!");
      display_color(GREEN);
    } else {
      Serial.println(" Go away!");
      display_color(RED);
    }
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
