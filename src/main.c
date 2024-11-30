/* três threads e mutexes - blinky modificado */

/*
 * 2 mutexes
 * - 1º para função ativa periodicamente
 * - 2º para função ativa por evento
 *
 * mutex == 0 - em uso
 * mutex != 0 - livre
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <string.h>

/* tamanho da área ocupada por cada thread */
#define STACKSIZE 1024

//cria os mutexes
K_MUTEX_DEFINE(mutex1);
K_MUTEX_DEFINE(mutex2);

/* prioridades */
#define PRIORITY0 0
#define PRIORITY1 0
#define PRIORITY2 0

int SLEEP_TIME_MS = 1000;

/* definições dos leds */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
#define LED0_NODE DT_ALIAS(led2)
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

/* configurações do botão e seu led azul */
#define SW0_NODE DT_ALIAS(sw0)

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
     {0});
static struct gpio_callback button_cb_data;

#define LED0_NODE DT_ALIAS(led1)
static struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios,
    {0});

///////////////////////////////////////////////////////////////////////////////////////

void button_pressed(const struct device *dev, struct gpio_callback *cb,
   uint32_t pins)
{ printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32()); }

/////////////////////////////////////////////////////////////////////////////////////////

//pisca led verde
void blink0(void){
    gpio_pin_configure_dt(&led0, GPIO_OUTPUT); //configura led1 como output
    gpio_pin_set_dt(&led0, 0);

    while (1) {
        k_mutex_lock(&mutex1, K_FOREVER); //pega mutex
        printf("\n 1 pego! LED1\n");

        gpio_pin_toggle_dt(&led0); //liga
        k_msleep(SLEEP_TIME_MS); //por SLEEP_TIME_MSs
    
        k_mutex_unlock(&mutex1); //solta mutex
        printf("\n 1 solto! LED1\n");

        gpio_pin_toggle_dt(&led0); //desliga
        k_msleep(SLEEP_TIME_MS); //espera SLEEP_TIME_MSs
    }
}

//pisca led azul
int blink1(void){
    //tem que segurar o mutex enquanto o botão estiver pressionado!

    //configuração do botão
    gpio_pin_configure_dt(&button, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

    //inicializa a callback
    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    if(led1.port){
        gpio_pin_configure_dt(&led1, GPIO_OUTPUT); //config led1
    }

    if(led1.port){
        while(1){
            int val = gpio_pin_get_dt(&button); //pega valor do botão
            
            if (val == 0) { //botão solto
                gpio_pin_set_dt(&led1, val); //apaga led
                k_mutex_unlock(&mutex2); //solta mutex2
                printf("\n 2 solto! \n");
            }
            if(val == 1){ //botão apertado
                gpio_pin_set_dt(&led1, val); //acende led
                k_mutex_lock(&mutex2, K_FOREVER); //pega mutex2
                printf("\n 2 pego! \n");
            }
            k_msleep(10);
        }
    }
}

//pisca led vermelho
void blink2(void){
    gpio_pin_configure_dt(&led2, GPIO_OUTPUT); //config led2
    gpio_pin_set_dt(&led2, 0); // 0 É DESLIGADO
 
	/* if(k_mutex_lock(&mutex, K_NO_WAIT)) 
	*		== 0 -> mutex está livre
	* 		!= 0 -> mutex está em uso -> segue para comandos no if
	*/

	while(2){
		if(k_mutex_lock(&mutex1, K_NO_WAIT)){
			if(k_mutex_lock(&mutex2, K_NO_WAIT)){
				printf("\nDEU CERTO!\n");
				gpio_pin_toggle_dt(&led2); //liga led2
			} else{
				printf("\n AHHH \n");
            	gpio_pin_set_dt(&led2, 0); //desliga led				
			}
		} else {
        	printf("\n NAO DEU !\n");
            gpio_pin_set_dt(&led2, 0); //desliga led
		}
		k_mutex_unlock(&mutex1); //solta mutex1
		k_mutex_unlock(&mutex2); //solta mutex2
		k_msleep(10);
	}
}

//define threads
K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0,
        NULL, NULL, NULL, PRIORITY0, 0, 0);
K_THREAD_DEFINE(blink1_id, STACKSIZE, blink1,
        NULL, NULL, NULL, PRIORITY1, 0, 0);
K_THREAD_DEFINE(blink2_id, STACKSIZE, blink2,
        NULL, NULL, NULL, PRIORITY2, 0, 0);