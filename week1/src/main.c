#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

int tila=0;
int prev=0;
// Red led thread initialization
#define STACKSIZE 500
#define PRIORITY 5
void red_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
void green_led_task(void *, void *, void*);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
void yellow_led_task(void *, void *, void*);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);
#define BUTTON_0 DT_ALIAS(sw0)
#define BUTTON_1 DT_ALIAS(sw1)
#define BUTTON_2 DT_ALIAS(sw2)
#define BUTTON_3 DT_ALIAS(sw3)
#define BUTTON_4 DT_ALIAS(sw4)
static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static struct gpio_callback button_0_data;
static const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET_OR(BUTTON_1, gpios, {1});
static struct gpio_callback button_1_data;
static const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {2});
static struct gpio_callback button_2_data;
static const struct gpio_dt_spec button_3 = GPIO_DT_SPEC_GET_OR(BUTTON_3, gpios, {3});
static struct gpio_callback button_3_data;
static const struct gpio_dt_spec button_4 = GPIO_DT_SPEC_GET_OR(BUTTON_4, gpios, {4});
static struct gpio_callback button_4_data;
int button_4_pressed=0;
int button_3_pressed=0;
int button_2_pressed=0;
int button_1_pressed=0;

// Button interrupt handler
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	
	
 printk("Button 0 pressed\n");

    if (tila != 4)
    {
        prev = tila;  // save current tila before pausing
        tila = 4;     // pause state

        k_thread_suspend(red_thread);
        k_thread_suspend(green_thread);
        k_thread_suspend(yellow_thread);

        printk("Paused. tila=%d prev=%d\n", tila, prev);
    }
    else
    {
        tila = prev;  // restore previous tila

        k_thread_resume(red_thread);
        k_thread_resume(green_thread);
        k_thread_resume(yellow_thread);

        printk("Resumed. tila=%d prev=%d\n", tila, prev);
    }
	
}
void button_1_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Button 1 pressed\n");
	
	button_1_pressed = !button_1_pressed;
if (button_1_pressed)
{
   gpio_pin_set_dt(&blue,0);
	gpio_pin_set_dt(&green,0);
	gpio_pin_set_dt(&red,1);
}
else
{
   gpio_pin_set_dt(&red,0);
}

	

}
void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Button 2 pressed\n");
	button_2_pressed = !button_2_pressed;
if (button_2_pressed)
{
   gpio_pin_set_dt(&blue,0);
	gpio_pin_set_dt(&green,1);
	gpio_pin_set_dt(&red,1);
}
else
{
   gpio_pin_set_dt(&red,0);
   gpio_pin_set_dt(&green, 0);
}
}
void button_3_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	printk("Button 3 pressed\n");
	button_3_pressed = !button_3_pressed;
	if (button_3_pressed)
{
   gpio_pin_set_dt(&blue,0);
	gpio_pin_set_dt(&green,1);
	gpio_pin_set_dt(&red,0);
}
else
{
   gpio_pin_set_dt(&green,0);
}
}
void button_4_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{ 
	printk("Button 4 pressed\n");
	
button_4_pressed = !button_4_pressed;
if (button_4_pressed)
{
    k_thread_resume(yellow_thread);
    tila = 2;
}
else
{
    k_thread_suspend(yellow_thread);
    tila = 4;
}

}
// Main program
int main(void)
{
	init_button();
	init_led();
	tila=1;
	return 0;
}

// Initialize leds
int  init_led() {

	// Led pin initialization
	int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");		
		return ret;
	}
	// set led off
	gpio_pin_set_dt(&red,0);

	printk("Red led initialized ok\n");

        	// Led pin initialization
	int ret2 = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret2 < 0) {
		printk("Error: Led configure failed\n");		
		return ret2;
	}
	// set led off
	gpio_pin_set_dt(&green,0);

	printk("Green led initialized ok\n");

        	// Led pin initialization
	int ret3 = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
	if (ret3 < 0) {
		printk("Error: Led configure failed\n");		
		return ret3;
	}
	// set led off
	gpio_pin_set_dt(&blue,0);

	printk("Blue led initialized ok\n");
	
	return 0;
}

