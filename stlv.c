/*
 * stlv.c
 *
 *  Created on: 2019年1月17日
 *      Author: nmsoccer
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <netinet/in.h>
#include "stlv.h"

extern int errno;

/*
 * LEN SETTNG
 */
#define STLV_LEN_AREA_1B 127 //2^7-1 share len byte.

#define STLV_LEN_AREA_UINT16 65534 //UINT16_MAX-1  extra 2bytes
#define STLV_LEN_AREA_UINT16_SIZE 2

#define STLV_LEN_AREA_UINT32  4294967294 //UINT32_MAX-1 extra 4bytes
#define STLV_LEN_AREA_UINT32_SIZE 4

#ifdef STLV_LITTLE_ENDIAN
typedef struct
{
	unsigned char num:5;
	unsigned char obj:1;
	unsigned char class:2;
}STLV_TAG;
#endif

#ifdef STLV_BIG_ENDIAN
typedef struct
{
	unsigned char class:2;
	unsigned char obj:1;
	unsigned char num:5;
}STLV_TAG;
#endif

/*
 * LEN AREA
 */
#ifdef STLV_LITTLE_ENDIAN
typedef struct
{
	unsigned char value:7;
	unsigned char flag:1; //0:value->real_len; 1:value->bytes of real_len
	unsigned char extra[0]; //if flag==1
}STLV_LEN;
#endif

#ifdef STLV_BIG_ENDIAN
typedef struct
{
	unsigned char flag:1;
	unsigned char value:7;
	unsigned char extra[0];	//if flag==1
}STLV_LEN;
#endif

/*
 * STLV
 */
typedef struct
{
	STLV_TAG tag;
	STLV_LEN len;
	unsigned char value[0];	//addr will be floated by len.extra in constructed obj
	//unsigned short check_sum  is append to tail of value
}STLV_PACK;

/*
 * STLV_ENV
 */
typedef struct
{
	int slog_fd; //slog descriptor
	int check_size;	//check-sum size [-1:no check; 0: all of data >0: min(size , real_value_len)]
}STLV_ENV;

static STLV_ENV stlv_env = {-1};

/*******************STATIC FUNC******/
static int _init_stlv_env(STLV_ENV *penv);
static unsigned int _pack_stlv_primitive(STLV_ENV *penv , char type , unsigned char *packed_buff , unsigned char *value);
static unsigned int _pack_stlv_construct(STLV_ENV *penv , char type , unsigned char *packed_buff , unsigned char *value , unsigned int vlen);
static unsigned int _stlv_value_info(STLV_ENV *penv , STLV_PACK *pack , unsigned char **start);
static unsigned short _stlv_check_sum(STLV_ENV *penv , STLV_PACK *pack);
static unsigned short _check_sum(unsigned char *array , int v_len);

/*******************API FUNC******/
/*
 * pack_stlv
 * 封装数据为stlv包
 * @type:refer STLV_T_PR_XX or STLV_T_CO_XX
 * @packed_buff:封装后的缓冲区
 * @value:将要封装的数据
 * @v_len:数据长度
 * @return: 0 failed; >0 封装后的长度
 */
NO_API unsigned int pack_stlv(char type , OUT unsigned char *packed_buff , IN unsigned char *value , unsigned int vlen)
{
	int slog_fd = -1;
	char obj = -1;
	unsigned int ret = 0;

	/***Try Init Env*/
	STLV_ENV *penv = &stlv_env;
	if(penv->slog_fd < 0)
		_init_stlv_env(penv);
	slog_fd = penv->slog_fd;

	/***Arg Check*/
	if(!packed_buff || !value)
	{
		slog_log(slog_fd , SL_ERR , "%s failed! some arg nil!" , __FUNCTION__);
		return 0;
	}

	/***Get OBJ*/
	obj = STLV_TYPE_2_OBJ(type);
	if(obj!=STLV_T_OBJ_PRIMITIVE && obj!=STLV_T_OBJ_COMPOS)
	{
		slog_log(slog_fd , SL_ERR , "%s failed! type is illegal!" , __FUNCTION__ , type);
		return 0;
	}

	/***PACK*/
	if(obj == STLV_T_OBJ_PRIMITIVE)
		ret =  _pack_stlv_primitive(penv , type , packed_buff , value);
	else
		ret =  _pack_stlv_construct(penv , type , packed_buff , value , vlen);

	if(!ret)
	{
		slog_log(slog_fd , SL_ERR , "%s failed! type:%d" , __FUNCTION__ , type);
		return 0;
	}

	/***CheckSum*/
	*((unsigned short *)(packed_buff+ret)) = htons(_stlv_check_sum(penv , (STLV_PACK  *)packed_buff));
	return ret + 2;	//include 2bytes of check_sum
}


