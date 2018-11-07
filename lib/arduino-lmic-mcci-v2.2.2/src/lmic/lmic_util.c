/*

Module:  lmic_util.c

Function:
        Encoding and decoding utilities for LMIC clients.

Copyright & License:
        See accompanying LICENSE file.

Author:
        Terry Moore, MCCI       September 2019

*/

#include "lmic_util.h"

#include <math.h>

/*

Name:   LMIC_f2sflt16()

Function:
        Encode a floating point number into a uint16_t.

Definition:
        uint16_t LMIC_f2sflt16(
                float f
                );

Description:
        The float to be transmitted must be a number in the range (-1.0, 1.0).
        It is converted to 16-bit integer formatted as follows:

                bits 15: sign
                bits 14..11: biased exponent
                bits 10..0: mantissa

        The float is properly rounded, and saturates.

        Note that the encoded value is sign/magnitude format, rather than
        two's complement for negative values.

Returns:
        0xFFFF for negative values <= 1.0;
        0x7FFF for positive values >= 1.0;
        Otherwise an appropriate float.

*/

uint16_t
LMIC_f2sflt16(
        float f
        )
        {
        if (f <= -1.0)
                return 0xFFFF;
        else if (f >= 1.0)
                return 0x7FFF;
        else
                {
                int iExp;
                float normalValue;
                uint16_t sign;

                normalValue = frexpf(f, &iExp);

                sign = 0;
                if (normalValue < 0)
                        {
                        // set the "sign bit" of the result
                        // and work with the absolute value of normalValue.
                        sign = 0x8000;
                        normalValue = -normalValue;
                        }

                // abs(f) is supposed to be in [0..1), so useful exp
                // is [0..-15]
                iExp += 15;
                if (iExp < 0)
                        iExp = 0;

                // bit 15 is the sign
                // bits 14..11 are the exponent
                // bits 10..0 are the fraction
                // we conmpute the fraction and then decide if we need to round.
                uint16_t outputFraction = ldexpf(normalValue, 11) + 0.5;
                if (outputFraction >= (1 << 11u))
                        {
                        // reduce output fraction
                        outputFraction = 1 << 10;
                        // increase exponent
                        ++iExp;
                        }

                // check for overflow and return max instead.
                if (iExp > 15)
                        return 0x7FFF | sign;

                return (uint16_t)(sign | (iExp << 11u) | outputFraction);
                }
        }

/*

Name:   LMIC_f2sflt12()

Function:
        Encode a floating point number into a uint16_t using only 12 bits.

Definition:
        uint16_t LMIC_f2sflt16(
                float f
                );

Description:
        The float to be transmitted must be a number in the range (-1.0, 1.0).
        It is converted to 16-bit integer formatted as follows:

                bits 15-12: zero
                bit 11: sign
                bits 10..7: biased exponent
                bits 6..0: mantissa

        The float is properly rounded, and saturates.

        Note that the encoded value is sign/magnitude format, rather than
        two's complement for negative values.

Returns:
        0xFFF for negative values <= 1.0;
        0x7FF for positive values >= 1.0;
        Otherwise an appropriate float.

*/

uint16_t
LMIC_f2sflt12(
        float f
        )
        {
        if (f <= -1.0)
                return 0xFFF;
        else if (f >= 1.0)
                return 0x7FF;
        else
                {
                int iExp;
                float normalValue;
                uint16_t sign;

                normalValue = frexpf(f, &iExp);

                sign = 0;
                if (normalValue < 0)
                        {
                        // set the "sign bit" of the result
                        // and work with the absolute value of normalValue.
                        sign = 0x800;
                        normalValue = -normalValue;
                        }

                // abs(f) is supposed to be in [0..1), so useful exp
                // is [0..-15]
                iExp += 15;
                if (iExp < 0)
                        iExp = 0;

                // bit 15 is the sign
                // bits 14..11 are the exponent
                // bits 10..0 are the fraction
                // we conmpute the fraction and then decide if we need to round.
                uint16_t outputFraction = ldexpf(normalValue, 7) + 0.5;
                if (outputFraction >= (1 << 7u))
                        {
                        // reduce output fraction
                        outputFraction = 1 << 6;
                        // increase exponent
                        ++iExp;
                        }

                // check for overflow and return max instead.
                if (iExp > 15)
                        return 0x7FF | sign;

                return (uint16_t)(sign | (iExp << 7u) | outputFraction);
                }
        }

