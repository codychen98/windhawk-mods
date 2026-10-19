# Implementation handoff 2 — fork 1.3.17 → 1.3.18 (rule-#1 window not in slot 1)

Edit ONLY `handoff/taskbar-thumbnail-reorder-postmessage/taskbar-thumbnail-reorder-fork.wh.cpp`. Local only; never push.

## Root cause (from taskbar-reorder-log.txt + source)
Engine ranks 7 windows (#1 = hwnd 20BD4, the YouTube rule). But the hover flyout only realizes 3 XAML thumbnails (`3 thumbnail(s)` on every hover; `thumbnails_size a=0x3`). The apply path is flyout-only: `BuildFlyoutAlignedApplyOrder(target7, mapped3)` intersects down to 3 items, so the rule-#1 window (not among the 3) can never reach slot 1. Proof it never persists: the identical full-reversal move set (`Moving 2→1, 1→0, 2→1`) ran at 17:55:47 AND again at 17:56:35 — a persisted first apply would have logged "already correct". The real group order is never changed: the code always takes the "flyout-only safe pass" and never calls the existing full-group apply `ApplyOrderToTaskGroup(..., flyoutOnlyApply=false)`, which reorders the actual CTaskGroup DPA (holds ALL task items; see `GetTaskItemsArray`).

## Fix: reorder the real task group when flyout coverage is partial

1) In `ApplyFlyoutOnlyFromCacheImpl` (search it), inside the per-app loop after `targetOrder` is fetched and `taskGroup` resolved (trace `T28`): compute coverage BEFORE the `flyoutMapped.size() < 2` check:
```cpp
const std::vector<HWND> groupHwnds = GetHwndsInTaskGroup(taskGroup);
size_t inGroup = 0;
for (HWND h : targetOrder)
    if (std::find(groupHwnds.begin(), groupHwnds.end(), h) != groupHwnds.end()) inGroup++;
const bool topMapped = !targetOrder.empty() &&
    std::find(flyoutMapped.begin(), flyoutMapped.end(), targetOrder.front()) != flyoutMapped.end();
if (inGroup >= 2 && (!topMapped || flyoutMapped.size() < inGroup)) {
    Wh_Log(L"partial flyout coverage (%zu mapped / %zu in group); applying to group DPA",
           flyoutMapped.size(), inGroup);
    if (ApplyOrderToTaskGroup(taskGroup, desktopId, targetOrder, false)) {
        anyApplied = true;
        Wh_Log(L"group DPA reordered (%zu window(s))", inGroup);
        continue;  // skip the flyout-only path for this app
    }
    Wh_Log(L"group DPA reorder failed; falling back to flyout-only");
}
```
Notes: `GetHwndsInTaskGroup`, `ApplyOrderToTaskGroup` already exist (declared later in file — add forward decls next to the existing `extern`/forward decls above `ApplyFlyoutOnlyFromCacheImpl` if not visible: `std::vector<HWND> GetHwndsInTaskGroup(void* taskGroup); bool ApplyOrderToTaskGroup(void* taskGroup, const GUID& desktopId, const std::vector<HWND>& targetOrder, bool flyoutOnlyApply);`). Add `#include <algorithm>` to top include block if missing.

2) Same guard in `AutoReorder_TryApplyPendingXamlFlyoutOrder`: the queued orders are already truncated to 3. Better: stop queuing truncated orders — in `ApplyFlyoutOnlyFromCacheImpl` and `QueuePendingFromEngineCache` (search), queue `targetOrder` (full) instead of `flyoutOrder`/`BuildFlyoutAlignedApplyOrder(...)` results when coverage is partial (`flyoutMapped.size() < inGroup`). `TryApplyXamlFlyoutOrder` already intersects at apply time via `BuildDesiredFlyoutHwndOrder`, so full lists are safe to queue.

3) Thread safety: the 17:55:47 apply ran on a timer thread (tid 10672) — `MoveTaskInGroup`/`TaskInclusionChanged` touch taskbar UI objects. Keep `ApplyOrderToTaskGroup` calls on whatever thread the current code already uses for the full path (`RunAutoReorder` timer path does the same today) — no new threading code.

4) Logging (must-have; next log must settle any residual doubt): inside the new guard, dump BOTH sides — loop `flyoutMapped`: `Wh_Log(L"mapped[%zu] hwnd %p", i, h)`; loop `groupHwnds`: `Wh_Log(L"group[%zu] hwnd %p", i, h)`. With the engine's ranked-HWND lines, these three lists pin whether the rule-#1 hwnd is missing from the mapping, the group, or neither. `ApplyOrderToTaskGroup` already logs counts, `already correct`, and move failures.

5) Bump `@version` to `1.3.18`.

## Do NOT change
`BuildFlyoutAlignedApplyOrder`, `TryApplyXamlFlyoutOrder` internals, `MoveTaskInGroup`, engine/UIA, trigger/listener/ack/retry (all working — log proves delivery + engine OK). Don't touch `mods/taskbar-thumbnail-reorder.wh.cpp`. No pushes.

## Validate (fileLogEnabled on)
1. 7 Floorp windows open; run AHK trigger WITHOUT touching the taskbar: expect `trigger msg received` → `computed order (7 window(s))` → `partial flyout coverage` → `group DPA reordered (7 window(s))`.
2. Hover Floorp taskbar button: slot 1 must be the YouTube window; log must show `already correct` or no move lines (NOT the `2→1, 1→0, 2→1` reversal again).
3. Re-trigger: second pass must log `already correct`/idempotent skip.
4. Manual drag a thumbnail, re-hover: drag order must persist (no pending re-apply).
If step 1 shows `group DPA reorder failed` or `(N in group)` with N<7, capture the log and stop — do not improvise.