// Task to handle red led
void red_led_task(void *, void *, void*) {
	
	while(true){
	if(tila==1) {
		printk("Red led thread started\n");
		// 1. set led on 
		gpio_pin_set_dt(&red,1);
		printk("Red on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&red,0);
		printk("Red off\n");
		
		// 4. sleep for 2 seconds
		
		k_sleep(K_SECONDS(1));
		tila=2;
		prev=1;
	}
	
	k_yield();
}
	
	
}

void green_led_task(void *, void *, void*) {
	
while(true){
	if(tila==3) {
		printk("Green led thread started\n");
		// 1. set led on 
		gpio_pin_set_dt(&green,1);
		printk("Green on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&green,0);
		printk("Green off\n");
		
		// 4. sleep for 2 seconds
		
		
		k_sleep(K_SECONDS(1));
		tila=2;
		prev=3;
	}
	k_yield();
	
}
}

void yellow_led_task(void *, void *, void*) {
	
while(true){
	if (tila==2) {
		printk("Blue led thread started\n");
		// 1. set led on 
		gpio_pin_set_dt(&green,1);
		gpio_pin_set_dt(&red,1);
		printk("Yellow on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&green,0);
		gpio_pin_set_dt(&red,0);
		
		printk("Yellow off\n");
		// 4. sleep for 2 seconds
		
		k_sleep(K_SECONDS(1));
		if(button_4_pressed==0)
		{
		if(prev==1)
		{
			tila=3;
		}
		else
		{
			tila=1;
		}
		prev=2;
	}
		
	}
	k_yield();
}
		
	
	
}

int init_button() {
	int ret;

	// Check readiness of all buttons
	if (!gpio_is_ready_dt(&button_0) ||
	    !gpio_is_ready_dt(&button_1) ||
	    !gpio_is_ready_dt(&button_2) ||
	    !gpio_is_ready_dt(&button_3) ||
	    !gpio_is_ready_dt(&button_4)) {
		printk("Error: one or more buttons are not ready\n");
		return -1;
	}

	// Configure each button and its interrupt
	ret = gpio_pin_configure_dt(&button_0, GPIO_INPUT);
	if (ret != 0) return -1;
	ret = gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) return -1;

	ret = gpio_pin_configure_dt(&button_1, GPIO_INPUT);
	if (ret != 0) return -1;
	ret = gpio_pin_interrupt_configure_dt(&button_1, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) return -1;

	ret = gpio_pin_configure_dt(&button_2, GPIO_INPUT);
	if (ret != 0) return -1;
	ret = gpio_pin_interrupt_configure_dt(&button_2, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) return -1;

	ret = gpio_pin_configure_dt(&button_3, GPIO_INPUT);
	if (ret != 0) return -1;
	ret = gpio_pin_interrupt_configure_dt(&button_3, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) return -1;

	ret = gpio_pin_configure_dt(&button_4, GPIO_INPUT);
	if (ret != 0) return -1;
	ret = gpio_pin_interrupt_configure_dt(&button_4, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) return -1;

	// Set up interrupt callbacks
	gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
	gpio_add_callback(button_0.port, &button_0_data);

	gpio_init_callback(&button_1_data, button_1_handler, BIT(button_1.pin));
	gpio_add_callback(button_1.port, &button_1_data);

	gpio_init_callback(&button_2_data, button_2_handler, BIT(button_2.pin));
	gpio_add_callback(button_2.port, &button_2_data);

	gpio_init_callback(&button_3_data, button_3_handler, BIT(button_3.pin));
	gpio_add_callback(button_3.port, &button_3_data);

	gpio_init_callback(&button_4_data, button_4_handler, BIT(button_4.pin));
	gpio_add_callback(button_4.port, &button_4_data);

	printk("Set up all buttons OK\n");
	return 0;
}

