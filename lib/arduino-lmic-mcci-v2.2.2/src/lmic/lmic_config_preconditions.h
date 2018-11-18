/* lmic_config_preconditions.h	Fri May 19 2017 23:58:34 tmm */

/*

Module:  lmic_config_preconditions.h

Function:
	Preconditions for LMIC configuration.

Version:
	V2.0.0	Sun Aug 06 2017 17:40:44 tmm	Edit level 1

Copyright notice:
	This file copyright (C) 2017 by

		MCCI Corporation
		3520 Krums Corners Road
		Ithaca, NY  14850

        MIT License

        Permission is hereby granted, free of charge, to any person obtaining a copy
        of this software and associated documentation files (the "Software"), to deal
        in the Software without restriction, including without limitation the rights
        to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
        copies of the Software, and to permit persons to whom the Software is
        furnished to do so, subject to the following conditions:

        The above copyright notice and this permission notice shall be included in all
        copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
        AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
        LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
        OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
        SOFTWARE.

Author:
	Terry Moore, MCCI Corporation	July 2017

Revision history:
   2.0.0  Sun Aug 06 2017 17:40:44  tmm
	Module created.

*/

#ifndef _LMIC_CONFIG_PRECONDITIONS_H_
# define _LMIC_CONFIG_PRECONDITIONS_H_

// We need to be able to compile with different options without editing source.
// When building with a more advanced environment, set the following variable:
// ARDUINO_LMIC_PROJECT_CONFIG_H=my_project_config.h
//
// otherwise the lmic_project_config.h from the ../../project_config directory will be used.
#ifndef ARDUINO_LMIC_PROJECT_CONFIG_H
# define ARDUINO_LMIC_PROJECT_CONFIG_H ../../project_config/lmic_project_config.h
#endif

#define CFG_TEXT_1(x)	CFG_TEXT_2(x)
#define CFG_TEXT_2(x)	#x

// constants for comparison
#define LMIC_REGION_eu868    1
#define LMIC_REGION_us915    2
#define LMIC_REGION_cn783    3
#define LMIC_REGION_eu433    4
#define LMIC_REGION_au921    5
#define LMIC_REGION_cn490    6
#define LMIC_REGION_as923    7
#define LMIC_REGION_kr921    8
#define LMIC_REGION_in866    9

// Some regions have country-specific overrides. For generality, we specify
// country codes using the LMIC_COUNTY_CODE_C() macro These values are chosen
// from the 2-letter domain suffixes standardized by ISO-3166-1 alpha2 (see
// https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2). They are therefore
// 16-bit constants. By convention, we use UPPER-CASE letters, thus
// LMIC_COUNTRY_CODE('J', 'P'), not ('j', 'p').
#define LMIC_COUNTRY_CODE_C(c1, c2)     ((c1) * 256 + (c2))

// this special code means "no country code defined"
#define LMIC_COUNTRY_CODE_NONE  0

// specific countries. Only the ones that are needed by the code are defined.
#define LMIC_COUNTRY_CODE_JP    LMIC_COUNTRY_CODE_C('J', 'P')

// include the file that the user is really supposed to edit. But for really strange
// ports, this can be suppressed
#ifndef ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS
# include CFG_TEXT_1(ARDUINO_LMIC_PROJECT_CONFIG_H)
#endif /* ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS */

// a mask of the supported regions
// TODO(tmm@mcci.com) consider moving this block to a central file as it's not
// user-editable.
#define LMIC_REGIONS_SUPPORTED  (                               \
                                (1 << LMIC_REGION_eu868) |      \
                                (1 << LMIC_REGION_us915) |      \
                             /* (1 << LMIC_REGION_cn783) | */   \
                             /* (1 << LMIC_REGION_eu433) | */   \
                                (1 << LMIC_REGION_au921) |      \
                             /* (1 << LMIC_REGION_cn490) | */   \
                                (1 << LMIC_REGION_as923) |      \
                             /* (1 << LMIC_REGION_kr921) | */   \
                                (1 << LMIC_REGION_in866) |      \
                                0)

