#include <SoftwareSerial.h>



SoftwareSerial RFID(10, 11); // RX, TX

// RFID
#define TIMEOUT_READ_RFID 100         // ms
#define RFID_END_CHECKSUM 5
#define RFID_ID_SIZE 5                // 5 bytes
#define RFID_RAW_MSG 14

long int last_id_sent = 0;
uint8_t id_rfid[RFID_ID_SIZE];        // Contem o ultimo dado valido na forma decimal
uint8_t id_rfid_ascii[10];            // Contem o ultimo dado recebido na forma ascii (original)
uint8_t id_valido = 0;

// Bluetooth
#define SIZE_RFID_ID_STR 11           // Tamanho da array enviado ao celular 
#define SIZE_RFID_RAW_STR 20

#define Bluetooth Serial             // Renomeia a uart0

void setup() {

  Serial.begin(9600);
  RFID.begin(9600);

  pinMode(13, OUTPUT);

}

/* Faz a leitura do leitor RFID e transforma em um vetor de 5bytes ~= 40bits
 * A funcao retorna caso nenhuma tag seja lida em certo tempo
 */
int readTag(){
  // Tenta ler por um certo tempo
  long int timeout_start = millis();

  // Indice da mensagem
  int msg_ind = 0;
  int data_ind = 0;
  uint8_t data_rfid[6] = {0,0,0,0,0,0};

  uint8_t checksum = 0;
  int ind_out = 0;

  int rfid_msg_state = 0;                 // 0 = idle   1 = processando
  
  // Reseta a variavel
  id_valido = 0;

  int i = 0;
  while(1){
    
    // Retorna leitura invalida(-1) caso de timeout
    if( millis()-timeout_start > TIMEOUT_READ_RFID ){
      return -1;
    }

    // Se existe dados para serem lidos da UART
    if(RFID.available()>0){
      uint8_t received_char = RFID.read();

      if( received_char == 2 ){                                   // Começo da msg
        rfid_msg_state = 1;
       
      }else if(received_char == 3 && rfid_msg_state == 1 ){       // Fim da msg
        rfid_msg_state = 0;
        msg_ind = 0;

        // Dado recebido nao contem erro para checksum==0
        if( checksum == 0 ){
          id_valido = 1;
        }

        return 0;
        
      }else if( rfid_msg_state == 1 ){                            // Dados
        // Salva o id no formato original
        if( data_ind < 10 ){
          id_rfid_ascii[data_ind] = received_char;
        }
        
        // ASCII para HEX
        uint8_t aux = received_char - 48;
        if( aux > 9 ) aux -= 7;

        // Salva primeiro char na parte alta, e segundo na parte baixa de um byte
        if( data_ind%2 == 0){
          data_rfid[ind_out] = (aux<<4);
        }else{
          data_rfid[ind_out] |= aux;

          // Calcula checksum 
          checksum ^= data_rfid[ind_out];

          // Salva ID no vetor global
          if( ind_out < 5 ){
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
 *  Ex: $0,12345;*
 *  0 = ID da mensagem
 *  12345 = ID tag HEX
 */
void bluetoothWriteId(){
  uint8_t data_package[SIZE_RFID_ID_STR] = {'$', '0', ';', '\0', '\0', '\0', '\0', '\0', ';', '*', '\0'};
  int i;

  // Coloca o ultimo ID lido no pacote
  for(i=0; i<RFID_ID_SIZE; i++ ){
    data_package[i+3] = id_rfid[i];
  }

  // Envia o pacote para o bluetooth
  for(i=0; i<SIZE_RFID_ID_STR ; i++){
    Bluetooth.print(data_package[i]);
  }
}

/*  Envia para o celular a mensagem bruta lida do RFID
 *  EX: $1,12345678912345;*
 *  1 = ID da mensagem
 *  12345678912345 = 14 dados ascii lidos do rfid
 */
void bluetoothWriteRfidMsg(){
  uint8_t data_package[SIZE_RFID_RAW_STR] = {'$', '0', ';', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', ';', '*', '\0'};
  int i;

  // Coloca o ultimo ID lido no pacote
  for(i=0; i<RFID_RAW_MSG; i++ ){
    data_package[i+3] = id_rfid_ascii[i];
  }

  // Envia o pacote para o bluetooth
  for(i=0; i<SIZE_RFID_RAW_STR ; i++){
    Bluetooth.print(data_package[i]);
  }
}

void loop() {

  // Le do leitor RFID
  if( readTag() == -1 ){
    Serial.println("kassinao");
  }
  
  // Envia ID da TAG para o celular via Bluetooth
  if( id_valido ){
    bluetoothWriteId();
    Serial.println("\n--------------");
    bluetoothWriteRfidMsg();
  }

  delay(500);

}
