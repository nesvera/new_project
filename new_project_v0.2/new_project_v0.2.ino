#include <SoftwareSerial.h>

//#define NOT_SOUND
#define DEBUG

// Cria um comunicação serial
SoftwareSerial RFID(10, 11); // RX, TX

// RFID
#define RFID_RX_PIN 10

#define TIMEOUT_READ_RFID 100         // ms
#define RFID_END_CHECKSUM 5
#define RFID_ID_SIZE 5                // 5 bytes
#define RFID_RAW_MSG 10
#define TIME_BTW_VALID_ID 2000        // delay entre leituras

long int last_id_sent_time = 0;       // salva o tempo da ultima leitura valida
uint8_t id_rfid[RFID_ID_SIZE];        // Contem o ultimo dado valido na forma decimal
uint8_t id_rfid_ascii[10];            // Contem o ultimo dado recebido na forma ascii (original)
uint8_t id_valido = 0;


// Bluetooth
#define BLUETOOTH_TX_PIN  1           // Ja é definido como saida
#define BLUETOOTH_STATE_PIN 5         // 0 = despareado         1 = pareado

#define Bluetooth Serial             // Renomeia a uart0

#define SIZE_RFID_ID_STR 11           // Tamanho da array enviado ao celular 
#define SIZE_RFID_RAW_STR 17

// Magnetometro
#define DIR_N   0
#define DIR_NE  1
#define DIR_E   2
#define DIR_SE  3
#define DIR_S   4
#define DIR_SO  5
#define DIR_O   6
#define DIR_NO  7

uint8_t mag_direcao = DIR_NE;                 // Utilizando a legenda (ex: norte=0) indica em que direcao o usuario esta indo

// Buzzer
#define BUZZER_PIN  9

#define TIME_AVISO_DESCONECTADO 3000  // A cada X milisegundos avisa a pessoa que o sistema nao esta paread

// Bateria
#define BATERIA_PIN A0

#define BATERIA_DIED        122
#define BATERIA_MIN_TENSAO  420         // Tensão baixa 8.90 volts
#define BATERIA_MAX_TENSAO  480
#define BATERIA_EMPTY       2           // Nivel da bateria vai de 0 até 100... nivel critico
#define BATTERY_OK_NIVEL    10          // 

//#define TIME_AVISO_BATERIA  300000
#define TIME_AVISO_BATERIA  120000

/*------------------------- Funções -------------------------------------------------------------------*/

/* Faz a leitura do leitor RFID e transforma em um vetor de 5bytes ~= 40bits
   A funcao retorna caso nenhuma tag seja lida em certo tempo
*/
int readTag() {
  // Tenta ler por um certo tempo
  long int timeout_start = millis();

  // Indice da mensagem
  int msg_ind = 0;
  int data_ind = 0;
  uint8_t data_rfid[6] = {0, 0, 0, 0, 0, 0};

  uint8_t checksum = 0;
  int ind_out = 0;

  int rfid_msg_state = 0;                 // 0 = idle   1 = processando

  // Reseta a variavel
  id_valido = 0;

  int i = 0;
  while (1) {

    // Retorna leitura invalida(-1) caso de timeout
    if ( millis() - timeout_start > TIMEOUT_READ_RFID ) {
      return -1;
    }

    // Se existe dados para serem lidos da UART
    if (RFID.available() > 0) {
      uint8_t received_char = RFID.read();

      if ( received_char == 2 ) {                                 // Começo da msg
        rfid_msg_state = 1;

      } else if (received_char == 3 && rfid_msg_state == 1 ) {     // Fim da msg
        rfid_msg_state = 0;
        msg_ind = 0;

        // Dado recebido nao contem erro para checksum==0
        if ( checksum == 0 ) {

          if( millis() - last_id_sent_time > TIME_BTW_VALID_ID ){
            id_valido = 1;

            last_id_sent_time = millis();
          }
          
        
          
        }

        return 0;

      } else if ( rfid_msg_state == 1 ) {                          // Dados
        // Salva o id no formato original
        if ( data_ind < 10 ) {
          id_rfid_ascii[data_ind] = received_char;
        }

        // ASCII para HEX
        uint8_t aux = received_char - 48;
        if ( aux > 9 ) aux -= 7;

        // Salva primeiro char na parte alta, e segundo na parte baixa de um byte
        if ( data_ind % 2 == 0) {
          data_rfid[ind_out] = (aux << 4);
        } else {
          data_rfid[ind_out] |= aux;

          // Calcula checksum
          checksum ^= data_rfid[ind_out];

          // Salva ID no vetor global
          if ( ind_out < 5 ) {
            id_rfid[ind_out] = data_rfid[ind_out];
          }
          ind_out++;

        }
        data_ind++;

      }
      msg_ind++;

    }
  }
}

