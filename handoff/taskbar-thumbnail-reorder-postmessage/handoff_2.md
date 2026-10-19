# Handoff 2: Position-1 thumbnail not applied in flyout (fork 1.3.17)

**Repo:** https://github.com/codychen98/windhawk-mods  
**Package:** `handoff/taskbar-thumbnail-reorder-postmessage/`  
**Prior handoff:** [`handoff.md`](handoff.md) (PostMessage delivery) — **largely fixed** in 1.3.17. Do not re-litigate AHK delivery unless the log contradicts.

**Audience:** Smart investigator — find why the window that should be flyout slot **#1** is not shown first after trigger + hover. Produce a ≤5000-character `implementation_handoff.md` for a weaker model. Local work only; **never** push / create repos / fork under the user’s GitHub.

---

## Operating rules (same as handoff 1)

1. **First action:** `git clone https://github.com/codychen98/windhawk-mods.git` into the current directory; work only in that clone.
2. **Never** `git push`, `gh repo create`, `gh pr create`, or create forks/repos on the user’s account.
3. **Primary deliverable:** local `implementation_handoff.md` (**≤ 5000 characters**) for a weaker implementer. Do not treat a full rewrite of the `.wh.cpp` as your deliverable.

---

## HARD GATE — read before diagnosing

| # | File | Why |
|---|------|-----|
| 1 | This brief | Scope + symptom |
| 2 | [`taskbar-reorder-log.txt`](taskbar-reorder-log.txt) | **Required evidence** (session where #1 failed) |
| 3 | [`taskbar-thumbnail-reorder-fork.wh.cpp`](taskbar-thumbnail-reorder-fork.wh.cpp) | Current **v1.3.17** code under test |
| 4 | [`mod_settings.json`](mod_settings.json) | Rules: `floorp.exe`, YouTube→pos1, … |
| 5 | [`ahk_startup_trigger.ahk`](ahk_startup_trigger.ahk) | Ack/`SendMessage` trigger (working) |
| 6 | [`NOTES_CODEMAP.md`](NOTES_CODEMAP.md) | Symbol map (may lag 1.3.17; verify in source) |
| 7 | [`mods/taskbar-thumbnail-reorder.wh.cpp`](../../mods/taskbar-thumbnail-reorder.wh.cpp) | Upstream baseline (drag-only) |

GitHub mirrors:

- Folder: https://github.com/codychen98/windhawk-mods/tree/main/handoff/taskbar-thumbnail-reorder-postmessage  
- Log: https://github.com/codychen98/windhawk-mods/blob/main/handoff/taskbar-thumbnail-reorder-postmessage/taskbar-reorder-log.txt  
- This brief: https://github.com/codychen98/windhawk-mods/blob/main/handoff/taskbar-thumbnail-reorder-postmessage/handoff_2.md  

**Intake confirmation (first reply):** list files read; one paragraph on what 1.3.17 already fixed; one paragraph summarizing the log timeline (trigger → compute 7 → apply 3); then root-cause work.

---

## Symptom (user)

After PostMessage trigger (and hover), the Floorp window that **should** be thumbnail position **1** (rules: `"YouTube"` → `position: 1`) is **not** moved to the first slot in the taskbar flyout.

PostMessage ack / listener path is **working** (see log). This is an **order compute vs flyout apply** problem, not “AHK never reaches the mod.”

---

## What the log already proves (do not ignore)

Evidence file: [`taskbar-reorder-log.txt`](taskbar-reorder-log.txt) (safe: HWND pointers + tab **counts** only; no tab title strings).

### Working (1.3.17)

- Listener ready; `trigger msg received`; `trigger from PostMessage`
- Engine runs: `floorp.exe computed order (7 window(s))` with ranks `#1`…`#7` and HWNDs
- `order cached` (retry gate satisfied)
- On hover: XAML moves run (`Moving from 2 to 1`, `1 to 0`, …) and `updated Win11 flyout preview (3 item(s))`

### Failure shape

1. **Compute sees 7 windows; flyout apply only sees 3 thumbnails**  
   - `computed order (7 window(s))`  
   - Later: `flyout_mapped_count … b=3`, `updated … (3 item(s))`, queue messages say `(3 window(s))`  
   - If engine `#1` HWND is **not** among the 3 HWNDs currently mapped into the open flyout, slot-1 in the rules sense can never appear as flyout index 0.

2. **Pending apply is brittle / often `not_queued` on hover**  
   - First hover path: `T78b try_pending:exit not_queued` then deferred apply still does moves from cache.  
   - Second trigger: queue for 3 windows; deferred apply hits `T70b xaml_apply:exit no_thumbnails` then “will retry on next hover”; later hover applies 3-item reorder.

3. **No log of which rule matched which HWND / which HWND is flyout index 0**  
   - Only `#N hwnd … (K tab title(s))` — investigator must add or infer mapping: engine order ∩ flyout-mapped HWNDs → desired XAML indices. Suspect **intersection / build_desired** drops or mis-orders the true position-1 window.

4. **`IVirtualDesktopManager unavailable (0x80040154); using single-desktop mode`**  
   - Note whether this wrongly merges or omits windows relative to the visible taskbar group.

5. Noise: many `TaskItemFilter_IsTaskAllowed_Hook` lines — ignore unless tied to apply.

---

## Investigation mandate

1. Trace `AutoReorder_ComputeAndLogOrders` → cache → `QueuePendingFromEngineCache` / `ApplyFlyoutOnlyFromCacheImpl` → `TryApplyXamlFlyoutOrder` / `build_desired` when **flyout has fewer thumbnails than cached order**.
2. Explain why user-visible slot 1 is wrong: wrong match, wrong intersection, apply only partial set, pending cleared too early, or drag/pending guards from 1.3.17 interacting badly.
3. Prefer a **root-cause fix** (correct desired order for visible thumbnails + ensure pos-1 HWND is included when it is on the taskbar group) over “hover longer” advice.
4. Specify **diagnostic logs**: for each flyout apply, log engine `#` rank, HWND, flyout index before/after, and which rule string matched (avoid logging full raw tab titles if possible; rule label is enough).
5. Encode the fix in ≤5000-char `implementation_handoff.md` for the weaker model. Target file: `taskbar-thumbnail-reorder-fork.wh.cpp` (bump version past 1.3.17). Do **not** edit `mods/taskbar-thumbnail-reorder.wh.cpp`.

---

## Settings reminder

From [`mod_settings.json`](mod_settings.json): `autoReorderOnModLoad=0`, PostMessage on, rules for `floorp.exe` with YouTube=1 … Aliexpress=7.

---

## Out of scope

- Reverting PostMessage ack/retry/file-log unless broken.  
- Catalog submission / upstream overwrite / any GitHub push.
