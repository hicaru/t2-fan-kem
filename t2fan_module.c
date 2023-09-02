
#include <linux/fs.h>
#include <linux/module.h>

#include <linux/acpi.h>
#include <linux/device.h>
#include <linux/dmi.h>
#include <linux/platform_device.h>

#include <linux/hwmon-sysfs.h>
#include <linux/hwmon.h>

#define dbg_msg(fmt, ...)                                                      \
  do {                                                                         \
    if (DEBUG)                                                                 \
      printk(KERN_INFO "asus-fan (debug) - " fmt "\n", ##__VA_ARGS__);         \
  } while (0)

#define info_msg(title, fmt, ...)                                              \
  do {                                                                         \
    printk(KERN_INFO "asus-fan (" title ") - " fmt "\n", ##__VA_ARGS__);       \
  } while (0)

#define err_msg(title, fmt, ...)                                               \
  do {                                                                         \
    printk(KERN_ERR "asus-fan (" title ") - " fmt "\n", ##__VA_ARGS__);        \
  } while (0)

#define warn_msg(title, fmt, ...)                                              \
  do {                                                                         \
    printk(KERN_WARNING "asus-fan (" title ") - " fmt "\n", ##__VA_ARGS__);    \
  } while (0)

struct apple_fan_driver {
  const char *name;
  struct module *owner;

  int (*probe)(struct platform_device *device);

  struct platform_driver platform_driver;
  struct platform_device *platform_device;
};

struct apple_fan {
  struct platform_device *platform_device;

  struct apple_fan_driver *driver;
  struct apple_fan_driver *driver_gfx;

  struct device *hwmon_dev;
};

struct apple_fan_data {
  struct apple_fan *apple_fan_obj;

  // 'fan_states' save last (manually) set fan state/speed
  int fan_states[2];
  // 'fan_manual_mode' keeps whether this fan is manually controlled
  bool fan_manual_mode[2];
  // 'true' - if first fan is available
  bool has_fan;
  // 'true' - if second fan is available
  bool has_gfx_fan;
  // max fan speed default
  int max_fan_speed_default;
  // ... user-defined max value
  int max_fan_speed_setting;
  // minimum allowed (set) speed for fan(s)
  int fan_minimum;
  // this speed will be reported as the minimal speed for the fans
  int fan_minimum_gfx;
  // regular fan name
  const char *fan_desc;
  // gfx-card fan name
  const char *gfx_fan_desc;
};

static int __init fan_module_init(void) {
  pr_info("start module job\n");

  return 0;
}

static void __exit fan_module_exit(void) {
  // i2c_unregister_device(client);
  pr_info("end module job\n");
}

module_init(fan_module_init);
module_exit(fan_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rinat");
MODULE_DESCRIPTION("The module for apple mac-book fan");
