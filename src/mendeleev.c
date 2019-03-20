/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 *
 * This library implements the Modbus protocol.
 * http://libmodbus.org/
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>

#include <config.h>

#include "mendeleev.h"
#include "mendeleev-private.h"

/* Internal use */
#define MSG_LENGTH_UNDEFINED -1

/* Exported version */
const unsigned int libmendeleev_version_major = LIBMENDELEEV_VERSION_MAJOR;
const unsigned int libmendeleev_version_minor = LIBMENDELEEV_VERSION_MINOR;
const unsigned int libmendeleev_version_micro = LIBMENDELEEV_VERSION_MICRO;

/* Max message length */
#define MAX_MESSAGE_LENGTH 260

const char *mendeleev_strerror(int errnum) {
    switch (errnum) {
    case EMBXILFUN:
        return "Illegal function";
    case EMBXILADD:
        return "Illegal data address";
    case EMBXILVAL:
        return "Illegal data value";
    case EMBXSFAIL:
        return "Slave device or server failure";
    case EMBXACK:
        return "Acknowledge";
    case EMBXSBUSY:
        return "Slave device or server is busy";
    case EMBXNACK:
        return "Negative acknowledge";
    case EMBXMEMPAR:
        return "Memory parity error";
    case EMBXGPATH:
        return "Gateway path unavailable";
    case EMBXGTAR:
        return "Target device failed to respond";
    case EMBBADCRC:
        return "Invalid CRC";
    case EMBBADDATA:
        return "Invalid data";
    case EMBBADEXC:
        return "Invalid exception code";
    case EMBMDATA:
        return "Too many data";
    case EMBBADSLAVE:
        return "Response not from requested slave";
    default:
        return strerror(errnum);
    }
}

void _error_print(mendeleev_t *ctx, const char *context)
{
    if (ctx->debug) {
        fprintf(stderr, "ERROR %s", mendeleev_strerror(errno));
        if (context != NULL) {
            fprintf(stderr, ": %s\n", context);
        } else {
            fprintf(stderr, "\n");
        }
    }
}

static void _sleep_response_timeout(mendeleev_t *ctx)
{
    /* Response timeout is always positive */
    /* usleep source code */
    struct timespec request, remaining;
    request.tv_sec = ctx->response_timeout.tv_sec;
    request.tv_nsec = ((long int)ctx->response_timeout.tv_usec) * 1000;
    while (nanosleep(&request, &remaining) == -1 && errno == EINTR) {
        request = remaining;
    }
}

int mendeleev_flush(mendeleev_t *ctx)
{
    int rc;

    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    rc = ctx->backend->flush(ctx);
    if (rc != -1 && ctx->debug) {
        /* Not all backends are able to return the number of bytes flushed */
        printf("Bytes flushed (%d)\n", rc);
    }
    return rc;
}

/* Computes the length of the expected response */
static unsigned int compute_response_length_from_request(mendeleev_t *ctx, uint8_t *req)
{
    int length;

    switch (req[MENDELEEV_CMD_OFFSET]) {
    case MENDELEEV_CMD_SET_COLOR:
    case MENDELEEV_CMD_SET_MODE:
    case MENDELEEV_CMD_SET_OUTPUT:
    case MENDELEEV_CMD_OTA:
        length = 0;
        break;
    case MENDELEEV_CMD_GET_VERSION:
        return MSG_LENGTH_UNDEFINED;
    default:
        /* we do not expect any data in the response */
        length = 0;
    }

    return MENDELEEV_DATA_OFFSET + length + MENDELEEV_CHECKSUM_LENGTH;
}

/* Sends a request/response */
static int send_msg(mendeleev_t *ctx, uint8_t *msg, int msg_length)
{
    int rc;
    int i;

    // Adds CRC
    msg_length = ctx->backend->send_msg_pre(msg, msg_length);

    if (ctx->debug) {
        for (i = 0; i < msg_length; i++)
            printf("[%.2X]", msg[i]);
        printf("\n");
    }

    /* In recovery mode, the write command will be issued until to be
       successful! Disabled by default. */
    do {
        rc = ctx->backend->send(ctx, msg, msg_length);
        if (rc == -1) {
            _error_print(ctx, NULL);
            if (ctx->error_recovery & MENDELEEV_ERROR_RECOVERY_LINK) {
                int saved_errno = errno;

                if ((errno == EBADF || errno == ECONNRESET || errno == EPIPE)) {
                    mendeleev_close(ctx);
                    _sleep_response_timeout(ctx);
                    mendeleev_connect(ctx);
                } else {
                    _sleep_response_timeout(ctx);
                    mendeleev_flush(ctx);
                }
                errno = saved_errno;
            }
        }
    } while ((ctx->error_recovery & MENDELEEV_ERROR_RECOVERY_LINK) &&
             rc == -1);

    if (rc > 0 && rc != msg_length) {
        errno = EMBBADDATA;
        return -1;
    }

    return rc;
}

