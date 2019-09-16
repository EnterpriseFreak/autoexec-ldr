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
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

typedef void (*entrypoint) (void);
void *Initialise();

void * Initialise() {

	void *framebuffer;

	VIDEO_Init();
	PAD_Init();
	
	rmode = VIDEO_GetPreferredMode(NULL);

	framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(framebuffer,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	return framebuffer;

}

void loop() //A endless loop
{
	while(1)
	{
		VIDEO_WaitVSync();
	}
}

int main()
{
	__io_gcsdb.startup();
	
	if(!__io_gcsdb.isInserted())
	{
		Initialise();
		iprintf("\nERROR: THERE IS NO SD GECKO INSERTED INTO MEMORY CARD SLOT B.");
		VIDEO_WaitVSync();
		loop();
	}
	
	if(!fatMountSimple("fat", &__io_gcsdb))
	{
		Initialise();
		iprintf("\nERROR: FAILED TO MOUNT THE SD CARD. CONFIRM THAT THE SD CARD IS PROPERLY FORMATED / INSERTED INTO THE SD GECKO.");
		VIDEO_WaitVSync();
		loop();
	}
	
	FILE * autoexecFile = fopen("fat:/autoexec.dol", "rb");
	u32 autoexecSize;
	char * autoexecBuff;
	
	if(!autoexecFile)
	{
		Initialise();
		iprintf("\nERROR: COULD NOT OPEN AUTOEXEC.DOL!");
		VIDEO_WaitVSync();
		loop();
	}
	
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
	