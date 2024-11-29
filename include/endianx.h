#ifndef TRY_ENDIAN_H
#define TRY_ENDIAN_H

#define TRY_ORDER_LITTLE_ENDIAN 1
#define TRY_ORDER_BIG_ENDIAN 2

#include <byteswap.h>
#include <endian.h>
#include <stdint.h>
#include <type_traits>

template <typename T>
typename std::enable_if<sizeof(uint64_t) == sizeof(T), T>::type
byteswap(T val)
{
    return (T)bswap_64((uint64_t)val);
}

template <typename T>
typename std::enable_if<sizeof(uint32_t) == sizeof(T), T>::type
byteswap(T val)
{
    return bswap_32(val);
}

template <typename T>
typename std::enable_if<sizeof(uint16_t) == sizeof(T), T>::type
byteswap(T val)
{
    return bswap_16(val);
}

#if BYTE_ORDER == TRY_ORDER_LITTLE_ENDIAN
#define TRY_BYTE_ORDER TRY_ORDER_LITTLE_ENDIAN
#else
#define TRY_BYTE_ORDER TRY_ORDER_BIG_ENDIAN
#endif

#if TRY_BYTE_ORDER == TRY_ORDER_LITTLE_ENDIAN

/**
 * @brief 在小端机器上执行 byteswap，在大端机器上什么都不用
 */
template <typename T>
T byteswapOnLittleEndian(t val)
{
    return byteswap(val);
}

/**
 * @brief 在大端机器上执行 byteswap，在小端机器上什么都不用
 */
template <typename T>
T byteswapOnBigEndian(t val)
{
    return val;
}
#else

/**
 * @brief 在小端机器上执行 byteswap，在大端机器上什么都不用
 */
template <typename T>
T byteswapOnLittleEndian(T val)
{
    return byteswap(val);
}

/**
 * @brief 在小端机器上执行 byteswap，在大端机器上什么都不用
 */
template <typename T>
T byteswapOnBigEndian(T val)
{
    return val;
}

#endif

#endif // TRY_ENDIAN_H