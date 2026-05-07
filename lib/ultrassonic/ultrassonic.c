#include "ultrassonic.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include "pwm_z42.h"

#define TRIG_PIN 5
#define ECHO_PIN 12

#define TPM_MODULO 65535
#define TPM_CLOCK 48000000.0f
#define PRESCALER 128.0f

static const struct device *gpioa;

static volatile uint16_t rise_time = 0;
static volatile uint16_t pulse_width = 0;
static volatile uint8_t waiting_rise = 1;

// ===================== ISR =====================
static void tpm1_isr(void *arg)
{
    TPM1->CONTROLS[0].CnSC |= TPM_CnSC_CHF_MASK;

    uint16_t now = TPM1->CONTROLS[0].CnV;

    if (waiting_rise) {
        rise_time = now;
        waiting_rise = 0;

        TPM1->CONTROLS[0].CnSC &= ~TPM_CnSC_ELSA_MASK;
        TPM1->CONTROLS[0].CnSC |= TPM_CnSC_ELSB_MASK;
    } else {
        if (now >= rise_time)
            pulse_width = now - rise_time;
        else
            pulse_width = (TPM_MODULO - rise_time) + now;

        waiting_rise = 1;

        TPM1->CONTROLS[0].CnSC &= ~TPM_CnSC_ELSB_MASK;
        TPM1->CONTROLS[0].CnSC |= TPM_CnSC_ELSA_MASK;
    }
}

// ===================== INIT =====================
int ultrasonic_init(void)
{
    gpioa = device_get_binding("GPIOA");

    if (!gpioa)
        return -1;

    gpio_pin_configure(gpioa, TRIG_PIN, GPIO_OUTPUT);

    IRQ_CONNECT(TPM1_IRQn, 1, tpm1_isr, NULL, 0);
    irq_enable(TPM1_IRQn);

    pwm_tpm_Init(TPM1, TPM_PLLFLL, TPM_MODULO, TPM_CLK, PS_128, EDGE_PWM);

    pwm_tpm_Ch_Init(TPM1, 0,
        TPM_CnSC_ELSA_MASK | TPM_CnSC_CHIE_MASK,
        GPIOA, ECHO_PIN);

    return 0;
}

// ===================== TRIGGER =====================
static void ultrasonic_trigger(void)
{
    gpio_pin_set(gpioa, TRIG_PIN, 1);
    k_busy_wait(10);
    gpio_pin_set(gpioa, TRIG_PIN, 0);

    waiting_rise = 1;
}

// ===================== LEITURA =====================
float ultrasonic_read_cm(void)
{
    ultrasonic_trigger();

    k_msleep(60);

    if (pulse_width > 0 && pulse_width < 40000)
    {
        float time_us = (pulse_width * PRESCALER * 1000000.0f) / TPM_CLOCK;
        return time_us / 58.0f;
    }

    return -1.0f;
}