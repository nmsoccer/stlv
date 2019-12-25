/*
 * stlv_base.h
 *
 *  Created on: 2019年1月24日
 *      Author: nmsoccer
 */

#ifndef PERFECT_STLV_STLV_BASE_H_
#define PERFECT_STLV_STLV_BASE_H_

#ifdef __cplusplus
extern "C"{
#endif

/*=================STLV====================*/
/****
 * Instruction:
 * Simple TLV is mainly based on Ber-TLV
 * Tag -- Len -- Content
 * =======TAG========
 * Tag is fixed 1 byte
 * ****high              ----                  low
 * ****01                [0/1]             xxxxx
 * ****class            obj                tag-number(type)
 *
 * =======LEN========
 * Length is flexible
 * ****[0][length] 1byte
 * ****[1][length-bytes][...]
 *
 * =======CONTENT========
 * Content flexible
 *[....]
 */
/*==========================================*/


/***DECORATE*/
#define API
#define NO_API

#define IN
#define OUT

/***LOG LEVEL*/
#define STLV_LOG_LEVEL SL_DEBUG
#define STLV_LOG_NAME	"./stlv.log"
/*
 * TAG AREA
 */
#define STLV_T_CLASS 1 // application

#define STLV_T_OBJ_PRIMITIVE 0  //primitive obj
#define STLV_T_OBJ_COMPOS 1  //constructed obj

//primitive obj -> type
#define STLV_T_PR_MIN 0x01
#define STLV_T_PR_INT8 0x01  //1byte
#define STLV_T_PR_INT16  0x02  //2bytes
#define STLV_T_PR_INT32 0x03  //4bytes
#define STLV_T_PR_INT64 0x04  //8bytes
#define STLV_T_PR_FLOAT32 0x05 //4bytes float
#define STLV_T_PR_FLOAT64 0x06 //8bytes double
#define STLV_T_PR_MAX 0x06

//constructed obj -> type
#define STLV_T_CO_MIN 0x11
#define STLV_T_CO_ARRAY 0x11  //byte array
#define STLV_T_CO_TLV 0x12  //another tlv
#define STLV_T_CO_MAX 0x12

//type-->obj
#define STLV_TYPE_2_OBJ(v) (((v)&0x10) >> 4)

/*
 * LEN AREA
 */
#define STLV_L_FLG_SINGLE	0 //1byte
#define STLV_L_FLG_MULTI 	1 //multi bytes

//safe len
#define STLV_PACK_SAFE_LEN(vlen) (2+sizeof(int)+2+vlen) //tag+len+most(len.extra)+value_len

//FUNCTION DECLARE --- NO APIs
NO_API unsigned int pack_stlv(char type , OUT unsigned char *packed_buff , IN unsigned char *value , unsigned int vlen);
NO_API unsigned int pack_stlv_long(unsigned char *packed_buff , unsigned char *value);
NO_API unsigned int unpack_stlv(char *info , OUT unsigned char *value , IN unsigned char *src_buff , unsigned int buff_len ,
		unsigned int *pkg_len);

#ifdef __cplusplus
}
#endif
#endif /* PERFECT_STLV_STLV_BASE_H_ */