/*
 * unpack_stlv
 * 解封STLV包
 * @info:
 *     解封成功则>=0 填充解封的数据类型 refer STLV_T_PR_XX or STLV_T_CO_XX
 *     解封失败则<0   填充错误码 refer STLV_UNPACK_FAIL_XX
 * @value:解封的数据
 * @src_buff:待解封数据的缓冲区
 * @buff_len:待解封数据的缓冲区长
 * @pkg_len:if not NULL,则返回本次解压获得的STLV完整包长度
 * @return: 0 failed; >0 解装后值的长度
 * ps:
 */
#define TEST_FILL_INFO_VALUE(pi , v) \
do \
{ \
	if(pi) \
		*pi = v; \
} \
while(0) \


NO_API unsigned int unpack_stlv(char *info , OUT unsigned char *value , IN unsigned char *src_buff , unsigned int buff_len ,
		unsigned int *pkg_len)
{
	int slog_fd = -1;
	STLV_PACK *pack = (STLV_PACK *)src_buff;

	unsigned int v_len = 0;
	unsigned char *v_start = NULL;
	int head_len = 0;

	unsigned short check_sum = 0;

	/***Try Init Env*/
	STLV_ENV *penv = &stlv_env;
	if(penv->slog_fd < 0)
		_init_stlv_env(penv);

	slog_fd = penv->slog_fd;
	/***Arg Check*/
	if(!info || !value || !src_buff)
	{
		slog_log(slog_fd , SL_ERR , "%s failed! some arg NULL!" , __FUNCTION__);
		TEST_FILL_INFO_VALUE(info , STLV_UNPACK_FAIL_ERR);
		return 0;
	}

	if(buff_len <= sizeof(STLV_PACK))
	{
		slog_log(slog_fd , SL_ERR , "%s failed! stlv pack len illegal! %d" , __FUNCTION__ , buff_len);
		TEST_FILL_INFO_VALUE(info , STLV_UNPACK_FAIL_BUFF_LEN);
		return 0;
	}

	head_len = sizeof(STLV_PACK);
	/***TAG*/
	if(pack->tag.class != STLV_T_CLASS)
	{
		slog_log(slog_fd , SL_ERR , "%s failed! stlv pack class illegal! %d" , __FUNCTION__ , pack->tag.class);
		TEST_FILL_INFO_VALUE(info , STLV_UNPACK_FAIL_ERR);
		return 0;
	}

	if(pack->tag.obj==STLV_T_OBJ_PRIMITIVE && (pack->tag.num<STLV_T_PR_MIN||pack->tag.num>STLV_T_PR_MAX))
	{
		slog_log(slog_fd , SL_ERR , "%s failed! primitive type illegal! obj:%d type:%d" , __FUNCTION__ , pack->tag.obj , pack->tag.num);
		TEST_FILL_INFO_VALUE(info , STLV_UNPACK_FAIL_ERR);
		return 0;
	}

	if(pack->tag.obj==STLV_T_OBJ_COMPOS && (pack->tag.num<STLV_T_CO_MIN||pack->tag.num>STLV_T_CO_MAX))
	{
		slog_log(slog_fd , SL_ERR , "%s failed! constructive type illegal! obj:%d type:%d" , __FUNCTION__ , pack->tag.obj , pack->tag.num);
		TEST_FILL_INFO_VALUE(info , STLV_UNPACK_FAIL_ERR);
		return 0;
	}

	/***Fetch value*/
	if(pack->len.flag == STLV_L_FLG_SINGLE)	//one byte
	{
		v_len = pack->len.value;
	}
	else if(pack->len.flag == STLV_L_FLG_MULTI)	//multi bytes
	{
		if(pack->len.value == STLV_LEN_AREA_UINT16_SIZE)
		{
			if(head_len + STLV_LEN_AREA_UINT16_SIZE >= buff_len)
			{
				slog_log(slog_fd , SL_ERR , "%s failed! packed len is not enough! p_len:%d head_len:%d extra:%d" , __FUNCTION__ , buff_len ,
						head_len , STLV_LEN_AREA_UINT16_SIZE);
				TEST_FILL_INFO_VALUE(info , STLV_UNPACK_FAIL_BUFF_LEN);
				return 0;
			}
			head_len += STLV_LEN_AREA_UINT16_SIZE;
			v_len = ntohs(*((unsigned short *)&pack->len.extra[0]));
		}
		else if(pack->len.value == STLV_LEN_AREA_UINT32_SIZE)
		{
			if(head_len + STLV_LEN_AREA_UINT32_SIZE >= buff_len)
			{
				slog_log(slog_fd , SL_ERR , "%s failed! packed len is not enough! p_len:%d head_len:%d extra:%d" , __FUNCTION__ , buff_len ,
						head_len , STLV_LEN_AREA_UINT32_SIZE);
				TEST_FILL_INFO_VALUE(info , STLV_UNPACK_FAIL_BUFF_LEN);
				return 0;
			}
			head_len += STLV_LEN_AREA_UINT32_SIZE;
			v_len = ntohl(*((unsigned int *)&pack->len.extra[0]));
		}
		else
		{
			slog_log(slog_fd , SL_ERR , "%s failed! multi size len.value illegal! %d" , __FUNCTION__ , pack->len.value);
			TEST_FILL_INFO_VALUE(info , STLV_UNPACK_FAIL_ERR);
			return 0;
		}
	}

	//check total_size
	if(head_len + v_len +2 >  buff_len)
	{
		slog_log(slog_fd , SL_ERR , "%s failed! packed_len is not enough! p_len:%d package len:%d" , __FUNCTION__ , buff_len ,
				head_len + v_len +2);
		TEST_FILL_INFO_VALUE(info , STLV_UNPACK_FAIL_BUFF_LEN);
		return 0;
	}

	//stlv real_size
	if(pkg_len)
		*pkg_len = (head_len + v_len + 2);

	//value start
	v_start = src_buff + head_len;

	/***Check Sum*/
	check_sum = ntohs(*((unsigned short *)(src_buff+head_len+v_len)));
	slog_log(slog_fd , SL_DEBUG , "%s check_sum:%04X" , __FUNCTION__ , check_sum);
	if(check_sum != _check_sum(v_start , v_len))
	{
		slog_log(slog_fd , SL_ERR , "%s failed! check_sum not match! pack_sum:%04X calc_sum:%04X" , __FUNCTION__ , check_sum ,
				_check_sum(v_start , v_len));
		TEST_FILL_INFO_VALUE(info , STLV_UNPACK_FAIL_CHECK_SUM);
		return 0;
	}

	/***UNPACK*/
	//constructed
	if(pack->tag.obj == STLV_T_OBJ_COMPOS)
	{
		//TLV
		if(pack->tag.num == STLV_T_CO_TLV)
			return unpack_stlv(info , value , v_start , v_len , NULL);

		//ARRAY
		*info = STLV_T_CO_ARRAY;
		memcpy(value , v_start , v_len);
		return v_len;
	}

	*info = pack->tag.num;
	//primitive
	switch(pack->tag.num)
	{
	case STLV_T_PR_INT8:
		value[0] = v_start[0];
	break;
	case STLV_T_PR_INT16:
		*((unsigned short *)value) = ntohs(*((unsigned short *)v_start));
	break;
	case STLV_T_PR_INT32:
		*((unsigned int *)value) = ntohl(*((unsigned int *)v_start));
	break;
	case STLV_T_PR_INT64:
		*((unsigned int *)value) = ntohl(*((unsigned int *)&v_start[4]));
		*((unsigned int *)&value[4]) = ntohl(*((unsigned int *)v_start));
	break;
	case STLV_T_PR_FLOAT32:
	case STLV_T_PR_FLOAT64:
		memcpy(value , v_start , v_len);
	break;
	default:
		slog_log(slog_fd , SL_ERR , "%s failed! primitive type:%d not support!" , __FUNCTION__ , *info);
		return 0;
	}
	return v_len;
}

