/*
 * stlv.h
 *
 *  Created on: 2019年1月17日
 *      Author: nmsoccer
 */

#ifndef PERFECT_STLV_STLV_H_
#define PERFECT_STLV_STLV_H_

#include <slog/slog.h>
#include "stlv_base.h"

#ifdef __cplusplus
extern "C"{
#endif

/**if CPU big endian should compile using gcc -DSTLV_BIG xx*/
#ifdef STLV_BIG
#define STLV_BIG_ENDIAN
#else
#define STLV_LITTLE_ENDIAN
#endif

/***Types*/
typedef enum
{
	STLV_TYPE_INT8 = STLV_T_PR_MIN,
	STLV_TYPE_INT16 ,
	STLV_TYPE_INT32,
	STLV_TYPE_INT64,
	STLV_TYPE_FLOAT32,
	STLV_TYPE_FLOAT64,
	STLV_TYPE_ARRAY = STLV_T_CO_MIN,
	STLV_TYPE_TLV
}STLV_TYPE;

/*=================APIs====================*/
/*
 * 为STLV_PACK分配内存
 * @vlen:数据的长度
 * @return:缓冲区地址
 */
#define STLV_BUFF_ALLOC(vlen) ((unsigned char *)calloc(1 , STLV_PACK_SAFE_LEN((vlen))))

/*
 * 释放STLV_PACK所分配内存
 * @ppack:缓冲区地址
 */
#define STLV_BUFF_FREE(ppack) (free((ppack)))

/*
 * 打包STLV
 * PACK_STLV_XX
 * @p:封装后的缓冲区指针(unsigned char *)
 * @v:将要封装的数据指针(unsigned char *)
 * @[l]数据长度(unsigned int)
 * @return: 0 failed; >0 封装后的长度(unsigned int)
 * warn:浮点型不会进行网络序转换，而直接按字节复制透传
 */
//打包char
#define STLV_PACK_CHAR(p , v) pack_stlv(STLV_TYPE_INT8 , (p) , (v) , 0)
//打包short
#define STLV_PACK_SHORT(p , v) pack_stlv(STLV_TYPE_INT16 , (p) , (v) , 0)
//打包int
#define STLV_PACK_INT(p , v) pack_stlv(STLV_TYPE_INT32 , (p) , (v) , 0)
//打包long
#define STLV_PACK_LONG(p , v) pack_stlv_long((p) , (v))
//打包LONGLONG
#define STLV_PACK_LLONG(p , v) pack_stlv(STLV_TYPE_INT64 , (p) , (v) , 0)
//打包 FLOAT32
#define STLV_PACK_FLOAT(p , v) pack_stlv(STLV_TYPE_FLOAT32 , (p) , (v) , 0);
//打包 FLOAT64
#define STLV_PACK_DOUBLE(p , v) pack_stlv(STLV_TYPE_FLOAT64 , (p) , (v) , 0);
//打包数组
#define STLV_PACK_ARRAY(p , v , l) pack_stlv(STLV_TYPE_ARRAY , (p) , (v) , (l))
//打包STLV包
#define STLV_PACK_TLV(p , v ,l) pack_stlv(STLV_TYPE_TLV , (p) , (v) , (l))


/*
 * STLV解包
 * @t:解封的数据类型 STL_TYPE(char *)
 * @v:解封的数据地址(unsigned char *)
 * @p:待解封的STLV包缓冲区地址(unsigned char *)
 * @l:STLV包缓冲区长(unsigned int)
 * @return: 0 failed; >0 解装后值的长度(unsigned int)
 * PS:对于值为TLV的包，会递归解封到值为基本类型或者字节数组为止
 */
#define STLV_UNPACK(t , v , p , l) unpack_stlv((t) , (v) , (p) , (l));

/*
 * 打印STLV包到日志
 * @p:stlv包地址
 * @return:-1 failed ==0 success
 */
#define STLV_DUMP_PACK(p) dump_stlv_pack((p))
/*==========================================*/

#ifdef __cplusplus
}
#endif
#endif /* PERFECT_STLV_STLV_H_ */
