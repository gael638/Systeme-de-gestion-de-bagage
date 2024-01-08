# include <stdio.h>
#include <LiquidCrystal.h>
#include <string.h>




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

Baggage ajouterBaggage(Baggage B);

/**
 * show all informations about luggages
 */
void afficherBaggage(Baggage B);

/**
 * to fill all the luggage informations
 */
int check_in(Baggage *B);

/**
 * function for loading
 * modify basicBaggage.hasPassedRfid_n_go to 1 if a luggage has passed throught one
 * return the modified struct Baggage
 */
Baggage loading(Baggage B, byte *choice);//function for loading

/*
 * the same as loading() but inverted, rfid 2 goeas before rfid 1
 * and modify basicBaggage.hasPassedRfid_n_back to 1
 */
Baggage unloading(Baggage B, byte *choice);//function for unloading

/**
 * MODE: 1/loading 2/unloading
 * to check if some baggage are missing
 * return 1 if at least one is missing and 0 otherwise
 */
byte check_baggage(Baggage B, byte mode);

/**
 * read an rfid
 * ID: a string containing the read id
 * RFID: to select an rfid
 */
byte read_rfid(char *ID, byte RFID);//TO DO: implement rfid object instead of byte

//--------------------globale variables----------------------------------//

//PIN
const byte rs = 7, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
const byte input_pin = 13, rfid_pin = 12;

//SIMPLE
byte choice = 0;
const int buttonpin = A0;
Baggage C;

//OBJECT
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


//----------------------main--------------------------------------------//

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  printf_begin();
  lcd.begin(16, 2);
  lcd.print("Welcome!");

  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(buttonpin, INPUT);
  pinMode(input_pin, OUTPUT);
  pinMode(rfid_pin, OUTPUT);
  digitalWrite(input_pin, HIGH);//disable user input and enable rfid input
  digitalWrite(rfid_pin, LOW);//choose rfid 1 input
  digitalWrite(buttonpin, HIGH);//initialize the buttonpin to HIGH to avoid syncing problem
  
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
      //set choice to not have a value above 3
      if(choice == 3){
        choice = 0;
      }
      else{
        choice++;
      }
    }
  }

  Serial.println(choice);
  
  switch(choice){
    case 1:
      lcd.clear();
      lcd.print("Check-in MODE");

      digitalWrite(input_pin, LOW);
      if(check_in(&C) == 1){
        choice = 2;
      }
      digitalWrite(input_pin, HIGH);
      //delay(1000);
      break;

      
    case 2:
      lcd.clear();
      lcd.print("LoadinG MODE");
      C = loading(C, &choice);
      if(choice == 3){
        if(check_baggage(C, 1) == 1){
          digitalWrite(A1, HIGH);
          lcd.clear();
          lcd.print("!!LUGGAGES LOST!!");
          Serial.println("Des baggages ont ete voles");
          digitalWrite(input_pin, LOW);
          scanInt();
          digitalWrite(input_pin, HIGH);
        }
      }
      break;

      
    case 3:
      lcd.clear();
      lcd.print("UnloadinG MODE");
      C = unloading(C, &choice);
      if(choice == 0){
        if(check_baggage(C, 2) == 1){
          digitalWrite(A1, HIGH);
          lcd.clear();
          lcd.print("!!LUGGAGES LOST!!");
          Serial.println("Des baggages ont ete voles");
          digitalWrite(input_pin, LOW);
          scanInt();
          digitalWrite(input_pin, HIGH);
        }
      }
      break;

    default:
        lcd.clear();
       lcd.print("!WELCOME!");
      free(C.baggageList);
      C.nombreBaggage = 0;
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







Baggage ajouterBaggage(Baggage B)
{
  if (B.nombreBaggage == 0){
    B.baggageList = (BasicBaggage *) malloc((++B.nombreBaggage)*sizeof(BasicBaggage));
  }
  else{
    realloc(B.baggageList, sizeof(BasicBaggage)*(++B.nombreBaggage));//ajouter un emçlacement de baggage
  }

      
  Serial.println("Enter le nom: ");
  scanString(B.baggageList[B.nombreBaggage - 1].nom, 10);
  
  Serial.println("Entrer l' ID: ");
  scanString(B.baggageList[B.nombreBaggage - 1].id, 15);

  for(byte i=0; i<2; i++){
    B.baggageList[B.nombreBaggage - 1].hasPassedRfid_n_go[i] = 0;
    B.baggageList[B.nombreBaggage - 1].hasPassedRfid_n_back[i] = 0;
  }
  return B;
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
    /*printf("  Poid(g): %d",B.baggageList[i].poid);
    Serial.println();*/
    Serial.println();
  }
}