//supported function of type:Long
NO_API unsigned int pack_stlv_long(unsigned char *packed_buff , unsigned char *value)
{
	unsigned int ret;
	do
		{
			if(sizeof(long) == 32)	//32bit OS
			{
				ret = pack_stlv(STLV_T_PR_INT32 , packed_buff , value , 0);
			}
			else	//64bit OS
			{
				ret = pack_stlv(STLV_T_PR_INT64 , packed_buff , value , 0);
			}
	}while(0);

	return ret;
}

//STLV_CHECK_SUM_SIZE
NO_API int set_stlv_check_size(int size)
{
	STLV_ENV *penv = &stlv_env;
	/***Try Init*/
	if(penv->slog_fd < 0)
		_init_stlv_env(penv);

	size = size<0?-1:size;
	slog_log(penv->slog_fd , SL_INFO , "%s set check-size from %d --> %d" , __FUNCTION__ , penv->check_size , size);
	penv->check_size = size;
	return 0;
}

NO_API  int dump_stlv_pack(unsigned char *pack_buff)
{
	STLV_ENV *penv = &stlv_env;
	int slog_fd = -1;
	int i = 0;
	int index = 0;
	int real_len = 0;
	STLV_PACK *pack = (STLV_PACK *)pack_buff;
	unsigned char *p_value = pack->value;

	char buff[1024] = {0};
	/***Try Init*/
	if(penv->slog_fd < 0)
		_init_stlv_env(penv);

	slog_fd = penv->slog_fd;
	if(slog_fd < 0)
		return -1;

	if(!pack)
	{
		slog_log(slog_fd , SL_ERR , "%s failed! pack null!" , __FUNCTION__);
		return -1;
	}

	/***Calc Real_Len*/
	if(pack->len.flag == STLV_L_FLG_SINGLE)	//one byte
	{
		real_len = pack->len.value;
	}
	else if(pack->len.flag == STLV_L_FLG_MULTI)	//multi bytes
	{
		if(pack->len.value == STLV_LEN_AREA_UINT16_SIZE)
		{
			real_len = ntohs(*((unsigned short *)&pack->len.extra[0]));
			p_value += STLV_LEN_AREA_UINT16_SIZE;
		}
		else if(pack->len.value == STLV_LEN_AREA_UINT32_SIZE)
		{
			real_len = ntohl(*((unsigned int *)&pack->len.extra[0]));
			p_value += STLV_LEN_AREA_UINT32_SIZE;
		}
		else
		{
			slog_log(slog_fd , SL_ERR , "%s fialed! multi size len.value illegal! %d" , __FUNCTION__ , pack->len.value);
			return -1;
		}
	}

	//chg attr
	slog_chg_attr(slog_fd , -1 , -1 , -1 , -1 , SLF_RAW);
	slog_log(slog_fd , SL_INFO , " ");

	/***Dump*/
	slog_log(slog_fd , SL_INFO , "===============STLV_PACK==============");
	slog_log(slog_fd , SL_INFO , "[TAG] class %d| obj %d | num(type) %d" , pack->tag.class , pack->tag.obj , pack->tag.num);
	slog_log(slog_fd , SL_INFO , "[LEN] flag %d | value %d | real_len %d" , pack->len.flag , pack->len.value , real_len);
		//value
	if(pack->tag.obj==STLV_T_OBJ_COMPOS && pack->tag.num==STLV_T_CO_TLV)
	{
		dump_stlv_pack(pack->value);
	}
	else
	{
		slog_log(slog_fd , SL_INFO , "[VALUE] ");
		for(i=0; i<real_len; i++)
		{
			snprintf(&buff[index] , sizeof(buff)-index , "\\x%02X " , p_value[i]);
			index += 5;	//0x%02x
			if(i!=0 && i%50 == 0)	//per 50bytes one line
			{
				slog_log(slog_fd , SL_INFO , "%s" , buff);
				memset(buff , 0 , sizeof(buff));
				index = 0;
			}
		}
		if(strlen(buff) > 0)
			slog_log(slog_fd , SL_INFO , "%s" , buff);
	}

		//check sum
	slog_chg_attr(slog_fd , -1 , -1 , -1 , -1 , SLF_RAW);
	slog_log(slog_fd , SL_INFO , "[SUM] ");
	slog_log(slog_fd , SL_INFO , "sum \\x%04X" , *(unsigned short *)(p_value+real_len));

	slog_log(slog_fd , SL_INFO , "===============END===================\n");
	slog_chg_attr(slog_fd , -1 , -1 , -1 , -1 , SLF_PREFIX);
	return 0;
}


