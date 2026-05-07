#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>

// Inicializa o sensor
int ultrasonic_init(void);

// Dispara leitura + retorna distância em cm
float ultrasonic_read_cm(void);

// Retorna último pulso bruto (debug)
uint16_t ultrasonic_get_pulse(void);

#endif
