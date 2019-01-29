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

* LEN (1字节)

|    字段  |    长度      |      说明    |
|:------:  | :----------:| :-----------:| 
| flag     |   [7]bit    |0:单字节表示长度; 1:后续的字节表示长度       |
| value    |   [0-6] bit |flag==0:表示数据的实际长度; flag==1表示后续的存储长度的字节数 | 

* extra (0|2|4字节)
