# Integrated Pointercrate

A Geode mod for Geometry Dash that brings the Pointercrate demon list and player ranking (including your own rank) into the game, inspired by [hiimjasmine00/IntegratedDemonlist](https://github.com/hiimjasmine00/IntegratedDemonlist).

- Mod ID: `clamscs.integratedpointercrate`
- Target: Geometry Dash 2.2081, Geode 4.x

## Build

Requires the [Geode SDK](https://docs.geode-sdk.org/getting-started/install).

```
export GEODE_SDK=/path/to/geode/sdk
cmake -B build
cmake --build build
```

## Folder structure

```
IntegratedPointercrate/
├── mod.json
├── CMakeLists.txt
├── about.md
├── README.md
└── src/
    ├── api/
    │   ├── PointercrateAPI.hpp
    │   └── PointercrateAPI.cpp        # URLs, JSON parsing, Link-header cursor pagination
    ├── ui/
    │   ├── cells/
    │   │   ├── DemonCell.hpp/.cpp     # one row in the demon list
    │   │   └── PlayerCell.hpp/.cpp    # one row in the ranking list
    │   └── popups/
    │       ├── DemonListLayer.hpp/.cpp
    │       └── PlayerRankingLayer.hpp/.cpp
    └── hooks/
        └── MenuLayerHook.cpp          # adds the two buttons to the main menu
```

Includes are resolved from `src/` (see `target_include_directories` in `CMakeLists.txt`), so files reference each other as e.g. `"api/PointercrateAPI.hpp"` or `"ui/cells/DemonCell.hpp"` regardless of folder depth.

## Pagination

Both lists fetch 10 items per page (`pointercrate::ITEMS_PER_PAGE` in `PointercrateAPI.hpp`) and page strictly through Pointercrate's `Link` response header (`rel="next"` / `rel="prev"`), the same cursor pagination Pointercrate itself uses. Pressing Next repeatedly walks forward page by page until the API stops returning a `next` link (list exhausted), at which point the Next button disables itself instead of looping or re-fetching everything at once.

## No settings

There is no mod settings menu. Page size is fixed in code, and your own rank is looked up by typing your name into the search box inside the Player Ranking popup itself.

## Note

This project was generated without a local Geode/GD toolchain to compile against. The code follows standard Geode SDK 4.x conventions, but please build it once with the real SDK before publishing and fix any API drift from the exact Geode version you target.
