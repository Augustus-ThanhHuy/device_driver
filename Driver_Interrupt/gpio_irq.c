#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#define GPIO_NUMBER 17 //(BCM)

static unsigned int gpio_irq_number;
static unsigned int gpio_pin = GPIO_NUMBER;
static char *gpio_desc = "GPIO Interrupt";

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    pr_info("GPIO Interrupt: Interrupt occurred!\n");
    return IRQ_HANDLED;
}

static int __init gpio_irq_init(void)
{
    int result;

    pr_info("GPIO Interrupt: Initializing\n");

    //Kiểm tra GPIO pin, chân vật lý có tồn tại trong board hay không
    if(!gpio_is_valid(gpio_pin)){
        pr_info("GPIO Interrupt: Invalid GPIO\n");
        return -ENODEV; //error no device
    }

    gpio_request(gpio_pin, gpio_desc); //load chân 17 (BCM) vào để điều khiển
    gpio_direction_input(gpio_pin); //set chân 17 thành chế độ input

    //Cấp số IRQ cho GPIO 
    gpio_irq_number = gpio_to_irq(gpio_pin);

    //Kiểm tra xem số IRQ đã được cấp hay chưa
    if(gpio_irq_number < 0){
       pr_info("GPIO Interrupt: Failed to get IRQ number GPIO\n");
        return gpio_irq_number;
    }

    pr_info("GPIO Interrupt: Mapped to IRQ %d\n", gpio_irq_number);

    // Request the interrupt line
    result = request_irq(gpio_irq_number, 
        (irq_handler_t)gpio_irq_handler, 
        IRQF_TRIGGER_RISING,
        "gpio_irq_handler",
        NULL);  
    
    if(result){
        pr_info("GPIO Interrupt: Failed to request IRQ\n");
        return result;
    }

    pr_info("GPIO Interrupt: Module loaded\n");
    return 0;
}

static void __exit gpio_irq_exit(void)
{
    pr_info("GPIO Interrupt: Exiting\n");

    //nhả số ngắt IRQ number 
    freee_irq(gpio_irq_number, NULL);

    //nhả chân gpio 
    gpio_free(gpio_pin);

    pr_info("GPIO Interrupt: Module unloaded\n");
}

module_init(gpio_irq_init);
module_exit(gpio_irq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thanh Huy");
MODULE_DESCRIPTION("A simple GPIO Interrupt Module");