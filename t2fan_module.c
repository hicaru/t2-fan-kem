
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/hwmon.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

static int __init fan_module_init(void) {
  pr_info("start module job\n");

  return 0;
}

static void __exit fan_module_exit(void) { pr_info("end module job\n"); }

module_init(fan_module_init);
module_exit(fan_module_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Rinat");
MODULE_DESCRIPTION("The module for apple mac-book fan");
