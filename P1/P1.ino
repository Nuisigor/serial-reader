#include <SPI.h>
#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

#define CE_PIN 7
#define CSN_PIN 8

#define BOT_1 2  // Botão 1 conectado ao pino D2
#define BOT_2 3  // Botão 2 conectado ao pino D3

#define TIMEOUTRECV 2000000  //us
#define TIMEOUTSEND 120000     //us

#define ACK 1
#define RTS 2
#define CTS 3
#define DATA 4

struct PacketStatus {
  bool ret;
  uint8_t remetente;
};

RF24 radio(CE_PIN, CSN_PIN);
uint64_t address[2] = { 0x3030303030LL, 0x3030303030LL };

char payloadT[10];  // Payload para os bits a serem enviados
uint8_t origem = 7;
uint8_t rede = 88;

// Variáveis de estado para os botões
uint8_t last_button;

bool botao1Pressionado = false;
bool botao2Pressionado = false;
unsigned long tempoBotao1 = 0; // Armazena o tempo do pressionamento
unsigned long tempoBotao2 = 0;
unsigned long last_time = 0;

bool envia(char* pacote, uint8_t destino, uint8_t tamanho, uint8_t controle, uint8_t rede, unsigned long timeout) {
  Serial.println("Iniciando envio...");
  pacote[0] = destino;
  pacote[1] = origem;
  pacote[2] = rede;
  pacote[3] = controle;

  unsigned long start_timer = micros();
  while (micros() - start_timer < timeout) {
    radio.startListening();
    delayMicroseconds(70);
    radio.stopListening();

    if (!radio.testCarrier()) {
      Serial.println("Canal livre, enviando pacote...");
      if (radio.write(&pacote[0], tamanho)) {
        Serial.println("Pacote enviado com sucesso.");
        return true;
      } else {
        Serial.println("Falha no envio do pacote.");
      }
    }
    delayMicroseconds(100);
  }
  Serial.println("Tempo de envio esgotado.");
  return false;
}

bool enviaTrem(char* pacote, uint8_t tamanho, uint8_t destino) {
  Serial.println("Iniciando envioTrem...");

  char controle[4];
  bool enviou = envia(&controle[0], destino, 4, RTS, rede, TIMEOUTSEND);
  if (!enviou) {
    Serial.println("Falha ao enviar RTS.");
    return false;
  }

  Serial.println("RTS enviado, aguardando CTS...");
  bool recebeu = envia(&controle[0], destino, 4, CTS, rede, TIMEOUTSEND);
  if (!recebeu) {
    Serial.println("Falha ao receber CTS.");
    return false;
  }

  Serial.println("CTS recebido, enviando dados...");
  enviou = envia(&pacote[0], destino, tamanho, DATA, rede, TIMEOUTSEND);
  if (!enviou) {
    Serial.println("Falha ao enviar dados.");
    return false;
  }

  Serial.println("Dados enviados com sucesso!");
  return true;
}

void setup(void) {
  pinMode(BOT_1, INPUT_PULLUP);  // Botão 1 (D2)
  pinMode(BOT_2, INPUT_PULLUP);  // Botão 2 (D3)
  Serial.begin(500000);

  if (!radio.begin()) {
    Serial.println(F("radio hardware not responding!"));
    while (true);
  }

  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(false);
  radio.disableCRC();
  radio.setDataRate(RF24_2MBPS);
  radio.setPayloadSize(sizeof(payloadT));
  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[1]);
  radio.setChannel(37);

  printf_begin();
  radio.printPrettyDetails();
}

void loop(void) {
  uint8_t destino = 44;

    if (digitalRead(BOT_1) == LOW && millis() - last_time > 1000) {
        if (last_button == BOT_2) {
            Serial.println("Sequência Botão 2 -> Botão 1 detectada. Enviando 16 bits...");
            for (int i = 0; i < 8; i++) payloadT[i] = 'o';  // 16 bits
            enviaTrem(&payloadT[0], 8, destino);
            last_button = NULL;
        }
        else if (last_button == BOT_1) {
            Serial.println("Reseting");
            last_button = NULL;
        } 
        else {
            Serial.println("Botão 1 pressionado");
            last_button = BOT_1;
        }
        last_time = millis();
        delay(200);
    }

    else if (digitalRead(BOT_2) == LOW && millis() - last_time > 1000) {
        if (last_button == BOT_1) {
            Serial.println("Sequência Botão 1 -> Botão 2 detectada. Enviando 8 bits...");
            for (int i = 0; i < 8; i++) payloadT[i] = 'i';  // 8 bits
            enviaTrem(&payloadT[0], 8, destino);
            last_button = NULL;
        }
        else if (last_button == BOT_2) {
            Serial.println("Reseting");
            last_button = NULL;
        } 
        else {
            Serial.println("Botão 2 pressionado");
            last_button = BOT_2;
        }
        last_time = millis();
        delay(200);
    }
}