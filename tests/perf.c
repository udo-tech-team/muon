/*
 * encapsulate.c - test encapsulate and decapsulate
 *
 * Copyright (C) 2014 - 2016, Xiaoxiao <i@pxx.io>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "../src/compress.h"
#include "../src/encapsulate.h"


extern void rc4(void *stream, size_t len, const void *key);


const int count = 50000;


int main()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand((int)((tv.tv_sec * 1000 + tv.tv_usec / 1000) % RAND_MAX));

    if (compress_init() != 0)
    {
        return -1;
    }

    const int mtu = 1452;


    printf("rc4, %d packets\n", count);
    printf(" size | time(ms)\n"
           "------+---------\n");

    uint8_t buf[1500];
    // random key generated by python
    //   ''.join([r'\x%02x' % char for char in open('/dev/urandom', 'rb').read(16)])
    uint8_t key[] = "\xf5\x95\x3b\xf4\xa7\x45\x3a\xa5\x5f\x73\xc3\xc9\x0d\xcd\x6c\x39";
    for (int len = 0; len <= mtu; len += 363)
    {
        for (int j = 0; j < len; j++)
        {
            buf[j] = (uint8_t)(rand() & 0xffU);
        }
        gettimeofday(&tv, NULL);
        int64_t start = tv.tv_sec * 1000 + tv.tv_usec / 1000;

        for (int k = 0; k < count; k++)
        {
            rc4(buf, len, key);
        }

        gettimeofday(&tv, NULL);
        int64_t end = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        printf("%5d |%6d\n", len, (int)(end - start));
    }
    printf("\n");

    printf("encapsulate/decapsulate, %d packets\n", count);
    printf("      |     time(ms)\n"
           " size | random | ascending\n"
           "------+--------+----------\n");

    pbuf_t pbuf;
    for (int len = 0; len <= mtu; len += 363)
    {
        for (int random = 1; random >= 0; random--)
        {
            for (int j = 0; j < len; j++)
            {
                if (random == 0)
                {
                    pbuf.payload[j] = (uint8_t)(rand() & 0xffU);
                }
                else if (random == 1)
                {
                    pbuf.payload[j] = (uint8_t)(j & 0xffU);
                }
            }
            pbuf.len = len;
            pbuf.flag = 0x0000;
            pbuf.ack = 0;

            pbuf_t copy;
            copy.ack = pbuf.ack;
            copy.flag = pbuf.flag;
            copy.len = pbuf.len;
            memcpy(copy.payload, pbuf.payload, pbuf.len);

            gettimeofday(&tv, NULL);
            int64_t start = tv.tv_sec * 1000 + tv.tv_usec / 1000;

            for (int k = 0; k < count; k++)
            {
                int n = encapsulate(&copy, mtu);
                decapsulate(&copy, n);
            }

            gettimeofday(&tv, NULL);
            int64_t end = tv.tv_sec * 1000 + tv.tv_usec / 1000;
            if (random)
            {
                printf("%5d |%6d", len, (int)(end - start));
            }
            else
            {
                printf("  |%7d\n", (int)(end - start));
            }
            if ((len == mtu) && (random == 0))
            {
                double speed = (double)len * count * 1000 / (double)(end - start) / 1024.0 / 1024.0;
                printf("Speed: %.3lfMB/s\n", speed);
            }
        }
    }
    return 0;
}