/*

Name:   LMIC_f2uflt16()

Function:
        Encode a floating point number into a uint16_t.

Definition:
        uint16_t LMIC_f2uflt16(
                float f
                );

Description:
        The float to be transmitted must be a number in the range [0, 1.0).
        It is converted to 16-bit integer formatted as follows:

                bits 15..12: biased exponent
                bits 11..0: mantissa

        The float is properly rounded, and saturates.

        Note that the encoded value is sign/magnitude format, rather than
        two's complement for negative values.

Returns:
        0x0000 for values < 0.0;
        0xFFFF for positive values >= 1.0;
        Otherwise an appropriate encoding of the input float.

*/

uint16_t
LMIC_f2uflt16(
        float f
        )
        {
        if (f < 0.0)
                return 0;
        else if (f >= 1.0)
                return 0xFFFF;
        else
                {
                int iExp;
                float normalValue;

                normalValue = frexpf(f, &iExp);

                // f is supposed to be in [0..1), so useful exp
                // is [0..-15]
                iExp += 15;
                if (iExp < 0)
                        // underflow.
                        iExp = 0;

                // bits 15..12 are the exponent
                // bits 11..0 are the fraction
                // we conmpute the fraction and then decide if we need to round.
                uint16_t outputFraction = ldexpf(normalValue, 12) + 0.5;
                if (outputFraction >= (1 << 12u))
                        {
                        // reduce output fraction
                        outputFraction = 1 << 11;
                        // increase exponent
                        ++iExp;
                        }

                // check for overflow and return max instead.
                if (iExp > 15)
                        return 0xFFFF;

                return (uint16_t)((iExp << 12u) | outputFraction);
                }
        }

/*

Name:   LMIC_f2uflt12()

Function:
        Encode positive floating point number into a uint16_t using only 12 bits.

Definition:
        uint16_t LMIC_f2sflt16(
                float f
                );

Description:
        The float to be transmitted must be a number in the range [0, 1.0).
        It is converted to 16-bit integer formatted as follows:

                bits 15-12: zero
                bits 11..8: biased exponent
                bits 7..0: mantissa

        The float is properly rounded, and saturates.

Returns:
        0x000 for negative values < 0.0;
        0xFFF for positive values >= 1.0;
        Otherwise an appropriate float.

*/

uint16_t
LMIC_f2uflt12(
        float f
        )
        {
        if (f < 0.0)
                return 0x000;
        else if (f >= 1.0)
                return 0xFFF;
        else
                {
                int iExp;
                float normalValue;

                normalValue = frexpf(f, &iExp);

                // f is supposed to be in [0..1), so useful exp
                // is [0..-15]
                iExp += 15;
                if (iExp < 0)
                        // graceful underflow
                        iExp = 0;

                // bits 11..8 are the exponent
                // bits 7..0 are the fraction
                // we conmpute the fraction and then decide if we need to round.
                uint16_t outputFraction = ldexpf(normalValue, 8) + 0.5;
                if (outputFraction >= (1 << 8u))
                        {
                        // reduce output fraction
                        outputFraction = 1 << 7;
                        // increase exponent
                        ++iExp;
                        }

                // check for overflow and return max instead.
                if (iExp > 15)
                        return 0xFFF;

                return (uint16_t)((iExp << 8u) | outputFraction);
                }
        }
