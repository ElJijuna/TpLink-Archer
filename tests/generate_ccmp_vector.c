/* Independent OpenSSL EVP CCM vector, public dummy PTK only; no network/USB. */
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
int main(int argc,char **argv){
 if(argc!=2)return 2;
 unsigned char ptk[48];FILE *f=fopen("tests/fixtures/wpa2-test-ptk.bin","rb");assert(f);assert(fread(ptk,1,48,f)==48);assert(fgetc(f)==EOF);fclose(f);
 const unsigned char header[24]={8,0x41,0,0,0x50,0x92,0x6a,0x11,6,0x66,2,0,0,0x89,0x22,0xa1,0x50,0x92,0x6a,0x11,6,0x66,0,1};
 unsigned char aad[22]={8,0x41},nonce[13]={0},plain[23],cipher[23],tag[8];
 memcpy(aad+2,header+4,18);aad[20]=0;aad[21]=0;
 memcpy(nonce+1,header+10,6);nonce[12]=1;
 for(unsigned i=0;i<23;++i)plain[i]=8+i;
 EVP_CIPHER_CTX *ctx=EVP_CIPHER_CTX_new();assert(ctx);
 assert(EVP_EncryptInit_ex(ctx,EVP_aes_128_ccm(),NULL,NULL,NULL)==1);
 assert(EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_CCM_SET_IVLEN,13,NULL)==1);
 assert(EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_CCM_SET_TAG,8,NULL)==1);
 assert(EVP_EncryptInit_ex(ctx,NULL,NULL,ptk+32,nonce)==1);
 int n=0,final=0;assert(EVP_EncryptUpdate(ctx,NULL,&n,NULL,23)==1);
 assert(EVP_EncryptUpdate(ctx,NULL,&n,aad,22)==1);
 assert(EVP_EncryptUpdate(ctx,cipher,&n,plain,23)==1&&n==23);
 unsigned char tail[16];assert(EVP_EncryptFinal_ex(ctx,tail,&final)==1&&final==0);
 assert(EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_CCM_GET_TAG,8,tag)==1);EVP_CIPHER_CTX_free(ctx);
 f=fopen(argv[1],"wbx");assert(f);const unsigned char iv[8]={1,0,0,0x20,0,0,0,0};
 assert(fwrite(header,1,24,f)==24);assert(fwrite(iv,1,8,f)==8);assert(fwrite(cipher,1,23,f)==23);assert(fwrite(tag,1,8,f)==8);assert(fclose(f)==0);
 return 0;
}
