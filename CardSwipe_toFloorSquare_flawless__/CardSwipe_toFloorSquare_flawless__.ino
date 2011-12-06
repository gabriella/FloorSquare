#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>
//#include "RTClib.h"
#include <SD.h>

const int LEDRED = 13;
const int LEDGREEN = 12;

int cld1Pin = 5;            // Card status pin
int rdtPin = 2;             // Data pin
int reading = 0;            // Reading status
volatile int buffer[400];   // Buffer for data
volatile int i = 0;         // Buffer counter
volatile int bit = 0;       // global bit
int cardData[40];          // holds card info
int charCount = 0;          // counter for info
int DEBUG = 0;

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x03 };
//IPAddress server(69,89,31,63); // my IP server
IPAddress server(75,101,163,44);//75.101.163.44
const int requestInterval = 5000;
long lastAttemptTime = 0;            // last time you connected to the server, in milliseconds
boolean requested;
const int resetLED = 13;

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):


const int chipSelect = 4;//changed from 8
const int LOCATION_FILE_NUMBER_LSB = 0x00;
const int LOCATION_FILE_NUMBER_MSB = 0x01;

EthernetClient client;


void setup() {
  // start the serial library:
  Serial.begin(9600);

  pinMode(LEDRED, OUTPUT);
  pinMode(LEDGREEN, OUTPUT);

digitalWrite(LEDRED, HIGH);

  attachInterrupt(0, changeBit, CHANGE);
  attachInterrupt(1, writeBit, FALLING);
  pinMode(4, OUTPUT);

  Wire.begin();


  // start the Ethernet connection:
  Ethernet.begin(mac);
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");

  }
  // connectToServer();
  // give the Ethernet shield a second to initialize:
  delay(500);
  // blink(resetLED, 3);
  Serial.println("connecting...");
  connectToServer();
}

void loop()
{
  swipeCard();

  // if the server's disconnected, stop the client:
//  if (!client.connected()) {
//    Serial.println();
//    Serial.println("disconnecting.");
//
//    client.stop();
//    delay(500);
//
//    connectToServer();
//
//  }
}

void connectToServer(){
  Serial.println("connected!");
  // Serial.println("connecting to server...");
  if (client.connect(server, 80)) {
    requested = false;
    digitalWrite(LEDRED, LOW);
    digitalWrite(LEDGREEN, HIGH);
  }

  lastAttemptTime = millis();
}

boolean makeRequest() {

  digitalWrite(LEDRED, HIGH);
  digitalWrite(LEDGREEN, LOW);
  Serial.println("making request");

  client.print("POST /floorsquare/swipes/new");
  delay(100);
  String dataString = "app_key=82ac7bdb2e8ef44b5a2124f43ee05479";
  dataString+="&user_nnumber=";
  
  for (int k = 2; k < charCount-7; k = k + 1) {

    dataString+=cardData[k];
  }

  Serial.println(dataString);

  client.println(" HTTP/1.1 ");
  client.println("HOST: www.itpirl.com");
  client.print("Content-Length: ");
  client.println(dataString.length(), DEC);
  client.print("Content-Type: application/x-www-form-urlencoded\n");
  client.println("Connection: close\n");
  //client.println();
  client.println(dataString);
  client.println();

  client.stop();
  Serial.println("request made!");
  return true;

 
}

void swipeCard(){

  // Active when card present
  while(digitalRead(cld1Pin) == LOW){
    reading = 1;
  }     

  // Active when read is complete
  // Reset the buffer
  if(reading == 1) {  

    if (DEBUG == 1) {
      printBuffer();
    }

    decode();
    reading = 0;
    i = 0;

    int l;
    for (l = 0; l < 40; l = l + 1) {
      cardData[l] = '\n';
    }

    charCount = 0;
  }
}


// Flips the global bit
void changeBit(){
  if (bit == 0) {
    bit = 1;
  } 
  else {
    bit = 0;
  }
}

// Writes the bit to the buffer
void writeBit(){
  buffer[i] = bit;
  i++;
}

// prints the buffer
void printBuffer(){
  int j;
  for (j = 0; j < 200; j = j + 1) {
    Serial.println(buffer[j]);
  }
}

int getStartSentinal(){
  int j;
  int queue[5];
  int sentinal = 0;

  for (j = 0; j < 400; j = j + 1) {
    queue[4] = queue[3];
    queue[3] = queue[2];
    queue[2] = queue[1];
    queue[1] = queue[0];
    queue[0] = buffer[j];

    if (DEBUG == 1) {
      Serial.print(queue[0]);
      Serial.print(queue[1]);
      Serial.print(queue[2]);
      Serial.print(queue[3]);
      Serial.println(queue[4]);
    }

    if (queue[0] == 0 & queue[1] == 1 & queue[2] == 0 & queue[3] == 1 & queue[4] == 1) {
      sentinal = j - 4;
      break;
    }
  }

  if (DEBUG == 1) {
    Serial.print("sentinal:");
    Serial.println(sentinal);
    Serial.println("");
  }

  return sentinal;
}

