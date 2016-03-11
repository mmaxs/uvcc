

#ifndef UVCC_ENDIAN__HPP
#define UVCC_ENDIAN__HPP

#include <cstdint>     // uint*_t
#ifdef _WIN32
#include <Winsock2.h>  // htons() htonl() ntohs() ntohl()
#else
#include <endian.h>    // htobe* be*toh
#endif


namespace uv
{
/*! \defgroup g__endian Byte order conversion
    \ingroup g__utility
    \details Templates of cross-platform (*nix/windows) functions for byte order conversion between host and network byte encoding.
    \sa For fairly complete version of C functions see e.g.: \n
    https://gist.github.com/panzi/6856583 \n
    https://github.com/blizzard4591/cmake-portable-endian */
//! \{

//! \cond
#ifdef _WIN32
#   define HOST_to_NET_16(x)  ( ::htons((uint16_t)(x)) )
#   define HOST_to_NET_32(x)  ( ::htonl((uint32_t)(x)) )
#   define NET_to_HOST_16(x)  ( ::ntohs((uint16_t)(x)) )
#   define NET_to_HOST_32(x)  ( ::ntohl((uint32_t)(x)) )
#   ifndef __BYTE_ORDER__
#       error __BYTE_ORDER__ macro not defined
#   endif
#   if defined(__ORDER_LITTLE_ENDIAN__) &&  (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#       define HOST_to_NET_64(x)  ( (uint64_t)::htonl((uint32_t)((uint64_t)(x) >> 32)) | ((uint64_t)::htonl((uint32_t)(x)) << 32) )
#       define NET_to_HOST_64(x)  ( (uint64_t)::ntohl((uint32_t)((uint64_t)(x) >> 32)) | ((uint64_t)::ntohl((uint32_t)(x)) << 32) )
#   elif defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#       define HOST_to_NET_64(x)  ( (uint64_t)(x) )
#       define NET_to_HOST_64(x)  ( (uint64_t)(x) )
#   else
#       error __BYTE_ORDER__ value not supported
#   endif
#else
#   define HOST_to_NET_16(x)  ( htobe16((uint16_t)(x)) )
#   define HOST_to_NET_32(x)  ( htobe32((uint32_t)(x)) )
#   define HOST_to_NET_64(x)  ( htobe64((uint64_t)(x)) )
#   define NET_to_HOST_16(x)  ( be16toh((uint16_t)(x)) )
#   define NET_to_HOST_32(x)  ( be32toh((uint32_t)(x)) )
#   define NET_to_HOST_64(x)  ( be64toh((uint64_t)(x)) )
#endif
//! \endcond


template< typename _I_ > inline uint16_t hton16(const _I_ &_i)  { return HOST_to_NET_16(_i); }  /*!< \brief Cross-platform `htons()` */
template< typename _I_ > inline uint32_t hton32(const _I_ &_i)  { return HOST_to_NET_32(_i); }  /*!< \brief Cross-platform `htonl()` */
template< typename _I_ > inline uint64_t hton64(const _I_ &_i)  { return HOST_to_NET_64(_i); }  /*!< \brief Cross-platform `htonll()` */

template< typename _I_ > inline uint16_t ntoh16(const _I_ &_i)  { return NET_to_HOST_16(_i); }  /*!< \brief Cross-platform `ntohs()` */
template< typename _I_ > inline uint32_t ntoh32(const _I_ &_i)  { return NET_to_HOST_32(_i); }  /*!< \brief Cross-platform `ntohl()` */
template< typename _I_ > inline uint64_t ntoh64(const _I_ &_i)  { return NET_to_HOST_64(_i); }  /*!< \brief Cross-platform `ntohll()` */


#undef HOST_to_NET_16
#undef HOST_to_NET_32
#undef HOST_to_NET_64
#undef NET_to_HOST_16
#undef NET_to_HOST_32
#undef NET_to_HOST_64

//! \}
}

#endif