//
// Our input is a -D of one of CFG_eu868, CFG_us915, CFG_as923, CFG_au915, CFG_in866
// More will be added in the the future. So at this point we create CFG_region with
// following values. These are in order of the sections in the manual. Not all of the
// below are supported yet.
//
// CFG_as923jp is treated as a special case of CFG_as923, so it's not included in
// the below.
//
// TODO(tmm@mcci.com) consider moving this block to a central file as it's not
// user-editable.
//
# define CFG_LMIC_REGION_MASK   \
                        ((defined(CFG_eu868) << LMIC_REGION_eu868) | \
                         (defined(CFG_us915) << LMIC_REGION_us915) | \
                         (defined(CFG_cn783) << LMIC_REGION_cn783) | \
                         (defined(CFG_eu433) << LMIC_REGION_eu433) | \
                         (defined(CFG_au921) << LMIC_REGION_au921) | \
                         (defined(CFG_cn490) << LMIC_REGION_cn490) | \
                         (defined(CFG_as923) << LMIC_REGION_as923) | \
                         (defined(CFG_kr921) << LMIC_REGION_kr921) | \
                         (defined(CFG_in866) << LMIC_REGION_in866) | \
                         0)

// the selected region.
// TODO(tmm@mcci.com) consider moving this block to a central file as it's not
// user-editable.
#if defined(CFG_eu868)
# define CFG_region     LMIC_REGION_eu868
#elif defined(CFG_us915)
# define CFG_region     LMIC_REGION_us915
#elif defined(CFG_cn783)
# define CFG_region     LMIC_REGION_cn783
#elif defined(CFG_eu433)
# define CFG_region     LMIC_REGION_eu433
#elif defined(CFG_au921)
# define CFG_region     LMIC_REGION_au921
#elif defined(CFG_cn490)
# define CFG_region     LMIC_REGION_cn490
#elif defined(CFG_as923jp)
# define CFG_as923	1			/* CFG_as923jp implies CFG_as923 */
# define CFG_region     LMIC_REGION_as923
# define LMIC_COUNTRY_CODE  LMIC_COUNTRY_CODE_JP
#elif defined(CFG_as923)
# define CFG_region     LMIC_REGION_as923
#elif defined(CFG_kr921)
# define CFG_region     LMIC_REGION_kr921
#elif defined(CFG_in866)
# define CFG_region     LMIC_REGION_in866
#else
# define CFG_region     0
#endif

// a bitmask of EU-like regions -- these are regions which have up to 16
// channels indidually programmable via downloink.
//
// TODO(tmm@mcci.com) consider moving this block to a central file as it's not
// user-editable.
#define CFG_LMIC_EU_like_MASK   (                               \
                                (1 << LMIC_REGION_eu868) |      \
                             /* (1 << LMIC_REGION_us915) | */   \
                                (1 << LMIC_REGION_cn783) |      \
                                (1 << LMIC_REGION_eu433) |      \
                             /* (1 << LMIC_REGION_au921) | */   \
                             /* (1 << LMIC_REGION_cn490) | */   \
                                (1 << LMIC_REGION_as923) |      \
                                (1 << LMIC_REGION_kr921) |      \
                                (1 << LMIC_REGION_in866) |      \
                                0)

// a bitmask of` US-like regions -- these are regions with 64 fixed 125 kHz channels
// overlaid by 8 500 kHz channels. The channel frequencies can't be changed, but
// subsets of channels can be selected via masks.
//
// TODO(tmm@mcci.com) consider moving this block to a central file as it's not
// user-editable.
#define CFG_LMIC_US_like_MASK   (                               \
                             /* (1 << LMIC_REGION_eu868) | */   \
                                (1 << LMIC_REGION_us915) |      \
                             /* (1 << LMIC_REGION_cn783) | */   \
                             /* (1 << LMIC_REGION_eu433) | */   \
                                (1 << LMIC_REGION_au921) |      \
                             /* (1 << LMIC_REGION_cn490) | */   \
                             /* (1 << LMIC_REGION_as923) | */   \
                             /* (1 << LMIC_REGION_kr921) | */   \
                             /* (1 << LMIC_REGION_in866) | */   \
                                0)

//
// booleans that are true if the configured region is EU-like or US-like.
// TODO(tmm@mcci.com) consider moving this block to a central file as it's not
// user-editable.
//
#define CFG_LMIC_EU_like        (!!(CFG_LMIC_REGION_MASK & CFG_LMIC_EU_like_MASK))
#define CFG_LMIC_US_like        (!!(CFG_LMIC_REGION_MASK & CFG_LMIC_US_like_MASK))

#endif /* _LMIC_CONFIG_PRECONDITIONS_H_ */
