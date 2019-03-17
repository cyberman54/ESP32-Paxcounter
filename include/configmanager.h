#ifndef _CONFIGMANAGER_H
#define _CONFIGMANAGER_H

#include <nvs.h>
#include <nvs_flash.h>
#include <bitset>

void eraseConfig(void);
void saveConfig(void);
void loadConfig(void);

#endif