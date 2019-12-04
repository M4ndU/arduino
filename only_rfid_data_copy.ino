//스케치는 프로그램 저장 공간 9272 바이트(30%)를 사용. 최대 30720 바이트.
//전역 변수는 동적 메모리 1436바이트(70%)를 사용, 612바이트의 지역변수가 남음.  최대는 2048 바이트.
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <MFRC522.h>



#define NEO_PIN 6
#define cdsPin A6
#define buttonPin A3  // select the input pin for the potentiometer
#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above
#define MENU_STR "button 1.R/W 2.mode"


Adafruit_NeoPixel neoPixel = Adafruit_NeoPixel(2, NEO_PIN, NEO_GRB + NEO_KHZ800);
const PROGMEM int8_t neo_color[][3] = {{32, 32,32}, {0, 0, 32}, {0, 32, 0}, {32, 21, 0}, {32, 0, 0}, {16, 0, 0}, {0, 8, 0}};

int8_t neo_color_index=0;
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

byte buffer[18];
byte block;
byte waarde[64][16];
MFRC522::StatusCode status;

MFRC522::MIFARE_Key key;

// Number of known default keys (hard-coded)
// NOTE: Synchronize the NR_KNOWN_KEYS define with the defaultKeys[] array
#define NR_KNOWN_KEYS   8
// Known keys, see: https://code.google.com/p/mfcuk/wiki/MifareClassicDefaultKeys
byte knownKeys[NR_KNOWN_KEYS][MFRC522::MF_KEY_SIZE] =  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, // FF FF FF FF FF FF = factory default
    {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5}, // A0 A1 A2 A3 A4 A5
    {0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5}, // B0 B1 B2 B3 B4 B5
    {0x4d, 0x3a, 0x99, 0xc3, 0x51, 0xdd}, // 4D 3A 99 C3 51 DD
    {0x1a, 0x98, 0x2c, 0x7e, 0x45, 0x9a}, // 1A 98 2C 7E 45 9A
    {0xd3, 0xf7, 0xd3, 0xf7, 0xd3, 0xf7}, // D3 F7 D3 F7 D3 F7
    {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}, // AA BB CC DD EE FF
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // 00 00 00 00 00 00
};

byte newUid[10] = {0xDE, 0xAD, 0xBE, 0xEF};

bool IsRfidWriteMode = false;


//==================== start setup ====================
// This section of code runs only once at start-up.
void setup() {
  pinMode(cdsPin, INPUT);
  pinMode(buttonPin, INPUT);
  Serial.begin(9600);

  while (!Serial);            // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();                // Init SPI bus
  mfrc522.PCD_Init();         // Init MFRC522 card
  Serial.println(MENU_STR);

  neoPixel.begin();
  neoPixel.show();
}
//==================== end setup ====================



//==================== start rfid func ====================

 //Via seriele monitor de bytes uitlezen in hexadecimaal

void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
//Via seriele monitor de bytes uitlezen in ASCI

void dump_byte_array1(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.write(buffer[i]);
  }
}

/*
 * Try using the PICC (the tag/card) with the given key to access block 0 to 63.
 * On success, it will show the key details, and dump the block data on Serial.
 *
 * @return true when the given key worked, false otherwise.
 */

