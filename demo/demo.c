#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "stlv.h"
#include <stlv/stlv.h>

int main(int argc , char **argv)
{
  char *buff = NULL;
  char *bf1 = NULL;
  char *bf2 = NULL;

  char array[201] = {0};
  long value = 11532;
  double float64 = 89456.496;
  unsigned int len = 0;
  unsigned int len1 = 0;
  unsigned int len2 = 0;
  int i;
  char type = -1;

  printf("value:0x%X len:%d\n" , value , sizeof(value));
  //primitive
  bf1 = STLV_BUFF_ALLOC(sizeof(value));
  len1 = STLV_PACK_LONG(bf1 , &value);
  printf("pack value len is %d\n" , len1);
  STLV_DUMP_PACK(bf1);

  //tlv
  bf2 = STLV_BUFF_ALLOC(len1); 
  len2 = STLV_PACK_TLV(bf2 , bf1 , len1);
  printf("pack tlv len is %d\n" , len2);
  STLV_DUMP_PACK(bf2);
  
  //unpack
  value = 0;
  len = STLV_UNPACK(&type , &value , bf2 , len2);
  printf("unpack from tlv len:%d and result type:%d value 0x%X len is %d\n" , len2 , type , value , len);


  STLV_BUFF_FREE(bf1);
  STLV_BUFF_FREE(bf2);

  printf("-------------------------\n");
  //array
  array[0] = 'A';
  strcpy(&array[18] , "hello world!");
  array[sizeof(array)-1] = 'Z';
  printf("array size:%d info:[%c][%s][%c]\n" , sizeof(array) , array[0],&array[18] , array[sizeof(array)-1]);

  bf1 = STLV_BUFF_ALLOC(sizeof(array));
  len1 = STLV_PACK_ARRAY(bf1 , array , sizeof(array));
  printf("pack array len is %d\n" , len1);
  STLV_DUMP_PACK(bf1);

    //unpack
  memset(array , 0 , sizeof(array));
  len2 = STLV_UNPACK(&type , array , bf1 , len1); 
  printf("unpack from array len:%d and result type:%d value:[%c][%s][%c] len is %d\n" , len1 , type , array[0],&array[18] , 
  array[sizeof(array)-1] , len2);

  STLV_BUFF_FREE(bf1);

  printf("-------------------------\n");
  //float
  printf("float value:%lf\n" , float64);
  bf1 = STLV_BUFF_ALLOC(sizeof(float64));
  len1 = STLV_PACK_ARRAY(bf1 , &float64 , sizeof(float64));
  printf("pack float64 len is %d\n" , len1);

    //unpack
  float64 = 0;
  len2 = STLV_UNPACK(&type , &float64 , bf1 , len1);
  printf("unpack from float64 len:%d and result type:%d value %lf len is %d\n" , len1 , type , float64 , len2);
  STLV_BUFF_FREE(bf1);

  return 0;
}
