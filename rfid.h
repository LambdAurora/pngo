/*
 * Little RFID library. / Petite bibliothèque RFID.
 *
 * Réalisé en 2018-2019 par "LambdAurora".
 *
 * Exemple d'utilisation:
 *
 * // Instanciation de la carte RFID sur les pins 7 et 8 de la Arduino.
 * RFID rfid_card(7, 8);
 *
 * void init()
 * {
 *     // Initialisation du capteur RFID et de la communication serial.
 *     rfid_card.init();
 *     delay(10);
 *     rfid_card.halt();
 * }
 *
 * void loop()
 * {
 *     // On mets à jour les informations de la carte.
 *     rfid_card.update();
 *
 *     // Maintenant on peut récupérer des informations!
 *     // Vérifier qu'il y a une carte.
 *     if (rfid_card.has_card_present()) {
 *         // On peut vérifier si le badge vient juste d'être posé sur le capteur, ce qui peut éviter que le code se répète à l'infini pour une action quand on laisse le badge RFID sur le capteur.
 *         if (rfid_card.has_new_card()) {
 *             // Une nouveau badge est détecté.
 *             Serial.println("Une nouveau badge a été détecté!");
 *         } else {
 *             // La carte est détecté mais elle était déjà présente sur le capteur.
 *         }
 *
 *         // Pour récupérer l'identifiant c'est simple et un objet a été fait pour:
 *         RFID_ID identifiant = rfid_card.get_current_id();
 *         // On peut le récupérer en tant que chaîne de caractères avec to_string():
 *         Serial.println("Identifiant: " + identifiant.to_string());
 *         // Si on a un autre objet identifiant on peut alors les comparer:
 *         RFID_ID id2(0x98, 0xC0, 0x6D, 0xDE);
 *         if (identifiant == id2) {
 *             Serial.println("L'identifiant est le même que id2!");
 *         } else Serial.println("L'identifiant n'est pas le même que id2.");
 *
 *         // Attention! des erreurs peuvent survenir dans l'identifiant et cela peut être causé par le capteur!
 *         // Les identifiants "0000" et "FFFF" doivent être considéré comme des erreurs!
 *     }
 *
 *     // On doit attendre obligatoirement 100ms pour que la carte puisse fournir les informations entre chaque boucle.
 *     delay(100);
 * }
 */

#include <SoftwareSerial.h>

/*!
 * Représente un identifiant RFID.
 */
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

/*!
 * Représente la carte RFID.
 */
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

  /*!
   * Donne l'instance de la connexion Serial.
   *
   * @return L'instance de la connexion Serial.
   */
  const SoftwareSerial &get_handle() const
  {
    return _handle;
  }

  /*!
   * Donne le tableau des données récupérées à la dernière mise à jour.
   *
   * @return Le tableau des dernières données de taille 11.
   */
  const int *get_last_data() const
  {
    return last_data;
  }

  /*!
   * Donne le dernier identifiant détecté.
   *
   * @return Le dernier identifiant détecté.
   */
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

  /*!
   * Fonction de mise à jour des informations de la carte.
   */
  void update()
  {
    // On demande les informations.
    seek();
    delay(10);
    // On les récupère.
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

  /*!
   * Indique si le badge présent est nouveau ou non.
   *
   * @return True si un badge est nouveau, sinon false.
   */
  bool has_new_card() const
  {
    return last_state == 1;
  }

  /*!
   * Indique si un badge est présent sur le capteur ou non.
   *
   * @return True si un badge est présent, sinon false.
   */
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
