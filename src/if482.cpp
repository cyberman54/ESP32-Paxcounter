#ifdef HAS_IF482

/*
IF482 Generator to control clocks with IF482 timeframe input (e.g. BÜRK BU190)
   
Example IF482 timeframe: "OAL160806F170400"

IF482 Specification:
https://www.buerk-mobatime.de/cfc/shop.cfc?method=deliverShopDoc&id=509

Standard Zeittelegramm (IF 482) mit lokaler Zeit

Definition: Zeittelegramm (ASCII), endend auf die im Telegramm bezeichnete
Sekunde: 9600 Bit/s, 7 Datenbits, gerade Parität, 1 Stoppbit. Jitter < 50ms.

Format:
Byte    Bedeutung                              Zeichen     HEX Code
1       Startzeichen                            O           4F
2       Überwachung*                            A           41
3       Lokale Zeit** (durch die Uhr angezeigt) L           4C
4       Jahr Zehner                             0 .. 9      30 .. 39
5       Jahr Einer                              0 .. 9      30 .. 39
6       Monat Zehner                            0 oder 1    30 oder 31
7       Monat Einer                             0 .. 9      30 .. 39
8       Tag Zehner                              0 .. 3      30 .. 33
9       Tag Einer                               0 .. 9      30 .. 39
10      nicht verwendet                         F           46
11      Stunden Zehner                          0 .. 2      30 .. 32
12      Stunden Einer                           0 .. 9      30 .. 39
13      Minuten Zehner                          0 .. 5      30 .. 35
14      Minuten Einer                           0 .. 9      30 .. 39
15      Sekunden Zehner                         0 .. 5      30 .. 35
16      Sekunden Einer                          0 .. 9      30 .. 39
17      Ende des Telegramms                     CR          0D

*) Überwachung:
'A': Korrekter Zeitempfang des Sendegerätes
'M': Sendegerät hat mehr als 12 Stunden kein Zeitsignal empfangen 
Zeit wird bei 'A' und 'M' vom Uhrwerk übernommen.

**) Lokale Zeit:
'W': Winterzeit
'S': Sommerzeit
'L': Lokalzeit
Wird überprüft, jedoch nicht vom Uhrwerk ausgewertet.
   
*/

#include "if482.h"

// Local logging tag
static const char TAG[] = "main";


#endif // HAS_IF482