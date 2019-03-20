/*
 * Copyright © 2001-2013 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef MENDELEEV_H
#define MENDELEEV_H

#include <sys/param.h>
#include <stdint.h>

#include "mendeleev-version.h"

#define MENDELEEV_API

#ifdef  __cplusplus
# define MENDELEEV_BEGIN_DECLS  extern "C" {
# define MENDELEEV_END_DECLS    }
#else
# define MENDELEEV_BEGIN_DECLS
# define MENDELEEV_END_DECLS
#endif

MENDELEEV_BEGIN_DECLS

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* Mendeleev commands */
#define MENDELEEV_CMD_SET_COLOR   0x00
#define MENDELEEV_CMD_SET_MODE    0x01
#define MENDELEEV_CMD_OTA         0x02
#define MENDELEEV_CMD_GET_VERSION 0x03
#define MENDELEEV_CMD_SET_OUTPUT  0x04

#define MENDELEEV_BROADCAST_ADDRESS    0xFF

/* Random number to avoid errno conflicts */
#define MENDELEEV_ENOBASE 112345678

/* Protocol exceptions */
enum {
    MENDELEEV_EXCEPTION_ILLEGAL_FUNCTION = 0x01,
    MENDELEEV_EXCEPTION_ILLEGAL_DATA_ADDRESS,
    MENDELEEV_EXCEPTION_ILLEGAL_DATA_VALUE,
    MENDELEEV_EXCEPTION_SLAVE_OR_SERVER_FAILURE,
    MENDELEEV_EXCEPTION_ACKNOWLEDGE,
    MENDELEEV_EXCEPTION_SLAVE_OR_SERVER_BUSY,
    MENDELEEV_EXCEPTION_NEGATIVE_ACKNOWLEDGE,
    MENDELEEV_EXCEPTION_MEMORY_PARITY,
    MENDELEEV_EXCEPTION_NOT_DEFINED,
    MENDELEEV_EXCEPTION_GATEWAY_PATH,
    MENDELEEV_EXCEPTION_GATEWAY_TARGET,
    MENDELEEV_EXCEPTION_MAX
};

#define EMBXILFUN  (MENDELEEV_ENOBASE + MENDELEEV_EXCEPTION_ILLEGAL_FUNCTION)
#define EMBXILADD  (MENDELEEV_ENOBASE + MENDELEEV_EXCEPTION_ILLEGAL_DATA_ADDRESS)
#define EMBXILVAL  (MENDELEEV_ENOBASE + MENDELEEV_EXCEPTION_ILLEGAL_DATA_VALUE)
#define EMBXSFAIL  (MENDELEEV_ENOBASE + MENDELEEV_EXCEPTION_SLAVE_OR_SERVER_FAILURE)
#define EMBXACK    (MENDELEEV_ENOBASE + MENDELEEV_EXCEPTION_ACKNOWLEDGE)
#define EMBXSBUSY  (MENDELEEV_ENOBASE + MENDELEEV_EXCEPTION_SLAVE_OR_SERVER_BUSY)
#define EMBXNACK   (MENDELEEV_ENOBASE + MENDELEEV_EXCEPTION_NEGATIVE_ACKNOWLEDGE)
#define EMBXMEMPAR (MENDELEEV_ENOBASE + MENDELEEV_EXCEPTION_MEMORY_PARITY)
#define EMBXGPATH  (MENDELEEV_ENOBASE + MENDELEEV_EXCEPTION_GATEWAY_PATH)
#define EMBXGTAR   (MENDELEEV_ENOBASE + MENDELEEV_EXCEPTION_GATEWAY_TARGET)

/* Native libmodbus error codes */
#define EMBBADCRC  (EMBXGTAR + 1)
#define EMBBADDATA (EMBXGTAR + 2)
#define EMBBADEXC  (EMBXGTAR + 3)
#define EMBUNKEXC  (EMBXGTAR + 4)
#define EMBMDATA   (EMBXGTAR + 5)
#define EMBBADSLAVE (EMBXGTAR + 6)

