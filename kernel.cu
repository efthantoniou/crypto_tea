#include "cuda.h"
#include "curand.h"
#include "cuda_runtime.h"
#include "curand_kernel.h"
#include "cuda_device_runtime_api.h"
#include "device_launch_parameters.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <limits.h>
#include <math.h>

//Antoniou Efthimios 2022201300011 dit13011@uop.gr
//Makes the permutation that is applied to the key.

__device__ 
uint16_t key_permutation(uint16_t key) {
    uint16_t new_key = key & 0xF00F;
    new_key = new_key | ((key & 0x00F0) << 4) | ((key & 0x0F00) >> 4);
	return new_key;
}
//Makes a circular shift.
__device__ 
uint16_t circular_shift(uint16_t key) {
    uint16_t shift_key = 2;
    assert((shift_key < 16) && shift_key !=0);
	return  (key << shift_key) | (key >> (-shift_key & 15));
}
//Calculates the keys for all the rounds.
__device__ 
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
__device__  
uint16_t s_box(uint16_t ckey) {
    uint16_t new_key = ckey & 0x8421;
    new_key = new_key | ((ckey & 0x0842) << 3) | ((ckey & 0x1000) >> 9);
    new_key = new_key | ((ckey & 0x0084) << 6) | ((ckey & 0x2100) >> 6);
    new_key = new_key | ((ckey & 0x0008) << 9) | ((ckey & 0x4210) >> 3);
    return new_key;
}
//Encrypts the message that we provide.
__device__
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
__device__
void decrypt(uint16_t cipher, uint16_t *keys){
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
    //return m_key; kanthn uint16_t an theleis na deis thn eksodo.

}

__global__ 
void kernel(float *devData){
	
	uint16_t cipher, keys_dev;
	size_t power26_dev= powf(2, 26);

	const int limit_16_dev = 65536;
	const int tid = threadIdx.x;
	const int bid = blockIdx.x;

	int index = bid * blockDim.x + tid;
	int stride = blockDim.x * gridDim.x;

	keys_dev = 0xa1e9;

	for (int j = index; j < power26_dev;j+=stride) {
		cipher = encrypt((uint16_t)(devData[j]*limit_16_dev), keys_enc_dec(keys_dev));
		decrypt(cipher, keys_enc_dec(keys_dev));
	}
}

int main(void){
	curandGenerator_t gen;
	float *devData;
	size_t power26 = (size_t)pow(2, 26);
	//size_t free_byte, total_byte;

	/*cudaError_t cuda_status = cudaMemGetInfo(&free_byte, &total_byte);
	if (cudaSuccess != cuda_status) {
		printf("Error ");
		exit(1);
	}

	double used_db = (double)total_byte - (double)free_byte;
	printf("USED MEMORY: \t%f, FREE: \t%f, TOTAL: \t%f\n", used_db / 1024.0, (double)free_byte / 1024.0, (double)total_byte / 1024.0);
	*/
	printf("2");
	cudaMalloc((void **)&devData, power26*sizeof(float));
	printf("1");
	curandCreateGenerator(&gen, CURAND_RNG_PSEUDO_DEFAULT);
	curandSetPseudoRandomGeneratorSeed(gen, 6666667ULL);
	curandGenerateUniform(gen, devData, power26);

	kernel<<<128, 128 >>>(devData);

	cudaDeviceSynchronize();
	/*
	cuda_status = cudaMemGetInfo(&free_byte, &total_byte);
	if (cudaSuccess != cuda_status) {
		printf("Error ");
		exit(1);
	}
	
	used_db = (double)total_byte-(double)free_byte;
	printf("USED MEMORY: \t%f, FREE: \t%f, TOTAL: \t%f\n",used_db/1024.0,(double)free_byte/1024.0,(double)total_byte/1024.0);
	*/
	cudaFree(devData);

	cudaDeviceReset();

	return 0;
}