bool check_status_failed(MFRC522::StatusCode ck_status){
  if (ck_status != MFRC522::STATUS_OK) {
      Serial.print(F("failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return true;
  }
  return false;
}

void end_rc522_sign(){
  mfrc522.PICC_HaltA();       // Halt PICC
  mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
}

bool try_key(MFRC522::MIFARE_Key *key)
{
    bool result = false;

    for(byte block = 0; block < 64; block++){

    // Serial.println(F("Authenticating using key A..."));
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, key, &(mfrc522.uid));
    if (check_status_failed(status)) {
        return false;
    }

    // Read block
    byte byteCount = sizeof(buffer);
    status = mfrc522.MIFARE_Read(block, buffer, &byteCount);
    if (!check_status_failed(status)) {
        // Successful read
        result = true;
        Serial.print(F("Succ key:"));
        dump_byte_array((*key).keyByte, MFRC522::MF_KEY_SIZE);
        Serial.println();

        //dump_uid
         for (int i = 0; i < mfrc522.uid.size; i++) {
          newUid[i] = mfrc522.uid.uidByte[i];
        }

        // Dump block data
        Serial.print(F("Block ")); Serial.print(block); Serial.print(F(":"));
        dump_byte_array1(buffer, 16); //omzetten van hex naar ASCI
        Serial.println();

        for (int p = 0; p < 16; p++) //De 16 bits uit de block uitlezen
        {
          waarde [block][p] = buffer[p];
          Serial.print(waarde[block][p]);
          Serial.print(" ");
        }

        }
    }
    Serial.println();

    Serial.println(MENU_STR);

    end_rc522_sign();
    return result;
}

void rfid_rw(bool writemode) {

  Serial.println(F("Insert card..."));
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
    return;
  }

  // Show some details of the PICC (that is: the tag/card)
  Serial.print(F("Card UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  if(!writemode)
  {
    Serial.println(F("Read"));
    // Try the known default keys
    MFRC522::MIFARE_Key key;
    for (byte k = 0; k < NR_KNOWN_KEYS; k++) {
        // Copy the known key into the MIFARE_Key structure
        for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) {
            key.keyByte[i] = knownKeys[k][i];
        }
        // Try the key
        if (try_key(&key)) {
            // Found and reported on the key and block,
            // no need to try other keys for this PICC
            break;
        }
    }

  }
  else
  {
    Serial.println(F("Copy to the new card"));
    for (byte i = 0; i < 6; i++) {
      key.keyByte[i] = 0xFF;
    }

    for(int i = 4; i <= 62; i++){ //De blocken 4 tot 62 kopieren, behalve al deze onderstaande blocken (omdat deze de authenticatie blokken zijn)
      if(i == 7 || i == 11 || i == 15 || i == 19 || i == 23 || i == 27 || i == 31 || i == 35 || i == 39 || i == 43 || i == 47 || i == 51 || i == 55 || i == 59){
        i++;
      }
      block = i;

        // Authenticate using key A
      Serial.println(F("Authenticating using key A..."));
      status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
      if (check_status_failed(status)) {
          return;
      }

      // Authenticate using key B
      Serial.println(F("Authenticating again using key B..."));
      status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, block, &key, &(mfrc522.uid));
      if (check_status_failed(status)) {
          return;
      }

      /*
      // Set new UID
      if ( mfrc522.MIFARE_SetUid(newUid, (byte)4, true) ) {
        Serial.println(F("Wrote new UID to card."));
      }
      */

      // Write data to the block
      Serial.print(F("Writing data into block "));
      Serial.print(block);
      Serial.println("\n");

      dump_byte_array(waarde[block], 16);


       status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(block, waarde[block], 16);
       check_status_failed(status);


       Serial.println("\n");

    }
    end_rc522_sign();
    Serial.println(MENU_STR);
  }
}
//==================== end rfid func ====================




//==================== start loop func ====================
void loop() {

  int8_t light = analogRead(cdsPin);
  int8_t brightness = map(light/2, 0, 1024, 255, 0);

  int16_t buttonValue = analogRead(buttonPin);
  if(buttonValue == 339) {
    rfid_rw(IsRfidWriteMode);
  }
  if(buttonValue == 682) {
    IsRfidWriteMode = !IsRfidWriteMode;
  }

  if (IsRfidWriteMode){
    neo_color_index = 5;
  } else{
    neo_color_index = 0; //READ MODE
    }
  neoPixel.setPixelColor(1, neoPixel.Color(neo_color[neo_color_index][0],neo_color[neo_color_index][1],neo_color[neo_color_index][2]));
  neoPixel.setBrightness(brightness);
  neoPixel.show();

}
//==================== end loop func ====================
