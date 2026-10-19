# Handoff 2: Position-1 thumbnail not first after reorder (fork 1.3.17)

**Repo:** https://github.com/codychen98/windhawk-mods  
**Package:** `handoff/taskbar-thumbnail-reorder-postmessage/`  
**Prior brief:** [`handoff.md`](handoff.md) (PostMessage delivery at startup).  
**This brief is facts + process only.** Do not treat any diagnosis, root cause, or fix direction as given — you must determine those yourself from the code and log.

**Audience:** Smart investigator. After intake and your own analysis, write a ≤5000-character local `implementation_handoff.md` for a weaker implementer. Work only in a local clone; **never** push, create repos, or fork under the user’s GitHub.

---

## Operating rules

1. **First action:** `git clone https://github.com/codychen98/windhawk-mods.git` into the current directory; work only inside that clone.
2. **Never** `git push`, `gh repo create`, `gh pr create`, or create forks/repos on the user’s account.
3. **Primary deliverable:** local `implementation_handoff.md` (**≤ 5000 characters**) for a weaker model that will edit the fork. That short file is where *you* record your conclusions and edit plan — not this brief.

---

## HARD GATE — read before you analyze

| # | File |
|---|------|
| 1 | This brief |
| 2 | [`taskbar-reorder-log.txt`](taskbar-reorder-log.txt) — session log from the failing run |
| 3 | [`taskbar-thumbnail-reorder-fork.wh.cpp`](taskbar-thumbnail-reorder-fork.wh.cpp) — **v1.3.17** source under test |
| 4 | [`mod_settings.json`](mod_settings.json) |
| 5 | [`ahk_startup_trigger.ahk`](ahk_startup_trigger.ahk) |
| 6 | [`NOTES_CODEMAP.md`](NOTES_CODEMAP.md) — may lag the source; verify against the `.wh.cpp` |
| 7 | [`mods/taskbar-thumbnail-reorder.wh.cpp`](../../mods/taskbar-thumbnail-reorder.wh.cpp) — upstream non-fork baseline |

GitHub:

- https://github.com/codychen98/windhawk-mods/tree/main/handoff/taskbar-thumbnail-reorder-postmessage  
- https://github.com/codychen98/windhawk-mods/blob/main/handoff/taskbar-thumbnail-reorder-postmessage/handoff_2.md  
- https://github.com/codychen98/windhawk-mods/blob/main/handoff/taskbar-thumbnail-reorder-postmessage/taskbar-reorder-log.txt  

**First reply:** confirm clone path; confirm no-push; list each file above as read; then begin your own investigation. Do not invent file contents.

---

## User-reported symptom

After the external PostMessage trigger (and interacting with the taskbar flyout as needed), the Floorp window that should be thumbnail **position 1** per the rules (`"YouTube"` → `position: 1` in settings) did **not** end up in the first slot of the taskbar thumbnail flyout.

---

## Settings in effect (from files, not interpretation)

From [`mod_settings.json`](mod_settings.json):

- `autoReorderOnModLoad`: 0  
- `externalPostMessageTriggerEnabled`: 1  
- `hotkeyEnabled`: 0  
- `delayAfterModReadySeconds`: 20  
- Rules app: `floorp.exe`  
- Match positions: YouTube=1, AnimeKAI=2, MakerWorld=3, XDA Forums=4, Gmail=5, Pulse SMS=6, Aliexpress=7  

Mod file version under test: **1.3.17** (`@version` in the fork `.wh.cpp`).

---

## Observed log facts (verbatim themes from the session file)

Evidence: [`taskbar-reorder-log.txt`](taskbar-reorder-log.txt).  
Privacy note: the log contains HWND values and tab **counts**; it does **not** contain tab title strings.

Chronological observations from that file (read the full log yourself):

1. Mod load; listener started with PostMessage enabled; `PostMessage window ready`.
2. Later: `trigger msg received` then `trigger from PostMessage` then a manual/test pass.
3. Log line: `IVirtualDesktopManager unavailable (0x80040154); using single-desktop mode`.
4. Log line: `floorp.exe computed order (7 window(s)):` followed by ranks `#1` through `#7` each with an HWND and a tab-title count.
5. Log line: `order cached`.
6. Apply path logs include: queuing / session apply pending / hover-to-apply messaging; deferred timer; `flyout not open` on at least one early apply attempt.
7. On flyout-related activity: `cached flyout task group … (3 thumbnail(s))`; traces with `flyout_mapped_count` showing **3**; `updated Win11 flyout preview (3 item(s))`; `Moving from …` lines appear.
8. Some hover paths log `T78b try_pending:exit not_queued`.
9. A second `trigger msg received` occurs later; again `computed order (7 window(s))` with the same seven HWND ranks in the excerpt; queue/apply messages referring to **3** window(s); at least one `T70b xaml_apply:exit no_thumbnails` / “not ready yet”; a later hover logs `applied queued flyout preview reorder (3 window(s))` with further `Moving from …` lines.
10. Many `TaskItemFilter_IsTaskAllowed_Hook` lines appear throughout.
11. Session ends with `=== mod unload ===`.

These are observations only. What they imply for root cause is for you to decide.

---

## Your job (no predetermined conclusion)

1. Using the fork source and the log, determine the root cause of the user-reported symptom.  
2. Decide the minimal correct fix (or fixes).  
3. Write `implementation_handoff.md` (≤5000 characters) with enough concrete edit guidance for a weaker model to change `taskbar-thumbnail-reorder-fork.wh.cpp` only.  
4. Do **not** modify `mods/taskbar-thumbnail-reorder.wh.cpp` unless you have a clear, justified reason documented in that short handoff.

---

## Out of scope for process

- Publishing to GitHub (push / PR / new repo / fork).  
- Using this brief’s wording as a substitute for reading the log and source.
