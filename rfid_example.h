#include "rfid.h" // Le fichier de la bibliothèque doit être dans le même dossier que le .ino

// Instanciation de la carte RFID sur les pins 7 et 8 de la Arduino.
RFID rfid_card(7, 8);

void init()
{
  // Initialisation du capteur RFID et de la communication serial.
  rfid_card.init();
  delay(10);
  rfid_card.halt();
}

void loop()
{
  // On mets à jour les informations de la carte.
  rfid_card.update();

  // Maintenant on peut récupérer des informations!
  // Vérifier qu'il y a une carte.
  if (rfid_card.has_card_present()) {
    // On peut vérifier si le badge vient juste d'être posé sur le capteur, ce qui peut éviter que le code se répète à l'infini pour une action quand on laisse le badge RFID sur le capteur.
    if (rfid_card.has_new_card()) {
      // Une nouveau badge est détecté.
      Serial.println("Une nouveau badge a été détecté!");
    } else {
      // La carte est détecté mais elle était déjà présente sur le capteur.
    }

    // Pour récupérer l'identifiant c'est simple et un objet a été fait pour:
    RFID_ID identifiant = rfid_card.get_current_id();
    // On peut le récupérer en tant que chaîne de caractères avec to_string():
    Serial.println("Identifiant: " + identifiant.to_string());
    // Si on a un autre objet identifiant on peut alors les comparer:
    RFID_ID id2(0x98, 0xC0, 0x6D, 0xDE);
    if (identifiant == id2) {
      Serial.println("L'identifiant est le même que id2!");
    } else Serial.println("L'identifiant n'est pas le même que id2.");

    // Attention! des erreurs peuvent survenir dans l'identifiant et cela peut être causé par le capteur!
    // Les identifiants "0000" et "FFFF" doivent être considéré comme des erreurs!
  }

  // On doit attendre obligatoirement 100ms pour que la carte puisse fournir les informations entre chaque boucle.
  delay(100);
}
