#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/spinlock_types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <asm/uaccess.h>

#define LOOPS 100000

static int list[LOOPS];
static int idx = 0, cs1 = 0, cs2 = 0;
static struct task_struct *t1, *t2;
static spinlock_t spinlock;

static int consumer(void* ptr)
{
	unsigned long flags = 0;
	printk(KERN_INFO "Consumer TID %d\n", (int)ptr);

	while (1) 
	{
		spin_lock_irqsave(&spinlock, flags);
		if (idx >= LOOPS)
		{
			spin_unlock_irqrestore(&spinlock, flags);
			break;
		}
		list[idx++] += 1;
		spin_unlock_irqrestore(&spinlock, flags);
		if ((int)ptr == 1)
			cs1++;
		else
			cs2++;
	}
	printk(KERN_INFO "Consumer %d done\n", (int)ptr);
	return 0;
}

static int spinlock_init(void)
{
	int i, id1 = 1, id2 = 2, lo_cnt = 0, hi_cnt = 0;
	for (i = 0; i < LOOPS; i++)
		list[i] = 0;

	spin_lock_init(&spinlock);

	t1 = kthread_create(consumer, (void*)id1, "cons1");
	t2 = kthread_create(consumer, (void*)id2, "cons2");

	if (t1 && t2) {
		printk(KERN_INFO "Starting...\n");
		wake_up_process(t1);
		wake_up_process(t2);
	} else {
		printk(KERN_EMERG "Error\n");
	}

	msleep(100);

	for (i = 0; i < LOOPS; i++) {
		if (list[i] == 0) {
			lo_cnt++;
			printk(KERN_INFO "lo:%d ", i);
		} else if (list[i] > 1) {
			hi_cnt++;
			printk(KERN_INFO "hi: %d ", i);
		}
	}
	printk(KERN_INFO "lo_cnt = %d hi_cnt = %d cs1 = %d cs2 = %d\n", lo_cnt, hi_cnt, cs1, cs2);
	return 0;
}

void spinlock_cleanup(void)
{
	printk(KERN_INFO "Inside cleanup_module\n");
}

MODULE_LICENSE("GPL");
module_init(spinlock_init);
module_exit(spinlock_cleanup);

