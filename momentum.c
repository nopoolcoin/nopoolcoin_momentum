#include <Python.h>

#include <stdlib.h>
#include <string.h>
#include "momentum.h"
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include "hashblock.h"

#define PSUEDORANDOM_DATA_SIZE 30 //2^30 = 1GB
#define PSUEDORANDOM_DATA_CHUNK_SIZE 6 //2^6 = 64 bytes
#define L2CACHE_TARGET 16 // 2^16 = 64K
#define AES_ITERATIONS 50

// useful constants
const unsigned int psuedoRandomDataSize=(1<<PSUEDORANDOM_DATA_SIZE);
const unsigned int cacheMemorySize = (1<<L2CACHE_TARGET);
const unsigned int chunks=(1<<(PSUEDORANDOM_DATA_SIZE-PSUEDORANDOM_DATA_CHUNK_SIZE));
const unsigned int chunkSize=(1<<(PSUEDORANDOM_DATA_CHUNK_SIZE));
const unsigned int comparisonSize=(1<<(PSUEDORANDOM_DATA_SIZE-L2CACHE_TARGET));

bool momentum_verify(unsigned char *midHash, uint32_t a, uint32_t b ){
	//return false;
	
	//Basic check
	if( a >= comparisonSize ) return false;
	
	//Allocate memory required
	unsigned char *cacheMemoryOperatingData;
	unsigned char *cacheMemoryOperatingData2;	
	cacheMemoryOperatingData=malloc(cacheMemorySize+16);
	cacheMemoryOperatingData2=malloc(cacheMemorySize);
	uint32_t* cacheMemoryOperatingData32 = (uint32_t*)cacheMemoryOperatingData;
	uint32_t* cacheMemoryOperatingData322 = (uint32_t*)cacheMemoryOperatingData2;
	
	unsigned char *hash_tmp = midHash;
	
	//unsigned char  hash_tmp[32];
	//memcpy((char*)&hash_tmp[0], midHash, 32 );
	uint32_t* index = (uint32_t*)hash_tmp;
	
	//AES_KEY AESkey;
	//unsigned char iv[AES_BLOCK_SIZE];
	
	uint32_t startLocation=a*cacheMemorySize/chunkSize;
	uint32_t finishLocation=startLocation+(cacheMemorySize/chunkSize);
		
	//copy 64k of data to first l2 cache		
	uint32_t h;
	for(h = startLocation; h <  finishLocation;  h++){
		*index = h;
		HashGroestl((unsigned char*)hash_tmp, 32, (unsigned char*)&(cacheMemoryOperatingData[(h-startLocation)*chunkSize]));
	}
	
	unsigned int useEVP = 1;

	//allow override for AESNI testing
	//if(*midHash==0){
	//	useEVP=0;
	//}else if(*midHash==1){
	//	useEVP=1;
	//}

	unsigned char key[32] = {0};
	unsigned char iv[AES_BLOCK_SIZE];
	int outlen1, outlen2;
	
	//memset(cacheMemoryOperatingData2,0,cacheMemorySize);
	int j;
	for(j=0;j<AES_ITERATIONS;j++){
		
		//use last 4 bits as next location
		startLocation = (cacheMemoryOperatingData32[(cacheMemorySize/4)-1]%comparisonSize)*cacheMemorySize/chunkSize;
		finishLocation=startLocation+(cacheMemorySize/chunkSize);
		uint32_t i;
		for(i = startLocation; i <  finishLocation;  i++){
			*index = i;
			HashGroestl(hash_tmp, 32, (unsigned char*)&(cacheMemoryOperatingData2[(i-startLocation)*chunkSize]));
		}

		//XOR location data into second cache
		for(i = 0; i < cacheMemorySize/4; i++){
			cacheMemoryOperatingData322[i] = cacheMemoryOperatingData32[i] ^ cacheMemoryOperatingData322[i];
		}
			
		//AES Encrypt using last 256bits as key
		
		if(useEVP){
			EVP_CIPHER_CTX ctx;
			memcpy(key,(unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-32],32);
			memcpy(iv,(unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-AES_BLOCK_SIZE],AES_BLOCK_SIZE);
			EVP_EncryptInit(&ctx, EVP_aes_256_cbc(), key, iv);
			EVP_EncryptUpdate(&ctx, cacheMemoryOperatingData, &outlen1, cacheMemoryOperatingData2, cacheMemorySize);
			EVP_EncryptFinal(&ctx, cacheMemoryOperatingData + outlen1, &outlen2);
			EVP_CIPHER_CTX_cleanup(&ctx);
		}else{
			AES_KEY AESkey;
			AES_set_encrypt_key((unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-32], 256, &AESkey);			
			memcpy(iv,(unsigned char*)&cacheMemoryOperatingData2[cacheMemorySize-AES_BLOCK_SIZE],AES_BLOCK_SIZE);
			AES_cbc_encrypt((unsigned char*)&cacheMemoryOperatingData2[0], (unsigned char*)&cacheMemoryOperatingData[0], cacheMemorySize, &AESkey, iv, AES_ENCRYPT);
		}
		
	}
		
	//use last X bits as solution
	uint32_t solution=cacheMemoryOperatingData32[(cacheMemorySize/4)-1]%comparisonSize;
	uint32_t proofOfCalculation=cacheMemoryOperatingData32[(cacheMemorySize/4)-2];
	//printf("verify solution - %d / %u / %u\n",a,solution,proofOfCalculation);
	
	//free memory
	free(cacheMemoryOperatingData);
	free(cacheMemoryOperatingData2);		
	CRYPTO_cleanup_all_ex_data();
	EVP_cleanup();
	
	if(solution==1968 && proofOfCalculation==b){
		return true;
	}
	
	return false;

}
