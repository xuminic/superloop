

/* Time-Triggered Co-operative Super-Loop*/

#include <stdio.h>
#include <string.h>

#include "superloop.h"

static	tcb_t		sloop_tcb[CFG_SLP_TASK_MAX];
static	tcb_t		*sloop_now;
static	int		sloop_idx = 0;
static	unsigned long	sloop_ticks = 0;


void sloop_lock(void)
{
}

void sloop_unlock(void)
{
}

/* scheduler tick: called by timer ISR */
void sloop_tick(void)
{
	register int	i;
	
	for (i = 0; i < CFG_SLP_TASK_MAX; i++) {
		if (!sloop_tcb[i].entry) {
			continue;
		}
		if (sloop_tcb[i].flags & SLOOP_STAT_ZOMBIE) {
			continue;
		}
		if (sloop_tcb[i].elaps) {
			sloop_tcb[i].elaps--;
		} else {
			SLOOP_RUNINC(&sloop_tcb[i]);
			if (sloop_tcb[i].period) {
				sloop_tcb[i].elaps = sloop_tcb[i].period - 1;
			} else {
				sloop_tcb[i].flags |= SLOOP_STAT_ZOMBIE;
			}
		}
	}
	sloop_ticks++;
}


/* scheduler dispatch */
void sloop_dispatch(void) 
{
	register tcb_t	*task;
	unsigned long	c_cnt;

	task = &sloop_tcb[sloop_idx++];
	if (sloop_idx >= CFG_SLP_TASK_MAX) {
		sloop_idx = 0;
	}

	sloop_lock();
	if (task->flags & SLOOP_STAT_BLOCK) {
		//printf("task %p blocked\n", task);
	} else if (SLOOP_RUNME(task)) {
		task->p_tcb = sloop_now;
		sloop_now = task;
		c_cnt = sloop_ticks;
		task->flags |= SLOOP_STAT_BLOCK;
		sloop_unlock();

		task->entry(task->extension);
		
		sloop_lock();
		task->flags &= ~SLOOP_STAT_BLOCK;
		SLOOP_RUNDEC(task);
		task->s_count += sloop_ticks - c_cnt + 1;
		
		if (task->flags & SLOOP_STAT_ZOMBIE) {
			sloop_task_kill(task);
		}
		sloop_now = task->p_tcb;
	}
	sloop_unlock();
}


tcb_t *sloop_task_create(task_f task, void *param, int period, int sched, char *name)
{
	int	i;

	for (i = 0; i < CFG_SLP_TASK_MAX; i++) {
		if (sloop_tcb[i].entry == (void*) 0) {
			sloop_tcb[i].entry = task;
			sloop_tcb[i].flags = 0;
			sloop_tcb[i].period = (period + CFG_SLP_TICK_MS - 1) / CFG_SLP_TICK_MS;
			sloop_tcb[i].elaps  = (sched + CFG_SLP_TICK_MS - 1) / CFG_SLP_TICK_MS;

#if	defined(CFG_SLP_TASK_NAME) && (CFG_SLP_TASK_NAME > 0)
			if (name) {
				strncpy(sloop_tcb[i].name, name, CFG_SLP_TASK_NAME-1);
				sloop_tcb[i].name[CFG_SLP_TASK_NAME-1] = 0;
			}
#endif
			sloop_tcb[i].extension = param;
			return &sloop_tcb[i];
		}
	}
	return (void*) 0;
}

void sloop_task_kill(tcb_t *task)
{
	task->entry = (void*) 0;
}

tcb_t *sloop_get_tcb(void)
{
	return sloop_now;
}

unsigned long sloop_get_tick(void)
{	
	return sloop_ticks;
}

