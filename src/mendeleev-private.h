/*
 * Copyright © 2010-2012 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef MENDELEEV_PRIVATE_H
#define MENDELEEV_PRIVATE_H

#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <config.h>

#include "mendeleev.h"

MENDELEEV_BEGIN_DECLS

/* It's not really the minimal length (the real one is report slave ID
 * in RTU (4 bytes)) but it's a convenient size to use in RTU or TCP
 * communications to read many values or write a single one.
 * Maximum between :
 * - HEADER_LENGTH_TCP (7) + function (1) + address (2) + number (2)
 * - HEADER_LENGTH_RTU (1) + function (1) + address (2) + number (2) + CRC (2)
 */

/* Timeouts in microsecond (0.5 s) */
#define _RESPONSE_TIMEOUT    500000
#define _BYTE_TIMEOUT        500000

typedef struct _mendeleev_backend {
    int (*set_slave) (mendeleev_t *ctx, int slave);
    int (*build_request_basis) (mendeleev_t *ctx, uint8_t command, uint8_t *req);
    int (*send_msg_pre) (uint8_t *req, int req_length);
    ssize_t (*send) (mendeleev_t *ctx, const uint8_t *req, int req_length);
    int (*receive) (mendeleev_t *ctx, uint8_t *req);
    ssize_t (*recv) (mendeleev_t *ctx, uint8_t *rsp, int rsp_length);
    int (*check_integrity) (mendeleev_t *ctx, uint8_t *msg,
                            const int msg_length);
    int (*pre_check_confirmation) (mendeleev_t *ctx, const uint8_t *req,
                                   const uint8_t *rsp, int rsp_length);
    int (*connect) (mendeleev_t *ctx);
    void (*close) (mendeleev_t *ctx);
    int (*flush) (mendeleev_t *ctx);
    int (*select) (mendeleev_t *ctx, fd_set *rset, struct timeval *tv, int msg_length);
    void (*free) (mendeleev_t *ctx);
} mendeleev_backend_t;

struct _mendeleev {
    /* Slave address */
    int slave;
    /* Socket or file descriptor */
    int s;
    int debug;
    int error_recovery;
    struct timeval response_timeout;
    struct timeval byte_timeout;
    const mendeleev_backend_t *backend;
    void *backend_data;
};

void _init_common(mendeleev_t *ctx);
void _error_print(mendeleev_t *ctx, const char *context);
int _receive_msg(mendeleev_t *ctx, uint8_t *msg);

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t dest_size);
#endif

MENDELEEV_END_DECLS

#endif  /* MENDELEEV_PRIVATE_H */
