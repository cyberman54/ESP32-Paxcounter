#ifndef _CONFIGMANAGER_H
#define _CONFIGMANAGER_H

#include "globals.h"
#include "reset.h"
#include <Preferences.h>

extern configData_t cfg;

void saveConfig(bool erase);
void loadConfig(void);
void eraseConfig(void);
int version_compare(const String v1, const String v2);

#endif