/*----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#------  This File is Part Of : ----------------------------------------------------------------------------------------#
#------- _  -------------------  ______   _   --------------------------------------------------------------------------#
#------ | | ------------------- (_____ \ | |  --------------------------------------------------------------------------#
#------ | | ---  _   _   ____    _____) )| |  ____  _   _   ____   ____   ----------------------------------------------#
#------ | | --- | | | | / _  |  |  ____/ | | / _  || | | | / _  ) / ___)  ----------------------------------------------#
#------ | |_____| |_| |( ( | |  | |      | |( ( | || |_| |( (/ / | |  --------------------------------------------------#
#------ |_______)\____| \_||_|  |_|      |_| \_||_| \__  | \____)|_|  --------------------------------------------------#
#------------------------------------------------- (____/  -------------------------------------------------------------#
#------------------------   ______   _   -------------------------------------------------------------------------------#
#------------------------  (_____ \ | |  -------------------------------------------------------------------------------#
#------------------------   _____) )| | _   _   ___   ------------------------------------------------------------------#
#------------------------  |  ____/ | || | | | /___)  ------------------------------------------------------------------#
#------------------------  | |      | || |_| ||___ |  ------------------------------------------------------------------#
#------------------------  |_|      |_| \____|(___/   ------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Licensed under the GPL License --------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Copyright (c) Nanni <lpp.nanni@gmail.com> ---------------------------------------------------------------------------#
#- Copyright (c) Rinnegatamante <rinnegatamante@gmail.com> -------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Credits : -----------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Smealum for ctrulib -------------------------------------------------------------------------------------------------#
#- StapleButter for debug font -----------------------------------------------------------------------------------------#
#- Lode Vandevenne for lodepng -----------------------------------------------------------------------------------------#
#- Jean-loup Gailly and Mark Adler for zlib ----------------------------------------------------------------------------#
#- Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations -------------------#
#-----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include "include/luaAudio.h"

//ADPCM Encoding support from latest libctru revision
void CSND_SetAdpcmState(u32 channel, int block, int sample, int index){
	u32 cmdparams[0x18>>2];
	memset(cmdparams, 0, 0x18);
	cmdparams[0] = channel & 0x1f;
	cmdparams[1] = sample & 0xFFFF;
	cmdparams[2] = index & 0x7F;
	CSND_writesharedmem_cmdtype0(block ? 0xC : 0xB, (u8*)&cmdparams);
}
void CSND_ChnSetAdpcmReload(u32 channel, bool reload){
	u32 cmdparams[0x18>>2];
	memset(cmdparams, 0, 0x18);
	cmdparams[0] = channel & 0x1f;
	cmdparams[1] = reload ? 1 : 0;
	CSND_writesharedmem_cmdtype0(0xD, (u8*)&cmdparams);
}

/* Custom CSND_playsound: 
 - Prevent audio desynchronization for Video module
 - Enable stereo sounds
 - Add ADPCM codec support
*/
void My_CSND_playsound(u32 channel, u32 looping, u32 encoding, u32 samplerate, u32 *vaddr0, u32 *vaddr1, u32 totalbytesize, u32 l_vol, u32 r_vol){
	u32 physaddr0 = 0;
	u32 physaddr1 = 0;
	physaddr0 = osConvertVirtToPhys((u32)vaddr0);
	physaddr1 = osConvertVirtToPhys((u32)vaddr1);
	if (vaddr0 && encoding == CSND_ENCODING_IMA_ADPCM){
		int adpcmSample = ((s16*)vaddr0)[-2];
		int adpcmIndex = ((u8*)vaddr0)[-2];
		CSND_SetAdpcmState(channel, 0, adpcmSample, adpcmIndex);
	}
	CSND_sharedmemtype0_cmde(channel, looping, encoding, samplerate, 2, 0, physaddr0, physaddr1, totalbytesize);
	CSND_sharedmemtype0_cmd8(channel, samplerate);
	if(looping){
		if(physaddr1>physaddr0)totalbytesize-= (u32)physaddr1 - (u32)physaddr0;
		CSND_sharedmemtype0_cmd3(channel, physaddr1, totalbytesize);
	}
	u32 cmdparams[0x18>>2];
	memset(cmdparams, 0, 0x18);
	cmdparams[0] = channel & 0x1f;
	cmdparams[1] = l_vol | (r_vol<<16);
	CSND_writesharedmem_cmdtype0(0x9, (u8*)&cmdparams);
}



Result ADPCM_PlaySound(u32 chn, u32 looping, u32 encoding, u32 samplerate, void* data0, void* data1, u32 size, u32 l_vol, u32 r_vol){
	u32 paddr0 = 0, paddr1 = 0;
	if (data0) paddr0 = osConvertVirtToPhys((u32)data0);
	if (data1) paddr1 = osConvertVirtToPhys((u32)data1);
	if (data0 && encoding == CSND_ENCODING_IMA_ADPCM){
		int adpcmSample = ((s16*)data0)[-2];
		int adpcmIndex = ((u8*)data0)[-2];
		CSND_SetAdpcmState(chn, 0, adpcmSample, adpcmIndex);
	}
	CSND_sharedmemtype0_cmde(chn, looping, encoding, samplerate, 2, 0, paddr0, paddr1, size);
	CSND_sharedmemtype0_cmd8(chn, samplerate);
	if(looping){
		if(paddr1>paddr0)size-= (u32)paddr1 - (u32)paddr0;
		CSND_sharedmemtype0_cmd3(chn, paddr1, size);
	}
	u32 cmdparams[0x18>>2];
	memset(cmdparams, 0, 0x18);
	cmdparams[0] = chn & 0x1f;
	cmdparams[1] = l_vol | (r_vol<<16);
	CSND_writesharedmem_cmdtype0(0x9, (u8*)&cmdparams);
	CSND_ChnSetAdpcmReload(chn, true);
	return CSND_processtype0cmds();
}