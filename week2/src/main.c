#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <stdlib.h>
#include <zephyr/timing/timing.h>
// Tämä on 2 pisteen toteutus viikkotehtävä 4 vielä pitää keksiä tapa jolla debugtask ei lisää tasking suoritus aikaa

// LED-konfiguraatiot
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

// Button-konfiguraatiot
#define BUTTON_0 DT_ALIAS(sw0)
#define BUTTON_1 DT_ALIAS(sw1)
#define BUTTON_2 DT_ALIAS(sw2)
#define BUTTON_3 DT_ALIAS(sw3)
#define BUTTON_4 DT_ALIAS(sw4)

static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET_OR(BUTTON_1, gpios, {0});
static const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {0});
static const struct gpio_dt_spec button_3 = GPIO_DT_SPEC_GET_OR(BUTTON_3, gpios, {0});
static const struct gpio_dt_spec button_4 = GPIO_DT_SPEC_GET_OR(BUTTON_4, gpios, {0});

static struct gpio_callback button_0_cb_data;
static struct gpio_callback button_1_cb_data;
static struct gpio_callback button_2_cb_data;
static struct gpio_callback button_3_cb_data;
static struct gpio_callback button_4_cb_data;

K_SEM_DEFINE(red_sem, 0, 1);
K_SEM_DEFINE(green_sem, 0, 1);
K_SEM_DEFINE(yellow_sem, 0, 1);

// UART ja dispatcher
const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
#define STACKSIZE 500
#define PRIORITY 5

//#define MSGQ_MAX_MSGS 32
//#define MSGQ_MSG_SIZE sizeof(char)
//K_MSGQ_DEFINE(color_msgq, MSGQ_MSG_SIZE, MSGQ_MAX_MSGS, 4);

K_SEM_DEFINE(dispatcher_release_sem, 1, 1);


void red_led_task(void *, void *, void *);
void green_led_task(void *, void *, void *);
void yellow_led_task(void *, void *, void *);
static void dispatcher_task(void *, void *, void *);
static void uart_task(void *, void *, void *);
static void debug_task(void*,void*,void*);

K_THREAD_DEFINE(red_thread, STACKSIZE, red_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(dispatcher_thread_id, STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_thread_id, STACKSIZE, uart_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(debug_thread_id, STACKSIZE, debug_task, NULL,NULL,NULL,6,0,0);

struct k_mutex light_mutex;
struct k_mutex time_mutex;
int paused = 0;
volatile int red_duration = 1000;
volatile int green_duration = 1000;
volatile int yellow_duration = 1000;
uint64_t total_sequence_time_us=0;
int debug_enabled=0;

struct data_t{
	void *fifo_reserved;
	char msg[24];
	
};
struct debug_data_t{
    void* fifo_reserved;
    char msg[64];
};

K_FIFO_DEFINE(debug_fifo);

K_FIFO_DEFINE(dispatcher_fifo);

void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	paused = !paused;
    debug_log("paused");

}

void button_1_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    
    struct data_t *buf =k_malloc(sizeof(struct data_t));
    if(buf !=NULL)
    {
        snprintf(buf->msg,sizeof(buf->msg),"R,1000");
        k_fifo_put(&dispatcher_fifo, buf);
    }
   // printk("Button 1 pressed\n");
    

}

void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    
	 struct data_t *buf =k_malloc(sizeof(struct data_t));
    if(buf !=NULL)
    {
        snprintf(buf->msg,sizeof(buf->msg),"Y,1000");
        k_fifo_put(&dispatcher_fifo, buf);
    }
    debug_log("Button 2 pressed\n");

}

void button_3_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	 struct data_t *buf =k_malloc(sizeof(struct data_t));
    if(buf !=NULL)
    {
        snprintf(buf->msg,sizeof(buf->msg),"G,1000");
        k_fifo_put(&dispatcher_fifo, buf);
    }
    //printk("Button 3 pressed\n");
}

void button_4_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    total_sequence_time_us=0;
    debug_log("Button 4 pressed\n");
}

