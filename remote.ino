#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <printf.h>

// Define pin numbers
const int powerLedPin_green = 5;
const int powerLedPin_red = 4;
const int connLedPin = 6;
const int armLedPin = 7;
const int triggrLedPin = 8;

const int safetySwitchPin = 3;
const int triggerSwitchPin = 2;

const int buzzerPin = A1;
const int batteryPin = A0;

bool readyState = false;
bool triggerState = false;

unsigned long previousMillis_buz = 0;
const long interval_buz = 300;
bool buz_sate = HIGH;
unsigned long previousMillis_snd = 0;
const long interval_snd = 100;

unsigned long lastSignalMillis = 0;
const long timeout = 1000;

unsigned long previousMillis_trggr = 0;
const long interval_trggr = 2000;

RF24 radio(9, 10);  // CE, CSN pins for nRF24L01
const byte address[6] = "00001";  // Same address for writing and listening

// define data structure for message com
struct RemoteData {
  bool remoteReady = false;
  bool remoteTrigger = false;
};
struct ReceiverData {
  bool receiverReady = false;
  bool receiverTrigger = false;
};
RemoteData dataOut;
ReceiverData dataIn;

void setup() {
  // start serial com
  Serial.begin(115200);
  printf_begin();
  
  // Initialize pins
  // outputs
  pinMode(powerLedPin_green, OUTPUT);
  pinMode(powerLedPin_red, OUTPUT);
  pinMode(connLedPin, OUTPUT);
  pinMode(armLedPin, OUTPUT);
  pinMode(triggrLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  // inputs
  pinMode(safetySwitchPin, INPUT_PULLUP);
  pinMode(triggerSwitchPin, INPUT_PULLUP);
  pinMode(batteryPin, INPUT);

  // Initialize the radio device
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(125); //select a channel (in which there is no noise!) 0 ... 125
  radio.openWritingPipe(address);
  radio.openReadingPipe(1, address);

  // Initialise active low devices
  digitalWrite(powerLedPin_red, HIGH);
  digitalWrite(buzzerPin, HIGH);
  // turn on the power led
  digitalWrite(powerLedPin_green, LOW);

  radio.printPrettyDetails();
  Serial.println(); // add blank space

  // startup chime
  buzzStartup(buzzerPin);
}

void loop() {
  unsigned long currentMillis = millis();

  // Read safety switch
  // cannot become ready if trigger is pressed
  dataOut.remoteReady = !digitalRead(safetySwitchPin);// && digitalRead(triggerSwitchPin);
  
  // Read trigger switch
  //dataOut.remoteTrigger = dataOut.remoteReady && dataIn.receiverReady && !digitalRead(triggerSwitchPin);

  // Ready state
  if (dataOut.remoteReady && dataIn.receiverReady) {
    // turn on conn led
    digitalWrite(connLedPin, HIGH);

    // turn on arm led
    digitalWrite(armLedPin, HIGH);
    
    // Beep every 300ms
    if (currentMillis - previousMillis_buz >= interval_buz) {
      previousMillis_buz = currentMillis;
      buz_sate = !buz_sate;
      digitalWrite(buzzerPin, buz_sate);
    }
  } else {
    // turn off conn led
    digitalWrite(connLedPin, LOW);
    // turn off arm led
    digitalWrite(armLedPin, LOW);
    // turn off trigger led
    digitalWrite(triggrLedPin, LOW);
    // turn off buzzer
    digitalWrite(buzzerPin, HIGH);
  }

  // Trigger
  if (dataOut.remoteReady &&
      dataIn.receiverReady &&
      !digitalRead(triggerSwitchPin) &&
      currentMillis - previousMillis_trggr >= interval_trggr)
      {
        dataOut.remoteTrigger = true;
        previousMillis_trggr = currentMillis;
      }
  else {
        dataOut.remoteTrigger = false;
      }

  // turn on connexion led
  digitalWrite(connLedPin, dataIn.receiverReady);

  // turn on trigger led
  digitalWrite(triggrLedPin, dataIn.receiverTrigger);

  //Received data too old
  if (currentMillis - lastSignalMillis > timeout) {
    // Serial.println("data too old");
    dataIn.receiverReady = false;
    dataIn.receiverTrigger = false;
  }

  //Send the data
  if (currentMillis - previousMillis_snd >= interval_snd) {
    //Serial.println("Sending data");
    radio.stopListening();
    radio.write(&dataOut, sizeof(dataOut));
    radio.startListening();
    delay(10);
  }
  // Check if new data incomming
  if (radio.available()) {
    // Serial.println("Receiving data");
    radio.read(&dataIn, sizeof(dataIn));
    // data intergrity check needed
    lastSignalMillis = currentMillis;
    delay(10);
  }
  Serial.print("Remote rdy: ");
  Serial.print(dataOut.remoteReady);
  Serial.print("\tRemote trggr: ");
  Serial.print(dataOut.remoteTrigger);
  Serial.print("\tReceiver rdy: ");
  Serial.print(dataIn.receiverReady);
  Serial.print("\tReceiver trggr: ");
  Serial.print(dataIn.receiverTrigger);
  Serial.print("\tlast signal: ");
  Serial.println(currentMillis - lastSignalMillis);
}

void buzzStartup(int buzzerPin) {
  digitalWrite(buzzerPin, LOW);
  delay(100);
  digitalWrite(buzzerPin, HIGH);
  delay(500);
  digitalWrite(buzzerPin, LOW);
  delay(80);
  digitalWrite(buzzerPin, HIGH);
  delay(80);
  digitalWrite(buzzerPin, LOW);
  delay(80);
  digitalWrite(buzzerPin, HIGH);
  delay(80);
  digitalWrite(buzzerPin, LOW);
  delay(80);
  digitalWrite(buzzerPin, HIGH);
}
