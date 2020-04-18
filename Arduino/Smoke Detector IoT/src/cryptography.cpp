#include <Arduino.h>
#include <ChaCha.h>
#include <Cipher.h>
#include <ESP8266TrueRandom.h>
#include <ArduinoJson.h>

#define CHA_CHA_KEY_SIZE 32
#define CHA_CHA_IV_SIZE 8
#define CHA_CHA_COUNTER_SIZE 8
#define CHA_CHA_MESSAGE_SIZE 120
#define CHA_CHA_ROUNDS 20

void to_decimal(byte * input, String * output, int size){
    for(int i = 0 ; i < size ; i++){
      *output += String(int(input[i]));
      *output += " ";
    }
}

void to_plaintext(byte * input, String * output, int size){
    for(int i = 0 ; i < size ; i++){
      *output += String(char(input[i]));
    }
}

void to_json(String * iv, String * ciphertext, long * time,  int * length, DynamicJsonDocument &json){
    json.clear();
    json["encrypted"] = *ciphertext;
    json["iv"] = *iv;
    json["time"] = *time;
    json["length"] = *length;
}

void regenerate_iv(byte * iv){
    for(int i = 0 ; i < CHA_CHA_IV_SIZE ; i++){
        iv[i] = byte(ESP8266TrueRandom.randomByte());
    }
}

void encrypt_chacha20(byte * key, byte * iv, byte * counter, byte * message, byte * ciphertext, int * length, ChaCha * chacha20){
    size_t keySize = CHA_CHA_KEY_SIZE;
    size_t counterSize = CHA_CHA_COUNTER_SIZE;

    chacha20->setKey(key, keySize);
    chacha20->setIV(iv, chacha20->ivSize());
    chacha20->setCounter(counter, counterSize);
    chacha20->setNumRounds(CHA_CHA_ROUNDS);
    
    chacha20->encrypt(ciphertext + *length, message + *length, *length);
}

void decrypt_chaha20(byte * key, byte * iv, byte * counter, byte * ciphertext, byte * plaintext, int * length, ChaCha * chacha20){
    size_t keySize = CHA_CHA_KEY_SIZE;
    size_t counterSize = CHA_CHA_COUNTER_SIZE;

    chacha20->setKey(key, keySize);
    chacha20->setIV(iv, chacha20->ivSize());
    chacha20->setCounter(counter, counterSize);
    chacha20->setNumRounds(CHA_CHA_ROUNDS);

    chacha20->decrypt(plaintext + *length, ciphertext + *length, *length);
}

void encrypt_aes_128(){

}

void decrypt_aes_128(){
    
}