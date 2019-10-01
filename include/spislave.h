/*

//////////////////////// ESP32-Paxcounter \\\\\\\\\\\\\\\\\\\\\\\\\\

Copyright  2018 Christian Ambach <christian.ambach@deutschebahn.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

NOTICE:
Parts of the source files in this repository are made available under different
licenses. Refer to LICENSE.txt file in repository for more details.

*/
#ifndef _SPISLAVE_H
#define _SPISLAVE_H

#include "globals.h"

esp_err_t spi_init();

void spi_enqueuedata(MessageBuffer_t *message);
void spi_queuereset();

#endif // _SPISLAVE_H