void red_led_task(void*, void*, void *) {
    while (1) {
        k_sem_take(&red_sem, K_FOREVER); 
        k_mutex_lock(&light_mutex, K_FOREVER); 

        timing_t start_time , end_time;
        timing_start();
        start_time = timing_counter_get();

        gpio_pin_set_dt(&red, 1);
        debug_log("Red ON\n");
        k_msleep(red_duration);
        gpio_pin_set_dt(&red, 0);
        debug_log("Red OFF\n");

        end_time = timing_counter_get();
        timing_stop();

        uint64_t elapsed_cycles = timing_cycles_get(&start_time, &end_time);
        uint64_t elapsed_ns = timing_cycles_to_ns(elapsed_cycles);
        uint64_t elapsed_us = elapsed_ns / 1000;

        k_mutex_lock(&time_mutex, K_FOREVER);
        total_sequence_time_us += elapsed_us;
        k_mutex_unlock(&time_mutex);

        debug_log(" Red task took %llu µs (%llu cycles)\n", elapsed_us, elapsed_cycles);
        debug_log(" Total sequence time: %llu µs\n", total_sequence_time_us);

        k_mutex_unlock(&light_mutex);
        k_sem_give(&dispatcher_release_sem);  
    }
}

void green_led_task(void * , void*, void *) {
    while (1) {

        k_sem_take(&green_sem, K_FOREVER);
        k_mutex_lock(&light_mutex, K_FOREVER);
        timing_t start_time , end_time;
        timing_start();
        start_time = timing_counter_get();


        gpio_pin_set_dt(&green, 1);
        debug_log("Green ON\n");
        k_msleep(green_duration);
        gpio_pin_set_dt(&green, 0);
        debug_log("Green OFF\n");

        end_time = timing_counter_get();
        timing_stop();

        uint64_t elapsed_cycles = timing_cycles_get(&start_time, &end_time);
        uint64_t elapsed_ns = timing_cycles_to_ns(elapsed_cycles);
        uint64_t elapsed_us = elapsed_ns / 1000;

        k_mutex_lock(&time_mutex, K_FOREVER);
        total_sequence_time_us += elapsed_us;
        k_mutex_unlock(&time_mutex);

        debug_log(" Green task took %llu µs (%llu cycles)\n", elapsed_us, elapsed_cycles);
        debug_log(" Total sequence time: %llu µs\n", total_sequence_time_us);

        k_mutex_unlock(&light_mutex);
        k_sem_give(&dispatcher_release_sem);
    }
}

void yellow_led_task(void * , void*, void *) {
    while (1) {
        k_sem_take(&yellow_sem, K_FOREVER);
        k_mutex_lock(&light_mutex, K_FOREVER);

        timing_t start_time , end_time;
        timing_start();
        start_time = timing_counter_get();

        gpio_pin_set_dt(&red, 1);
        gpio_pin_set_dt(&green, 1);
        debug_log("Yellow ON\n");
        k_msleep(yellow_duration);
        gpio_pin_set_dt(&red, 0);
        gpio_pin_set_dt(&green, 0);
        debug_log("Yellow OFF\n");

        end_time = timing_counter_get();
        timing_stop();

        uint64_t elapsed_cycles = timing_cycles_get(&start_time, &end_time);
        uint64_t elapsed_ns = timing_cycles_to_ns(elapsed_cycles);
        uint64_t elapsed_us = elapsed_ns / 1000;

        k_mutex_lock(&time_mutex, K_FOREVER);
        total_sequence_time_us += elapsed_us;
        k_mutex_unlock(&time_mutex);

        debug_log(" Yellow task took %llu µs (%llu cycles)\n", elapsed_us, elapsed_cycles);
        debug_log(" Total sequence time: %llu µs\n", total_sequence_time_us);

        k_mutex_unlock(&light_mutex);
        k_sem_give(&dispatcher_release_sem);
    }
}