/*  Envia para o celular o ID da tag em formato DECIMAL
    Ex: $0,12345;
    0 = ID da mensagem
    12345 = ID tag HEX
*/
void bluetoothWriteId() {
  uint8_t data_package[SIZE_RFID_ID_STR] = {'$', '0', ';', '\0', '\0', '\0', '\0', '\0', ';', '*', '\0'};
  int i;

  // Coloca o ultimo ID lido no pacote
  for (i = 0; i < RFID_ID_SIZE; i++ ) {
    data_package[i + 3] = id_rfid[i];
  }

  // Envia o pacote para o bluetooth
  for (i = 0; i < SIZE_RFID_ID_STR ; i++) {
    Bluetooth.print(data_package[i]);
  }

  // Bluetooth utiliza '\n' com fim de msg
  Bluetooth.print("\n");
}

/*  Envia para o celular a mensagem bruta lida do RFID
    EX: $1,12345678912345;
    1 = ID da mensagem
    12345678912345 = 14 dados ascii lidos do rfid
*/
void bluetoothWriteRfidMsg() {
  uint8_t data_package[SIZE_RFID_RAW_STR] = {'$', '1', ';', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', ';', '*', '\0'};
  int i;

  // Coloca o ultimo ID lido no pacote
  for (i = 0; i < RFID_RAW_MSG; i++ ) {
    data_package[i + 3] = id_rfid_ascii[i];
  }

  // Envia o pacote para o bluetooth
  for (i = 0; i < SIZE_RFID_RAW_STR ; i++) {
    Bluetooth.print(char(data_package[i]));
  }

  // Bluetooth utiliza '\n' com fim de msg
  Bluetooth.print("\n");
}

/*  Envia para o celular a mensagem bruta lida do RFID e a direção do usuario
    EX: $1,31000062DE8D;1;*
    1 = ID da mensagem
    31000062DE8D = 14 dados ascii lidos do rfid
    1 = sul
*/
void bluetoothWriteIdAndDir() {
  char data_package[SIZE_RFID_RAW_STR] = {'$', '2', ';', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', ';', '\0', ';', '*'};
  int i;

  // Coloca o ultimo ID lido no pacote
  for (i = 0; i < RFID_RAW_MSG; i++ ) {
    data_package[i + 3] = id_rfid_ascii[i];
  }

  // Adiciona a direçao na mensagem
  data_package[14] = mag_direcao + 48;

  // Envia o pacote para o bluetooth
  for (i = 0; i < SIZE_RFID_RAW_STR ; i++) {
    Bluetooth.print(data_package[i]);
  }

  // Bluetooth utiliza '\n' com fim de msg
  Bluetooth.print("\n");
}

/*
 * $3;-355;-355;*  ($pacote,bateria,bateria;*)
 */
void bluetoothWriteBateria(int nivel) {
  String str1, str2, data_package;

  str1 = String("$3;");
  str2 = str1 + nivel;
  str2 = str2 + ";";
  str2 = str2 + nivel;
  data_package = str2 + ";*";

  Bluetooth.print(data_package);

  // Bluetooth utiliza '\n' com fim de msg
  Bluetooth.print("\n");
}

void avisoLigado() {
#ifndef NOT_SOUND
  tone(BUZZER_PIN, 523.25, 150);  // C5
  delay(150);
  tone(BUZZER_PIN, 783.99, 150);  // G5
  delay(150);
  tone(BUZZER_PIN, 1046.5, 150);  // C6
  delay(150);
  noTone(BUZZER_PIN);
#endif
 
}

void avisoPareado() {
#ifndef NOT_SOUND
  tone(BUZZER_PIN, 783.99, 100);   // G5
  delay(100);
  tone(BUZZER_PIN, 1174.66, 100);  // D6
  delay(140);
  tone(BUZZER_PIN, 783.99, 100);
  delay(100);
  tone(BUZZER_PIN, 1174.66, 100);
  delay(140);
  tone(BUZZER_PIN, 783.99, 100);
  delay(100);
  tone(BUZZER_PIN, 1174.66, 100);
  delay(100);
  noTone(BUZZER_PIN);
#endif
}

void avisoBateriaFraca() {
#ifndef NOT_SOUND
  tone(BUZZER_PIN, 261.63, 80);
  delay(200);
  tone(BUZZER_PIN, 261.63, 200);
  delay(200);
#endif
}

void avisoDesligar() {
#ifndef NOT_SOUND
  tone(BUZZER_PIN, 261.63, 80);
  delay(300);
  tone(BUZZER_PIN, 261.63, 200);
  delay(300);
  noTone(BUZZER_PIN);
#endif
}

void avisoDespareado() {
#ifndef NOT_SOUND
  tone(BUZZER_PIN, 1975.53, 150);  // B6
  delay(190);
  tone(BUZZER_PIN, 783.99, 150);   // G5
  delay(150);
  noTone(BUZZER_PIN);
#endif
}

void avisoBateriaVazia() {
#ifndef NOT_SOUND
  tone(BUZZER_PIN, 261.63, 80);
  delay(100);
  tone(BUZZER_PIN, 261.63, 200);
  delay(100);
#endif

/*
#ifndef NOT_SOUND
  tone(BUZZER_PIN, 1500, 150);  // B6
  delay(100);
  tone(BUZZER_PIN, 1400, 150);   // G5
  delay(150);
  noTone(BUZZER_PIN);
#endif
*/
}

int esperaConexao() {
  long int time_s = millis();

  // Lê o pino de status do bluetooth. 1 = conectado
  while (1) {
    // Retorna quando o bluetooth estiver pareado
    if ( digitalRead(BLUETOOTH_STATE_PIN) == 1 ) {
      //break;
    }

    // Emite som de despareado a cada x milisegundos
    if ( millis() - time_s > TIME_AVISO_DESCONECTADO ) {
      avisoDespareado();
      time_s = millis();
    }

  }
  
  return 0;
}

/* --------------------------------------------------- SETUP ---------------------------------------------- */
void setup() {

  // Inicializa GPIO
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BATERIA_PIN, INPUT);

  // Inicializa comunicação UART
  Serial.begin(9600);
  RFID.begin(9600);

  // Emite som pra informar que ligou
  avisoLigado();
  delay(1000);

  //Espera o celular se conectar ao bluetooth
  //esperaConexao();
  
}

/* -------------------------------------------------- LOOP ------------------------------------------------*/
void loop() {
  // Variaveis para salvar tempo
  long int time_bateria = millis();
  long int time_bluetooth = millis();

  while (1) {

#ifndef DEBUG
    // Verifica nivel da bateria
    if ( millis() - time_bateria > TIME_AVISO_BATERIA ) {
      int bateria_read = analogRead(BATERIA_PIN);

      int bateria_nivel = map(bateria_read, BATERIA_MIN_TENSAO, BATERIA_MAX_TENSAO, 0, 100);

      // Avisa o usuário que o sistema esta com a bateria vazia
      if (bateria_nivel < 0 ) {
        avisoBateriaVazia();
        bluetoothWriteBateria( bateria_nivel );
        delay(5000);
        continue;

        // Avisa o usuário que esta acabando
      } else if ( bateria_nivel < BATTERY_OK_NIVEL ) {
        bluetoothWriteBateria( bateria_nivel );
        avisoBateriaFraca();

        // Apenas envia para o celular
      } else {
        bluetoothWriteBateria( bateria_nivel );
      }

      time_bateria = millis();
    }
#endif

    // Confere se o celular esta conectado
    //if( digitalRead(BLUETOOTH_STATE_PIN) == 1 ){
    if (1) {
      // Le do leitor RFID
      if ( readTag() == -1 ) {
        //Serial.println("Sem leitura");
      }

      // Envia ID da TAG para o celular via Bluetooth
      if ( id_valido ) {
        bluetoothWriteIdAndDir();
      }

    // Bluetooth nao esta conectado
    } else {
      //Espera o celular se conectar ao bluetooth
      esperaConexao();
      
    }

  }
}
