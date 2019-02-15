/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef MENDELEEV_RTU_H
#define MENDELEEV_RTU_H

#include "mendeleev.h"

MENDELEEV_BEGIN_DECLS

#define PREAMBLE (0xA5)

MENDELEEV_API mendeleev_t* mendeleev_new_rtu(const char *device, int baud, char parity, int data_bit, int stop_bit);

#define MENDELEEV_RTU_RS232 0
#define MENDELEEV_RTU_RS485 1

MENDELEEV_API int set_serial_mode(mendeleev_t *ctx, int mode);
MENDELEEV_API int get_serial_mode(mendeleev_t *ctx);

#define MENDELEEV_RTU_RTS_NONE   0
#define MENDELEEV_RTU_RTS_UP     1
#define MENDELEEV_RTU_RTS_DOWN   2

MENDELEEV_API int set_rts(mendeleev_t *ctx, int mode);
MENDELEEV_API int get_rts(mendeleev_t *ctx);

MENDELEEV_API int set_custom_rts(mendeleev_t *ctx, void (*custom_set_rts) (mendeleev_t *ctx, int on));

MENDELEEV_API int set_rts_delay(mendeleev_t *ctx, int us);
MENDELEEV_API int get_rts_delay(mendeleev_t *ctx);

MENDELEEV_END_DECLS

#endif /* MENDELEEV_RTU_H */
