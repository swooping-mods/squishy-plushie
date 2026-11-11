#ifndef SQUISHY_PLUSHIE_AUDIO_BANK_H
#define SQUISHY_PLUSHIE_AUDIO_BANK_H
#include "generated/plush_sounds.h"
/* Recorded derivatives retain their source ranges in design/mod-sfx-import.json.
 * Every slot is registered once; physics owns event timing and emitter motion. */
static const MoonSoundCue qp_sound_bank[]={
 {MA_ABI,sizeof(MoonSoundCue),"squishy-plushie.pickup",ms_pickup_pcm,MS_PICKUP_SAMPLES,MS_PICKUP_RATE,.55f,.8f,12,.15f},
 {MA_ABI,sizeof(MoonSoundCue),"squishy-plushie.drop",ms_drop_pcm,MS_DROP_SAMPLES,MS_DROP_RATE,.40f,.8f,12,.15f},
 {MA_ABI,sizeof(MoonSoundCue),"squishy-plushie.throw",ms_throw_pcm,MS_THROW_SAMPLES,MS_THROW_RATE,.55f,.8f,12,.15f},
 {MA_ABI,sizeof(MoonSoundCue),"squishy-plushie.squeeze",ms_squeeze_pcm,MS_SQUEEZE_SAMPLES,MS_SQUEEZE_RATE,.55f,.8f,12,.25f},
 {MA_ABI,sizeof(MoonSoundCue),"squishy-plushie.impact-soft",ms_impact_soft_pcm,MS_IMPACT_SOFT_SAMPLES,MS_IMPACT_SOFT_RATE,.45f,.8f,12,.12f},
 {MA_ABI,sizeof(MoonSoundCue),"squishy-plushie.impact-hard",ms_impact_hard_pcm,MS_IMPACT_HARD_SAMPLES,MS_IMPACT_HARD_RATE,.60f,.8f,12,.12f}
};
#endif
