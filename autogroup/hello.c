#define pr_fmt(fmt)	KBUILD_MODNAME "->%s:%d:  " fmt, __func__,__LINE__
#define iamhere(x)	pr_info("IAMHERE: " x "\n")


#include<linux/cgroup-defs.h>
#include<linux/jiffies.h>
#include<linux/timer.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/sched.h>
#include<linux/sched/signal.h>

static int pid;
module_param(pid, int, 0);
MODULE_PARM_DESC(pid, "Process PID to freeze");


struct cfs_bandwidth {
#ifdef CONFIG_CFS_BANDWIDTH
	raw_spinlock_t		lock;
	ktime_t			period;
	u64			quota;
	u64			runtime;
	s64			hierarchical_quota;

	u8			idle;
	u8			period_active;
	u8			slack_started;
	struct hrtimer		period_timer;
	struct hrtimer		slack_timer;
	struct list_head	throttled_cfs_rq;

	/* Statistics: */
	int			nr_periods;
	int			nr_throttled;
	u64			throttled_time;
#endif
};

struct task_group {
	struct cgroup_subsys_state css;

#ifdef CONFIG_FAIR_GROUP_SCHED
	/* schedulable entities of this group on each CPU */
	struct sched_entity** se;
	/* runqueue "owned" by this group on each CPU */
	struct cfs_rq** cfs_rq;
	unsigned long		shares;

#ifdef	CONFIG_SMP
	/*
	 * load_avg can be heavily contended at clock tick time, so put
	 * it in its own cacheline separated from the fields above which
	 * will also be accessed at each tick.
	 */
	atomic_long_t		load_avg ____cacheline_aligned;
#endif
#endif

#ifdef CONFIG_RT_GROUP_SCHED
	struct sched_rt_entity** rt_se;
	struct rt_rq** rt_rq;

	struct rt_bandwidth	rt_bandwidth;
#endif

	struct rcu_head		rcu;
	struct list_head	list;

	struct task_group* parent;
	struct list_head	siblings;
	struct list_head	children;

#ifdef CONFIG_SCHED_AUTOGROUP
	struct autogroup* autogroup;
#endif

	struct cfs_bandwidth	cfs_bandwidth;

#ifdef CONFIG_UCLAMP_TASK_GROUP
	/* The two decimal precision [%] value requested from user-space */
	unsigned int		uclamp_pct[UCLAMP_CNT];
	/* Clamp values requested for a task group */
	struct uclamp_se	uclamp_req[UCLAMP_CNT];
	/* Effective clamp values used for a task group */
	struct uclamp_se	uclamp[UCLAMP_CNT];
#endif

};


struct autogroup {
	/*
	 * Reference doesn't mean how many threads attach to this
	 * autogroup now. It just stands for the number of tasks
	 * which could use this autogroup.
	 */
	struct kref		kref;
	struct task_group* tg;
	struct rw_semaphore	lock;
	unsigned long		id;
	int			nice;
};


// Also, check out
// module_param_named, module_param_array, module_param_string
struct timer_list simple_timer;

struct task_struct* dest_task = NULL;

void modify_vruntime(struct timer_list* timer)
{
	struct task_struct* freeze_task = NULL;
	for_each_process(freeze_task) {
		if (pid == freeze_task->pid)
		{
			int flags;
			smp_wmb();
			raw_spin_lock_irqsave(&freeze_task->pi_lock, flags);
			dest_task = freeze_task;
			(freeze_task->se).vruntime = 1984674407370955161;
			pr_info("sleeper ptr: %p\n", &((freeze_task->se).run_node));
			//pr_info("rootnode ptr %p\n", ((freeze_task->se).cfs_rq->tasks_timeline.rb_root.rb_node));
			pr_info("Param 1: on_rq %d\n", (freeze_task->sched_task_group->autogroup->id));
			pr_info("Param 1: pid %d\n", (freeze_task->pid));
			//pr_info("on rq: %d\n" (freeze_task->se).on_rq);
			pr_info("Param 1: vruntime %llu\n", (freeze_task->se).vruntime);
			raw_spin_unlock_irqrestore(&freeze_task->pi_lock, flags);
		}
	}
	mod_timer(&simple_timer, jiffies);
}


static int __init lkm_init(void) {
	pr_info("inserted\n");
	//pr_info("rightest ptr %p\n", rb_last(&((dest_task->se).cfs_rq->tasks_timeline.rb_root)));
	timer_setup(&simple_timer, modify_vruntime, 0);
	mod_timer(&simple_timer, jiffies);

	return 0;

}


static void __exit lkm_exit(void) {

	pr_info("Wow! You exited early\n");
	del_timer(&simple_timer);
	iamhere("exiting");
}

module_init(lkm_init);
module_exit(lkm_exit);




MODULE_AUTHOR("Alex");
MODULE_DESCRIPTION("A Hello World Module");
MODULE_LICENSE("GPL");
