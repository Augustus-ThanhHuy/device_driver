#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/sched.h>

#define GPIO_NUMBER 17 //(BCM)
 
static unsigned int gpio_irq_number;
static unsigned int gpio_pin = GPIO_NUMBER;
static char *gpio_desc = "GPIO Interrupt";
static int device_open_count = 0;
static int major_number;
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);
static int flag = 0;

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    flag = 1;
    wake_up_interruptible(&wait_queue);
    pr_info("GPIO Interrupt: Interrupt occured!\n");
    return IRQ_HANDLED;
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset)
{
    wait_event_interruptible(wait_queue, flag != 0); 
    //đợi đến khi flag kh còn bằng 0
    flag = 0;
    return 0;
}

/*
    Hàm open được gọi khi code C trên user_space load Driver lên sử dụng
    Nội dung trong hàm open thường là cấp vùng nhớ cho các hoạt động
    tiếp theo của Driver 
    Đoạn code dưới đây đảm bảo cho drive mỗi lần chỉ có 1 chương trình 1 code C
    được phép load Driver  
*/
static int device_open(struct inode *inode, struct file *file)
{
    if (device_open_count > 0)
        return -EBUSY;
    
    device_open_count++; // Nếu có src code C load device thì count lên 1
    try_module_get(THIS_MODULE);
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    device_open_count--; // -1 giá trị khi hủy Driver 
    module_put(THIS_MODULE);
    return 0;
}

// Khai báo những hàm nào được đưa lên lớp user_space
// để người dùng có thể tương tác với Driver này
static struct file_operations fops = {
    .open = device_open, //dc gọi khi Driver dc load bởi code c
    .release = device_release, // Nếu kh muốn dùng Driver nữa
    .read = device_read, // (hàm ioctl vẫn dc, do hàm ioctl dùng để
    // gửi data 2 chiều qua lại)
    // dùng hàm read vì chỉ cần lấy sk ngắt xảy ra lên user_space kh có chiều ngc lại
}

static int __init gpio_irq_init(void)
{
    int result;

    pr_info("GPIO Interrupt: Initializing\n");

    // Đăng ký Driver với danh nghĩa là character device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        pr_info("GPIO Interrupt: Failed to register a major number\n");
        return major_number;
    }
    pr_info("GPIO Interrupt: Registered correctly with major number %d\n", major_number);

    //Kiểm tra GPIO pin, chân vật lý có tồn tại trong board hay không
    if(!gpio_is_valid(gpio_pin)){
        pr_info("GPIO Interrupt: Invalid GPIO\n");
        unregister_chrdev(major_number, DEVICE_NAME); //Hủy dky khi ctrinh bị lỗi  
        return -ENODEV; //error no device
    }

    gpio_request(gpio_pin, gpio_desc); //load chân 17 (BCM) vào để điều khiển
    gpio_direction_input(gpio_pin); //set chân 17 thành chế độ input

    //Cấp số IRQ cho GPIO 
    gpio_irq_number = gpio_to_irq(gpio_pin);


    //Kiểm tra xem số IRQ đã được cấp hay chưa
    if(gpio_irq_number < 0){
       pr_info("GPIO Interrupt: Failed to get IRQ number GPIO\n");
        unregister_chrdev(major_number, DEVICE_NAME);
        return gpio_irq_number
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
        gpio_free(gpio_pin);
        unregister_chrdev(major_number, DEVICE_NAME);
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

    unregister_chrdev(major_number, DEVICE_NAME);


    pr_info("GPIO Interrupt: Module unloaded\n");
}

module_init(gpio_irq_init);
module_exit(gpio_irq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CDT");
MODULE_DESCRIPTION("A simple GPIO Interrupt Module");