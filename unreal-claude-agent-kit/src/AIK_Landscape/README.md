# AIK_Landscape — Custom Module

> Reference implementation of one of the custom modules described in the parent project writeup. Exposes typed landscape operations to AI agents via the host kit's `AgentService`.

This module is the smallest viable example of the architectural pattern called out in the parent writeup: an interface-spine-only dependency, a typed tool surface that fails fast instead of silently no-op'ing, and a clean separation between protocol (the host kit's `AgentService`) and capability (this subsystem).

## File layout

```
AIK_Landscape/
├── AIK_Landscape.Build.cs              UBT module dependencies
├── Public/
│   ├── AIK_LandscapeTypes.h            Region, Peak, Result types
│   ├── AIK_LandscapeModule.h           IModuleInterface, tool registration
│   └── AIK_LandscapeSubsystem.h        UEditorSubsystem, the actual ops
└── Private/
    ├── AIK_LandscapeModule.cpp         Module lifecycle + RegisterTool calls
    └── AIK_LandscapeSubsystem.cpp      Sculpt / Paint / Query implementations
```

## What's interesting here, not just "what does it do"

### 1. Region type discipline

`FAIK_LandscapeRegion` carries `{X1, Y1, X2, Y2}` in **landscape grid coords** (0..resolution), not world cm. The stock kit's landscape ops accept a positional `TArray<float>` and silently treat unrecognized lengths as a no-op — which is the kind of bug that costs an afternoon when you're a hundred miles into a debugging session and the agent reports "success" on operations that did nothing.

This module rejects degenerate or out-of-bounds regions at the boundary with a typed error code, not a stringly-typed warning. `EAIK_LandscapeError` enumerates every reason a call can fail — the agent branches on `bSuccess` and reads the code, never parses an error string.

### 2. Tool registration is the API contract

`AIK_LandscapeModule.cpp::RegisterAgentTools` is the *only* place tool names are defined. The strings in those `Service->RegisterTool` calls are the agent-visible contract — changes to those names or signatures are an API break, treated as such.

The corresponding spec lives in [`../../docs/tool-surface.md`](../../docs/tool-surface.md). The two must agree. (In production this would be enforced with a build-time check; in a portfolio scaffold the hand-maintained discipline is enough to demonstrate the pattern.)

### 3. Optional host kit compile-time dependency

The module compiles cleanly without the closed-source host kit present. `#if __has_include("AgentService.h")` gates the registration calls; when the host isn't there, the subsystem is still usable from Blueprint and direct C++. This lets the module be open-sourced (as it is here) while the host stays proprietary.

### 4. Transactions wrap mutating ops

Every sculpt / paint method is wrapped in a `FScopedTransaction` so the operation appears in the editor's Undo stack as a single, named entry. If a designer wants to undo what the agent just did, it works exactly like undoing their own brush stroke. This is the kind of integration detail that makes agent-driven editing feel native rather than alien.

## What this scaffold doesn't fully implement

Two implementations are stubbed for brevity:

- **`PaintHeightmapFromTexture`** — texture-format decode (BC4 / R8 / R16 with format-specific mip walks) is omitted. The production module reads via `FTexture2DMipMap::BulkData` with a format dispatch table.
- **Async edit batching** — when an agent issues a hundred small sculpts in quick succession, the production module batches them inside a single transaction and a single normal-recompute pass. The scaffold here does one transaction per call.

Both are intentional omissions, called out so the gap is honest. The architectural shape is the point of this scaffold; the surface area is the production module's job.

## Build dependencies

| Module | Why |
|---|---|
| `Landscape` | `ALandscape`, `ULandscapeInfo` |
| `LandscapeEditor` | `FLandscapeEditDataInterface` |
| `EditorSubsystem` | `UEditorSubsystem` base class |
| `UnrealEd` | Transaction support, editor world access |
| `EditorScriptingUtilities` | Editor-only helper APIs |
| `AgentIntegrationKit` (optional) | `FAgentService::RegisterTool` |

## License

MIT — see [../../LICENSE](../../LICENSE) at the portfolio root.
