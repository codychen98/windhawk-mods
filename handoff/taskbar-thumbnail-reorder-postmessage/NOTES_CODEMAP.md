# Code map — trigger / listener / reorder pass

Line numbers refer to [`taskbar-thumbnail-reorder-fork.wh.cpp`](taskbar-thumbnail-reorder-fork.wh.cpp) (v1.3.16). Re-verify after edits.

## Settings that matter for this bug

Loaded into `g_settings` / `g_rules` (search `LoadSettings`, `externalPostMessageTriggerEnabled`, `rulesJson`):

| Setting | User value | Effect |
|---------|------------|--------|
| `autoReorderOnModLoad` | `0` | Startup timer path off |
| `externalPostMessageTriggerEnabled` | `1` | Creates listener + hidden HWND |
| `hotkeyEnabled` | `0` | No `RegisterHotKey`; PostMessage-only |
| `rulesJson` | `floorp.exe` matches | Test/auto pass needs valid parsed rules |

## Lifecycle

| Lines | Symbol | Notes |
|------:|--------|-------|
| 5065 | `Wh_ModInit` | Sets `g_initialized = true` near end (~5127) |
| 5132 | `Wh_ModAfterInit` | `StartAutoReorderTimerIfNeeded()` then `UpdateReorderListener()` |
| 5157 | `Wh_ModUninit` | Stops timer, stops listener thread |
| 5168 | `Wh_ModSettingsChanged` | Reloads settings; **restarts** listener via `UpdateReorderListener()` |

## External trigger path

| Lines | Symbol | Notes |
|------:|--------|-------|
| 2152–2155 | `kReorderTriggerWindowClass` / `kReorderTriggerRegisteredMessageName` | `WindhawkThumbnailReorderTrigger` / `Windhawk.TaskbarThumbnailReorder.Trigger` |
| 2163–2166 | `HandleManualReorderTrigger` | Logs source; calls `RunAutoReorderTestPass()` |
| 2168–2184 | `ReorderTriggerWndProc` | Registered message → PostMessage path; `WM_HOTKEY` → hotkey path |
| 2186–2202 | `EnsureReorderTriggerWindowClassRegistered` | `RegisterClassExW` |
| 2204–2278 | `ReorderListenerThreadProc` | Own thread: register message, create hidden `WS_POPUP` (comment: not `HWND_MESSAGE` so AHK `ahk_class` works), optional hotkey, `GetMessageW` pump |
| 2280–2299 | `StopReorderListenerThread` | `PostThreadMessage(WM_QUIT)` + wait |
| 2301–2342 | `UpdateReorderListener` | Stop then start if hotkey and/or PostMessage wanted; no-op if `!g_initialized` |

## Reorder pass (what runs after a successful trigger)

| Lines | Symbol | Notes |
|------:|--------|-------|
| 2126–2149 | `RunAutoReorderTestPass` | Early exit: invalid rules, empty apps, `!g_initialized`; then compute + apply |
| 2116 | `RunAutoReorder` | Session one-shot path (timer); not used when `autoReorderOnModLoad` is 0 |
| 2423 | `StartAutoReorderTimerIfNeeded` | Called from `Wh_ModAfterInit`; gated by settings |

Early-exit log strings useful for diagnosis (search these in Windhawk / DebugView):

- `Auto-reorder test: rules invalid or JSON parse failed`
- `Auto-reorder test: no apps in rules`
- `Auto-reorder test: mod not initialized yet`
- `Auto-reorder: trigger from PostMessage` / `trigger from hotkey`
- `Auto-reorder listener: PostMessage window ready`
- `Auto-reorder listener: CreateThread failed`
- `Auto-reorder listener: CreateWindowExW failed`

## Upstream comparison

[`mods/taskbar-thumbnail-reorder.wh.cpp`](../../mods/taskbar-thumbnail-reorder.wh.cpp) is upstream **1.1.5** (manual drag only). It has **no** listener thread, trigger HWND, or auto-reorder. Use it only to see which pieces are fork-added.

## Prior art (hotkey / message queue)

v1.2.4 fixed F1 by putting `RegisterHotKey` + `GetMessageW` on one dedicated listener thread (wrong-thread delivery in 1.2.2–1.2.3). PostMessage targeting uses the same HWND/thread. Startup PostMessage failures may still be race or pass no-op even when that design is correct.
