# Portfolio — Lina Hal Hasnawi

> Senior engineer · Unreal Engine 5 · AI agent integrations · full-stack web

I evaluate large commercial frameworks, port them across engine versions, and extend them with custom modules where they fall short. Each project below is a writeup of real engineering work — not a republish of upstream source. Where I've forked or built on a commercial product, I credit the upstream and disclose what's mine vs. what isn't.

→ **[About me](./ABOUT.md)** for the short bio · **[github.com/linahalhasnawi-boop](https://github.com/linahalhasnawi-boop)** for everything else.

## Projects

### [Unreal Claude Agent Kit](./unreal-claude-agent-kit/)

Custom fork of Betide Studio's *Agent Integration Kit* ported from UE 5.4 to 5.7, stripped to the modules I use, and extended with `AIK_Landscape`, `AIK_Sequencer`, and an Errant Biomes bridge. Lets Claude Code / Gemini CLI / OpenRouter drive Unreal Engine 5 via the Agent Client Protocol.

**Highlights:** ACP-over-stdio transport, per-module lazy loading, name-based mask binding for biome workflows, 5.4→5.7 port surface notes (MSVC 14.44 ban, Live Coding lock gotchas).

### [Project Sariya — Action RPG Foundation](./project-sariya/)

Third-person action RPG built on Ascent Combat Framework (Pask / Dark Tower Interactive). 21 ACF modules ported 5.5 → 5.7, scope cut to the 11 the game needs, two custom extension modules planned on top. Six demo clips at the top of the page show the foundation systems verified working under the port.

**Highlights:** spine-driven module architecture, anim-notify-driven action lifecycle, why ACF over rolling custom GAS, verified 5.5 → 5.7 port observations, combat / swimming / mount / vehicle / interactables / status effects demos.

### [SKG Shooter Framework — Integration Plan](./skg-shooter-framework/)

Architecture analysis and integration plan for evaluating Sneaky Kitty Game Dev's *SKG Shooter Framework* as the weapon foundation for a multiplayer FPS project. 16 modules, 171 C++ files mapped against a UE 5.7 target; the procedural animation pattern and modular attachment architecture broken down with scoped extensions for ammo economy and tactical reload.

**Highlights:** procedural weapon animation (no hand-keying), socket-based attachment composition, why SKG over Lyra/custom, 5.6→5.7 port surface, upstream descriptor defect spotted.

---

## License

Writeups are MIT-licensed. Linked upstream products are governed by their own licenses.

## Contact

[github.com/linahalhasnawi-boop](https://github.com/linahalhasnawi-boop)
