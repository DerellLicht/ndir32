# ndir32 directory-tree bug fix

*2026-09-27*

## You

I would like your assistance in analyzing and fixing a bug in the directory-tree function of my `ndir32` color directory lister.  I have prepared a document, included in this post, that hopefully will explain the situation clearly; that document is `dirtree.info.md`; please review it and the attached files, then we can proceed from there...

gods... I hope I covered everything, and that I actually made sense...

![ndir.tree.fault.jpg: file not found](<ndir.tree.fault.jpg>)

*? Attached file: `ndir.tree.list.txt` (not included in export — save manually if needed)*

*? Attached file: `target.output.txt` (not included in export — save manually if needed)*

*? Attached file: `conio32.h` (not included in export — save manually if needed)*

*? Attached file: `conio32.cpp` (not included in export — save manually if needed)*

*? Attached file: `nio.cpp` (not included in export — save manually if needed)*

*? Attached file: `tdisplay.cpp` (not included in export — save manually if needed)*

*? Attached file: `dirtree.info.md` (not included in export — save manually if needed)*

*? Attached file: `ndir.tree.list.txt` (not included in export — save manually if needed)*

*? Attached file: `target.output.txt` (not included in export — save manually if needed)*

*? Attached file: `conio32.h` (not included in export — save manually if needed)*

*? Attached file: `conio32.cpp` (not included in export — save manually if needed)*

*? Attached file: `nio.cpp` (not included in export — save manually if needed)*

*? Attached file: `tdisplay.cpp` (not included in export — save manually if needed)*

*? Attached file: `dirtree.info.md` (not included in export — save manually if needed)*

## Claude

*? Used tool: Write*

Found it. The bug's in `dscroll()` in conio32.cpp, line 328-330:

```cpp
CHAR_INFO ci ;
ci.Char.AsciiChar = ' ' ;
ci.Attributes = tBG ;
```

