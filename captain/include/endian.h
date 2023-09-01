#ifndef __CAPTAIN_ENDIAN_H__
#define __CAPTAIN_ENDIAN_H__

/* 这段代码是一个用于处理字节序（endianness）的工具，字节序是用来表示多字节数据在内存中的存储方式。
   常见的有大端序（big-endian）和小端序（little-endian）两种。 */


//小端 1  大端 2
#define CAPTAIN_LITTLE_ENDIAN 1
#define CAPTAIN_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>

namespace captain {

//根据数据类型的字节长度来选择合适的字节序转换函数，从而实现字节序的转换。
//std::enable_if 来限制函数只能在满足特定条件时才可用。
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    //bswap_64：这是一个内建的函数，它在 <byteswap.h> 头文件中定义。
    //它接受一个 64 位整数（uint64_t）作为参数，并返回其字节序逆转后的结果。
    return (T)bswap_64((uint64_t)value);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

/* 
在这段代码中，它首先检查宏 BYTE_ORDER 是否等于 BIG_ENDIAN，这是一个在系统中预定义的宏，表示大端字节序。
如果条件成立，就定义宏 CAPTAIN_BYTE_ORDER 为 CAPTAIN_BIG_ENDIAN，否则就定义为 CAPTAIN_LITTLE_ENDIAN。
 */
#if BYTE_ORDER == BIG_ENDIAN
#define CAPTAIN_BYTE_ORDER CAPTAIN_BIG_ENDIAN
#else
#define CAPTAIN_BYTE_ORDER CAPTAIN_LITTLE_ENDIAN
#endif

/* 
如果系统的字节序是大端字节序（CAPTAIN_BYTE_ORDER == CAPTAIN_BIG_ENDIAN），则对于 byteswapOnLittleEndian 函数，
它不进行任何字节序转换，直接返回传入的值 t；而对于 byteswapOnBigEndian 函数，它调用了之前定义的 byteswap 函数来执行字节序转换。
 */
#if CAPTAIN_BYTE_ORDER == CAPTAIN_BIG_ENDIAN
template<class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}

#else

/* 
如果系统的字节序是小端字节序，即 CAPTAIN_BYTE_ORDER != CAPTAIN_BIG_ENDIAN，那么 byteswapOnLittleEndian 
和 byteswapOnBigEndian 函数都会调用 byteswap 函数来执行字节序转换。
 */

template<class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

template<class T>
T byteswapOnBigEndian(T t) {
    return t;
}

#endif

}

#endif
