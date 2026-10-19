# Handoff 4: Startup reorder still wrong on 1.3.18 (facts only)

**Repo:** https://github.com/codychen98/windhawk-mods  
**Package:** `handoff/taskbar-thumbnail-reorder-postmessage/`  

**This brief is facts + process only.** It does not state a root cause or prescribe a fix. You must determine those from the source and logs.

**Audience:** Stronger investigator model. After intake and your own analysis, write a local `implementation_handoff.md` (**≤ 5000 characters**) for a weaker implementer. Work only in a local clone. **Never** push, create repos, or fork under the user’s GitHub.

---

## Operating rules

1. **First action:** `git clone https://github.com/codychen98/windhawk-mods.git` into the current directory; work only inside that clone.
2. **Never** `git push`, `gh repo create`, `gh pr create`, or create forks/repos on the user’s account.
3. **Primary deliverable:** local `implementation_handoff.md` (≤5000 characters) — your conclusions and edit plan for the weaker model go there, not here.

---

## HARD GATE — read before you analyze

| # | File |
|---|------|
| 1 | This brief (`handoff_4.md`) |
| 2 | [`new log/taskbar-reorder-log.txt`](new%20log/taskbar-reorder-log.txt) — **primary** session log (same content as `taskbar-reorder-log-1.3.18.txt`) |
| 3 | [`taskbar-thumbnail-reorder-fork.wh.cpp`](taskbar-thumbnail-reorder-fork.wh.cpp) — **v1.3.18** source under test |
| 4 | [`mod_settings.json`](mod_settings.json) |
| 5 | [`ahk_startup_trigger.ahk`](ahk_startup_trigger.ahk) |
| 6 | [`handoff_3.md`](handoff_3.md) — prior implementation brief that produced 1.3.18 (for history only; do not treat as proven) |
| 7 | [`handoff_2.md`](handoff_2.md) / [`handoff.md`](handoff.md) — earlier briefs (history) |
| 8 | [`taskbar-reorder-log.txt`](taskbar-reorder-log.txt) — earlier 1.3.17 session (optional contrast) |
| 9 | [`mods/taskbar-thumbnail-reorder.wh.cpp`](../../mods/taskbar-thumbnail-reorder.wh.cpp) — upstream non-fork baseline |

GitHub:

- Folder: https://github.com/codychen98/windhawk-mods/tree/main/handoff/taskbar-thumbnail-reorder-postmessage  
- This brief: https://github.com/codychen98/windhawk-mods/blob/main/handoff/taskbar-thumbnail-reorder-postmessage/handoff_4.md  
- Primary log: https://github.com/codychen98/windhawk-mods/blob/main/handoff/taskbar-thumbnail-reorder-postmessage/new%20log/taskbar-reorder-log.txt  
- Mirror (no spaces in path): https://github.com/codychen98/windhawk-mods/blob/main/handoff/taskbar-thumbnail-reorder-postmessage/taskbar-reorder-log-1.3.18.txt  

**First reply:** confirm local clone path; confirm no-push; list each required file as read; then investigate. Do not invent file contents.

---

## User problem (as reported)

1. On **Windows startup**, after Floorp windows load, the AHK script triggers the mod. The thumbnail that should be **position 1** (rules: YouTube → position 1) is **not** correct in the taskbar flyout after that startup trigger alone.
2. The user had to **manually run the AHK trigger again later**. After that, things appeared to work when checking the flyout.
3. The user asks whether the second success required **placing the cursor on the browser / taskbar** to inspect the preview (hover), or something else.

---

## Settings in effect (from files)

From [`mod_settings.json`](mod_settings.json):

- `autoReorderOnModLoad`: 0  
- `externalPostMessageTriggerEnabled`: 1  
- `hotkeyEnabled`: 0  
- `delayAfterModReadySeconds`: 20  
- App rules: `floorp.exe` with YouTube=1 … Aliexpress=7  