`ci` is never initialized, and this build is Unicode (TCHAR = wchar_t — confirmed by `dputsi()`'s use of `std::wstring` and `WideCharToMultiByte`). That means `ScrollConsoleScreenBufferW` is the actual API called, and it reads the fill character as `CHAR_INFO.Char.UnicodeChar` (2 bytes), not `.AsciiChar` (1 byte). `Char` is a union, so writing only `.AsciiChar` sets the low byte to `' '` (0x20) and leaves the high byte as whatever garbage was already sitting on the stack — because `ci` was never zero-initialized.

That's exactly your symptom:

- Screen buffer is 2000 rows (`dwSize.Y = 2000`, per your syslog comment).
- `dnewline()` only calls `dscroll()` once the cursor hits `dwSize.Y - 1` — i.e. once you're at/past row 1999, which is why the corruption starts right around line 2000.
- Every scroll blanks the vacated bottom row with `ci` as the fill cell — but since the top byte of `Char.UnicodeChar` is uninitialized garbage, every "blank" row gets filled with garbage wide characters instead of real spaces. That's the corrupted glyphs you're seeing in the screenshot (they look like fragments of CJK characters — that's literally stack garbage being interpreted as UTF-16 code units).
- Redirected output never goes through `dscroll()` at all (it takes the `WriteFile`/UTF-8 branch in `dputsi()`), which is exactly why `target.output.txt` came out perfect at 4600+ lines — this bug is purely a screen-scrolling issue, invisible when redirected.

Fix: initialize `ci` and set `UnicodeChar`, not `AsciiChar`:

*? Used tool: Read*

Good, that file's readable. Let me apply the fix directly.

*? Used tool: Edit*

*? Used tool: SendUserFile*

That's the whole fix — one function, `dscroll()` in conio32.cpp. Nothing else in the chain (`ncrlf`, `dnewline`, `display_dir_tree`) needed to change; the scrolling logic and buffer-vs-screen math (Jason Hood's fix) were already correct. It was purely the uninitialized `CHAR_INFO` union member.

A couple of side notes while I was in there, not required for the fix:

- `dnewline()`'s scroll-trigger comment ("*this* probably shouldn't use dwSize.Y either...") is fine as-is — `dwSize.Y` is the full buffer height (2000), which is exactly what you want to compare against for this bug, since the corruption is a buffer-wrap issue, not a window-height issue.
- If you ever want a second line of defense, you could `memset(&ci, 0, sizeof(ci))` explicitly instead of `{ }` — same effect in C++, just more visually obvious to a reader who isn't fluent in aggregate-init rules. I used `{ }` to keep the diff minimal.

Let me know once you've rebuilt and tested past line 2000 — if the corruption is gone this was it, but if anything's still off past the buffer wrap I'd want to double check `SetConsoleCursorPosition`'s handling right at `dwSize.Y - 1`.

*? Attached file: `conio32.cpp` (not included in export — save manually if needed)*

## You

Wow... just Wow...
So I first tested on `d:\SourceCode\win32`, which has about 4600 folders, and it ran clean...
Then I tested on `d:\SourceCode`, which has over 200,000 folders, and it ran clean... it took quite a while to output, but it ran clean!!

So it was that simple???
And that's why I never saw it in the past, because I only started supporting Unicode in the past year or so, when I ended up with some Unicode filenames due to the Stalker games coming out of Ukraine...

Okay, I would ask one more thing... I have a watered-down version of `conio32.cpp` in `der_libs`, called `conio_min.cpp`; it has everything that this file has, except for screen-positioning code; it outputs in color, supports redirection, but is intended for direct console output...

So could you make a variant of your fix that supports *both* UNICODE and ASCII ??  Just gate them on UNICODE

## Claude

That's it exactly — the bug was dormant until you had wide characters actually landing on-screen at volume, but the `Char` union field being wrong doesn't care whether wide chars are in the *data* or not — it just needed enough scroll cycles (i.e. buffer wraps) to expose garbage often enough to notice, and Unicode support is what made 2000+ line trees more common/likely to hit it. Nice catch on your part connecting the STALKER-filename timing.

For `conio_min.cpp` — I don't have that file in front of me (only the four you originally attached). Since `dscroll()` is a WinAPI-level construct, not an ndir-specific one, here's the UNICODE-gated version you can drop straight in if the function is the same or nearly the same shape as the one in `conio32.cpp`:

```cpp
static void dscroll(WORD tBG)
{
   SMALL_RECT src ;
   COORD co = { 0, 0 } ;
   CHAR_INFO ci = { } ;      //  zero-init so no field is left as garbage
#ifdef UNICODE
   ci.Char.UnicodeChar = L' ' ;
#else
   ci.Char.AsciiChar = ' ' ;
#endif
   ci.Attributes = tBG ;

   // Jason Hood's "buffer > screen" fix
   src.Left   = 0 ;
   src.Right  = sinfo.dwSize.X - 1;
   src.Top    = 1 ;
   src.Bottom = sinfo.dwSize.Y - 1;

   ScrollConsoleScreenBuffer(hStdOut, &src, 0, co, &ci) ;
}
```

The two things that matter, gated or not:
1. `ci` must be zero-initialized (`= { }`) regardless of branch — that's what kills the stack-garbage problem, independent of Unicode.
2. Which union member you write must match whichever `ScrollConsoleScreenBuffer[A|W]` the compiler resolves to, which is exactly what `#ifdef UNICODE` picks for you (same macro that already gates your `TCHAR`/`_T()` usage elsewhere).

If you attach `conio_min.cpp` I'll apply this directly in its actual context (variable names, any wrapper differences, etc.) rather than you having to hand-mer

## You

Wow... just Wow...
So I first tested on `d:\SourceCode\win32`, which has about 4600 folders, and it ran clean...
Then I tested on `d:\SourceCode`, which has over 200,000 folders, and it ran clean... it took quite a while to output, but it ran clean!!

So it was that simple???
And that's why I never saw it in the past, because I only started supporting Unicode in the past year or so, when I ended up with some Unicode filenames due to the Stalker games coming out of Ukraine...

Okay, I would ask one more thing... I have a watered-down version of `conio32.cpp` in `der_libs`, called `conio_min.cpp`; it has everything that this file has, except for screen-positioning code; it outputs in color, supports redirection, but is intended for direct console output...

So could you make a variant of your fix that supports *both* UNICODE and ASCII ??  Just gate them on UNICODE macro, which I define.

## Claude

Makes sense — that timeline lines up exactly (uninitialized garbage only reads as visible corruption when `UnicodeChar`'s high byte happens to be nonzero, which becomes near-certain once you're doing thousands of scrolls, but the bug existed the whole time; it just didn't matter until TCHAR became wchar_t).

Here's the portable version, gated on the standard `UNICODE` macro:

*? Used tool: Read*

*? Used tool: Edit*

*? Used tool: SendUserFile*

That's the gated version — `ScrollConsoleScreenBuffer` is itself a TCHAR-mapped macro (resolves to the `W` or `A` entry point depending on `UNICODE`), so the fix just has to match: zero-init `ci`, then set `Char.UnicodeChar` under `#ifdef UNICODE` or `Char.AsciiChar` otherwise.

For `conio_min.cpp` — you didn't attach that one, so I don't have its actual `dscroll()` (or whatever it's called there) to patch directly. If it's structurally the same function, just drop in the same pattern:

```cpp
CHAR_INFO ci = { } ;
#ifdef UNICODE
   ci.Char.UnicodeChar = L' ' ;
#else
   ci.Char.AsciiChar = ' ' ;
#endif
ci.Attributes = tBG ;
```

If you'd rather I patch the actual file, attach `conio_min.cpp` and I'll apply it directly and hand back the whole file, same as here.

*? Attached file: `conio32.cpp` (not included in export — save manually if needed)*

## You

No, I can handle that... 
THANK YOU SO MUCH !!!!!!!!!!!!!!!  I was utterly baffled by this issue... I no longer have *any* understanding of the core console concepts any more, two decades after I was originally developing it...
I owe you *much* karma; I gotta figure out how to pay that to a software entity!!!

## Claude

Ha — I'll take payment in the form of you finding another gnarly two-decade-old bug and dragging me into it. Glad this one landed cleanly, and good luck getting the fix folded into `conio_min.cpp`.

## You

I would like an explanation of one more detail, please...
Did this have *anything* to do with the fact that the console buffer was 2000 lines deep??  The fault started happening around line 2000, but this fix doesn't look like it has anything to do with that??

## Claude

Good catch to double check — yes, the 2000 is directly relevant, just not as the cause. It's the *trigger condition*, and the fix doesn't need to reference it because the bug itself has nothing to do with that number.

Here's the chain:

- `dwSize.Y` (2000, in your console) is the total depth of the screen buffer — not a hardcoded constant anywhere in your code, just whatever your console window's buffer height happens to be set to.
- `dnewline()` only calls `dscroll()` once the cursor reaches `sinfo.dwCursorPosition.Y >= (sinfo.dwSize.Y - 1)` — i.e., row 1999. Every line before that, `dnewline()` just does `sinfo.dwCursorPosition.Y++` and returns; `dscroll()` is never touched.
- `dscroll()` is the *only* place in the whole file that calls `ScrollConsoleScreenBuffer` and builds a `CHAR_INFO` fill cell — and it's the only place with the union bug.

So the 2000 is why the bug was invisible for the first ~2000 lines and then suddenly live for every line after that — it's the threshold where `dscroll()` starts getting invoked at all, and once it's invoked, it's invoked on every single subsequent newline (since once you're pinned at the bottom row, every `ncrlf()` scrolls). That's also why the corruption didn't get worse over time or fade in — it hit at 100% the instant you crossed that line, because it's not a probabilistic or resource-exhaustion bug, it's "wrong union member, every single call, starting now."

If your buffer had been 500 lines deep, you'd have seen it at line 500. If it had been 10,000, you'd never have hit it in the `win32` test run at all. The number 2000 belongs entirely to your console configuration — the fix works identically regardless of what that number is, which is why it doesn't appear anywhere in the corrected code.

## You

Very good!!  Thank you again, I understand perfectly...

## Claude

Glad it clicked. Good luck with the `conio_min.cpp` patch — enjoy the rest of your Sunday!
