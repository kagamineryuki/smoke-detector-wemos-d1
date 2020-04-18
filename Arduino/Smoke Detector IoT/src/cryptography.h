#ifndef CRYPTOGRAPHY
#define CRYPTOGRAPHY
#include <Arduino.h>
#include <ChaCha.h>
#include <Cipher.h>

#define CHA_CHA_KEY_SIZE 32
#define CHA_CHA_IV_SIZE 8
#define CHA_CHA_COUNTER_SIZE 8
#define CHA_CHA_MESSAGE_SIZE 120
#define CHA_CHA_ROUNDS 20

void to_decimal(byte * input, String *output, int size);
void to_json(String * iv, String * ciphertext, long * time, int * length, DynamicJsonDocument &json);
void regenerate_iv(byte * iv);
void to_plaintext(byte * input, String * output, int size);
void encrypt_chacha20(byte * key, byte * iv, byte * counter, byte * message, byte * ciphertext, int * length, ChaCha * chacha20);
void decrypt_chaha20(byte * key, byte * iv, byte * counter, byte * ciphertext, byte * plaintext, int * length, ChaCha * chacha20);

#endif