# Adding a new region to Arduino LMIC
This variant of the Arduino LMIC code supports adding additional regions beyond the eu868 and us915 bands supoprted by the original IBM LMIC 1.6 code.

This document sketches how to add a new region.

## Planning

### Determine the region/region category
Compare the target region (in the LoRaWAN regional specification) to the EU868 and US915 regions. There are three possibilities.
1. the region is like the EU region. There are a limited number of channels (up to 8), and only a small number of channels are used for OTAA join operations. The response masks refer to individual channels, and the JOIN-response can send frequencies of specific channels to be added.

2. The region is like the US region. There are many channels (the US has 64) with fixed frequences, and the channel masks refer to subsets of the fixed channels.

3. The region is not really like either the EU or  US. At the moment, it seems that CN470-510MHz (section 2.6 of LoRaWAN Regional Parameters spec V1.0.2rB) falls into this category.

Bandplans in categories (1) and (2) are easily supported. Bandplans in category (3) are not supoprted by the current code.

### Check whether the region is already listed in `lmic_config_preconditions.h`
Check `src/lmic/lmic_config_preconditions.h` and scan the `LMIC_REGION_...` definitions. The numeric values are assigned based on the subchapter in section 2 of the LoRaWAN 1.0.2 Regional Parmaters document. If your symbol is already there, then the first part of adaptation has already been done. There will already be a corresponding `CFG_...` symbol. But if your region isn't supported, you'll need to add it here.  

- `LMIC_REGION_myregion` must be a distinct integer, and must be less than 32 (so as to fit into a bitmask)

## Make the appropriate changes in `lmic_config_preconditions.h`

- `LMIC_REGION_SUPPORTED` is a bit mask of all regions supported by the code. Your new region must appear in this list.
- `CFG_LMIC_REGION_MASK` is a bit mask that, when expanded, returns a bitmask for each defined `CFG_...` variable. You must add your `CFG_myregion` symbol to this list.
- `CFG_region` evaluates to the `LMIC_REGION_...` value for the selected region (as long as only one region is selected). The header files check for this, so you don't have to.
- `CFG_LMIC_EU_like_MASK` is a bitmask of regions that are EU-like, and `CFG_LMIC_US_like_MASK` is a bitmask of regions that are US-like. Add your region to the appropriate one of these two variables.

## Document your region in `config.h`
You'll see where the regions are listed. Add yours.

## Document your region in `README.md`
You'll see where the regions are listed. Add yours.

## Add the definitions for your region in `lorabase.h`
- If your region is EU like, copy the EU block. Document any duty-cycle limitations.
- if your region is US like, copy the US block. 
- As appropriate, copy `lorabase_eu868.h` or `lorabase_us915.h` to make your own `lorabase_myregion.h`.  Fill in the symbols.

At time of writing, you need to duplicate some code to copy some settings from `..._CONFIG_SYMBOL` to the corresponding `CONFIG_SYMBOL`; and you need to put some region-specific knowledge into the `lorabase.h` header file. The long-term direction is to put all the regional knowledge into the region-specific header, and then the central code will just copy. The architectural impulse is that we'll want to be able to reuse the regional header files in other contexts. On the other hand, because it's error prone, we don't want to `#include` files that aren't being used; otherwise you could accidentally use EU parameters in US code, etc.

- Now's a good time to test-compile and clean out errors introduced. You'll still have problems compiling, but they should look like this:
    ```
    lmic.c:29: In file included from
 
    lmic_bandplan.h: 52:3: error: #error "maxFrameLen() not defined by bandplan"
       # error "maxFrameLen() not defined by bandplan"
 
    lmic_bandplan.h: 56:3: error: #error "pow2dBm() not defined by bandplan"
       # error "pow2dBm() not defined by bandplan"
    ```

## Edit `lmic_bandplan.h`
The next step is to add the region-specific interfaces for your region.

Do this by editing `lmic_bandplan.h` and adding the appropriate call to a (new) region-specific file `lmic_bandplan_myregion.h`, where "myregion" is the abbreviation for your region.

Then, if your region is eu868-like, copy `lmic_bandplan_eu868.h` to create your new region-specific header file; otherwise copy `lmic_bandplan_us915.h`.

## Create `lmic_myregion.c`
Once again, you will start by copying either `lmic_eu868.c` or `lmic_us915.c` to create your new file. Then touch it up as necessary.

## General Discussion
- You'll find it easier to do the test compiles using the example scripts in this directory, rather than trying to get all the Catena framework going too. On the other hand, working with the Catena framework will expose more problems.

## Addding the region to the Arduino_LoRaWAN library

In `Arduino_LoRaWAN_ttn.h`:
- Add a new class with name `Arduino_LoRaWAN_ttn_myregion`, copied either from the `Arduino_LoRaWAN_ttn_eu868` class or the `Arduino_LoRaWAN_ttn_us915` class.
- Extend the list of `#if defined(CFG_eu868)` etc to define `Arduino_LoRaWAN_REGION_TAG` to the suffix of your new class if `CFG_myregion` is defined.

Then copy either `ttn_eu868_netbegin.cpp`/`ttn_eu868_netjoin.cpp` or `ttn_us915_netbegin.cpp`/`ttn_us915_netjoin.cpp` to make your own file(s) for the key functions.