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
 * 设置STLV的LOG接口
 * 如果不设置则默认使用自己的日志(stlv.log)
 * @n:外部打开的slog句柄
 */
#define STLV_SET_LOG(n) (set_stlv_log(n))

/*
 * 设置对数据进行CHECK-SUM校验的最大长度
 * -1:不进行check-sum校验
 * 0:全部数据进行校验[默认]
 * >0:从首字节开始最多的校验字节数(如果数据不足该长度则以实际长度进行校验)
 * @return:0:success -1:failed
 */
#define STLV_CHECK_SUM_SIZE(n) (set_stlv_check_size(n))


/*
 * 不解包而获得一个STLV包的数据信息
 * p:stlv包的地址(char *)
 * v:stlv包内数据的地址(char **)
 * @return:包内数据的长度
 */
#define STLV_VALUE_INFO(p , v) (stlv_value_info(p , v))

/*
 * 打包STLV
 * PACK_STLV_XX
 * @p:封装后的缓冲区指针(unsigned char *)
 * @v:将要封装的数据指针(unsigned char *)
 * @[l]数据长度(unsigned int)
 * @return: -1 failed; >=0 封装后的长度(unsigned int)
 * warn:浮点型不会进行网络序转换，而直接按字节复制透传
 */
//打包char
#define STLV_PACK_CHAR(p , v) pack_stlv(STLV_TYPE_INT8 , (unsigned char *)(p) , (unsigned char *)(v) , 0)
//打包short
#define STLV_PACK_SHORT(p , v) pack_stlv(STLV_TYPE_INT16 , (unsigned char *)(p) , (unsigned char *)(v) , 0)
//打包int
#define STLV_PACK_INT(p , v) pack_stlv(STLV_TYPE_INT32 , (unsigned char *)(p) , (unsigned char *)(v) , 0)
//打包long
#define STLV_PACK_LONG(p , v) pack_stlv_long((p) , (v))
//打包LONGLONG
#define STLV_PACK_LLONG(p , v) pack_stlv(STLV_TYPE_INT64 , (unsigned char *)(p) , (unsigned char *)(v) , 0)
//打包 FLOAT32
#define STLV_PACK_FLOAT(p , v) pack_stlv(STLV_TYPE_FLOAT32 , (unsigned char *)(p) , (unsigned char *)(v) , 0);
//打包 FLOAT64
#define STLV_PACK_DOUBLE(p , v) pack_stlv(STLV_TYPE_FLOAT64 , (unsigned char *)(p) , (unsigned char *)(v) , 0);
//打包数组
#define STLV_PACK_ARRAY(p , v , l) pack_stlv(STLV_TYPE_ARRAY , (unsigned char *)(p) , (unsigned char *)(v) , (l))
//打包STLV包
#define STLV_PACK_TLV(p , v ,l) pack_stlv(STLV_TYPE_TLV , (unsigned char *)(p) , (unsigned char *)(v) , (l))


#define STLV_UNPACK_FAIL_ERR -1	//数据格式错误
#define STLV_UNPACK_FAIL_BUFF_LEN -2 //待解包缓冲区长度不足
#define STLV_UNPACK_FAIL_CHECK_SUM -3 //校验和错误
/*
 * STLV解包
  * @info: (int *)
 *     解封成功则>=0 填充解封的数据类型 refer STLV_T_PR_XX or STLV_T_CO_XX
 *     解封失败则<0   填充错误码 refer STLV_UNPACK_FAIL_XX
 * @v:解封的数据地址(unsigned char *)
 * @p:待解封数据的缓冲区地址(unsigned char *)
 * @l: 待解封数据的缓冲区长(unsigned int)
 * @rl:本次解压获得的STLV完整包长度(unsigned int *)
 * @return: 0 failed; >0 解装后值的长度(unsigned int)
 * PS:对于值为TLV的包，会递归解封到值为基本类型或者字节数组为止
 */
#define STLV_UNPACK(t , v , p , l , rl) unpack_stlv((t) , (unsigned char *)(v) , (unsigned char *)(p) , (l) , (rl))

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
