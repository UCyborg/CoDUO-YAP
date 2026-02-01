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
#include "utils/common.h"
namespace sound {
	struct snd_volume_info_t
	{
		float volume;
		float goalvolume;
		float goalrate;
	};




#define CHANNEL_AUTO 0
#define CHANNEL_MENU 1
#define CHANNEL_WEAPON 2
#define CHANNEL_VOICE 3
#define CHANNEL_ITEM 4
#define CHANNEL_BODY 5
#define CHANNEL_LOCAL 6
#define CHANNEL_MUSIC 7
#define CHANNEL_ANNOUNCER 8
#define CHANNEL_SHELLSHOCK 9



	enum g_snd_channelvol_array {
		AUTO,
		MENU,
		WEAPON,
		VOICE,
		ITEM,
		BODY,
		LOCAL,
		MUSIC,
		ANNOUNCER,
		SHELLSHOCK,
		MAX_CHANNELVOL_ARRAY
	};

	struct __declspec(align(4)) snd_channelvolgroup
	{
		snd_volume_info_t channelvol[MAX_CHANNELVOL_ARRAY];
		bool active;
	};
	SafetyHookInline MSS_UpdateD;
	cevar_s* snd_test_volume[MAX_CHANNELVOL_ARRAY];
	cevar_s* mss_volume_music;
	cevar_s* mss_volume_effects;
	cevar_s* mss_volume_voice;

	const char* channelNames[] = {
		"auto",
		"menu",
		"weapon",
		"voice",
		"item",
		"body",
		"local",
		"music",
		"announcer",
		"shellshock"
	};

	SAFETYHOOK_NOINLINE void UpdateVolume_ForChannel(float* volume, int index, bool soundmod = false) {
		float category_multiplier = 1.0f;

		// Determine category multiplier
		if (index == MUSIC || index == MENU) {
			category_multiplier = mss_volume_music->base->value;
		}
		else if (index == VOICE || index == ANNOUNCER) {
			category_multiplier = mss_volume_voice->base->value;
		}
		else {
			category_multiplier = mss_volume_effects->base->value;
		}

		if (soundmod) {
			// SET mode: calculate absolute value
			*volume = snd_test_volume[index]->base->value * category_multiplier;
		}
		else {
			// MULTIPLY mode: stack on existing
			*volume *= category_multiplier * snd_test_volume[index]->base->value;
		}
	}

	void soundmod() {
		snd_channelvolgroup* snd_grb = (snd_channelvolgroup*)exe(0x8EDB24,0x9CBEF4);

		for (int i = 0; i < MAX_CHANNELVOL_ARRAY; i++) {
			UpdateVolume_ForChannel(&snd_grb->channelvol[i].volume, i,true);
		}

	}

	int MSS_Update() {
		auto result = MSS_UpdateD.unsafe_ccall<int>();

		soundmod();

		return result;

	}
	uintptr_t MSS_FadeSelectSounds_original = 0x445F90;
	void  MSS_FadeSelectSounds(float* volumes, float unk) {
		__asm
		{
			mov eax, volumes
			push unk
			call MSS_FadeSelectSounds_original
			add esp, 4
		}
	}

	void MSS_FadeSelectSounds_hook(float* volumes, float unk) {
		float* channelvolume_readonly = volumes;
		float channelvolume[MAX_CHANNELVOL_ARRAY]{};
		memcpy(channelvolume, channelvolume_readonly, sizeof(channelvolume));

		for (int i = 0; i < MAX_CHANNELVOL_ARRAY; i++) {

			UpdateVolume_ForChannel(&channelvolume[i], i);
		}

		MSS_FadeSelectSounds(channelvolume, unk);
	}



	uintptr_t MSS_FadeSelectSounds_return_jmp = 0x40268A;
	void __declspec(naked) MSS_FadeSelectSounds_stub() {
		__asm
		{
			push eax
			call MSS_FadeSelectSounds_hook
			add esp, 4
			jmp MSS_FadeSelectSounds_return_jmp
		}
	}



    class component final : public component_interface
    {
    public:

        void post_unpack() override
        {
			static auto pattern = hook::pattern("51 A1 ? ? ? ? 85 C0 53 56 0F 84");
			if (!pattern.empty()) {

				for (int i = 0; i < MAX_CHANNELVOL_ARRAY; i++) {
					auto string = std::string("snd_volume_") + channelNames[i];
					char* cstr = new char[string.length() + 1];
					std::strcpy(cstr, string.c_str());

					snd_test_volume[i] = Cevar_Get(cstr, 1.f, 0, 0.f, 5.f);
				}

				mss_volume_music = Cevar_Get("mss_volume_music",1.f,CVAR_ARCHIVE,0.f,1.f);
				mss_volume_effects = Cevar_Get("mss_volume_effects", 1.f, CVAR_ARCHIVE, 0.f, 1.f);
				mss_volume_voice = Cevar_Get("mss_volume_voice", 1.f, CVAR_ARCHIVE, 0.f, 1.f);

				MSS_UpdateD = safetyhook::create_inline(pattern.get_first(), MSS_Update);

			}
			//static auto whatever = safetyhook::create_mid(0x445F90, [](SafetyHookContext& ctx) {

			//	float* channelvolume_readonly = (float*)(ctx.eax);
			//	float channelvolume[11]{};
			//	memcpy(channelvolume, channelvolume_readonly, sizeof(channelvolume));
			//	for (int i = 0; i < 11; i++) {
			//		int channelIndex = channelMap[i];
			//		Memory::VP::Patch<float>(&channelvolume[channelIndex], channelvolume[channelIndex] * snd_test_volume[i]->base->value);
			//		//printf("channelvolume[%d]: %f\n", channelIndex, channelvolume[channelIndex]);
			//	}

			//	});
			MSS_FadeSelectSounds_return_jmp = exe(0x40268A, 0x402A60);
			MSS_FadeSelectSounds_original = exe(0x445F90, 0x453420);
			Memory::VP::InjectHook(exe(0x402685,0x402A5B), MSS_FadeSelectSounds_stub, Memory::VP::HookType::Jump);
        }


    };

}
REGISTER_COMPONENT(sound::component);