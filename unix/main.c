
#include <string.h> // memcpy
#include <unistd.h> // sbrk
#include <stdio.h> // sprintf
#include "os/hal_plat.h" // HAL
#include "os/commands.h"
#include "os/globals.h"
#include "os/resource.h"
#include "os/parser.h"




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
	char c = (char) getchar();
	return c;
}


int main(void)
{


  blockdevice_t *ramdrive;
  fs_t *ramfs;


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

  setup_commands();

  program = program_create(20, 80); // 20 lines, 80 characters each -> 1.6k max program size
  if (!program) {
  	  console_printf("program init error");
  	  cpu_halt();
  }

  ledflash(2);



  program_ip = 0;
  program_run = 0;
  subroutine_sp = 0;



  online_input = terminal_input_create(get_online_char, program->header.fields.linelen - 2);
  if (!online_input) {
  	  console_printf("input init error");
  	  cpu_halt();
  }


  while (1)
  {
	  int chunks = t_chunks();
	  // Online command parser
	  parser_t* online_parser = parser_create(program->header.fields.linelen); // align to the program line length
	  if (!online_parser) {
		  console_printf("parser init error");
		  cpu_halt();
	  }
	  int prompt_pidx = 0;
	  char prompt[8];
	  sprintf(&(prompt[prompt_pidx]), "%c:> ", get_current_fs(fs));
	  // Interpreting and executing commands, till the eternity
	  cmd_line_parser(online_parser, terminal_get_line(online_input, prompt, 0), NULL, NULL);
	  parser_destroy(online_parser);

	  chunks -= t_chunks();
	  if (chunks) { // simple memory leak check, normally shouldn't ever happen
		  console_printf("t_malloc-t_free imbalance :%i", chunks);
	  }


  }

}