/*
 *  ---------- Request     Indication ----------
 *  | Client | ---------------------->| Server |
 *  ---------- Confirmation  Response ----------
 */

/* Waits a response from a modbus server or a request from a modbus client.
   This function blocks if there is no replies (3 timeouts).

   The function shall return the number of received characters and the received
   message in an array of uint8_t if successful. Otherwise it shall return -1
   and errno is set to one of the values defined below:
   - ECONNRESET
   - EMBBADDATA
   - EMBUNKEXC
   - ETIMEDOUT
   - read() or recv() error codes
*/

int _receive_msg(mendeleev_t *ctx, uint8_t *msg)
{
    int rc;
    fd_set rset;
    struct timeval tv;
    struct timeval *p_tv;
    uint16_t length_to_read;
    int msg_length = 0;
    uint16_t datalen;

    if (ctx->debug) {
        printf("Waiting for a confirmation...\n");
    }

    /* Add a file descriptor to the set */
    FD_ZERO(&rset);
    FD_SET(ctx->s, &rset);

    length_to_read = MENDELEEV_DATA_OFFSET;

    tv.tv_sec = ctx->response_timeout.tv_sec;
    tv.tv_usec = ctx->response_timeout.tv_usec;
    p_tv = &tv;

    while (length_to_read != 0) {
        rc = ctx->backend->select(ctx, &rset, p_tv, length_to_read);
        if (rc == -1) {
            _error_print(ctx, "select");
            if (ctx->error_recovery & MENDELEEV_ERROR_RECOVERY_LINK) {
                int saved_errno = errno;

                if (errno == ETIMEDOUT) {
                    _sleep_response_timeout(ctx);
                    mendeleev_flush(ctx);
                } else if (errno == EBADF) {
                    mendeleev_close(ctx);
                    mendeleev_connect(ctx);
                }
                errno = saved_errno;
            }
            return -1;
        }

        rc = ctx->backend->recv(ctx, msg + msg_length, length_to_read);
        if (rc == 0) {
            errno = ECONNRESET;
            rc = -1;
        }

        if (rc == -1) {
            _error_print(ctx, "read");
            if ((ctx->error_recovery & MENDELEEV_ERROR_RECOVERY_LINK) &&
                (errno == ECONNRESET || errno == ECONNREFUSED ||
                 errno == EBADF)) {
                int saved_errno = errno;
                mendeleev_close(ctx);
                mendeleev_connect(ctx);
                /* Could be removed by previous calls */
                errno = saved_errno;
            }
            return -1;
        }

        /* Display the hex code of each character received */
        if (ctx->debug) {
            int i;
            for (i=0; i < rc; i++)
                printf("<%.2X>", msg[msg_length + i]);
        }

        /* Sums bytes received */
        msg_length += rc;
        /* Computes remaining bytes */
        length_to_read -= rc;

        if (length_to_read == 0) {
            datalen = ((msg[MENDELEEV_DATALEN_OFFSET] << 8) | msg[MENDELEEV_DATALEN_OFFSET + 1]);
            if (msg_length < (MENDELEEV_DATA_OFFSET + datalen + MENDELEEV_CHECKSUM_LENGTH)) {
                length_to_read = datalen + MENDELEEV_CHECKSUM_LENGTH;
            }
        }

        if (length_to_read > 0 &&
            (ctx->byte_timeout.tv_sec > 0 || ctx->byte_timeout.tv_usec > 0)) {
            /* If there is no character in the buffer, the allowed timeout
               interval between two consecutive bytes is defined by
               byte_timeout */
            tv.tv_sec = ctx->byte_timeout.tv_sec;
            tv.tv_usec = ctx->byte_timeout.tv_usec;
            p_tv = &tv;
        }
        /* else timeout isn't set again, the full response must be read before
           expiration of response timeout (for CONFIRMATION only) */
    }

    if (ctx->debug)
        printf("\n");

    return ctx->backend->check_integrity(ctx, msg, msg_length);
}

/* Receive the request from a modbus master */
int mendeleev_receive(mendeleev_t *ctx, uint8_t *req)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    return ctx->backend->receive(ctx, req);
}

/* Receives the confirmation.

   The function shall store the read response in rsp and return the number of
   values (bits or words). Otherwise, its shall return -1 and errno is set.

   The function doesn't check the confirmation is the expected response to the
   initial request.
*/
int mendeleev_receive_confirmation(mendeleev_t *ctx, uint8_t *rsp)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    return _receive_msg(ctx, rsp);
}