Mod under test: **`@version` 1.3.18** in [`taskbar-thumbnail-reorder-fork.wh.cpp`](taskbar-thumbnail-reorder-fork.wh.cpp).  
Desktop file logging was used; the session copy is [`new log/taskbar-reorder-log.txt`](new%20log/taskbar-reorder-log.txt).

---

## Observed facts from `new log/taskbar-reorder-log.txt`

(Log contains HWND pointers and tab **counts**; no tab title strings.)

### A. Startup / first AHK trigger (~18:31:54)

- `trigger msg received` → `trigger from PostMessage`
- `floorp.exe computed order (7 window(s)):` with ranks `#1`…`#7` and HWNDs
- `order cached`
- Apply path: `queuing flyout-only apply` / `session apply pending (hover the app's taskbar button to apply)`
- Deferred timer: `flyout-only safe pass` then **`flyout not open; hover the taskbar button to apply`**
- In this interval there is **no** log line `partial flyout coverage` and **no** `group DPA reordered`

### B. First hover activity (~18:32:28) — before the second AHK trigger

- `HoverFlyoutModel_TargetItemKey_Hook` and related thumbnail/group hooks appear
- Then: `partial flyout coverage (3 mapped / 7 in group); applying to group DPA`
- Log dumps `mapped[0..2]` HWNDs and `group[0..6]` HWNDs
- `ApplyOrderToTaskGroup`: `reordering 7 window(s) on desktop (3 in flyout mapping)`
- XAML path runs moves (`Moving from 2 to 1`, `Moving from 1 to 0`) and `updated Win11 flyout preview (3 item(s))`
- Same apply also logs: **`internal task-group moves skipped on thread 8476 (taskbar UI is 7808); XAML/pending flyout only`**
- Then logs: `group DPA reordered (7 window(s))` (appears after the “skipped” line in the same apply)

### C. Second AHK trigger (~18:32:57)

- Again `trigger msg received` / computed order 7 windows / `order cached`
- Queues pending with messages referring to **7** window(s) for the task group
- Deferred apply: pending try hits `T70b xaml_apply:exit no_thumbnails` / “not ready yet”
- Again `partial flyout coverage (3 mapped / 7 in group)`
- Again `internal task-group moves skipped on thread 8476 (taskbar UI is 7808)`
- Again `group DPA reordered (7 window(s))` after the skip line

### D. Later hover (~18:33:04)

- Hover hooks again; pending apply runs with `Moving from 2 to 1` / `1 to 0`
- `updated Win11 flyout preview (3 item(s))`
- `applied queued flyout preview reorder (7 window(s))` (message text says 7; XAML update line says 3 item(s))

### E. Other recurring lines

- `IVirtualDesktopManager unavailable (0x80040154); using single-desktop mode` (on first engine pass in this log)
- Many `TaskItemFilter_IsTaskAllowed_Hook` lines

---

## Context the user already has from prior rounds (history, not conclusions)

- Earlier handoffs addressed PostMessage delivery (ack/retry) and partial flyout vs 7-window engine order; 1.3.18 added a “partial coverage → ApplyOrderToTaskGroup(..., false)” path per [`handoff_3.md`](handoff_3.md).
- Primary evidence for **this** report is [`new log/taskbar-reorder-log.txt`](new%20log/taskbar-reorder-log.txt).

---

## Your job

1. Explain, from code + this log, what happened on startup vs after hover vs on the second trigger — including whether hover was required for apply, and what the “skipped on thread … / group DPA reordered” pair means for the user’s visible result.  
2. Decide the root cause(s) and a minimal fix.  
3. Write ≤5000-character `implementation_handoff.md` for a weaker model to edit **only** `taskbar-thumbnail-reorder-fork.wh.cpp` (bump version past 1.3.18).  
4. Do not modify `mods/taskbar-thumbnail-reorder.wh.cpp` unless you document a clear reason in that short handoff.

---

## Process out of scope

- GitHub push / PR / new repo / fork under the user’s account.  
- Treating this brief’s wording as a substitute for reading the log and source.
