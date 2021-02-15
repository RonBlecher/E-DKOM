#include<linux/jiffies.h>
#include<linux/timer.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/sched.h>
#include<linux/cpumask.h>
#include<linux/delay.h>
#include<linux/kthread.h>
#include<linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
#include <linux/signal.h>
#else
#include <linux/sched/signal.h>
#endif

#define pr_fmt(fmt)	KBUILD_MODNAME "->%s:%d:  " fmt, __func__,__LINE__
#define iamhere(x)	pr_info("IAMHERE: " x "\n")

static int pid;
module_param(pid, int, 0);
MODULE_PARM_DESC(pid, "Process PID to freeze");

struct timer_list simple_timer;
struct task_struct* dest_task = NULL;
struct task_struct* cpu_kthreads[100];
struct task_struct* cpu_kthreads2[100];
u64 vruntime_before = 0;

int dummy_kthread_func(void* data) {
    bool freeze;
    while (!kthread_freezable_should_stop(&freeze))
    {
    }
    printk(KERN_INFO "Thread Stopping\n");
    do_exit(0);
    return 0;
}

u64 max_vruntime(void) {
    u64 max = 0;
    struct task_struct* task = NULL;
    struct task_struct* max_task = NULL;
    for_each_process(task) {
        if ((task->pid != dest_task->pid) && (((task->se).vruntime) >= max) && (((task->se).vruntime) < 1844674407370655161))
        {
            max = ((task->se).vruntime);
            max_task = task;
        }
    }
    pr_info("max task : %d", max_task->pid);
    pr_info("max : %llu", (max_task->se).vruntime);

    //if(max_task->pid == dest_task->pid)
    //	return -1;
    //else
    return max;
}

void modify_vruntime(struct timer_list* timer)
{
    int flags;
    smp_wmb();
    raw_spin_lock_irqsave(&dest_task->pi_lock, flags);
    u64 max = 0;
    max = max_vruntime();
    //if((max = max_vruntime()) != -1)
    //{
    (dest_task->se).vruntime = max + 10000000000;
    //(dest_task->se).vruntime = 1844674407370655160;
    //}
    raw_spin_unlock_irqrestore(&dest_task->pi_lock, flags);
    mod_timer(&simple_timer, jiffies);
}

static int __init lkm_init(void) {
    int i, cpu;
    pr_info("inserted\n");
    struct task_struct* freeze_task = NULL;
    for_each_process(freeze_task) {
        if (pid == freeze_task->pid)
        {
            int flags;
            smp_wmb();
            raw_spin_lock_irqsave(&freeze_task->pi_lock, flags);
            dest_task = freeze_task;
            raw_spin_unlock_irqrestore(&freeze_task->pi_lock, flags);
        }
    }
    vruntime_before = ((dest_task->se).vruntime);

    // Creating kthread that are bound to cpu
    for (i = 0; i < num_online_cpus(); i++) {
        cpu_kthreads[i] = kthread_create(dummy_kthread_func, NULL, "dummy_kthread");
        cpu_kthreads2[i] = kthread_create(dummy_kthread_func, NULL, "dummy_kthread2");
        //cpu_kthreads[i]->sched_task_group->autogroup = dest_task->sched_task_group->autogroup;
        //cpu_kthreads2[i]->sched_task_group->autogroup = dest_task->sched_task_group->autogroup;
        pr_info("1");
        kthread_bind(cpu_kthreads[i], i);
        kthread_bind(cpu_kthreads2[i], i);
        pr_info("2");
        if (cpu_kthreads[i]) {
            pr_info("3");
            wake_up_process(cpu_kthreads[i]);
            wake_up_process(cpu_kthreads2[i]);
            pr_info("4");
        }
        else
        {
            pr_info("Canno't create process on CPU: %d", i);
        }
    }
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
    setup_timer(&simple_timer, modify_vruntime, 0);
#else
    timer_setup(&simple_timer, modify_vruntime, 0);
#endif
    mod_timer(&simple_timer, jiffies);

    return 0;
}

static void __exit lkm_exit(void) {
    int i;
    del_timer(&simple_timer);
    ((dest_task->se).vruntime) = vruntime_before;
    //for(i = 0; i<num_online_cpus(); i++){
    //	kthread_stop(cpu_kthreads[i]);
    //}
    iamhere("exiting");
}

module_init(lkm_init);
module_exit(lkm_exit);

MODULE_AUTHOR("Alex");
MODULE_DESCRIPTION("Freeze a process");
MODULE_LICENSE("GPL");
