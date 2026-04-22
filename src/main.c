#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include "pwm_z42.h"                // API para controle de PWM

#define TPM_IRQ_LINE TPM1_IRQn  // relaciona a interrupção ao timer TPM1
#define TPM_IRQ_PRIORITY 1      // define a prioridade da interrupção

volatile uint16_t captured= 0; 

void tpm1_isr(void *arg)
{
       TPM1->STATUS |= TPM_STATUS_CH0F_MASK; // zerra a flag que gerou a interrupção

       captured = TPM1->CONTROLS[0].CnV; // coloca o valor atual do timer na variável "captured"
}

#define INPUT_PORT  "gpio@400ff0c0"   // Porta D configurada com endereço direto
                                      // Pegue o endereço no zephyr.dts dentro de pio \ build
#define INPUT_PIN7   7                // PTD7

void main(void)
{
    int ret, val;  
    const struct device *input_dev;   
    
    // Conecta a interrupção via Zephyr
    IRQ_CONNECT(TPM_IRQ_LINE, TPM_IRQ_PRIORITY, tpm1_isr, NULL, 0);
    irq_enable(TPM_IRQ_LINE);
 
    // Inicializa TPM1 com módulo e prescaler desejado
    pwm_tpm_Init(TPM1, TPM_PLLFLL, 65535, TPM_CLK, PS_128, EDGE_PWM);

    // Configura TPM1_CH0 como input capture na borda de subida
    pwm_tpm_Ch_Init(TPM1, 0, TPM_INPUT_CAPTURE_RISING | TPM_CHANNEL_INTERRUPT, GPIOD, 7);

    input_dev = device_get_binding(INPUT_PORT);
    if (!input_dev) {
        printk("Erro ao acessar porta %s\n", INPUT_PORT);
        return;
    }
    //ret = gpio_pin_configure(input_dev, INPUT_PIN7, GPIO_INPUT | GPIO_PULL_UP);

    while (1)
    {
        printk("Valor do TPM1: %u\n", captured);
        val = gpio_pin_get(input_dev, INPUT_PIN7);
        printk("Valor do PTD7: %d\n", val);
        k_msleep(1000); 
    }
}