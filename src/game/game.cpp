#include "game.h"
#include "../framework.h"
#include "../structs.h"
uintptr_t cg_game_offset = 0;
uintptr_t ui_offset = 0;
uintptr_t game_offset = 0;
uint32_t* player_flags = nullptr;
Com_PrintfT Com_Printf = (Com_PrintfT)NULL;
namespace game {
	void __cdecl Cmd_AddCommand(const char* command_name, void* function) {
		if(exe(0x00422860))
		cdecl_call<int>(exe(0x00422860), command_name, function);
	}

	int FS_FOpenFileByMode(const char* qpath, fileHandle_t* f, fsMode_t mode) {
		return cdecl_call<int>(exe(0x00427240), qpath, f, mode);
	}

	int FS_FCloseFile(fileHandle_t* f) {
		return cdecl_call<int>(exe(0x00423510), f);
	}

	int FS_GetFileList_CALL(const char* path, const char* extension, char* listbuf, int bufsize, uintptr_t CALLADDRESS)
	{
		int result;
		__asm
		{
			mov eax,path
			push bufsize
			push listbuf
			push extension
			call CALLADDRESS
			add esp, 0xC
			mov result, eax
		}
		return result;
	}

	int FS_GetFileList(const char* path, const char* extension, char* listbuf, int bufsize)
	{
		return FS_GetFileList_CALL(path, extension, listbuf,bufsize ,exe(0x425730));
	}

}