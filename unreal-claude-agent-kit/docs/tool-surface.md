# Agent Tool Surface — Reference

The kit registers a typed tool surface with the host's `AgentService` at module startup. Every tool listed below is callable by any connected ACP-compatible agent. This document is the source of truth for tool names, arguments, return shapes, and failure modes.

## Conventions

- **Names.** Dot-separated namespaces (`landscape.sculpt_region_to_height`). Namespace prefix matches the owning module. Never renamed silently — a renamed tool is a major version bump.
- **Region inputs.** Always landscape grid coordinates as a named object `{x1, y1, x2, y2}`, never world centimeters and never a positional array. Positional input is explicitly rejected at the boundary.
- **Result shape.** Every tool returns `{success: bool, error_code: enum, error_message: string, value?: object}`. Agents branch on `success` and inspect `error_code`. The `error_message` field is developer-facing only — never surface it to end users.
- **Idempotence.** Read-only tools (anything in `query.*`) are idempotent and safe to retry. Mutating tools are NOT idempotent — caller is responsible for tracking what's been applied.
- **Transactions.** Every mutating tool is a single transaction. Undo from the editor undoes the entire operation as one step.

## Landscape namespace

### `landscape.sculpt_region_to_height`

Set every grid sample inside a region to an absolute height (in centimeters).

```json
{
  "landscape": "<editor object path>",
  "region": { "x1": 0, "y1": 0, "x2": 64, "y2": 64 },
  "absolute_height_cm": 1500.0
}
```

Returns `{success: true}` on completion.

**Errors:**
- `region_invalid` — degenerate region (`x2 <= x1` or `y2 <= y1`).
- `region_out_of_bounds` — region exceeds the landscape's grid extent.
- `landscape_not_found` — the named landscape object can't be resolved.
- `edit_failed` — `FLandscapeEditDataInterface` could not be acquired (typically a missing `ULandscapeInfo`).

### `landscape.sculpt_region_by_delta`

Add a delta height (cm) to every sample. Saturating clamp at the landscape's 16-bit height range. Same arg shape as above with `delta_height_cm` in place of `absolute_height_cm`.

### `landscape.paint_heightmap_from_texture`

Apply a grayscale texture as heightmap data over a region. Sample values in `[0, 1]` are mapped to `[min_height_cm, max_height_cm]`.

```json
{
  "landscape": "...",
  "region": { ... },
  "heightmap_texture": "<texture object path>",
  "min_height_cm": 0.0,
  "max_height_cm": 5000.0
}
```

Texture sampling supports `BC4`, `R8`, and `R16` formats. Sample dimensions are stretched to fit the region — agents are responsible for choosing a texture resolution that matches the region's grid size if 1:1 sampling is required.

### `landscape.query_peak_in_region`

Find the highest Z point inside a region. Read-only.

Returns:
```json
{
  "success": true,
  "value": {
    "grid_x": 42, "grid_y": 17,
    "world_z_cm": 2384.5,
    "world_location": [4200.0, 1700.0, 2384.5]
  }
}
```

### `landscape.query_normal_at_point`

Return the surface normal at a world-space point. Implemented as a downward line trace; misses (e.g., point not over the landscape) return `edit_failed`.

```json
{
  "landscape": "...",
  "world_location": [4200.0, 1700.0, 0.0]
}
```

Returns:
```json
{
  "success": true,
  "value": { "normal": [0.12, -0.34, 0.93] }
}
```

## Failure-mode philosophy

The host kit's stock tool wrappers tend toward leniency: malformed input gets a warning log and a no-op result. This kit inverts that policy.

- **Invalid args → typed error, never silent.** Region degenerate? Returns `region_invalid`, not `success: true` with no work done.
- **Missing dependency → typed error.** Landscape not found? Returns `landscape_not_found`, not a null-deref crash.
- **Editor-only operation in PIE → typed error.** Returns `editor_only_operation`, not undefined behavior.

This shifts the burden from "agent has to parse free-text errors to know what went wrong" to "agent reads a stable error code." Cheaper for the agent, faster to debug, more predictable across model versions.

## Versioning

Every breaking change to a tool's name, argument shape, or return shape bumps the kit's major version. Argument additions with defaults are minor bumps. Error code additions (new enum values) are minor bumps — agents must tolerate unknown codes by treating them as generic failure.

## Source of truth

Tool registration calls in [`../src/AIK_Landscape/Private/AIK_LandscapeModule.cpp`](../src/AIK_Landscape/Private/AIK_LandscapeModule.cpp) (function `RegisterAgentTools`). This document and that code must agree; the production module enforces this with a build-time check that parses the markdown and diffs against the C++ registration array.
