/*
 * RokkitHash - Arduino port for Paul Hsieh's "SuperFastHash"
 *
 * A very quick hash function, (c) Paul Hsieh
 *
 * See http://www.azillionmonkeys.com/qed/hash.html for more information
 * about its inner workings
 *
 * - Initial Arduino version: 2014 Alex K
 * - 8-bit improvements: robtillaart
 * - Current maintainer: SukkoPera <software@sukkology.net>
 *
 * See http://forum.arduino.cc/index.php?topic=226686.0 for some talk
 * about the various improvements.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <inttypes.h>

uint32_t rokkit(const char * data, int len) {
  uint32_t hash, tmp;
  int rem;

    if (len <= 0 || data == 0) return 0;
    hash = len;
    rem = len & 3;
    len >>= 2;

    /* Main loop */
    while (len > 0) {
        hash  += *((uint16_t*)data);
        tmp    = (*((uint16_t*)(data+2)) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*2;
        hash  += hash >> 11;
		len--;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += *((uint16_t*)data);
                hash ^= hash << 16;
                hash ^= ((signed char)data[2]) << 18;
                hash += hash >> 11;
                break;
        case 2: hash += *((uint16_t*)data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += (signed char)*data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}
