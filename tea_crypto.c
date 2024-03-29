#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <inttypes.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>
//Antoniou Efthimios 2022201300011 dit13011@uop.gr
//Makes the permutation that is applied to the key.
uint16_t key_permutation(uint16_t key) {
    uint16_t new = key & 0xF00F;
    new = new | ((key & 0x00F0) << 4) | ((key & 0x0F00) >> 4);
}
//Makes a circular shift.
uint16_t circular_shift(uint16_t key) {
    uint16_t shift_key = 2;
    assert((shift_key < 16) && shift_key !=0);
	return  (key << shift_key) | (key >> (-shift_key & 15));
}
//Calculates the keys for all the rounds.
uint16_t *keys_enc_dec(uint16_t key){
    static uint16_t calc_keys[5]; 
    calc_keys[0] = key;
    calc_keys[1] = key_permutation(circular_shift(key));
    calc_keys[2] = key_permutation(circular_shift(calc_keys[1]));
    calc_keys[3] = key_permutation(circular_shift(calc_keys[2]));
    calc_keys[4] = circular_shift(calc_keys[3]);
    return calc_keys;
}
//Creates the s-box that permutates the bits of the ciphertext.
uint16_t s_box(uint16_t ckey) {
    uint16_t new = ckey & 0x8421;
    new = new | ((ckey & 0x0842) << 3) | ((ckey & 0x1000) >> 9);
    new = new | ((ckey & 0x0084) << 6) | ((ckey & 0x2100) >> 6);
    new = new | ((ckey & 0x0008) << 9) | ((ckey & 0x4210) >> 3);
    return new;
}
//Encrypts the message that we provide.
uint16_t encrypt(uint16_t message, uint16_t *keys){
    //1st round
    uint16_t c_key = message ^ keys[0];
    c_key = s_box(c_key);
    //2nd round
    c_key = c_key ^ keys[1];
    c_key = s_box(c_key);
    //3rd round
    c_key = c_key ^ keys[2];
    c_key = s_box(c_key);
    //4th round
    c_key = c_key ^ keys[3];
    //5th round
    c_key = c_key ^ keys[4];
    return c_key;
}
//Decrypts the ciphertext that we provide.
uint16_t decrypt(uint16_t cipher, uint16_t *keys){
    uint16_t m_key;
    //1st round
    m_key = keys[4] ^ cipher;
    //2nd round
    m_key = m_key ^ keys[3];
    m_key = s_box(m_key);
    //3rd round
    m_key = m_key ^ keys[2];
    m_key = s_box(m_key);
    //4th round
    m_key = m_key ^ keys[1];
    m_key = s_box(m_key);
    //5th round
    m_key = m_key ^ keys[0];
    return m_key;

}

void main(void){
    uint16_t key, message, ciphertext, message_dec, msg_rnd;
    uint16_t *keys = NULL, *keys_1a = NULL, key_2a, c_1a, c_2a;
    int i;
    FILE *fp1, *fp2, *fp3, *fp4, *fp5;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int limit_16 = 65536;

    fp1 = fopen("erwthma1a1.txt", "w");
    fp2 = fopen("erwthma1a2.txt", "w");

    keys_1a =  keys_enc_dec(0x0000);
    message =  0x0000;
    //In order to have all the possible messages that the 16-bit supports.
    for(i=0;i<limit_16;i++){
        c_1a =  encrypt(message, keys_1a);
        fprintf(fp1, "M = %4x C = %4x\n", message, c_1a);
        message++;
    }
    printf("Erwthma 1.a.1 Done\n");
    fclose(fp1);
    message = 0x000;
    key_2a = 0x0000;
    //In order to have all the possible keys that the 16-bit supports.
    for(i=0;i<limit_16;i++){
        c_2a = encrypt(message, keys_enc_dec(key_2a));
        fprintf(fp2, "K = %4x C = %4x\n", key_2a, c_2a);
        key_2a++;
    }
    fclose(fp2);
    printf("Erwthma 1.a.2 Done\n");

    key = 0xa1e9;
    keys = keys_enc_dec(key);
    printf("Erwthma 1.b\n");
    for(i=1;i<5;i++){
        printf("k%1d: %4x\n",i ,keys[i]);
    }
    printf("Generating random messages\n");
    clock_t start_dec, start_enc, end_dec, end_enc;
    srand(time(NULL));
    //Creates 2 ^ 26 random messages and then store them to a file.
    fp3 = fopen("random.txt","w");
    for(i=0;i<pow(2, 26);i++){
        msg_rnd = (uint16_t)(((double) rand()/ (double)(RAND_MAX + 1.0)) * limit_16);
        fprintf(fp3,"%x\n", msg_rnd);
    }
    fclose(fp3);
    fp3 = fopen("random.txt","r");
    fp4 = fopen("random_c.txt","w");
    key = 0xa1e9;

    //Start of the encryption
    printf("*Encryption*\n");
    start_enc = clock();
    keys = keys_enc_dec(key);
    //https://linux.die.net/man/3/getline  The following code allows us to read a line from a file.
    while((read = getline(&line, &len, fp3)) != -1){
        message = (uint16_t)*line;
        ciphertext = encrypt(message, keys);
        fprintf(fp4,"%x\n", ciphertext);
    }
    end_enc = clock();
    fclose(fp3);
    fclose(fp4);
    //End of the encryption
    printf("Encryption took: %f ticks or ticks per second %f\n",(float)(end_enc - start_enc), (float)(end_enc - start_enc)/CLOCKS_PER_SEC);

    fp4= fopen("random_c.txt","r");
    fp5 = fopen("random_d.txt", "w");
    //Start of the decryption
    printf("*Decryption*\n");
    start_dec = clock();
    
    while((read = getline(&line, &len, fp4)) != -1){
        message_dec = decrypt(ciphertext, keys);
        fprintf(fp5,"%x\n", message_dec);
    }
    end_dec = clock();
    fclose(fp4);
    fclose(fp5);
    //End of the decryption
    printf("Decryption took: %f ticks or ticks per second %f\n",(float)(end_dec - start_dec), (float)(end_dec - start_dec)/CLOCKS_PER_SEC);
}