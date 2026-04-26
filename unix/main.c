
#include <string.h> // memcpy
#include <unistd.h> // sbrk
#include <stdio.h> // sprintf
#include "os/hal_plat.h" // HAL
#include "os/commands.h"
#include "os/globals.h"
#include "os/resource.h"
#include "os/parser.h"
#include "os/fatsmall_fs.h"
#include "instances.h"
#include "unixfs_wrapper.h"
#include "os/terminal_input.h"




#define RAMDRIVE_PAGE (64)
#define RAMDRIVE_SIZE (32768)
#define RAMDRIVE_MAX_PAGEADDRESS ((32768) / (RAMDRIVE_PAGE))


int
ramdrive_read_page (blockdevice_t* blockdevice, int pageaddress) {
	if (pageaddress >= RAMDRIVE_MAX_PAGEADDRESS) {
		return 0;
	}
	memcpy(blockdevice->buffer, blockdevice->context + (pageaddress * RAMDRIVE_PAGE), RAMDRIVE_PAGE);
	return RAMDRIVE_PAGE;
}


int
ramdrive_write_page (blockdevice_t* blockdevice, int pageaddress) {
	if (pageaddress >= RAMDRIVE_MAX_PAGEADDRESS) {
		return 0;
	}
	memcpy(blockdevice->context + (pageaddress * RAMDRIVE_PAGE), blockdevice->buffer, RAMDRIVE_PAGE);
	return RAMDRIVE_PAGE;
}


char get_online_char (void) {
    int rc = getc(stdin);
    char c;
    if (rc == EOF) {
        c = 0x04; // Passing Ctrl + D down
        clearerr(stdin); // Clearing EOF
    } else {
        c = (char)rc;
    }
    return c;
}


void put_online_char (char c) {
	putchar(c);
}


int main(void)
{


  blockdevice_t *ramdrive;
  fs_t *ramfs;
  unixfs_wrapper_t unixfs;


  console_printf("");

  scheduler = scheduler_create();
  if (!scheduler) {
	  console_printf("scheduler init error");
	  cpu_halt();
  }

  // Variables, resources
  for (int i = 'z'; i >= 'a'; i--) {
	  char name[2];
	  name [0] = i; name[1] = '\0';
	  resource_add(name, NULL, variable_setter, variable_getter);
  }
  resource_add("rnd", NULL, rnd_setter, rnd_getter);



  void* ramdrvmem = t_malloc(RAMDRIVE_SIZE);
  if (!ramdrvmem) {
  	  console_printf("RAMDRIVE alloc error");
  	  cpu_halt();
    }
  // RAM instance
  ramdrive = blockdevice_create(RAMDRIVE_PAGE, RAMDRIVE_MAX_PAGEADDRESS, ramdrive_read_page, ramdrive_write_page, ramdrvmem);
  if (!ramdrive) {
	  console_printf("RAMDRIVE init error");
	  cpu_halt();
  }

  ramfs = fs_create(ramdrive);
  if (!ramfs) {
  	  console_printf("ramFS init error");
  	  cpu_halt();
  }
  fs_format(ramfs, 16); // Always format the ramdrive at each stratup


  unixfswrapper_init(&unixfs);

  fs = fs_broker_create();
  if (!fs) {
	  console_printf("FSbroker init error");
	  cpu_halt();
  }

  fs_broker_register_fs(fs,
                        ramfs,
						'M',
                        (int (*)(void*, char*, int)) fs_open,
						(void (*) (void*, int)) fs_close,
						(void (*) (void*, int)) fs_rewind,
						(int (*) (void*, int, char*, int)) fs_read,
						(int (*) (void*, int, char*, int)) fs_write,
						(int (*) (void*, char*)) fs_delete,
						(int (*) (void*)) fs_opendir,
						(int (*) (void*, char**, int*)) fs_walkdir,
						(int (*) (void*)) fs_closedir);

  fs_broker_register_fs(fs,
                        &unixfs,
						'U',
                        (int (*)(void*, char*, int)) unixfswrapper_open,
						(void (*) (void*, int)) unixfswrapper_close,
						(void (*) (void*, int)) unixfswrapper_rewind,
						(int (*) (void*, int, char*, int)) unixfswrapper_read,
						(int (*) (void*, int, char*, int)) unixfswrapper_write,
						(int (*) (void*, char*)) unixfswrapper_delete,
						(int (*) (void*)) unixfswrapper_opendir,
						(int (*) (void*, char**, int*)) unixfswrapper_walkdir,
						(int (*) (void*)) unixfswrapper_closedir);


  program = program_create(20, 80); // 20 lines, 80 characters each -> 1.6k max program size
  if (!program) {
  	  console_printf("program init error");
  	  cpu_halt();
  }


  online_reader = line_reader_create(program->header.fields.linelen - 2, terminal_input_create(get_online_char, put_online_char, 1));
  if (!online_reader) {
  	  console_printf("input init error");
  	  cpu_halt();
  }

  generic_fs_t *devfs = generic_fs_create();
  if (!ramfs) {
	  console_printf("devfs init error");
	  cpu_halt();
  }

  generic_file_t *nullfile = generic_file_create ("null", NULL,
													  nullfile_open,
													  nullfile_close,
													  nullfile_read,
													  nullfile_write);

  if (generic_fs_register_file(devfs, nullfile) < 0) {
	  console_printf("file reg error");
	  cpu_halt();
  }

  generic_file_t *confile = generic_file_create ("con", online_reader,
													  nullfile_open,
													  nullfile_close,
													  consolefile_read_raw,
													  consolefile_write);

  if (generic_fs_register_file(devfs, confile) < 0) {
	  console_printf("file reg error");
	  cpu_halt();
  }

  fs_broker_register_fs(fs,
		  	  	  	    devfs,
						'D',
		  	  	  	    (int (*)(void*, char*, int)) generic_fs_open,
						(void (*) (void*, int)) generic_fs_close,
						(void (*) (void*, int)) generic_fs_rewind,
						(int (*) (void*, int, char*, int)) generic_fs_read,
						(int (*) (void*, int, char*, int)) generic_fs_write,
						(int (*) (void*, char*)) generic_fs_delete,
						(int (*) (void*)) generic_fs_opendir,
						(int (*) (void*, char**, int*)) generic_fs_walkdir,
						(int (*) (void*)) generic_fs_closedir);


  setup_commands();
  setup_persona_commands();


  program_ip = 0;
  program_run = 0;
  subroutine_sp = 0;

  int fcon = open_f(fs, "D:con", 0);
  if (fcon < 0) {
	  console_printf("conio file error");
	  cpu_halt();
  }

  iostack = NULL;
  stdiostack_push(&iostack, fcon, fcon, fcon);

  while (1)
  {
	  command_line_loop();
	  printf_f(STDERR, "respawning..\n");
  }

}

