#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

#define pr_fmt(fmt) KBUILD_MODNAME "->%s:%d:  " fmt, __func__,__LINE__
#define iamhere(x)  pr_info("IAMHERE: " x "\n")

static int pid;
module_param(pid, int, 0);
MODULE_PARM_DESC(pid, "Process PID to freeze");

// Also, check out
// module_param_named, module_param_array, module_param_string
struct timer_list simple_timer;

struct task_struct* dest_task = NULL;

void modify_vruntime(struct timer_list* timer)
{
    struct task_struct* freeze_task = NULL;
    for_each_process(freeze_task)
    {
        if (pid == freeze_task->pid)
        {
            dest_task = freeze_task;
            (freeze_task->se).vruntime = 551615;
            pr_info("sleeper ptr: %p\n", &((freeze_task->se).run_node));
            //pr_info("rootnode ptr %p\n", ((freeze_task->se).cfs_rq->tasks_timeline.rb_root.rb_node));
            pr_info("Param 1: pid %d\n", (freeze_task->pid));
            pr_info("Param 1: vruntime %llu\n", (freeze_task->se).vruntime);
        }
    }
    mod_timer(&simple_timer, jiffies);
}

static int __init lkm_init(void)
{
    pr_info("inserted\n");
    //pr_info("rightest ptr %p\n", rb_last(&((dest_task->se).cfs_rq->tasks_timeline.rb_root)));
    timer_setup(&simple_timer, modify_vruntime, 0);
    mod_timer(&simple_timer, jiffies);
    return 0;
}

static void __exit lkm_exit(void)
{
    pr_info("Wow! You exited early\n");
    del_timer(&simple_timer);
    iamhere("exiting");
}

module_init(lkm_init);
module_exit(lkm_exit);

MODULE_AUTHOR("Alex");
MODULE_DESCRIPTION("A Hello World Module");
MODULE_LICENSE("GPL");
