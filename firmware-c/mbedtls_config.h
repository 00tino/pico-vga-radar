// Configuracion de mbedTLS para el radar.
//
// Lo unico que tiene que poder hacer es abrir una conexion TLS 1.2 de cliente
// contra el proxy en Vercel. Todo lo que no haga falta para eso esta apagado,
// porque el handshake es lo que mas memoria pide de todo el firmware.
//
// Que se deja prendido y por que:
//   - ECDHE + ECDSA con la curva P-256: es lo que negocia Vercel.
//   - RSA y SHA-256: para la cadena del certificado.
//   - AES-GCM: el cifrado del canal.
// Si algun dia el proxy cambia de hosting y el handshake falla, es aca donde
// hay que mirar primero.
#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

// Hay fuentes de mbedTLS que usan INT_MAX sin incluir limits.h. Sale del
// mbedtls_config.h de ejemplo del propio SDK.
#include <limits.h>

// La capa que enchufa mbedTLS abajo de lwIP mete mano en campos internos del
// contexto de TLS, asi que hay que dejarla. Y necesita el reloj en
// milisegundos para los reintentos; el de fecha NO se prende a proposito, ver
// abajo del todo.
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
#define MBEDTLS_HAVE_TIME
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MS_TIME_ALT

#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_SERVER_NAME_INDICATION

#define MBEDTLS_CIPHER_C
#define MBEDTLS_MD_C
#define MBEDTLS_AES_C
#define MBEDTLS_GCM_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA224_C
#define MBEDTLS_SHA512_C
#define MBEDTLS_SHA384_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_OID_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_RSA_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED

#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_CHECK_KEY_USAGE
#define MBEDTLS_X509_CHECK_EXTENDED_KEY_USAGE

// Tablas mas chicas a cambio de unos ciclos: la placa tiene de sobra y lo que
// falta es memoria.
#define MBEDTLS_AES_FEWER_TABLES
#define MBEDTLS_SHA256_SMALLER

#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_BASE64_C
#define MBEDTLS_PEM_PARSE_C

// El buffer de entrada tiene que aguantar un record entero de TLS, y ahi no
// hay margen para achicar: si el servidor manda uno de 16 kB y no entra, la
// conexion se corta. El de salida si, porque lo unico que mandamos es un GET
// de doscientos bytes.
#define MBEDTLS_SSL_IN_CONTENT_LEN  16384
#define MBEDTLS_SSL_OUT_CONTENT_LEN 2048
#define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH

#define MBEDTLS_ERROR_STRERROR_DUMMY

// MBEDTLS_HAVE_TIME_DATE queda afuera a proposito: la placa no tiene reloj
// con fecha, asi que no puede decir si un certificado vencio. Es parte de por
// que la conexion no verifica al servidor; el porque completo esta en sky.c.

#endif
