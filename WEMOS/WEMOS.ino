/*
 *  Envoie du data d'un capteur de couleur depuis un Wemos à une Google Sheets pour effectuer de l'apprentissage supervisé
 *  Send data from a color sensor from a Wemos to a Google Sheets for Supervised machine learning
 *  
 *  
 *  Pour que le code fonctionne, vous devez installer les drivers pour le wemos, visitez ce lien.
 *  For the code to work, you need the drivers for the wemos, here is the link :
 *  https://wiki.wemos.cc/downloads
 *  
 *  Vous avez aussi besoin d'installer le package pour pouvoir utiliser le wemos. Dans les préférences, entrer le lien URL suivant dans le URL de gestionnaires de cartes supplémentaires. 
 *  You also need to install the Wemos package in the Arduino IDE. In the preferences window, enter the following URL in the Additional Board Manager URL field :
 *  http://arduino.esp8266.com/versions/2.3.0/package_esp8266com_index.json
 *  
 *  Après, aller dans Outils, Type de carte, Gestionnaires de carte et installer la plateforme esp8266
 *  After, go to tools, Board menum Board manager and install the esp8266.
 *  
 *  Pour des informations supplémentaires visitez - For additional info visit : 
 *  https://wiki.wemos.cc/tutorials:get_started:get_started_in_arduino
 *  
 *  
 *  
 *  Le capteur de couleur utilisé est - The color sensor used is : https://www.adafruit.com/product/1334
 *  
 *  Pour le capteur, il faut installer les librairie, aller dans Croquis, Inclure une librarie, Gestionnaire de librairies et installer la librairie pour le TCS34725 de Adafruit.
 *  For the sensor, we need to install the library, go to Sketch, Include library, Manage libraries and install the library for the TCS34725 from Adafruit.
 *  
 *  Pour des informations supplémentaires visitez - For additional info visit : 
 *  https://learn.adafruit.com/adafruit-color-sensors/arduino-code
 *  
 *  
 *  
 *  Le circuit - the circuit:
 *   - D1 => SCL TCS34725 
 *   - D2 => SDA TCS34725
 *   - D3 => INT TCS34725
 *   - 3V3 => 3V3 TCS34725
 *   - GND => GND TCS34725
 *   - D5 => un interrupteur jaune - a yellow switch
 *   - D6 => un interrupteur bleu - a blue switch
 *   - D7 => un interrupteur rouge - a red switch
 *   - D8 => un interrupteur blanc - a white switch
 *   - GND => chaque autre pin de chacun des interrupteur - each other pin of all the switches
 *
 *
 *  Créer le 18 janvier 2019
 *  Par Frédéric Larochelle
 *
 *  Created 18 January 2019
 *  By Frédéric Larochelle
 *  
 */



// Importe les librairies - Import the libraries
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h"


// Variable constante qui ne change pas - constant variable that won't change
// *** Se référer à la conversion au pinout en arduino du Wemos - Refer to the arduino Wemos pinout ***
const int YELLOW_BUTTON_PIN = 16;    // pin que le bouton jaune est attaché - pin that the yellow button is attached to
const int BLUE_BUTTON_PIN = 14;    // pin que le bouton bleu est attaché - pin that the blue button is attached to
const int RED_BUTTON_PIN = 12;   // pin que le bouton rouge est attaché - pin that the red button is attached to
const int NOTHING_BUTTON_PIN = 13;   // pin que le bouton clear est attaché - pin that the clear button is attached to

// variables non-constante - non-constant variables
int buttonStateYellow = 0;    // état du bouton jaune - state of the yellow switch
int buttonStateBlue = 0;   // état du bouton bleu - state of the blue switch
int buttonStateRed = 0;    // état du bouton rouge - state of the red switch
int buttonStateNothing = 0;    // état du bouton clear - state of the clear switch

