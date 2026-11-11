# Squishy Plushie

A free starter-base plush: pick up, throw, bounce and squeeze.

## Install

Install [Moon Sweepers Mod Loader](https://github.com/swooping-mods/moon-sweepers-mod-loader) first.

Download [squishy-plushie-0.2.0.msmod](content/squishy-plushie-0.2.0.msmod), then drag and drop the `.msmod` file onto the [Moon Sweepers Mod Loader](https://github.com/swooping-mods/moon-sweepers-mod-loader) window. You can also use **Add mods…** (older loader builds: **Install mod package**).

Select the mod and its required frameworks, click **Prepare files**, then **Launch**.

Required mods: [sound-framework](https://github.com/swooping-mods/sound-framework), [pickup-hands](https://github.com/swooping-mods/pickup-hands), [hints-framework](https://github.com/swooping-mods/hints-framework).

## Controls

- A free plush astronaut spawns inside the starter station.
- Aim at the plush and E to pick up; E drops, left click throws, right click squeezes. These exclusive interactions consume overlapping keys.
- Session-local host prototype. Generated fabric, squeeze, release and landing sounds follow the toy.

## Compatibility

Version 0.2.0. Targets the pinned Steam Windows x64 IL2CPP build of Moon Sweepers (Steam app 5335510). On Linux, the game runs through Proton. Experimental: back up saves before using save-affecting mods.

## Build

Run `python3 build.py --check` to verify source hashes, then `python3 build.py` to write a build under `.local/build/`. Requires installed Clang, lld-link and llvm-dlltool.

## Credits

See [CREDITS.txt](CREDITS.txt) and [third-party notices](vendored/THIRD_PARTY_NOTICES.md).
