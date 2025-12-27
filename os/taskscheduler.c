#include "taskscheduler.h"
#include "hal_plat.h" //malloc


void scheduler_remove_current (taskscheduler_t* instance) {
	task_t* task = *(instance->cpp);
	if (!task) {
		return;
	}
	task->cleanup(task->context);
	*(instance->cpp) = (*(instance->cpp))->next;

	t_free(task);
}


taskscheduler_t* scheduler_create (void) {
	taskscheduler_t* instance;

	instance = (taskscheduler_t*)t_malloc(sizeof(taskscheduler_t));
	if (!instance) {
		return instance;
	}
	instance->head = NULL;
	instance->cpp = &(instance->head);
	return instance;
}


void scheduler_destroy (taskscheduler_t* instance) {
	scheduler_killall(instance);
	t_free(instance);
}


task_t* scheduler_install_task (taskscheduler_t* instance, task_rc_t (*taskfn) (void*), void (*cleanup) (void*), void* context) {
	task_t* task = (task_t*)t_malloc(sizeof(task_t));
	if (!task) {
		return task;
	}
	task->next = instance->head;
	instance->head = task;
	task->taskfn = taskfn;
	task->cleanup = cleanup;
	task->context = context;
	return task;
}


int scheduler_burst_run (taskscheduler_t* instance) {
	int rc = 0;
	for (instance->cpp = &(instance->head); *(instance->cpp); instance->cpp = &(*(instance->cpp))->next) {
		task_rc_t trc;
		rc = 1;
		do {
			trc = (*(instance->cpp))->taskfn((*(instance->cpp))->context);
		} while (trc == TASK_RC_AGAIN);
		if (trc == TASK_RC_END) {
			scheduler_remove_current(instance);
			break;
		}
	}
	return rc;
}


void scheduler_killall (taskscheduler_t* instance) {
	instance->cpp = &(instance->head);
	while (*(instance->cpp)) {
		scheduler_remove_current(instance);
	}
}