#define MENDELEEV_PREAMBLE_LENGTH    8
#define MENDELEEV_ADDR_LENGTH        1
#define MENDELEEV_DEST_LENGTH        (MENDELEEV_ADDR_LENGTH)
#define MENDELEEV_SRC_LENGTH         (MENDELEEV_ADDR_LENGTH)
#define MENDELEEV_SEQNR_LENGTH       2
#define MENDELEEV_HEADER_LENGTH      (MENDELEEV_PREAMBLE_LENGTH + MENDELEEV_DEST_LENGTH + MENDELEEV_SRC_LENGTH + MENDELEEV_SEQNR_LENGTH)
#define MENDELEEV_CMD_LENGTH         1
#define MENDELEEV_DATALEN_LENGTH     2
// #define MENDELEEV_PRESET_RSP_LENGTH  2

#define MENDELEEV_PREAMBLE_OFFSET    0
#define MENDELEEV_DEST_OFFSET        (MENDELEEV_PREAMBLE_OFFSET + MENDELEEV_PREAMBLE_LENGTH)
#define MENDELEEV_SRC_OFFSET         (MENDELEEV_DEST_OFFSET + MENDELEEV_ADDR_LENGTH)
#define MENDELEEV_SEQNR_OFFSET       (MENDELEEV_SRC_OFFSET + MENDELEEV_ADDR_LENGTH)
#define MENDELEEV_CMD_OFFSET         (MENDELEEV_SEQNR_OFFSET + MENDELEEV_SEQNR_LENGTH)
#define MENDELEEV_DATALEN_OFFSET     (MENDELEEV_CMD_OFFSET + MENDELEEV_CMD_LENGTH)
#define MENDELEEV_DATA_OFFSET        (MENDELEEV_DATALEN_OFFSET + MENDELEEV_DATALEN_LENGTH)

#define MENDELEEV_CHECKSUM_LENGTH    2
#define MENDELEEV_MSG_OVERHEAD       (MENDELEEV_HEADER_LENGTH + MENDELEEV_CMD_LENGTH + MENDELEEV_DATALEN_LENGTH + MENDELEEV_CHECKSUM_LENGTH)

extern const unsigned int libmendeleev_version_major;
extern const unsigned int libmendeleev_version_minor;
extern const unsigned int libmendeleev_version_micro;

typedef struct _mendeleev mendeleev_t;

typedef enum
{
    MENDELEEV_ERROR_RECOVERY_NONE          = 0,
    MENDELEEV_ERROR_RECOVERY_LINK          = (1<<1),
    MENDELEEV_ERROR_RECOVERY_PROTOCOL      = (1<<2)
} error_recovery_mode;

MENDELEEV_API int mendeleev_set_slave(mendeleev_t* ctx, int slave);
MENDELEEV_API int mendeleev_get_slave(mendeleev_t* ctx);
MENDELEEV_API int mendeleev_set_error_recovery(mendeleev_t *ctx, error_recovery_mode error_recovery);
MENDELEEV_API int mendeleev_set_socket(mendeleev_t *ctx, int s);
MENDELEEV_API int mendeleev_get_socket(mendeleev_t *ctx);

MENDELEEV_API int mendeleev_get_response_timeout(mendeleev_t *ctx, uint32_t *to_sec, uint32_t *to_usec);
MENDELEEV_API int mendeleev_set_response_timeout(mendeleev_t *ctx, uint32_t to_sec, uint32_t to_usec);

MENDELEEV_API int mendeleev_get_byte_timeout(mendeleev_t *ctx, uint32_t *to_sec, uint32_t *to_usec);
MENDELEEV_API int mendeleev_set_byte_timeout(mendeleev_t *ctx, uint32_t to_sec, uint32_t to_usec);

MENDELEEV_API int mendeleev_connect(mendeleev_t *ctx);
MENDELEEV_API void mendeleev_close(mendeleev_t *ctx);

MENDELEEV_API void mendeleev_free(mendeleev_t *ctx);

MENDELEEV_API int mendeleev_flush(mendeleev_t *ctx);
MENDELEEV_API int mendeleev_set_debug(mendeleev_t *ctx, int flag);

MENDELEEV_API const char *mendeleev_strerror(int errnum);

MENDELEEV_API int mendeleev_send_command(mendeleev_t *ctx, uint8_t command, uint8_t *data, uint16_t data_length, uint8_t *rsp_buf, uint16_t *rsp_length);

MENDELEEV_API int mendeleev_receive(mendeleev_t *ctx, uint8_t *req);

MENDELEEV_API int mendeleev_receive_confirmation(mendeleev_t *ctx, uint8_t *rsp);

#include "mendeleev-rtu.h"

MENDELEEV_END_DECLS

#endif  /* MENDELEEV_H */