int buttonLastStateYellow = 0;    // dernier état du bouton jaune - last state of the yellow switch
int buttonLastStateBlue = 0;   // dernier état du bouton bleu - last state of the blue switch
int buttonLastStateRed = 0;    // dernier état du bouton rouge - last state of the red switch
int buttonLastStateNothing = 0;    // dernier état du bouton clear - last state of the clear switch


// String qui stock la couleur - String to store the color value
String color;


// Si il faut envoyer des données au Google Sheets à ce tour - If we need to send data to the google sheet this turn
bool siEnvoyer = false;


/* 
 *  Valeur légale de temps d'intégration et de gain pour le capteur - Legal integration time value and gain value for the sensor
 *  
 *  Plus la temps d'intégration est élever plus la mesure sera précise
 *  The longer the integration time, the more accurate the measurement is
 *  
 *  Plus le gain est élever plus il y a de bruit dans les mesures, mais un gain éléver peut être nécessaire dans des condition de basse lumière
 *  The higher the gain is, the more noise there is in the measurements, but a higher gain value might be necessary for low light condition
 *  
 *  
 *  Temps d'intégration - Integration Time :
 *    TCS34725_INTEGRATIONTIME_2_4MS
 *    TCS34725_INTEGRATIONTIME_24MS
 *    TCS34725_INTEGRATIONTIME_50MS
 *    TCS34725_INTEGRATIONTIME_101MS
 *    TCS34725_INTEGRATIONTIME_154MS
 *    TCS34725_INTEGRATIONTIME_700MS
 *  
 *  Gain :
 *    TCS34725_GAIN_1X
 *    TCS34725_GAIN_4X
 *    TCS34725_GAIN_16X
 *    TCS34725_GAIN_60X
 *  
 *  Pour plus d'information visiter - For more information visit :
 *  https://learn.adafruit.com/adafruit-color-sensors/library-reference
 *  
 */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);
uint16_t r, g, b, c;    // Variable à remplir avec les données du capteur - Variable to fill with the sensor data


// Nom et mot de passe du réseau - Name and password for the network
const char* ssid = "VOTRE NOM DE WIFI - YOUR WIFI NAME";
const char* password = "VOTRE MOT DE PASSE DE WIFI - YOUR WIFI PASSWORD";


// Utilise la classe WifiClientSecure pour créer une conenction TLS sécurisé - Use WiFiClientSecure class to create TLS connection
WiFiClientSecure client;


// Adresse serveur Google Script - Google Script Server Address
const char* host = "script.google.com";
const int httpsPort = 443;


// Identifiant pour le web app de Google - Identifier for the Google web app
const char* fingerprint = "46 B2 C3 44 9C 59 09 8B 01 B6 F8 BD 4C FB 00 74 91 2F EF F6";    // À moins qu'on veut avoir une trace des utilisateurs qui utilisent notre web app de Google, on peut simplement garder cet identifiant général - Unless we want to have a record of users who use our Google web app, we can just keep this general identifier
String GAS_ID = "VOTRE GAS_ID - YOUR GAS_ID";   // Remplacer par le GAS de votre web app = Replace with the GAS of your web app


// Préparation du programme - Program setup
void setup() {
  // Active le port monitor avec un baudrate de 115200 - Start the serial monitor with a baudrate of 115200 
  Serial.begin(115200);

  // Connexion au réseau wifi - Connection to the wifi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Montre l'adresse IP du Wemos dans le moniteur serial - Show the IP adress of the Wemos on the Serial monitor
  Serial.println("Adresse IP - IP address : ");
  Serial.print(WiFi.localIP());
  Serial.println("");

  //Initialise les bouttons en tant que input - Initialize the switches as input
  pinMode(YELLOW_BUTTON_PIN, INPUT);
  pinMode(BLUE_BUTTON_PIN, INPUT);
  pinMode(RED_BUTTON_PIN, INPUT);
  pinMode(NOTHING_BUTTON_PIN, INPUT);
  
}



