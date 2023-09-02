
#include <linux/fs.h>
#include <linux/module.h>

static int cpu_info_print(void) {
  unsigned int cpu = 0;
  struct cpuinfo_x86 *c;

  for_each_online_cpu(cpu) {
    const char *vendor = NULL;
    c = &cpu_data(cpu);
    if (c->x86_vendor < X86_VENDOR_NUM) {
      vendor = "Unknown";
    } else {
      if (c->cpuid_level >= 0)
        vendor = c->x86_vendor_id;
    }

    if (vendor && !strstr(c->x86_model_id, vendor))
      pr_cont("%s ", vendor);

    if (c->x86_model_id[0])
      pr_cont("%s", c->x86_model_id);
    else
      pr_cont("%d86", c->x86);

    pr_cont(" (family: 0x%x, model: 0x%x", c->x86, c->x86_model);

    if (c->x86_stepping || c->cpuid_level >= 0)
      pr_cont(", stepping: 0x%x)\n", c->x86_stepping);
    else
      pr_cont(")\n");
  }
  return 0;
}

static int __init fan_module_init(void) {
  pr_info("start module job\n");
  cpu_info_print();

  return 0;
}

static void __exit fan_module_exit(void) { pr_info("end module job\n"); }

module_init(fan_module_init);
module_exit(fan_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rinat");
MODULE_DESCRIPTION("The module for apple mac-book fan");