static int check_confirmation(mendeleev_t *ctx, uint8_t *req,
                              uint8_t *rsp, int rsp_length)
{
    int rc;
    int rsp_length_computed;
    const int function = rsp[MENDELEEV_CMD_OFFSET];

    if (ctx->backend->pre_check_confirmation) {
        /* check if destination of request is source of response */
        rc = ctx->backend->pre_check_confirmation(ctx, req, rsp, rsp_length);
        if (rc == -1) {
            if (ctx->error_recovery & MENDELEEV_ERROR_RECOVERY_PROTOCOL) {
                _sleep_response_timeout(ctx);
                mendeleev_flush(ctx);
            }
            return -1;
        }
    }

    rsp_length_computed = compute_response_length_from_request(ctx, req);

    /* If the command is one's complement */
    if (function >= 0x80) {
        if ((rsp_length == (MENDELEEV_DATA_OFFSET + MENDELEEV_CHECKSUM_LENGTH)) && (req[MENDELEEV_CMD_OFFSET] == (uint8_t)(~function))) {
            /* Valid exception code received */
            errno = MENDELEEV_ENOBASE + MENDELEEV_EXCEPTION_NEGATIVE_ACKNOWLEDGE;
            _error_print(ctx, NULL);
            return -1;
        } else {
            errno = EMBBADEXC;
            _error_print(ctx, NULL);
            return -1;
        }
    }

    /* Check length */
    if ((rsp_length == rsp_length_computed ||
         rsp_length_computed == MSG_LENGTH_UNDEFINED) &&
        function < 0x80) {

        const uint16_t rsp_sequence_number = *((uint16_t *)(rsp + MENDELEEV_SEQNR_OFFSET));
        const uint16_t req_sequence_number = *((uint16_t *)(req + MENDELEEV_SEQNR_OFFSET));

        /* Check function code */
        if (function != req[MENDELEEV_CMD_OFFSET]) {
            if (ctx->debug) {
                fprintf(stderr,
                        "Received function not corresponding to the request (0x%X != 0x%X)\n",
                        function, req[MENDELEEV_CMD_OFFSET]);
            }
            if (ctx->error_recovery & MENDELEEV_ERROR_RECOVERY_PROTOCOL) {
                _sleep_response_timeout(ctx);
                mendeleev_flush(ctx);
            }
            errno = EMBBADDATA;
            return -1;
        }

        if (req_sequence_number != rsp_sequence_number) {
            if (ctx->debug) {
                fprintf(stderr,
                        "Received sequence number not corresponding to the request (0x%X != 0x%X)\n",
                        rsp_sequence_number, req_sequence_number);
            }
            if (ctx->error_recovery & MENDELEEV_ERROR_RECOVERY_PROTOCOL) {
                _sleep_response_timeout(ctx);
                mendeleev_flush(ctx);
            }
            errno = EMBBADDATA;
            return -1;
        }

        /* Check the response according to the request */
        switch (function) {
        case MENDELEEV_CMD_SET_COLOR:
        case MENDELEEV_CMD_SET_MODE:
        case MENDELEEV_CMD_OTA:
        case MENDELEEV_CMD_GET_VERSION:
        case MENDELEEV_CMD_SET_OUTPUT:
        default:
            rc = 1;
        }
    } else {
        if (ctx->debug) {
            fprintf(stderr,
                    "Message length not corresponding to the computed length (%d != %d)\n",
                    rsp_length, rsp_length_computed);
        }
        if (ctx->error_recovery & MENDELEEV_ERROR_RECOVERY_PROTOCOL) {
            _sleep_response_timeout(ctx);
            mendeleev_flush(ctx);
        }
        errno = EMBBADDATA;
        rc = -1;
    }

    return rc;
}


int mendeleev_send_command(mendeleev_t *ctx, uint8_t command, uint8_t *data, uint16_t data_length, uint8_t *rsp_buf, uint16_t *rsp_length)
{
    int rc;
    int req_length;
    uint8_t req[MAX_MESSAGE_LENGTH];

    if (data_length > (MAX_MESSAGE_LENGTH - MENDELEEV_MSG_OVERHEAD)) {
        errno = EINVAL;
        return -1;
    }

    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    req_length = ctx->backend->build_request_basis(ctx, command, req);
    req[req_length++] = data_length >> 8;
    req[req_length++] = data_length & 0x00FF;
    memcpy(req + req_length, data, data_length);
    req_length += data_length;

    rc = send_msg(ctx, req, req_length);
    if (rc > 0) {
        uint8_t rsp[MAX_MESSAGE_LENGTH];

        rc = _receive_msg(ctx, rsp);
        if (rc == -1)
            return -1;

        rc = check_confirmation(ctx, req, rsp, rc);
        if (rc == -1)
            return -1;

        uint16_t datalen = ((rsp[MENDELEEV_DATALEN_OFFSET] << 8) | rsp[MENDELEEV_DATALEN_OFFSET + 1]);
        if (datalen > 0 && rsp != NULL) {
            memcpy(rsp_buf, rsp + MENDELEEV_DATA_OFFSET, datalen);
        }
        if (rsp_length != NULL) {
            *rsp_length = datalen;
        }
    }

    return rc;
}


