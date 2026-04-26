#ifndef INC_FS_BROKER_H_
#define INC_FS_BROKER_H_


typedef struct stdio_stack_s {
	int fin;
	int fout;
	int ferr;
	struct stdio_stack_s *next;
} stdio_stack_t;

void stdiostack_push (stdio_stack_t **head, int fin, int fout, int ferr);
void stdiostack_pop (stdio_stack_t **head);

/* ------------------------------- */


typedef struct generic_file_s {
	char name[12];
	void* context;
	int (*open) (struct generic_file_s*, int);
	void (*close) (struct generic_file_s*);
	int (*read) (struct generic_file_s*, int, char*);
	int (*write) (struct generic_file_s*, int, char*);
} generic_file_t;



generic_file_t * generic_file_create (char* name,
									  void *context,
										int (*open) (struct generic_file_s*, int),
										void (*close) (struct generic_file_s*),
										int (*read) (struct generic_file_s*, int, char*),
										int (*write) (struct generic_file_s*, int, char*) );

typedef struct generic_fs_filp_s {
	int	file;
	int reserved;
} generic_fs_filp_t;

#define MAX_GENERIC_FS_FILES (8)
#define MAX_GENERIC_FS_FILP (8)

typedef struct generic_fs_s {
	generic_file_t *file[MAX_GENERIC_FS_FILES];
	generic_fs_filp_t fp[MAX_GENERIC_FS_FILP];
	int dirp;
} generic_fs_t;

generic_fs_t * generic_fs_create (void);
int generic_fs_register_file (generic_fs_t* instance, generic_file_t* file);

int generic_fs_open (generic_fs_t* instance, char* name, int flags);
void generic_fs_close (generic_fs_t* instance, int fd);
void generic_fs_rewind (generic_fs_t* instance, int fd);
int generic_fs_write (generic_fs_t* instance, int fd, void* buf, int count);
int generic_fs_read (generic_fs_t* instance, int fd, void* buf, int count);
int generic_fs_delete (generic_fs_t* instance, char* name);

int generic_fs_opendir (generic_fs_t* instance);
int generic_fs_walkdir (generic_fs_t* instance, char** name, int* size);
int generic_fs_closedir (generic_fs_t* instance);

/* ------------------------------- */

#define FS_O_CREAT			(0x01)
#define FS_O_APPEND			(0x02)
#define FS_O_TRUNC			(0x04)
#define FS_O_READONLY		(0x80)


#define MAX_FS_BROKER_FS (4)
#define MAX_FS_BROKER_FILP (8)


typedef struct fs_instance_s {
	void* instance;
	char  letter;
	int (*open) (void*, char*, int);
	void (*close) (void*, int);
	void (*rewind) (void*, int);
	int (*read) (void*, int, char*, int);
	int (*write) (void*, int, char*, int);
	int (*delete) (void*, char*);
    int (*opendir) (void*);
    int (*walkdir) (void*, char**, int*);
    int (*closedir) (void*);

} fs_instance_t;


typedef struct fs_broker_filp_s {
	int	fs_instance_idx;
	int fs_instance_filp;
	int reserved;
} fs_broker_filp_t;


typedef struct fs_broker_s {
	fs_instance_t fs_instance[MAX_FS_BROKER_FS];
	fs_broker_filp_t fp[MAX_FS_BROKER_FILP];
	int current_fs;
} fs_broker_t;


fs_broker_t* fs_broker_create (void);
void fs_broker_destroy (fs_broker_t* instance);

int fs_broker_register_fs (fs_broker_t* instance,
						   void* fs,
						   char letter,
						   int (*open) (void*, char*, int),
						   void (*close) (void*, int),
						   void (*rewind) (void*, int),
						   int (*read) (void*, int, char*, int),
						   int (*write) (void*, int, char*, int),
						   int (*delete) (void*, char*),


						   int (*opendir) (void*),
						   int (*walkdir) (void*, char**, int*),
						   int (*closedir) (void*)
						   );


char get_current_fs (fs_broker_t* broker);
int change_current_fs (fs_broker_t* broker, char letter);


int open_f (fs_broker_t* broker, char* name, int flags);
void close_f (fs_broker_t* broker, int fd);
void rewind_f (fs_broker_t* broker, int fd);
int read_f (fs_broker_t* broker, int fd, char* buf, int count);
int read_f_all (fs_broker_t* broker, int fd, char* buf, int count);
int write_f (fs_broker_t* broker, int fd, char* buf, int count);
int write_f_all (fs_broker_t* broker, int fd, char* buf, int count);
int delete_f (fs_broker_t* broker, char* name);


int opendir_f (fs_broker_t* broker);
int walkdir_f (fs_broker_t* broker, char** name, int* size);
int closedir_f (fs_broker_t* broker);


#endif /* INC_FS_BROKER_H_ */
