#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include "pwm_z42.h"

#define TPM_IRQ_LINE TPM1_IRQn
#define TPM_IRQ_PRIORITY 1
#define TPM_MODULE 1000
#define TRIG_PORT GPIOA
#define TRIG_PIN 5
const struct device *gpioa;

// ===================== VARIÁVEIS =====================
volatile uint16_t rise_time = 0;
volatile uint16_t fall_time = 0;
volatile uint16_t pulse_width = 0;
volatile uint8_t waiting_rise = 1;

// ===================== ISR (ECHO) =====================
void tpm1_isr(void *arg) {

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
            pulse_width = (65535 - rise_time) + now;

        waiting_rise = 1;

        TPM1->CONTROLS[0].CnSC &= ~TPM_CnSC_ELSB_MASK;
        TPM1->CONTROLS[0].CnSC |= TPM_CnSC_ELSA_MASK;
    }
}

// ===================== TRIGGER HC-SR04 =====================
static void ultrasonic_trigger(void)
{
    gpio_pin_set(gpioa, TRIG_PIN, 1);
    k_busy_wait(10);
    gpio_pin_set(gpioa, TRIG_PIN, 0);

    waiting_rise = 1;   // <<< IMPORTANTE resetar estado
}

// ===================== MAIN =====================
void main(void) {

    // TIMER INPUT CAPTURE
    IRQ_CONNECT(TPM_IRQ_LINE, TPM_IRQ_PRIORITY, tpm1_isr, NULL, 0);
    irq_enable(TPM_IRQ_LINE);

    pwm_tpm_Init(TPM1, TPM_PLLFLL, 65535, TPM_CLK, PS_128, EDGE_PWM);

    // começa capturando subida
    pwm_tpm_Ch_Init(TPM1, 0,
        TPM_INPUT_CAPTURE_RISING | TPM_CHANNEL_INTERRUPT,GPIOA, 12);

        gpioa = DEVICE_DT_GET(DT_NODELABEL(gpioa));

    // Checa o gpio
    if (!device_is_ready(gpioa)) {
        printk("GPIO nao pronto!\n");
        return;
    } 

    gpio_pin_configure(gpioa, TRIG_PIN, GPIO_OUTPUT_INACTIVE);
 
    while (1)
    {
        // dispara medição
        ultrasonic_trigger();

        // espera resposta estabilizar
        k_msleep(60);

        // filtro básico (descarta lixo)
        if (pulse_width > 0 && pulse_width < 40000)
        {
            // conversão aproximada HC-SR04
            float distance_cm = pulse_width / 58.0f;

            printk("Pulse: %u | Distance: %.2f cm\n",
                   pulse_width, distance_cm);
        }
        else
        {
            printk("Leitura invalida: %u\n", pulse_width);
        }

        k_msleep(300);
    }
}