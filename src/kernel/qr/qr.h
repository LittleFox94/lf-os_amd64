#ifndef _QR_H_INCLUDED
#define _QR_H_INCLUDED

#include <stdint.h>

typedef uint8_t qr_data[177*177];

uint8_t qr_log(qr_data out);

#endif
