
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include <linux/hwmon-sysfs.h>
#include <linux/hwmon.h>
#include <linux/thermal.h>

static struct hrtimer temperature_timer;
static struct thermal_zone_device *thermal_zone;

static int log_temperature(void) {
  struct device *hwmon_dev;
  struct device_attribute *attr;
  struct sensor_device_attribute *sensor_attr;
  struct hwmon_chip_info *chip_info;
  struct device *dev;
  int temp;

  hwmon_dev = hwmon_device_register(NULL);
  if (IS_ERR(hwmon_dev)) {
    printk("Failed to register hwmon device\n");
    return PTR_ERR(hwmon_dev);
  }

  printk("Current temperature: %d millidegrees Celsius\n", temp);

  hwmon_device_unregister(hwmon_dev);
  return 0;
}

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
