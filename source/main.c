#include <stdio.h>
#include <gccore.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>

#include <fat.h>
#include <sdcard/gcsd.h>

typedef struct _dolheader {
	u32 text_pos[7];
	u32 data_pos[11];
	u32 text_start[7];
	u32 data_start[11];
	u32 text_size[7];
	u32 data_size[11];
	u32 bss_start;
	u32 bss_size;
	u32 entry_point;
} dolheader;

typedef void (*entrypoint) (void);

void loop()
{
	while(1)
	{
		/**/
	}
}

int main()
{
	__io_gcsdb.startup();
	
	if(!__io_gcsdb.isInserted())
	{
		loop();
	}
	
	if(!fatMountSimple("fat", &__io_gcsdb))
	{
		loop();
	}
	
	FILE * autoexecFile = fopen("fat:/autoexec.dol", "rb");
	u32 autoexecSize;
	char * autoexecBuff;
	
	fseek(autoexecFile, 0, SEEK_END);
	autoexecSize = ftell(autoexecFile);
	fseek(autoexecFile, 0, SEEK_SET);
	
	autoexecBuff = (char*) malloc(sizeof(char)*autoexecSize);
	
	fread(autoexecBuff, 1, autoexecSize, autoexecFile);
	fclose(autoexecFile);
	
	u32 i;
	dolheader *autoexecDol = (dolheader*)autoexecBuff;
	
	for(i = 0; i < 7; i++)
	{
		if ((!autoexecDol -> text_size[i]) || (autoexecDol -> text_start[i] < 0x100))
			continue;
		
		size_t size = autoexecDol -> text_size[i];
		u8 *buf = (u8*)autoexecDol -> text_start[i];
		memcpy(buf, autoexecBuff + autoexecDol -> text_pos[i], size);
		DCFlushRange(buf, size);
		ICInvalidateRange(buf, size);
	}
	
	for(i = 0; i < 11; i++)
	{
		if ((!autoexecDol -> data_size[i]) || (autoexecDol -> data_start[i] < 0x100))
			continue;
		
		size_t size = autoexecDol -> data_size[i];
		u8 *buf = (u8*)autoexecDol -> data_start[i];
		memcpy(buf, autoexecBuff + autoexecDol -> data_pos[i], size);
		DCFlushRange(buf, size);
		ICInvalidateRange(buf, size);
	}
	
	if (autoexecBuff != NULL)
		free(autoexecBuff);
	
	SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
	entrypoint entry = (entrypoint)autoexecDol -> entry_point;
	entry();
	
	loop();
	
	return 0;
}
	