#ifndef LORAWAN_H
#define LORAWAN_H

void onEvent(ev_t ev);
void do_send(osjob_t *j);
void gen_lora_deveui(uint8_t *pdeveui);
void RevBytes(unsigned char *b, size_t c);
void get_hard_deveui(uint8_t *pdeveui);

#endif