// Loop principale - Main loop
void loop() {
  // Récupére les messures du capteur de couleur = Retrieve the measurements from the color sensor 
  tcs.getRawData(&r, &g, &b, &c);


  // délai pour que le capteur puisse effectuer une mesure - delay for the sensor to take a measurement
  delay(750);

  
  // lit les pins des boutons - read the pushbuttons input pins
  buttonStateYellow = digitalRead(YELLOW_BUTTON_PIN);
  buttonStateBlue = digitalRead(BLUE_BUTTON_PIN);
  buttonStateRed = digitalRead(RED_BUTTON_PIN);
  buttonStateNothing = digitalRead(NOTHING_BUTTON_PIN);
  

  // compare l'état actuel du bouton au précèdent - Compare the present state of the button with the last one
  if( buttonStateYellow != buttonLastStateYellow) {
    if(buttonStateYellow = HIGH) {    // Si le bouton est appuyer - if the button is pressed
      color = "yellow";   // couleur de cette mesure - color for this measurement
      siEnvoyer = true;   // place le bool siEnvoyer à envoyer - set the bool siEnvoyer to sent
    }
  } else if( buttonStateBlue != buttonLastStateBlue ) {
      if(buttonStateBlue = HIGH) {    // Si le bouton est appuyer - if the button is pressed
        color = "blue";   // couleur de cette mesure - color for this measurement
        siEnvoyer = true;   // place le bool siEnvoyer à envoyer - set the bool siEnvoyer to sent
      }
  } else if( buttonStateRed != buttonLastStateRed ) {
      if(buttonStateRed = HIGH) {    // Si le bouton est appuyer - if the button is pressed
        color = "red";    // couleur de cette mesure - color for this measurement
        siEnvoyer = true;   // place le bool siEnvoyer à envoyer - set the bool siEnvoyer to sent
      }
  } else if( buttonStateNothing != buttonLastStateNothing ) {
      if(buttonStateNothing = HIGH) {    // Si le bouton est appuyer - if the button is pressed
        color = "empty";   ;// couleur de cette mesure - color for this measurement
        siEnvoyer = true;   // place le bool siEnvoyer à envoyer - set the bool siEnvoyer to sent
      }
  }


  // Si plus que un bouton est appuyer / If more than one button is pressed
  if(buttonStateYellow + buttonStateBlue + buttonStateRed + buttonStateNothing > 1 ) {
    siEnvoyer = false;
  }

  
  // sauvegarde l'état actuel pour la prochaine fois = save the current state for the next time
  buttonLastStateYellow =  buttonStateYellow;
  buttonLastStateBlue =  buttonStateBlue;
  buttonLastStateRed =  buttonStateRed;
  buttonLastStateNothing =  buttonStateNothing;

  // Envoie les donnnées du capteur au Google Sheets - Sent the sensor data to the Google Sheets
  if(siEnvoyer) {
    sendData(r, g, b, c, color);
  }


  // réinitialise pour le prochain tour - reset for next time
  siEnvoyer = false;


  // petit délai pour éviter les rebonds des boutons - small delay to stop button bouncing
  delay(100);
}



// Fonction pour envoyer des données au Google Sheet - Function to send data into Google Sheet
void sendData(float rFinal, float gFinal, float bFinal, float cFinal, String color){

  // Convertit les float en String - Convert float to String
  String string_red = String(rFinal, DEC); 
  String string_green = String(gFinal, DEC);
  String string_blue = String(bFinal, DEC);
  String string_clear = String(cFinal, DEC);


  // Remplit le URL avec nos données du capteur - Fill the URL with our sensor data
  String url = "/macros/s/" + GAS_ID + "/exec?red=" + string_red + "&green=" + string_green + "&blue=" + string_blue + "&clear=" + string_clear + "&tag=" + color;


  // Envoie le URL - Sent the URL
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");
 
} 