static void dispatcher_task(void *, void *, void *)
{
    while (true) {
        struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
        while(paused){
            k_msleep(100);
        }
        
        if (!rec_item) continue;

        char sequence[20];
        strncpy(sequence, rec_item->msg, sizeof(sequence));
        k_free(rec_item);

        debug_log("Dispatcher: %s\n", sequence);

        
        char color = sequence[0];
        int duration = atoi(&sequence[2]); 

        debug_log("Parsed: %c, %d ms\n", color, duration);

        
        if (color == 'R') {
            red_duration = duration;
            k_sem_give(&red_sem);
        } else if (color == 'Y') {
            yellow_duration = duration;
            k_sem_give(&yellow_sem);
        } else if (color == 'G') {
            green_duration = duration;
            k_sem_give(&green_sem);
        }
        

       
        k_sem_take(&dispatcher_release_sem, K_FOREVER);
    }
}

static void uart_task(void *, void *, void *)
{
    char rc = 0;
    char uart_msg[20];
    memset(uart_msg, 0, sizeof(uart_msg));
    int uart_msg_cnt = 0;

    while (true) {
        if (uart_poll_in(uart_dev, &rc) == 0) {
            if (rc != '\r') {
                if (uart_msg_cnt < sizeof(uart_msg) - 1) {
                    uart_msg[uart_msg_cnt++] = rc;
                }
            } else {
                printk("UART msg: %s\n", uart_msg);
                if(strcmp(uart_msg,"D,ON")==0){
                    debug_enabled=1;
                    printk("DEBUG ENABLED\n");
                }else if(strcmp(uart_msg,"D,OFF")==0){
                    debug_enabled=0;
                    printk("DEBUG DISABLED \n");
                }
                else{
                
                struct data_t *buf = k_malloc(sizeof(struct data_t));
                if (buf != NULL) {
                    snprintf(buf->msg, sizeof(buf->msg), "%s", uart_msg);
                    k_fifo_put(&dispatcher_fifo, buf);
                }
            }
                uart_msg_cnt = 0;
                memset(uart_msg, 0, sizeof(uart_msg));
            }
        }
        k_msleep(10);
    }
}
static void debug_task(void* ,void *, void *){
    while(1){
        struct debug_data_t *dbg = k_fifo_get(&debug_fifo, K_FOREVER);

        if(dbg){
            printk("[DEBUG] %s\n", dbg->msg);
            k_free(dbg);
        }
    }
}
void debug_log(const char *format,...)
{
    if (!debug_enabled) return; 

    struct debug_data_t *msg = k_malloc(sizeof(struct debug_data_t));
    if (!msg) return;

    va_list args;
    va_start(args, format);
    vsnprintf(msg->msg, sizeof(msg->msg), format, args);
    va_end(args);

    k_fifo_put(&debug_fifo, msg);
}

int init_led() {
    gpio_pin_configure_dt(&red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&blue, GPIO_OUTPUT_INACTIVE);
    return 0;
}

int init_buttons() {
    gpio_pin_configure_dt(&button_0, GPIO_INPUT);
    gpio_pin_configure_dt(&button_1, GPIO_INPUT);
    gpio_pin_configure_dt(&button_2, GPIO_INPUT);
    gpio_pin_configure_dt(&button_3, GPIO_INPUT);
    gpio_pin_configure_dt(&button_4, GPIO_INPUT);

    gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_1, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_2, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_3, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_pin_interrupt_configure_dt(&button_4, GPIO_INT_EDGE_TO_ACTIVE);

    gpio_init_callback(&button_0_cb_data, button_0_handler, BIT(button_0.pin));
    gpio_add_callback(button_0.port, &button_0_cb_data);

    gpio_init_callback(&button_1_cb_data, button_1_handler, BIT(button_1.pin));
    gpio_add_callback(button_1.port, &button_1_cb_data);

    gpio_init_callback(&button_2_cb_data, button_2_handler, BIT(button_2.pin));
    gpio_add_callback(button_2.port, &button_2_cb_data);

    gpio_init_callback(&button_3_cb_data, button_3_handler, BIT(button_3.pin));
    gpio_add_callback(button_3.port, &button_3_cb_data);

    gpio_init_callback(&button_4_cb_data, button_4_handler, BIT(button_4.pin));
    gpio_add_callback(button_4.port, &button_4_cb_data);

    return 0;
}

int main(void) {
    k_mutex_init(&light_mutex);
    k_mutex_init(&time_mutex);
    timing_init();
	init_led();
	init_buttons();
    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return 1;
    }

}