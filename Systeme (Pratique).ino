# include <stdio.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <Stepper.h>
#include <MFRC522.h>
#include <string.h>


#define READ 0// read rfid
#define WRITE 1// write rfid


//-----------------i/o functions--------------------------------------------//

int serial_putc(char c, FILE *);
void printf_begin(void);
void scanString(char *T, int taille);
int scanInt();

//-----------------structures----------------------------------------------//

typedef struct
{
    char nom[10];
    char id[15];
    byte hasPassedRfid_n_go[2];//Pour savoir si le baggage est passee par un rfid
    byte hasPassedRfid_n_back[2];
}BasicBaggage;


typedef struct
{
    BasicBaggage *baggageList;
    int nombreBaggage;
}Baggage;

//-------------------functions prototype-----------------------------------//

/**
 * resize B.baggageList
 * enter new luggage info
 * return the new Baggage
 */
Baggage ajouterBaggage(Baggage B);

/**
 * show all informations about luggages
 */
void afficherBaggage(Baggage B);

/**
 * show all missing luggage and return their number
 */
byte afficherBaggagePerdu(Baggage B, byte MODE);

/**
 * to fill all the luggage informations
 */
int check_in(Baggage *B);

/**
 * function for loading
 * modify basicBaggage.hasPassedRfid_n_go to 1 if a luggage has passed throught one
 * return the modified struct Baggage
 */
Baggage loading(Baggage B, byte SS1_PIN, byte SS2_PIN, byte RST_PIN);//function for loading

/*
 * the same as loading() but inverted, rfid 2 goeas before rfid 1
 * and modify basicBaggage.hasPassedRfid_n_back to 1
 */
Baggage unloading(Baggage B, byte SS1_PIN, byte SS2_PIN, byte RST_PIN);//function for unloading

/**
 * MODE: 1/loading 2/unloading
 * to check if some baggage are missing
 * return the indice of the first missing baggage if there is and -1 otherwise
 */
byte check_baggage(Baggage B, byte mode, byte number_of_rfid);

/**
 * read an rfid
 * ID: a string containing the read id
 * RFID: to select an rfid
 */
byte read_rfid(char *ID, byte RFID);//TO DO: implement rfid object instead of byte

/**
 * read "0"  or write "1"  a rfid
 * return 1 if there is a problem
 */
byte rw_rfid(byte SS_PIN, byte RST_PIN, byte MODE, char *DATA);


//--------------------globale variables----------------------------------//

//PIN
const byte rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;//<- for the lcd
const byte ss1 = 10, ss2 = 9, rst = 8;//<- for the rfids

//SIMPLE
byte choice = 0;//<- to select the mode
byte check = 0;//<- to know whether the luggages should be checked or not
const int buttonpin = A0;
const int ledpin = A1;
const int motorSpeed = 50;
const int stepsPerRevolution = 200;
Baggage C;

//OBJECT
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
Stepper stepper_normal(stepsPerRevolution, A2, A3, A4, A5);
Stepper stepper_reverse(stepsPerRevolution, A5, A3, A2, A4);


//----------------------main--------------------------------------------//

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  printf_begin();
  lcd.begin(16, 2);
  lcd.print("Welcome!");

  while(! Serial);
  Serial.println("Welcome!");
  
  //Initilise the SS pins to high
  pinMode(ss1, OUTPUT);
  pinMode(ss2, OUTPUT);
  pinMode(ledpin, OUTPUT);
  pinMode(buttonpin, INPUT);
  digitalWrite(ss1, HIGH);
  digitalWrite(ss2, HIGH);
  digitalWrite(buttonpin, HIGH);//initialize the buttonpin to HIGH to avoid syncing problem
  
  SPI.begin();

  C.nombreBaggage = 0;
}

