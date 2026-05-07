#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/devicetree.h>
#include <pwm_z42.h>
#include "ultrassonic.h"   

// ===================== PORTAS =====================
#define PORTA_NODE DT_NODELABEL(gpioa)
#define PORTB_NODE DT_NODELABEL(gpiob)
#define PORTD_NODE DT_NODELABEL(gpiod)

static const struct device *porta = DEVICE_DT_GET(PORTA_NODE);
static const struct device *portb = DEVICE_DT_GET(PORTB_NODE);
static const struct device *portd = DEVICE_DT_GET(PORTD_NODE);

// ===================== PWM =====================
#define TPM_MODULE 1000

int main(void)
{
    // ===================== CHECK =====================
    if (!device_is_ready(porta) ||
        !device_is_ready(portb) ||
        !device_is_ready(portd)) {
        printk("Erro device\n");
        return 0;
    }

    // ===================== ULTRASSOM =====================
    ultrasonic_init();   // <<< inicia sua lib

    // ===================== PWM =====================
    pwm_tpm_Init(TPM1, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);

    pwm_tpm_Ch_Init(TPM1, 0, TPM_PWM_H, GPIOB, 0); // esquerda
    pwm_tpm_Ch_Init(TPM1, 1, TPM_PWM_H, GPIOB, 1); // direita

    // ===================== DIREÇÃO =====================
    gpio_pin_configure(portb, 2, GPIO_OUTPUT); // IN1
    gpio_pin_configure(porta, 4, GPIO_OUTPUT); // IN2
    gpio_pin_configure(porta, 13, GPIO_OUTPUT);// IN3
    gpio_pin_configure(portd, 4, GPIO_OUTPUT); // IN4

    int velocidade = 600;

    while (1)
    {
        float d = ultrasonic_read_cm();   // <<< leitura da sua biblioteca

        printk("Distancia: %.2f cm\n", d);

        if (d > 20.0)
        {
            // ===================== ANDA RETO =====================
            gpio_pin_set(porta, 13, 1);
            gpio_pin_set(portd, 4, 0);
            pwm_tpm_CnV(TPM1, 0, velocidade);

            gpio_pin_set(portb, 2, 1);
            gpio_pin_set(porta, 4, 0);
            pwm_tpm_CnV(TPM1, 1, velocidade);
        }
        else
        {
            // ===================== PARA =====================
            pwm_tpm_CnV(TPM1, 0, 0);
            pwm_tpm_CnV(TPM1, 1, 0);
        }

        k_msleep(100);
    }
}