/*******************STATIC FUNC*********/
static int _init_stlv_env(STLV_ENV *penv)
{
	SLOG_OPTION option;
	int ret = -1;

	if(!penv)
		return -1;

	if(penv->slog_fd>=0)
	{
		slog_log(penv->slog_fd , SL_INFO , "%s is inited!" , __FUNCTION__);
		return 0;
	}

	//open slog descriptor
	memset(&option , 0 , sizeof(option));
	strncpy(option.type_value._local.log_name , STLV_LOG_NAME , sizeof(option.type_value._local.log_name));

	ret = slog_open(SLT_LOCAL , STLV_LOG_LEVEL , &option , NULL);
	if(ret < 0)
	{
		printf("%s slog_open failed!\n" , __FUNCTION__);
		return -1;
	}

	penv->slog_fd = ret;
	slog_log(penv->slog_fd , SL_INFO , "%s success!" , __FUNCTION__);
	return 0;
}

//pack primitive value
//most arg no check
static unsigned int _pack_stlv_primitive(STLV_ENV *penv , char type , unsigned char *packed_buff , unsigned char *value)
{
	int slog_fd = penv->slog_fd;
	STLV_PACK *pack = (STLV_PACK *)packed_buff; //point to pack
	unsigned int pack_size = sizeof(STLV_PACK);	//total size

	if(type<STLV_T_PR_MIN || type>STLV_T_PR_MAX)
	{
		slog_log(slog_fd , SL_ERR , "%s failed! type:%d illegal!" , __FUNCTION__ , type);
		return 0;
	}

	/***Handle*/
	//1.general
	memset(pack , 0 , sizeof(STLV_PACK));
	pack->tag.class = STLV_T_CLASS;
	pack->tag.obj = STLV_T_OBJ_PRIMITIVE;
	pack->tag.num = type;

	pack->len.flag = STLV_L_FLG_SINGLE;

	//2.each type[use network endian]
	switch(type)
	{
	case STLV_T_PR_INT8:
		pack->len.value = sizeof(char);
		pack->value[0] = *value;
		break;
	break;

	case STLV_T_PR_INT16:
		pack->len.value = sizeof(short);
		*((unsigned short *)(&pack->value[0])) = htons(*value);
		break;
	break;

	case STLV_T_PR_INT32:
		pack->len.value = sizeof(int);
		*((unsigned int *)(&pack->value[0])) = htonl(*value);
		break;
	break;

	case STLV_T_PR_INT64:
		pack->len.value = sizeof(long long);
		*((unsigned int *)(&pack->value[0])) = htonl(*((unsigned int *)(value+4)));
		*((unsigned int *)(&pack->value[4])) = htonl(*((unsigned int *)value));
		break;

	case STLV_T_PR_FLOAT32:
		pack->len.value = sizeof(float);
		memcpy(pack->value , value , sizeof(float));
		break;

	case STLV_T_PR_FLOAT64:
		pack->len.value = sizeof(double);
		memcpy(pack->value , value , sizeof(double));
		break;

	default:
		slog_log(slog_fd , SL_ERR , "%s failed! Illegal primitive type:%d" , __FUNCTION__ , type);
		return 0;
	}

	pack_size += pack->len.value;
	return pack_size;
}

