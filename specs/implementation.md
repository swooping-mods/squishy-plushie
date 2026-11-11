# Squishy Plushie: behavior and implementation

Describes the exported implementation **0.2.0**. The linked source and metadata define this version; gameplay qualification is a separate requirement.

## Behavior

Spawns a free plush astronaut in the starter station. Aim and E picks it up, E drops, left click throws and right click squeezes. Pickup first validates ray/clearance and acquires Pickup Hands before moving the toy; failed admission leaves it in place. Exclusive interaction delivery consumes overlapping keys. Soft deformation, bounce/carry presentation and spatial fabric/squeeze/release/landing sounds follow the toy.

## State and lifecycle

This exported prototype is host-local and session-only; no saved toy identity or guest replication exists. Successful drop/throw releases hands; blocked drops retain the claim and pose. World teardown releases claims, hints, scene meshes and sound emitters. Native floor/collision queries bound movement rather than teleporting through obstacles.

## Integration and configuration

Runtime dependencies: [sound-framework](https://github.com/swooping-mods/sound-framework), [pickup-hands](https://github.com/swooping-mods/pickup-hands), [hints-framework](https://github.com/swooping-mods/hints-framework).

The [module metadata](../source/mods/squishy-plushie/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Requires `sound-framework.sound` | 1 | at least 48 bytes |
| Requires `pickup-hands.hands` | 1 | at least 32 bytes |
| Requires `hints-framework.hints` | 1 | at least 24 bytes |
| Requires `sound-framework.sound-preparation` | 1 | at least 16 bytes |

## Implementation sources

- [mod.c](../source/mods/squishy-plushie/mod.c)
- [soft_core.h](../source/mods/squishy-plushie/soft_core.h)
- [carry_core.h](../source/mods/squishy-plushie/carry_core.h)
- [interaction.h](../source/mods/squishy-plushie/interaction.h)
- [mesh_runtime.h](../source/mods/squishy-plushie/mesh_runtime.h)
- [profile.h](../source/mods/squishy-plushie/profile.h)

## Verification boundary

Visual softness is a bounded presentation model, not a general soft-body solver. Throw/bounce, blocked carry/drop, sound and multiplayer require separate game qualification; reloading does not preserve the session toy.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
