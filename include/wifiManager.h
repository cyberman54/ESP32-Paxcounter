#ifndef _WIFIMANAGER_H
#define _WIFIMANAGER_H

#include <rcommand.h> // must be included here so that Arduino library object file references work
#include "globals.h"
#include "timekeeper.h"

bool connectWifi();
void startWifiScan();
void stopWifiScan();

#endif // _WIFIMANAGER_H