void loop() {
  // put your main code here, to run repeatedly:
  int value  = digitalRead(buttonpin);
  if(value == LOW){
    delay(500);
    value = digitalRead(buttonpin);
    //if the value is still low after 0.5s then change mode
    if(value == LOW){
      //set choice to not have a value above 4
      if(choice == 4){
        choice = 0;
      }else{
        choice++;
      }
    }
  }
  
  switch(choice){
    case 1:
      lcd.clear();
      lcd.print("Check-in MODE");
      
      if(check_in(&C, ss1, rst) == 1){
        choice = 2;
      }
      break;
      
    case 2:
      lcd.clear();
      lcd.print("LoadinG MODE");
      C = loading(C, ss1, ss2, rst);
      break;

      
    case 3:
      if(afficherBaggagePerdu(C, 1) != 0 && check == 0){
          byte test = check_baggage(C, 1, 2);//<- to put the indice of the missing luggage
          Serial.println(test);
          digitalWrite(ledpin, HIGH);
          lcd.clear();
          lcd.print("!LUGGAGES LOST!");

          //show the missing luggage info
          Serial.println("Des baggages ont ete voles");
          byte baggagePerdu = afficherBaggagePerdu(C, 1);
          lcd.setCursor(0, 1);
          lcd.print("Nombre: ");
          lcd.print(baggagePerdu);
          Serial.println("Continuer le chargement? 1:OUI 2:NON");
          if(scanInt() == 1){
            check = 1;
          }else{
            choice = 0;
          }
          digitalWrite(ledpin, LOW);
        }
      lcd.clear();
      lcd.print("UnloadinG MODE");
      C = unloading(C, ss1, ss2, rst);
        
      break;

    case 4:
      if(afficherBaggagePerdu(C, 2) != 0 != 0){
        byte test = check_baggage(C, 1, 2);//<- to put the indice of the missing luggage
        digitalWrite(ledpin, HIGH);
        lcd.clear();
        lcd.print("!LUGGAGES LOST!");

        //show the missing luggage info
        Serial.println("Des baggages ont ete voles");
        byte baggagePerdu = afficherBaggagePerdu(C, 1);
        lcd.setCursor(0, 1);
        lcd.print("Nombre: ");
        lcd.print(baggagePerdu);
        Serial.println("Continuer ? 1:OUI 2:NON");
        if(scanInt() == 1){
          choice = 0;
        }
        digitalWrite(ledpin, LOW);
      }else{
        choice = 0;
      }
      break;

    default:
      free(C.baggageList);
      C.nombreBaggage = 0;
      lcd.clear();
      lcd.print("Welcome!");
  }
  delay(200);
}





//-----------------------function definitions------------------------------//


int serial_putc(char c, FILE *) 
{
  Serial.write(c);
  return c;
} 

void printf_begin(void)
{
  fdevopen(&serial_putc, 0);
}

//---------------------

void scanString(char *buff, int taille)
{
  byte index = 0;
  bool ended = false;
  while(ended == false){
     while(Serial.available() > 0){
        if(index < taille){
           char letter = Serial.read();
           if(letter != '\n' && letter != '\r'){
              buff[index++] = letter;
              buff[index] = '\0';
           }
           else{
              ended = true;
           }
        }
     }
  }  
}

int scanInt()
{
  char buff[8];
  int value = 0;
  bool ended = false;
  while(ended == false){
    scanString(buff, 8);
    value = atoi(buff);
    if(value != 0){
      ended = true;
    }
  }
  return value;
}

//----------------





Baggage ajouterBaggage(Baggage B, byte rfid_ss, byte rfid_rst)
{
  char buf[15] = {'\0'};
  char data[15] = {'\0'};

  //if no rfid is detected return
  while(rw_rfid(10, 8, READ, buf) == 1){
    Serial.println("RFID non detectee, veuillez bien le mettre en place");
    delay(1000);
  }
  //resize the baggage list
  if (B.nombreBaggage == 0){
    B.baggageList = (BasicBaggage *) malloc((++B.nombreBaggage)*sizeof(BasicBaggage));
  }
  else{
    realloc(B.baggageList, sizeof(BasicBaggage)*(++B.nombreBaggage));//ajouter un emçlacement de baggage
  }

  //scan info
  Serial.println("Enter le nom: ");
  scanString(B.baggageList[B.nombreBaggage - 1].nom, 10);
  Serial.println("Entrer l' ID: ");
  scanString(data, 15);
  
  //write the id into the rfid
  while(rw_rfid(10, 8, WRITE, data) == 1){
    Serial.println("Erreur lors de l'ecriture dans RFID");
    Serial.println("Veuillez bien le mettre en place");
    delay(500);
  }
  strcpy(B.baggageList[B.nombreBaggage - 1].id, data);

  for(byte i=0; i<2; i++){
    B.baggageList[B.nombreBaggage - 1].hasPassedRfid_n_go[i] = 0;
    B.baggageList[B.nombreBaggage - 1].hasPassedRfid_n_back[i] = 0;
  }
  return B;
}


