#ifndef MOON_SOUND_PREPARATION_V1_H
#define MOON_SOUND_PREPARATION_V1_H
#define MA_PREPARATION_ABI 1u
/* Main-thread, world-owned silent preparation. cue=0 only prepares voices;
 * voices is a total pool target (0..MA_VOICES), never a reservation. Repeated
 * calls reuse resources. This never plays, binds, stops or changes cooldowns.
 * Partial success is retained for retry and released on world teardown. */
typedef struct {
    unsigned abi,size;
    int (*prepare)(unsigned cue,unsigned voices);
} MoonSoundPreparation;
typedef const MoonSoundPreparation *(*MoonSoundPreparationGet)(unsigned,unsigned);
#endif
