
/**
  ******************************************************************************
  * @file           : superloop.h
  * @brief          : the head file of all
  ******************************************************************************
  */
#ifndef	_SUPERLOOP_H_
#define _SUPERLOOP_H_

#ifndef CFG_SLP_TASK_MAX
#define CFG_SLP_TASK_MAX        4       /* the maximum number of tasks */
#endif
#ifndef	CFG_SLP_TICK_MS
#define CFG_SLP_TICK_MS         1       /* how many milliseconds per timer tick */
#endif
#define CFG_SLP_TASK_NAME       16      /* define task name, or 0=disable */


#define SLOOP_STAT_RUNME	0xff	/* should be enough for Run flag */
#define SLOOP_STAT_BLOCK	0x100	/* task is blocked in case re-entry */
#define SLOOP_STAT_ZOMBIE	0x200	/* task is scheduled for the last run */


#define SLOOP_RUNME(t)		((t)->flags & SLOOP_STAT_RUNME)
#define SLOOP_RUNINC(t)		(SLOOP_RUNME(t) < 255 ? (t)->flags++ : 255)
#define SLOOP_RUNDEC(t)		(SLOOP_RUNME(t) ? (t)->flags-- : 0)


#ifdef __cplusplus
extern "C"
{
#endif

typedef int (*task_f)(void *);

/* Time-Triggered Co-operative Super-Loop Task control block */
typedef	struct	{
	task_f		entry;
	unsigned	flags;
	unsigned	elaps;		/* current tick counting */
	unsigned	period;		/* period of scheduling the task */
	unsigned	s_count;	/* schedule counter */

#if	defined(CFG_SLP_TASK_NAME) && (CFG_SLP_TASK_NAME > 0)
	char		name[CFG_SLP_TASK_NAME];
#endif
	void		*p_tcb;		/* parent's tcb */
	void		*extension;
} tcb_t;


void sloop_init(void *tty);
void sloop_tick(void);
void sloop_dispatch(void);
tcb_t *sloop_task_create(task_f task, void *param, int period, int sched, char *name);
void sloop_task_kill(tcb_t *task);
tcb_t *sloop_get_tcb(void);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif	/* _SUPERLOOP_H_ */








