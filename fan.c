
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/hwmon.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

static struct thermal_zone_device *tz_dev;
static struct file_operations fops;

static int temp_callback(struct thermal_zone_device *thermal, int trip,
                         int temp, void *data) {

  pr_info("temp chaned %d degres\n", temp);

  return 0;
}

static int __init my_module_init(void) {
  int ret;

  pr_info("start module job\n");

  tz_dev = thermal_zone_get_zone_by_name("cpu_thermal");

  if (!tz_dev) {
    pr_err("cannot find temperature zone\n");

    return -ENODEV;
  }

  ret = thermal_zone_bind_cooling_device(tz_dev, 0, NULL, temp_callback, NULL);

  if (ret) {
    pr_err("callback-error\n");
    return ret;
  }

  return 0;
}

static void __exit my_module_exit(void) {
  pr_info("end module job\n");
  thermal_zone_unbind_cooling_device(tz_dev, 0, NULL);
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("MIT");
MODULE_AUTHOR("Rinat");
MODULE_DESCRIPTION("The module for apple mac-book fan");
