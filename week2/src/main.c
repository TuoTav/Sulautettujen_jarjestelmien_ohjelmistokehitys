#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
// Tämä on vasta yhden pisteen toteutus viikkotehtävä 3 koska tein kolme pisteen toteutuksen käytän tässä singelshot taskeja

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

// UART ja dispatcher
const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
#define STACKSIZE 500
#define PRIORITY 5

#define MSGQ_MAX_MSGS 32
#define MSGQ_MSG_SIZE sizeof(char)
K_MSGQ_DEFINE(color_msgq, MSGQ_MSG_SIZE, MSGQ_MAX_MSGS, 4);

K_SEM_DEFINE(dispatcher_release_sem, 1, 1);

// Thread-handlet
void red_led_task(void *, void *, void *);
void green_led_task(void *, void *, void *);
void yellow_led_task(void *, void *, void *);
void dispatcher_thread(void *, void *, void *);
void uart_receiver_thread(void *, void *, void *);

K_THREAD_DEFINE(red_thread, STACKSIZE, red_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread, STACKSIZE, green_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(dispatcher_thread_id, STACKSIZE, dispatcher_thread, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_thread_id, STACKSIZE, uart_receiver_thread, NULL, NULL, NULL, PRIORITY, 0, 0);

// Pause-tuki
int paused = 0;

void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    if (!paused) {
        paused = 1;
        k_thread_suspend(red_thread);
        k_thread_suspend(green_thread);
        k_thread_suspend(yellow_thread);
        printk("Paused\n");
    } else {
        paused = 0;
        k_thread_resume(red_thread);
        k_thread_resume(green_thread);
        k_thread_resume(yellow_thread);
        printk("Resumed\n");
    }
}

void button_1_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    printk("Button 1 pressed\n");
}

void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    printk("Button 2 pressed\n");
}

void button_3_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    printk("Button 3 pressed\n");
}

void button_4_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    printk("Button 4 pressed\n");
}

void red_led_task(void *, void *, void *) {
    while (1) {
        k_thread_suspend(k_current_get());
        gpio_pin_set_dt(&red, 1);
        printk("Red ON\n");
        k_sleep(K_SECONDS(1));
        gpio_pin_set_dt(&red, 0);
        printk("Red OFF\n");
        k_sem_give(&dispatcher_release_sem);
    }
}

void green_led_task(void *, void *, void *) {
    while (1) {
        k_thread_suspend(k_current_get());
        gpio_pin_set_dt(&green, 1);
        printk("Green ON\n");
        k_sleep(K_SECONDS(1));
        gpio_pin_set_dt(&green, 0);
        printk("Green OFF\n");
        k_sem_give(&dispatcher_release_sem);
    }
}

void yellow_led_task(void *, void *, void *) {
    while (1) {
        k_thread_suspend(k_current_get());
        gpio_pin_set_dt(&red, 1);
        gpio_pin_set_dt(&green, 1);
        printk("Yellow ON\n");
        k_sleep(K_SECONDS(1));
        gpio_pin_set_dt(&red, 0);
        gpio_pin_set_dt(&green, 0);
        printk("Yellow OFF\n");
        k_sem_give(&dispatcher_release_sem);
    }
}

void dispatcher_thread(void *, void *, void *) {
    char color;
    while (1) {
        k_msgq_get(&color_msgq, &color, K_FOREVER);
        k_sem_take(&dispatcher_release_sem, K_FOREVER);
        if (paused) continue;
        switch (color) {
            case 'R':
                k_thread_resume(red_thread);
                break;
            case 'Y':
                k_thread_resume(yellow_thread);
                break;
            case 'G':
                k_thread_resume(green_thread);
                break;
            default:
                break;
        }
    }
}

void uart_receiver_thread(void *, void *, void *) {
    char c;
    while (1) {
        if (uart_poll_in(uart_dev, &c) == 0) {
            if (c == 'R' || c == 'Y' || c == 'G') {
                k_msgq_put(&color_msgq, &c, K_NO_WAIT);
            }
        }
        k_msleep(10);
    }
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
	init_led();
	init_buttons();
    if (!device_is_ready(uart_dev)) {
        printk("UART device not ready\n");
        return 1;
    }

}