void _init_common(mendeleev_t *ctx)
{
    /* Slave and socket are initialized to -1 */
    ctx->slave = -1;
    ctx->s = -1;

    ctx->debug = FALSE;
    ctx->error_recovery = MENDELEEV_ERROR_RECOVERY_NONE;

    ctx->response_timeout.tv_sec = 0;
    ctx->response_timeout.tv_usec = _RESPONSE_TIMEOUT;

    ctx->byte_timeout.tv_sec = 0;
    ctx->byte_timeout.tv_usec = _BYTE_TIMEOUT;
}

/* Define the slave number */
int mendeleev_set_slave(mendeleev_t *ctx, int slave)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    return ctx->backend->set_slave(ctx, slave);
}

int mendeleev_get_slave(mendeleev_t *ctx)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    return ctx->slave;
}

int mendeleev_set_error_recovery(mendeleev_t *ctx,
                              error_recovery_mode error_recovery)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    /* The type of error_recovery_mode is unsigned enum */
    ctx->error_recovery = (uint8_t) error_recovery;
    return 0;
}

int mendeleev_set_socket(mendeleev_t *ctx, int s)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    ctx->s = s;
    return 0;
}

int mendeleev_get_socket(mendeleev_t *ctx)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    return ctx->s;
}

/* Get the timeout interval used to wait for a response */
int mendeleev_get_response_timeout(mendeleev_t *ctx, uint32_t *to_sec, uint32_t *to_usec)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    *to_sec = ctx->response_timeout.tv_sec;
    *to_usec = ctx->response_timeout.tv_usec;
    return 0;
}

int mendeleev_set_response_timeout(mendeleev_t *ctx, uint32_t to_sec, uint32_t to_usec)
{
    if (ctx == NULL ||
        (to_sec == 0 && to_usec == 0) || to_usec > 999999) {
        errno = EINVAL;
        return -1;
    }

    ctx->response_timeout.tv_sec = to_sec;
    ctx->response_timeout.tv_usec = to_usec;
    return 0;
}

/* Get the timeout interval between two consecutive bytes of a message */
int mendeleev_get_byte_timeout(mendeleev_t *ctx, uint32_t *to_sec, uint32_t *to_usec)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    *to_sec = ctx->byte_timeout.tv_sec;
    *to_usec = ctx->byte_timeout.tv_usec;
    return 0;
}

int mendeleev_set_byte_timeout(mendeleev_t *ctx, uint32_t to_sec, uint32_t to_usec)
{
    /* Byte timeout can be disabled when both values are zero */
    if (ctx == NULL || to_usec > 999999) {
        errno = EINVAL;
        return -1;
    }

    ctx->byte_timeout.tv_sec = to_sec;
    ctx->byte_timeout.tv_usec = to_usec;
    return 0;
}

int mendeleev_connect(mendeleev_t *ctx)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    return ctx->backend->connect(ctx);
}

void mendeleev_close(mendeleev_t *ctx)
{
    if (ctx == NULL)
        return;

    ctx->backend->close(ctx);
}

void mendeleev_free(mendeleev_t *ctx)
{
    if (ctx == NULL)
        return;

    ctx->backend->free(ctx);
}

int mendeleev_set_debug(mendeleev_t *ctx, int flag)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    ctx->debug = flag;
    return 0;
}

#ifndef HAVE_STRLCPY
/*
 * Function strlcpy was originally developed by
 * Todd C. Miller <Todd.Miller@courtesan.com> to simplify writing secure code.
 * See ftp://ftp.openbsd.org/pub/OpenBSD/src/lib/libc/string/strlcpy.3
 * for more information.
 *
 * Thank you Ulrich Drepper... not!
 *
 * Copy src to string dest of size dest_size.  At most dest_size-1 characters
 * will be copied.  Always NUL terminates (unless dest_size == 0).  Returns
 * strlen(src); if retval >= dest_size, truncation occurred.
 */
size_t strlcpy(char *dest, const char *src, size_t dest_size)
{
    register char *d = dest;
    register const char *s = src;
    register size_t n = dest_size;

    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0) {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    /* Not enough room in dest, add NUL and traverse rest of src */
    if (n == 0) {
        if (dest_size != 0)
            *d = '\0'; /* NUL-terminate dest */
        while (*s++)
            ;
    }

    return (s - src - 1); /* count does not include NUL */
}
#endif
