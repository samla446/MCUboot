#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <zephyr/dfu/mcuboot.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static const struct device *const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

#define APP_TAG "A"

int main(void)
{
	if (!device_is_ready(led.port)) {
		printk("Error: LED device %s is not ready\n", led.port->name);
		return 0;
	}

	int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error %d: failed to configure LED pin %d\n", ret, led.pin);
		return 0;
	}

	printk("\n%s: running, signed version = %s\n",
	       APP_TAG, CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION);
	printk("%s: press 'u' + Enter to request upgrade from slot1\n", APP_TAG);
	/* 模拟"自检通过"：确认当前镜像，避免下次复位被回滚。
	 * 想复现"不确认 → 自动回滚"，把这一行注释掉后重新编译即可。
	 */
	boot_write_img_confirmed();

	while (1) {
		uint8_t ch;

		/* 串口收到 'u' → 让 MCUboot 下次启动时去试运行 slot1 里的固件 */
		if (uart_poll_in(console, &ch) == 0 && ch == 'u') {
			int rc = boot_request_upgrade(false);

			printk("%s: boot_request_upgrade() -> %d (0 = OK), now RESET the board\n",
			       APP_TAG, rc);
		}

		gpio_pin_toggle_dt(&led);
		printk(APP_TAG ": LED is %s\n", gpio_pin_get_dt(&led) ? "ON" : "OFF");
		k_msleep(1000);
	}
}