byte afficherBaggagePerdu(Baggage B, byte MODE){
  byte nombreBaggagePerdu = 0;
  for(byte i=0; i<B.nombreBaggage; i++){
    for(byte j=0; j<2; j++){
      switch(MODE){
        case 1://loading mode
          if(B.baggageList[i].hasPassedRfid_n_go[j] == 0){
            printf(" *Baggage No %d:",i+1);
            Serial.println();
            printf("    Nom: %s",B.baggageList[i].nom);
            Serial.println();
            printf("    ID: %s",B.baggageList[i].id);
            Serial.println();
            ++nombreBaggagePerdu;
            ++i;
          }
          break;
          
        case 2:
          if(B.baggageList[i].hasPassedRfid_n_back[j] == 0){
            printf("Baggage No %d:",i+1);
            Serial.println();
            printf("  Nom: %s",B.baggageList[i].nom);
            Serial.println();
            printf("  ID: %s",B.baggageList[i].id);
            Serial.println();
            ++nombreBaggagePerdu;
            ++i;
          }
          break;
      }     
    }
  }
  return nombreBaggagePerdu;
}


void afficherBaggage(Baggage B)
{
  for(int i=0; i<B.nombreBaggage; i++)
  {
    Serial.println("Voici les baggages: ");
    printf("Baggage No %d:",i+1);
    Serial.println();
    printf("  Nom: %s",B.baggageList[i].nom);
    Serial.println();
    printf("  ID: %s",B.baggageList[i].id);
    Serial.println();
    printf("  Rfid1-go: %d",B.baggageList[i].hasPassedRfid_n_go[0]);
    Serial.println();
    printf("  Rfid2-go: %d",B.baggageList[i].hasPassedRfid_n_go[1]);
    Serial.println();
    printf("  Rfid1-back: %d",B.baggageList[i].hasPassedRfid_n_back[0]);
    Serial.println();
    printf("  Rfid2-back: %d",B.baggageList[i].hasPassedRfid_n_back[1]);
    Serial.println();
  }
}




int check_in(Baggage *B, byte rfid_ss, byte rfid_rst){
  int fin = 0;
  while(fin == 0){
    *B = ajouterBaggage(*B, rfid_ss, rfid_rst);
    Serial.println();
    Serial.println("En ajouter plus? 2:OUI/1:NON");
    char temp = scanInt();
    if(temp == 1){
      break;
    }
  }
  afficherBaggage(*B);
  return 1;
}


byte read_rfid(char *ID, byte RFID){
  switch(RFID){
    case 1:
      Serial.println("RFID 1:");
      scanString(ID, 15);
      if(ID[0] == '0') 
        return 1;  
      break;
    case 2:
      Serial.println("RFID 2:");
      scanString(ID, 15);
      if(ID[0] == '0')
        return 1;
      break;
  }
  return 0;
}


Baggage loading(Baggage B, byte SS1_PIN, byte SS2_PIN, byte RST_PIN){
  
  char id_temp1[15] = {'\0'};//<- to store the read id from rfid 1
  char id_temp2[15] = {'\0'};//<- to store the read id from rfid 2
  //set motor
  stepper_normal.setSpeed(motorSpeed);
  stepper_normal.step(stepsPerRevolution / 100);
  //for rfid 1
  Serial.print("rfid 1:");
  rw_rfid(SS1_PIN, RST_PIN, READ, id_temp1);
  Serial.println();
  //mofify B.baggageList[i].hasPassedRfid_n_go if the id is found
  for(byte i=0; i< B.nombreBaggage; i++){
    if(strcmp(id_temp1, B.baggageList[i].id) == 0){
      B.baggageList[i].hasPassedRfid_n_go[0] = 1;
    }
  }

  //for rfid 2
  Serial.print("rfid 2:");
  rw_rfid(SS2_PIN, RST_PIN, READ, id_temp2);
  Serial.println();
  for(byte i=0; i< B.nombreBaggage; i++){
    if(strcmp(id_temp2, B.baggageList[i].id) == 0){
      B.baggageList[i].hasPassedRfid_n_go[1] = 1;
    }
  }
  //afficherBaggage(B);
  delay(1000);
  return B;
}