int check_in(Baggage *B){
  int fin = 0;
  while(fin == 0){
    *B = ajouterBaggage(*B);
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


Baggage loading(Baggage B, byte *choice){
  
  char id_temp[15] = {'\0'};//<- to store the read id
  //set motor forward
  digitalWrite(A2, HIGH);
  digitalWrite(A3, LOW);
  //for rfid 1
  digitalWrite(rfid_pin, LOW);
  //to do: replace with real rfid
  if(read_rfid(id_temp, 1) == 1){
    *choice = 3;
    return B;
  }
  //mofify B.baggageList[i].hasPassedRfid_n_go if the id is found
  for(byte i=0; i< B.nombreBaggage; i++){
    if(strcmp(id_temp, B.baggageList[i].id) == 0){
      B.baggageList[i].hasPassedRfid_n_go[0] = 1;
    }
  }

  //for rfid 2
  digitalWrite(rfid_pin, HIGH);
  //to do: the same
  if(read_rfid(id_temp, 2) == 1){
    *choice = 3;
    return B;
  }
  for(byte i=0; i< B.nombreBaggage; i++){
    if(strcmp(id_temp, B.baggageList[i].id) == 0){
      B.baggageList[i].hasPassedRfid_n_go[1] = 1;
    }
  }
  return B;
}




Baggage unloading(Baggage B, byte *choice){
  
  char id_temp[15] = {'\0'};//<- to store the read id
  //set motor inverted
  digitalWrite(A2, HIGH);
  digitalWrite(A3, HIGH);
  //for rfid 2
  digitalWrite(rfid_pin, HIGH);
  //to do: replace with real rfid
  if(read_rfid(id_temp, 2) == 1){
    *choice = 0;
    return B;
  }
  //mofify B.baggageList[i].hasPassedRfid_n_back if the id is found
  for(byte i=0; i< B.nombreBaggage; i++){
    if(strcmp(id_temp, B.baggageList[i].id) == 0){
      B.baggageList[i].hasPassedRfid_n_back[1] = 1;
    }
  }

  //for rfid 1
  digitalWrite(rfid_pin, LOW);
  //to do: the same
  if(read_rfid(id_temp, 1) == 1){
    *choice = 0;
    return B;
  }
  for(byte i=0; i< B.nombreBaggage; i++){
    if(strcmp(id_temp, B.baggageList[i].id) == 0){
      B.baggageList[i].hasPassedRfid_n_back[0] = 1;
    }
  }
  return B;
}





byte check_baggage(Baggage B, byte mode){
  //loop over luggages
  for(byte i=0; i<  B.nombreBaggage; i++){
    switch(mode){
      //loading mode
      case 1:
        //check if every luggages has passed by rfids
        for(byte j=0; j<2; j++){
          if(B.baggageList[i].hasPassedRfid_n_go[j] == 0){
            return 1;
          }
        }
        break;
      //unloading mode
      case 2:
        for(byte j=0; j<2; j++){
          if(B.baggageList[i].hasPassedRfid_n_back[j] == 0){
            return 1;
          }
        }
        break;
    }
  }
  return 0;
}
