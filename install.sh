#!/bin/bash
SRC_FILE=(stlv.c)
HEADER_FILE='stlv.h stlv_base.h'
STATIC_LIB=libstlv.a
HEADER_DIR=/usr/local/include/stlv/
LIB_DIR=/usr/local/lib

#compile
for file in ${SRC_FILE[@]}
do
  gcc -g -c ${file}
  if [[ $? -ne 0 ]]
  then
    echo "compile ${file} failed!"
    exit 1
  fi
done


#static lib
ar csrv ${STATIC_LIB} *.o
if [[ ! -e ${STATIC_LIB} ]]
then
  echo "create ${STATIC_LIB} failed!"
  exit 1
fi
rm *.o

#install header
mkdir -p ${HEADER_DIR}
cp ${HEADER_FILE} ${HEADER_DIR}
if [[ $? -ne 0 ]]
then
  echo "install ${HEADER_FILE} to ${HEADER_DIR} failed!"
  exit 1
fi

#install static lib
cp ${STATIC_LIB} ${LIB_DIR}
if [[ $? -ne 0 ]]
then
  echo "install ${STATIC_LIB} to ${LIB_DIR} failed!"
  exit 1
fi
rm ${STATIC_LIB}

echo "install success!"
