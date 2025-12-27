#ifndef INC_FS_BROKER_H_
#define INC_FS_BROKER_H_



#define FS_O_CREAT			(0x01)
#define FS_O_APPEND			(0x02)
#define FS_O_TRUNC			(0x04)
#define FS_O_READONLY		(0x80)


#define MAX_FS_BROKER_FS (4)
#define MAX_FS_BROKER_FILP (4)



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
int write_f (fs_broker_t* broker, int fd, char* buf, int count);
int delete_f (fs_broker_t* broker, char* name);


int opendir_f (fs_broker_t* broker);
int walkdir_f (fs_broker_t* broker, char** name, int* size);
int closedir_f (fs_broker_t* broker);


#endif /* INC_FS_BROKER_H_ */
