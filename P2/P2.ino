#include <SPI.h>
#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

#define CE_PIN 7
#define CSN_PIN 8

#define TIMEOUTRECV 1000000  //us
#define TIMEOUTSEND 6000     //us

#define ACK 1
#define RTS 2
#define CTS 3
#define DATA 4


// instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);
uint64_t address[2] = { 0x3030303030LL, 0x3030303030LL };

char payloadT[5] = "Hello";
char payloadR[5];
uint8_t origem = 44;
uint8_t rede = 88;

bool envia(char* pacote, uint8_t destino, uint8_t tamanho, uint8_t controle, uint8_t rede, unsigned long timeout) {
  pacote[0] = destino;
  pacote[1] = origem;
  pacote[2] = rede;
  pacote[3] = controle;
  unsigned long start_timer = micros();  // start the timer
  while (micros() - start_timer < timeout) {
    radio.startListening();
    delayMicroseconds(70);
    radio.stopListening();
    if (!radio.testCarrier()) {
      radio.write(&pacote[0], tamanho);
      return true;
    } else {
      delayMicroseconds(150);
    }
  }
  return false;
}
bool recebe(char* pacote, uint8_t destino, uint8_t tamanho, uint8_t controle, uint8_t rede, unsigned long timeout) {
  unsigned long start_timer = micros();
  radio.startListening();
  while (micros() - start_timer < timeout) {
    if (radio.available()) {
      radio.read(&pacote[0], tamanho);
      if (pacote[0] == origem && pacote[1] == destino && pacote[2] == rede && pacote[3] == controle) {
        Serial.print("[");
        Serial.print(int(pacote[1]));
        Serial.print("]");
        Serial.println(int(pacote[4]));
        return true;
      }
      radio.flush_rx();
    }
  }
  return false;
}

bool enviaTrem(char* pacote, uint8_t tamanho, uint8_t destino) {
  char controle[4];
  bool enviou = false;
  bool recebeu = false;

  enviou = envia(&controle[0], destino, 4, RTS, rede, TIMEOUTSEND);
  if (enviou) {
    recebeu = recebe(&controle[0], destino, 4, CTS, rede, TIMEOUTRECV);
  } else {
    return false;
  }
  if (recebeu) {
    enviou = envia(&pacote[0], destino, tamanho, DATA, rede, TIMEOUTSEND);
  } else {
    return false;
  }
  if (enviou) {
    recebeu = recebe(&controle[0], destino, 4, ACK, rede, TIMEOUTRECV);
  } else {
    return false;
  }
  return true;
}

bool recebeTrem(char* pacote, uint8_t tamanho, uint8_t destino) {
  char controle[4];
  bool recebeu = false;
  bool enviou = false;
  recebeu = recebe(&controle[0], destino, 4, RTS, rede, TIMEOUTRECV);
  if (recebeu) {
    enviou = envia(&controle[0], destino, 4, CTS, rede, TIMEOUTSEND);
  } else {
    return false;
  }
  if (enviou) {
    recebeu = recebe(&pacote[0], destino, tamanho, DATA, rede, TIMEOUTRECV);
  } else {
    return false;
  }
  if (recebeu) {
    enviou = envia(&controle[0], destino, 4, ACK, rede, TIMEOUTSEND);
  } else {
    return false;
  }
  return true;
}


void setup(void) {

  Serial.begin(500000);

  // Setup and configure rf radio
  if (!radio.begin()) {
    Serial.println(F("radio hardware not responding!"));
    while (true) {
      // hold in an infinite loop
    }
  }

  radio.setPALevel(RF24_PA_MAX);  // RF24_PA_MAX is default.
  radio.setAutoAck(false);        // Don't acknowledge arbitrary signals
  radio.disableCRC();             // Accept any signal we find
  radio.setDataRate(RF24_1MBPS);


  radio.setPayloadSize(sizeof(payloadT));

  radio.openWritingPipe(address[0]);     // always uses pipe 0
  radio.openReadingPipe(1, address[1]);  // using pipe 1

  radio.setChannel(100);

  printf_begin();
  radio.printPrettyDetails();

  radio.startListening();
  radio.stopListening();
  radio.flush_rx();
}

void loop(void) {
  uint8_t destino = 13;
  if (Serial.available()) {
    destino = int(Serial.read());
    Serial.print("Destino = ");
    Serial.println(destino);
  }
  for ( int i = 0; i < 5; i++ ) {
    payloadR[i] = 1;
  }
  bool sucesso = recebeTrem(&payloadR[0], 5, destino);
  // if (sucesso) {
  //   printAula(&payloadR[0], 5);
  // }
}

void printAula(char* texto, byte tamanho) {
  Serial.print(int(texto[0]));
  Serial.print(int(texto[1]));
  for (byte i = 2; i < tamanho; i++) {
    Serial.print(char(texto[i]));
  }
  Serial.println();
}
