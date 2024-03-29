#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>

void error(char *errorMsg)
{
    static GXRModeObj *rmode = NULL;
    void *framebuffer;

	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);

	framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(framebuffer,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if(rmode->viTVMode&VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    iprintf("\nERROR: %s\n\nPress the RESET button to restart the console.", errorMsg);

    for(;;)
    {
        VIDEO_WaitVSync();
		
		if(SYS_ResetButtonDown())
		{
			SYS_ResetSystem(SYS_RESTART, 0, true);
			exit(0);
		}
    }
}
