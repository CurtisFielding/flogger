#include <linux/kernel.h>
#include <linux/module.h>
#include <sys/syscall.h>

exern void *sys_call_table[];

asmlinkage int (og_sys_read)(size_t);

asmlinkage size_t flog_read(int filedes, void *buf, size_t bytes)
{
  printk("it\'s flogging time!\n");
  /* LATER: write buf to whatever file. */
  /*    carry on with original read syscall. */
  return og_sys_read(filedes, buf, bytes);
}

int init_module()
{
  og_sys_read = sys_call_table[__NR_read];
  sys_call_table[__NR_read] = flog_read;
}

void cleanup_module()
{
  printk("done flogging!\n");
  sys_call_table[__NR_read] = og_sys_read;
}