Baggage unloading(Baggage B, byte SS1_PIN, byte SS2_PIN, byte RST_PIN){
  
  char id_temp1[15] = {'\0'};//<- to store the read id from rfid 1
  char id_temp2[15] = {'\0'};//<- to store the read id from rfid 2
  
  //set motor inverted
  stepper_reverse.setSpeed(motorSpeed);
  stepper_reverse.step(stepsPerRevolution / 100);
  //for rfid 1
  Serial.print("rfid 1:");
  rw_rfid(SS1_PIN, RST_PIN, READ, id_temp1);
  Serial.println();
  //mofify B.baggageList[i].hasPassedRfid_n_go if the id is found
  for(byte i=0; i< B.nombreBaggage; i++){
    if(strcmp(id_temp1, B.baggageList[i].id) == 0){
      B.baggageList[i].hasPassedRfid_n_back[0] = 1;
    }
  }

  //for rfid 2
  Serial.print("rfid 2:");
  rw_rfid(SS2_PIN, RST_PIN, READ, id_temp2);
  Serial.println();
  for(byte i=0; i< B.nombreBaggage; i++){
    if(strcmp(id_temp2, B.baggageList[i].id) == 0){
      B.baggageList[i].hasPassedRfid_n_back[1] = 1;
    }
  }
  //afficherBaggage(B);
  delay(1000);
  return B;
}





byte check_baggage(Baggage B, byte mode, byte number_of_rfid){
  //loop over luggages
  for(byte i=0; i<  B.nombreBaggage; i++){
    switch(mode){
      //loading mode
      case 1:
        //check if every luggages has passed by rfids
        for(byte j=0; j< number_of_rfid; j++){
          if(B.baggageList[i].hasPassedRfid_n_go[j] == 0){
            return i;
          }
        }
        break;

      //unloading mode
      case 2:
        for(byte j=0; j< number_of_rfid; j++){
          if(B.baggageList[i].hasPassedRfid_n_back[j] == 0){
            return i;
          }
        }
        break;
      
    }
  }
  return -1;
}



byte rw_rfid(byte SS_PIN, byte RST_PIN, byte MODE, char *DATA){
  
    MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
    MFRC522::MIFARE_Key key;//initialize the key
    SPI.begin();// Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card

    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return 1;
    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
        return 1;

    // Show some details of the PICC (that is: the tag/card)
    Serial.print(F("Card UID:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    //Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    //Serial.println(mfrc522.PICC_GetTypeName(piccType));

    // Check for compatibility
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("This sample only works with MIFARE Classic cards."));
        return 1;
    }

    // that is: sector #1, covering block #4 up to and including block #7
    byte sector         = 1;//<- sector to write into
    byte blockAddr      = 4;//<- starting block
    byte trailerBlock   = 7;//<- ending block
    MFRC522::StatusCode status;
    
    byte dataBlock[16] = {(byte)'\0'};//<- data to write in the rfid
    strcpy(dataBlock, DATA);
    byte buffer[18];//<- to put the data read from the rfid
    byte size = sizeof(buffer);

    // Authenticate using key A for read
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return 1;
    }
    // Read data from the block
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }

    //if MODE = 1
    if(MODE == 1){
      // Authenticate using key B for write
      status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
      if (status != MFRC522::STATUS_OK) {
          Serial.print(F("PCD_Authenticate() failed: "));
          Serial.println(mfrc522.GetStatusCodeName(status));
          return 1;
      }
      // Write data to the block
      status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
      if (status != MFRC522::STATUS_OK) {
          Serial.print(F("MIFARE_Write() failed: "));
          Serial.println(mfrc522.GetStatusCodeName(status));
      }
      
    }else{
      //copy buffer into DATA
      strcpy(DATA, buffer);
    }
      
    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();

    return 0;
}
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
