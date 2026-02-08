#include <helper.hpp>
#include <game.h>
#include "component_loader.h"
#include "cevar.h"
#include <Hooking.Patterns.h>
#include "utils/hooking.h"

#include <filesystem>
#include <fstream>
#include "nlohmann/json.hpp"

#include <optional>
#include "GMath.h"
#include <buildnumber.h>
#include "framework.h"
namespace files_hook {
    cevar_s* fs_rawfiles;
    uintptr_t FS_filelength_addr = 0x422EF0;
    int FS_filelength(int a1) {
        int result;
        __asm {
            mov eax, a1
            call FS_filelength_addr
            mov result,eax
        }
        return result;
    }
    uintptr_t FS_HandleForFile_addr = 0x422E30;
    int FS_HandleForFile(int a1) {
        int result;
        __asm {
            mov eax, a1
            call FS_HandleForFile_addr
            mov result, eax
        }
        return result;
    }

    char* FS_BuildOSPath_Custom(const char* base, const char* game, const char* qpath, char* ospath) {
        if (!game || !*game) {
            game = "Main";
        }

        size_t base_len = strlen(base);
        size_t game_len = strlen(game);
        size_t qpath_len = strlen(qpath);

        // Check buffer overflow
        if (base_len + game_len + qpath_len + 3 >= 256) {
            *ospath = 0;
            return ospath;
        }

        // Build: base\game\qpath
        char* ptr = ospath;

        // Copy base path
        memcpy(ptr, base, base_len);
        ptr += base_len;

        // Add separator
        *ptr++ = '\\';

        // Copy game directory
        memcpy(ptr, game, game_len);
        ptr += game_len;

        // Add separator
        *ptr++ = '\\';

        // Copy qpath and null terminator
        memcpy(ptr, qpath, qpath_len + 1);

        return ospath;
    }
    SafetyHookInline FS_FOpenFileReadD;
    SafetyHookInline FS_ReadFileD;
    int __cdecl FS_ReadFile(char* filename, int** buffer) {
        if (!filename || !*filename)
            return FS_ReadFileD.unsafe_ccall<int>(filename, buffer);

        size_t len = strlen(filename);
        bool trySpecial = false;


        auto vfilename = std::string_view(filename);
        if (vfilename.ends_with(".menu")) {
            
        }
        if (fs_rawfiles) {
            if (fs_rawfiles->base->integer == 2)
                trySpecial = true;
            else if (fs_rawfiles->base->integer == 1) {
                
                if (len > 4) {
                    if ((vfilename.ends_with(".gsc") && sp_mp(1)) ||
                        vfilename.ends_with(".tga") ||
                        vfilename.ends_with(".menu")) {
                        trySpecial = true;
                    }
                }

            }
        }
        else {
            trySpecial = true;
        }


        if (trySpecial) {
            auto fs_basepath = Cvar_Find("fs_basepath");
            char filepath[512];
            sprintf(filepath, "%s\\YAP_raw\\%s", fs_basepath->string, filename);

            FILE* f = fopen(filepath, "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                long size = ftell(f);
                fseek(f, 0, SEEK_SET);


                void* mem = game::Hunk_AllocateTempMemory(size + 1);

                fread(mem, 1, size, f);
                ((char*)mem)[size] = 0;
                fclose(f);

                if (buffer) {
                    *buffer = (int*)mem;
                }

                auto fs_debug = Cvar_Find("fs_debug");

                if (fs_debug && fs_debug->integer) {
                    Com_Printf("FS_ReadFile: %s (found in 'YAP_raw')\n", filename);
                    printf("FS_ReadFile: %s (found in 'YAP_raw')\n", filename);
                }

                return size;
            }
        }


        return FS_ReadFileD.unsafe_ccall<int>(filename, buffer);
    }
    class component final : public component_interface
    {
    public:

        void post_unpack() override
        {
            fs_rawfiles = Cevar_Get("fs_rawfiles", 2, CVAR_ARCHIVE, 0, 2);
        }

        void post_start() override
        {
            FS_ReadFileD = safetyhook::create_inline(exe(0x4249B0,0x42E600), FS_ReadFile);

        }

    };

}
REGISTER_COMPONENT(files_hook::component);