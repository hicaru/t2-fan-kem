
#include <linux/fs.h>
#include <linux/module.h>

#include <linux/acpi.h>
#include <linux/device.h>
#include <linux/dmi.h>
#include <linux/platform_device.h>

#include <linux/hwmon-sysfs.h>
#include <linux/hwmon.h>

#define DRIVER_NAME "apple_fan"
#define apple_FAN_VERSION "#MODULE_VERSION#"

#define TEMP1_CRIT 105
#define TEMP1_LABEL "gfx_temp"
#define DEBUG true

#define dbg_msg(fmt, ...)                                                      \
  do {                                                                         \
    if (DEBUG)                                                                 \
      printk(KERN_INFO "apple-fan (debug) - " fmt "\n", ##__VA_ARGS__);        \
  } while (0)

#define info_msg(title, fmt, ...)                                              \
  do {                                                                         \
    printk(KERN_INFO "apple-fan (" title ") - " fmt "\n", ##__VA_ARGS__);      \
  } while (0)

#define err_msg(title, fmt, ...)                                               \
  do {                                                                         \
    printk(KERN_ERR "apple-fan (" title ") - " fmt "\n", ##__VA_ARGS__);       \
  } while (0)

#define warn_msg(title, fmt, ...)                                              \
  do {                                                                         \
    printk(KERN_WARNING "apple-fan (" title ") - " fmt "\n", ##__VA_ARGS__);   \
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

/*
 *  GLOBALS.........
 * */

static struct apple_fan_data apple_data = {
    NULL, {-1, -1}, {false, false}, false,    false, 255, 255,
    10,   10,       "CPU Fan",      "GFX Fan"};

const static char *fan_mode_manual_string = "manual";
const static char *fan_mode_auto_string = "auto";

// params struct used frequently for acpi-call-construction
static struct acpi_object_list params;
// force loading i.e., skip device existance check
static short force_load = false;
// allow checking but override rpm check
static short force_rpm_override = false;

// housekeeping structs
static struct apple_fan_driver apple_fan_driver = {
    .name = DRIVER_NAME,
    .owner = THIS_MODULE,
};

bool used;

static struct attribute *platform_attributes[] = {NULL};
static struct attribute_group platform_attribute_group = {
    .attrs = platform_attributes};

// hidden fan api funcs used for both (wrap into them)
static int __fan_get_cur_state(int fan, unsigned long *state);
static int __fan_set_cur_state(int fan, unsigned long state);

// get current mode (auto, manual, perhaps auto mode of module in future)
static int __fan_get_cur_control_state(int fan, int *state);
// switch between modes (auto, manual, perhaps auto mode of module in future)
static int __fan_set_cur_control_state(int fan, int state);

// regular fan api funcs
static ssize_t fan_get_cur_state(struct device *dev,
                                 struct device_attribute *attr, char *buf);
static ssize_t fan_set_cur_state(struct device *dev,
                                 struct device_attribute *attr, const char *buf,
                                 size_t count);

// gfx fan api funcs
static ssize_t fan_get_cur_state_gfx(struct device *dev,
                                     struct device_attribute *attr, char *buf);
static ssize_t fan_set_cur_state_gfx(struct device *dev,
                                     struct device_attribute *attr,
                                     const char *buf, size_t count);

// regular fan api funcs
static ssize_t fan_get_cur_control_state(struct device *dev,
                                         struct device_attribute *attr,
                                         char *buf);
static ssize_t fan_set_cur_control_state(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t count);

// gfx fan api funcs
static ssize_t fan_get_cur_control_state_gfx(struct device *dev,
                                             struct device_attribute *attr,
                                             char *buf);
static ssize_t fan_set_cur_control_state_gfx(struct device *dev,
                                             struct device_attribute *attr,
                                             const char *buf, size_t count);

static ssize_t _fan_set_mode(int fan, const char *buf, size_t count);

// gfx fan api funcs
static ssize_t fan1_get_mode(struct device *dev, struct device_attribute *attr,
                             char *buf);
static ssize_t fan1_set_mode(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t count);

// gfx fan api funcs
static ssize_t fan2_get_mode(struct device *dev, struct device_attribute *attr,
                             char *buf);
static ssize_t fan2_set_mode(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t count);

// generic fan func (no sense as long as auto-mode is bound to both or none of
// the fans...
// - force 'reset' of max-speed (if reset == true) and change to auto-mode
static int fan_set_max_speed(unsigned long state, bool reset);
// acpi-readout
static int fan_get_max_speed(unsigned long *state);

// set fan(s) to automatic mode
static int fan_set_auto(void);

// set fan with index 'fan' to 'speed'
// - includes manual mode activation
static int fan_set_speed(int fan, int speed);

// reports current speed of the fan (unit:RPM)
static int __fan_rpm(int fan);

// Writes RPMs of fan0 (CPU fan) to buf => needed for hwmon device
static ssize_t fan_rpm(struct device *dev, struct device_attribute *attr,
                       char *buf);

// Writes RPMs of fan1 (GPU fan) to buf => needed for hwmon device
static ssize_t fan_rpm_gfx(struct device *dev, struct device_attribute *attr,
                           char *buf);
// Writes Label of fan0 (CPU fan) to buf => needed for hwmon device
static ssize_t fan_label(struct device *dev, struct device_attribute *attr,
                         char *buf);

// Writes Label of fan1 (GPU fan) to buf => needed for hwmon device
static ssize_t fan_label_gfx(struct device *dev, struct device_attribute *attr,
                             char *buf);
// Writes Minimal speed of fan0 (CPU fan) to buf => needed for hwmon device
static ssize_t fan_min(struct device *dev, struct device_attribute *attr,
                       char *buf);

// Writes Minimal speed of fan1 (GPU fan) to buf => needed for hwmon device
static ssize_t fan_min_gfx(struct device *dev, struct device_attribute *attr,
                           char *buf);

// sets maximal speed for auto and manual mode => needed for hwmon device
static ssize_t set_max_speed(struct device *dev, struct device_attribute *attr,
                             const char *buf, size_t count);

// writes maximal speed for auto and manual mode to buf => needed for hwmon
// device
static ssize_t get_max_speed(struct device *dev, struct device_attribute *attr,
                             char *buf);

// GFX temperature
static ssize_t temp1_input(struct device *dev, struct device_attribute *attr,
                           char *buf);
// GFX label
static ssize_t temp1_crit(struct device *dev, struct device_attribute *attr,
                          char *buf);
// GFX crit
static ssize_t temp1_label(struct device *dev, struct device_attribute *attr,
                           char *buf);

// is the hwmon interface visible?
static umode_t apple_hwmon_sysfs_is_visible(struct kobject *kobj,
                                            struct attribute *attr, int idx);

// initialization of hwmon interface
static int apple_fan_hwmon_init(struct asus_fan *asus);

// remove "apple_fan" subfolder from /sys/devices/platform
static void apple_fan_sysfs_exit(struct platform_device *device);

// set up platform device and call hwmon init
static int apple_fan_probe(struct platform_device *pdev);

// do anything needed to remove platform device
static int apple_fan_remove(struct platform_device *device);

// prepare platform device and let it create
int __init_or_module apple_fan_register_driver(struct asus_fan_driver *driver);

// remove the driver
void apple_fan_unregister_driver(struct asus_fan_driver *driver);

// housekeeping (module) stuff...
static void __exit fan_exit(void);
static int __init fan_init(void);

// ----------------------IMPLEMENTATIONS-------------------------- //

static int __fan_get_cur_state(int fan, unsigned long *state) {
  // RPM*RPM*0,0000095+0,01028*RPM+26,5

  int rpm = __fan_rpm(fan);

  dbg_msg("fan-id: %d | get RPM", fan);

  if (apple_data.fan_manual_mode[fan]) {
    *state = apple_data.fan_states[fan];
  } else {
    if (rpm == 0) {
      *state = 0;
      return 0;
    }

    *state = rpm * rpm * 100 / 10526316 + rpm * 1000 / 97276 + 26;
    // ensure state is within a valid range
    if (*state > 255) {
      *state = 0;
    }
  }
  return 0;
}

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