//pack constructed value
//most arg no check
static unsigned int _pack_stlv_construct(STLV_ENV *penv , char type , unsigned char *packed_buff , unsigned char *value , unsigned int vlen)
{
	int slog_fd = penv->slog_fd;
	STLV_PACK *pack = (STLV_PACK *)packed_buff; //point to pack
	unsigned char *p_value = pack->value;	//may float
	unsigned int pack_size = sizeof(STLV_PACK);	//total size

	if(type<STLV_T_CO_MIN || type>STLV_T_CO_MAX)
	{
		slog_log(slog_fd , SL_ERR , "%s failed! type:%d illegal!" , __FUNCTION__ , type);
		return 0;
	}

	/***Handle*/
	//1.general
	memset(pack , 0 , sizeof(STLV_PACK));
	pack->tag.class = STLV_T_CLASS;
	pack->tag.obj = STLV_T_OBJ_COMPOS;
	pack->tag.num = type;

	//2.calc len
	do
	{
		//<=127Bytes [use 1byte]
		if(vlen <= STLV_LEN_AREA_1B)
		{
			pack->len.flag = STLV_L_FLG_SINGLE;
			pack->len.value = vlen;
			break;
		}

		pack->len.flag = STLV_L_FLG_MULTI;

		//vlen <= 65534Bytes [UINT16_MAX-1  2bytes]
		if(vlen <= STLV_LEN_AREA_UINT16)
		{
			pack->len.value = STLV_LEN_AREA_UINT16_SIZE;
			*((unsigned short *)pack->len.extra) = htons((unsigned short)vlen);

			p_value += pack->len.value;
			pack_size += pack->len.value;
			break;
		}

		//vlen <= 4294967294 [UINT32_MAX-1  4 bytes]
		if(vlen <= STLV_LEN_AREA_UINT32)
		{
			pack->len.value = STLV_LEN_AREA_UINT32_SIZE;
			*((unsigned int *)pack->len.extra) = htonl((unsigned int)vlen);

			p_value += pack->len.value;
			pack_size += pack->len.value;
			break;
		}

		//vlen illegal
		slog_log(slog_fd , SL_ERR , "%s failed! len is too large:%ld" , __FUNCTION__ , vlen);
		return 0;

	}while(0);

	//2.copy value
	memcpy(p_value , value , vlen);
	pack_size += vlen;

	return pack_size;
}

