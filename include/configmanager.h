#ifndef _CONFIGMANAGER_H
#define _CONFIGMANAGER_H

#include <Preferences.h>
#include "globals.h"

void saveConfig(bool erase = false);
void loadConfig(void);
void eraseConfig(void);

#endif