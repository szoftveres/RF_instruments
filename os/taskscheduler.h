#ifndef INC_TASKSCHEDULER_H_
#define INC_TASKSCHEDULER_H_

typedef enum task_rc_e {
	TASK_RC_AGAIN,
	TASK_RC_YIELD,
	TASK_RC_END
} task_rc_t;


typedef struct task_s {
	void* context;
	task_rc_t (*taskfn) (void*);
	void (*cleanup) (void*);
	struct task_s* next;
} task_t;


typedef struct taskscheduler_s {
	task_t *head;
	task_t **cpp;
} taskscheduler_t;


taskscheduler_t* scheduler_create (void);
void scheduler_destroy (taskscheduler_t* instance);

task_t* scheduler_install_task (taskscheduler_t* instance, task_rc_t (*taskfn) (void*), void (*cleanup) (void*), void* context);
int scheduler_burst_run (taskscheduler_t* instance);
void scheduler_killall (taskscheduler_t* instance);

#endif /* INC_TASKSCHEDULER_H_ */