/*
 * Get Info of STLV value
 * @v_start:返回的value起始地址 [NULL failed]
 * @return:value长度
 */
static unsigned int _stlv_value_info(STLV_ENV *penv , STLV_PACK *pack , unsigned char **v_start)
{
	int slog_fd = penv->slog_fd;
	int real_len = 0;
	int extra_size = 0;	 //len.extra
	*v_start = NULL;

	/***Arg Check*/
	if(!pack || !v_start)
	{
		slog_log(slog_fd , SL_ERR , "%s failed for arg NULL!" , __FUNCTION__);
		return 0;
	}

	/***Get Real_Len*/
	if(pack->len.flag == STLV_L_FLG_SINGLE)	//one byte
	{
		real_len = pack->len.value;
	}
	else if(pack->len.flag == STLV_L_FLG_MULTI)	//multi bytes
	{
		if(pack->len.value == STLV_LEN_AREA_UINT16_SIZE)
		{
			extra_size = STLV_LEN_AREA_UINT16_SIZE;
			real_len = ntohs(*((unsigned short *)&pack->len.extra[0]));
		}
		else if(pack->len.value == STLV_LEN_AREA_UINT32_SIZE)
		{
			extra_size = STLV_LEN_AREA_UINT32_SIZE;
			real_len = ntohl(*((unsigned int *)&pack->len.extra[0]));
		}
		else
		{
			slog_log(slog_fd , SL_ERR , "%s failed! multi size len.value illegal! %d" , __FUNCTION__ , pack->len.value);
			return 0;
		}
	}

	/***Set*/
	*v_start = &pack->len.extra[0] + extra_size;
	return real_len;
}


/*
 *计算pack的checksum值
 *arg no check
 */
static unsigned short _stlv_check_sum(STLV_ENV *penv , STLV_PACK *pack)
{
	unsigned char *v_start = NULL;
	unsigned int v_len = 0;
	unsigned short check_sum = 0;

	/***value info*/
	v_len = _stlv_value_info(penv , pack , &v_start);
	if(!v_start)
	{
		slog_log(penv->slog_fd , SL_ERR , "%s failed!" , __FUNCTION__);
		return 0;
	}

	/***check sum*/
	check_sum = _check_sum(v_start , v_len);
	//*((unsigned short *)(v_start + v_len)) = htons(check_sum);

	/***Return*/
	//slog_log(penv->slog_fd , SL_DEBUG , "%s check sum:0x%04X" , __FUNCTION__ , check_sum);
	return check_sum;
}


/*
 * check sum
 */
static unsigned short _check_sum(unsigned char *array , int v_len)
{
	unsigned int sum = 0;
	unsigned short value = 0;
	int i = 0;
	int len = v_len;
	STLV_ENV *penv = &stlv_env;

	if(len <= 1)
		return 0;

	if(penv->check_size < 0)
		return 0;

	if(penv->check_size > 0)
	{
		len = v_len>penv->check_size?penv->check_size:v_len;
		//slog_log(penv->slog_fd , SL_DEBUG , "%s check bytes is real:%d check-size:%d v_len:%d" , __FUNCTION__ , len , penv->check_size , v_len);
	}
	//calc each two bytes
	while(1)
	{
		if(i+2>len)
			break;

		sum = sum + ((array[i]<<8) + array[i+1]);
		i += 2;
	}

	//last byte
	if(i<len)
	{
		sum = sum + (array[len-1]<<8);
	}

	//low 16bit + high 16bit < 0xffff
	do
	{
		sum = (sum & 0x0000ffff) + (sum >> 16);
	}while(sum > 0xffff);

	//~
	value = sum & 0x0000ffff;
	value =  ~value;
	return value;
}