void decode() {
  int sentinal = getStartSentinal();
  int j;
  int i = 0;
  int k = 0;
  int thisByte[5];

  for (j = sentinal; j < 400 - sentinal; j = j + 1) {
    thisByte[i] = buffer[j];
    i++;
    if (i % 5 == 0) {
      i = 0;
      if (thisByte[0] == 0 & thisByte[1] == 0 & thisByte[2] == 0 & thisByte[3] == 0 & thisByte[4] == 0) {
        break;
      }
      printMyByte(thisByte);
    }
  }

  //Serial.print("N");
  for (k = 2; k < charCount-7; k = k + 1) {
    Serial.print(cardData[k]);


  }
  Serial.println("");
 connectToServer();
  if(client.connected()){

    delay(200);
    if(!requested){

      requested = makeRequest(); 
      Serial.println("requesting!");
    }
  }

  // connectToServer();
}



void printMyByte(int thisByte[]) {
  int i;
  for (i = 0; i < 5; i = i + 1) {
    if (DEBUG == 1) {
      Serial.print(thisByte[i]);
    }
  }
  if (DEBUG == 1) {
    Serial.print("\t");
    Serial.print(decodeByte(thisByte));
    Serial.println("");
  }


  cardData[charCount] = decodeByte(thisByte);
  charCount ++;
}

int decodeByte(int thisByte[]) {
  if (thisByte[0] == 0 & thisByte[1] == 0 & thisByte[2] == 0 & thisByte[3] == 0 & thisByte[4] == 1){
    return 0;
  }
  if (thisByte[0] == 1 & thisByte[1] == 0 & thisByte[2] == 0 & thisByte[3] == 0 & thisByte[4] == 0){
    return 1;
  }

  if (thisByte[0] == 0 & thisByte[1] == 1 & thisByte[2] == 0 & thisByte[3] == 0 & thisByte[4] == 0){
    return 2;
  }

  if (thisByte[0] == 1 & thisByte[1] == 1 & thisByte[2] == 0 & thisByte[3] == 0 & thisByte[4] == 1){
    return 3;
  }

  if (thisByte[0] == 0 & thisByte[1] == 0 & thisByte[2] == 1 & thisByte[3] == 0 & thisByte[4] == 0){
    return 4;
  }

  if (thisByte[0] == 1 & thisByte[1] == 0 & thisByte[2] == 1 & thisByte[3] == 0 & thisByte[4] == 1){
    return 5;
  }

  if (thisByte[0] == 0 & thisByte[1] == 1 & thisByte[2] == 1 & thisByte[3] == 0 & thisByte[4] == 1){
    return 6;
  }

  if (thisByte[0] == 1 & thisByte[1] == 1 & thisByte[2] == 1 & thisByte[3] == 0 & thisByte[4] == 0){
    return 7;
  }

  if (thisByte[0] == 0 & thisByte[1] == 0 & thisByte[2] == 0 & thisByte[3] == 1 & thisByte[4] == 0){
    return 8;
  }

  if (thisByte[0] == 1 & thisByte[1] == 0 & thisByte[2] == 0 & thisByte[3] == 1 & thisByte[4] == 1){
    return 9;
  }

  if (thisByte[0] == 0 & thisByte[1] == 1 & thisByte[2] == 0 & thisByte[3] == 1 & thisByte[4] == 1){
    return -1;
  }

  if (thisByte[0] == 1 & thisByte[1] == 1 & thisByte[2] == 0 & thisByte[3] == 1 & thisByte[4] == 0){
    return -1;
  }

  if (thisByte[0] == 0 & thisByte[1] == 0 & thisByte[2] == 1 & thisByte[3] == 1 & thisByte[4] == 1){
    return -1;
  }

  if (thisByte[0] == 1 & thisByte[1] == 0 & thisByte[2] == 1 & thisByte[3] == 1 & thisByte[4] == 0){
    return -2;
  }

  if (thisByte[0] == 0 & thisByte[1] == 1 & thisByte[2] == 1 & thisByte[3] == 1 & thisByte[4] == 0){
    return -1;
  }

  if (thisByte[0] == 1 & thisByte[1] == 1 & thisByte[2] == 1 & thisByte[3] == 1 & thisByte[4] == 1){
    return -1;


  }
}
//add refresh to site
//parse it
//reading magnetic swipes 
//document it
//WATCHDOG

