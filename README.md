# STLV
A Simple TLV-Format Pack and Unpack Library  
一个简单的基于tlv格式的数据封装和解封库

### 数据格式
数据格式主要基于Ber-TLV描述，并进行了一定的修改
格式地址升序  

* TAG (1字节)

|    字段  |    长度      |      说明    |
|:------:  | :----------:| :-----------:| 
| class    |   [6-7]bit  |       =1      |
| obj      |   [5] bit   |0：基础类型(primitive);1:复合类型(array or tlv) | 
| num(type)|   [0-4]bit  |数据类型 refer STLV_TYPE |

* LEN (1字节+(0|2|4)字节)

|    字段  |    长度       |      说明    |
|:------:  | :-----------:| :-----------:| 
| flag     |   [7]bit     |0:单字节表示长度; 1:后续的字节表示长度       |
| value    |   [0-6] bit  |flag==0:表示数据的实际长度; flag==1表示后续的存储长度的字节数 |
| extra    |   0,2,4字节  |flag==0 则extra=0; flag==1则value记录extra的字节数同时extra=2|4字节长，记录值的长度 |



* value (1-N字节)  
封装的数据

### 数据类型

|       类型        |   长度    |    说明       |
| :----------------:| :---------:|:-----------:|
| STLV_TYPE_INT8    |    1Byte   | 单字节整型   |
| STLV_TYPE_INT16   |    2Byte   | 双字节整型   |
| STLV_TYPE_INT32   |    4Btye   | 四字节整型   |
| STLV_TYPE_INT64   |    8Byte   | 八字节整型   |
| STLV_TYPE_FLOAT32 |    4Byte   | 单精度浮点型 |
| STLV_TYPE_FLOAT64 |    8Byte   | 双精度浮点型 |
| STLV_TYPE_ARRAY   |    nByte   | 字节数组     |
| STLV_TYPE_TLV     |    nByte   | 子TLV数据    |

说明:
* _整型数据打包时会转换成网络序，解包会转成主机序，适配大小端通信_
* _浮点型数据由于不同系统实现不同，打包时按字节复制透传，不做网络序转换_
* _复合数据类型按字节复制_  

### API
* ```STLV_BUFF_ALLOC(vlen)``` 为产生的STLV包分配缓冲区    
  * vlen:数据的长度
  * return:NULL 失败; ELSE:缓冲区指针

* ```STLV_BUFF_FREE(ppack)``` 释放由STLV_BUFF_ALLOC分配的缓冲区      
  * ppack:缓冲区地址

* STLV_PACK_** 打包数据  
  * p:封装后的缓冲区指针(unsigned char * )
  * v:将要封装的数据指针(unsigned char * )
  * l:数据长度(unsigned int)
  * return:0 failed; >0 封装后的长度(unsigned int)

|          接口                |       说明         |
|:---------------------------:|:------------------:|
|```STLV_PACK_CHAR(p,v)```    |   打包一个char整数      |       
|```STLV_PACK_SHORT(p,v)```   |   打包一个short整数     |
|```STLV_PACK_INT(p,v)```     |   打包一个int整数       |
|```STLV_PACK_LONG(p,v)```    |   打包一个long整数      |
|```STLV_PACK_LLONG(p,v)```   |   打包一个long long整数 |
|```STLV_PACK_FLOAT(p,v)```   |   打包一个float浮点数   |
|```STLV_PACK_DOUBLE(p,v)```  |   打包一个double浮点数  |
|```STLV_PACK_ARRAY(p,v,l)``` |   打包一个字节数组      |
|```STLV_PACK_TLV(p,v,l)```   |   打包一个TLV格式数据   |  

* ```STLV_UNPACK(t , v , p , l)```  解包一个STLV格式封装的数据
  * t: 解封的数据类型(char * ) refer STLV_TYPE_xx
  * v: 解封后的数据地址(unsigned char * )
  * p: 待解封的STLV包缓冲区地址(unsigned char * )
  * l: 待解封的STLV包缓冲区长(unsigned int)
  * return: 0 failed; >0 解装后值的长度(unsigned int)  
  * _ps: 对于值为TLV类型的包，会持续解封到值为基本类型或者字节数组为止_
  
  


