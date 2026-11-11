#ifndef MOON_SOUND_PREPARATION_CLIENT_H
#define MOON_SOUND_PREPARATION_CLIENT_H
#include "client.h"
#include "preparation.h"
static inline const MoonSoundPreparation *ma_connect_preparation(PTR domain,PTR game){
    if(moon_runtime_loader_present()){
        const MoonRuntimeApiV1 *runtime=moon_runtime_connect();
        const MoonSoundPreparation *api=runtime?(const MoonSoundPreparation *)runtime->find("sound-framework","sound-preparation",MA_PREPARATION_ABI,sizeof(MoonSoundPreparation)):0;
        return api&&api->abi==MA_PREPARATION_ABI&&api->size>=sizeof(MoonSoundPreparation)?api:0;
    }
    if(!ma_connect(domain,game))return 0;
    char path[1200];DWORD n=GetModuleFileNameA(0,path,1000);if(!n||n>=1000)return 0;
    while(n&&path[n-1]!='\\'&&path[n-1]!='/')n--;
    if(!n)return 0;cat(path+n,"MoonLab\\mods\\sound-framework\\mod.dll");
    PTR module=LoadLibraryA(path);if(!module)return 0;
    MoonSoundPreparationGet get=(MoonSoundPreparationGet)GetProcAddress(module,"MoonLab_SoundPreparation_v1");
    return get?get(MA_PREPARATION_ABI,sizeof(MoonSoundPreparation)):0;
}
#endif
