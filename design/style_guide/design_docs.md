# Style Guide for Design Files

<!-- Remember to update code example in ## Table of Contents -->
<!-- Table of Contents only links to level 2 headers -->
\[ [Table of Contents](#table-of-contents) \]
\[ [Inline Code](#inline-code) \]
\[ [Note and ToDO](#note-and-todo) \]
\[ [Alerts / Callouts / Admonitions](#alerts--callouts--admonitions) \]

## Table of Contents

The auto-generated bullet point list has very limited control over content and
styling with editors like vscode where the list is automatically updated. A
cleaner solution is a single line linking to the primary sections of the
document, showing only a single level (eg. all level 2 headers).

**Example** this is the ToC for this document. A comment is included to explain
what headers are included for future additions or changes.

```markdown
<!-- Table of Contents only links to level 2 headers -->
\[ [Table of Contents](#table-of-contents) \]
\[ [Inline Code](#inline-code) \]
\[ [Note and ToDO](#note-and-todo) \]
\[ [Alerts / Callouts / Admonitions](#alerts--callouts--admonitions) \]
```

One drawback is the need for manual updates, however these headers are not
frequently updated for well established documents.

## Inline Code

> [!NOTE]
> I'm feeling conflicted after writing this section. A the time of writing I was
> using dark mode. Light mode has better contrast, reducing the issue to a level
> where inline code is acceptable. I will have to put more thought into it's use
> before concluding this section.
>
> _2026-07-01_ I had another revelation. Inline code is for that, code. It can
> be used for names and text. Most of the examples I posed relate to numbers and
> values. **0x00** is very different from `name`.

<!-- Why it's good -->
Inline code can be used to display potentially ambiguous characters (eg. lower
and upper case L, letter O and number 0) and to distinguish text from it's
surroundings like hexadecimal numbers (eg. `0x123`). It's use is not mandatory
and typically used as needed rather than by default.

<!-- Why it's bad -->
Inline code, however, reduce the contrast between text and background
making long strings of repeated characters more difficult to read (eg. `0000` vs
0000) or close, similar patterns of characters (eg. `0x0000` near `0x0600` vs
0x0000 near 0x06000). In most case, **bold** can be used to distinguish text
from it's surroundings (eg. **0x00000** near **0x06000**). Avoid using inline
code for tables, near formulas, in headers and in links. Exceptions can be made
when deemed necessary. Put a comment `<!-- -->` near these exceptional use cases
to explain why they are appropriate.

<!-- Examples, using text with bold instead of headers to avoid wasted space -->
**Good Usage** - Inline hexadecimal numbers with area between

> Kernel Size in Protected Mode is smaller than in real mode. Reserved memory in
> protected mode starts at `0x9fc00` while real mode starts at `0xa0000`

**Bad Usage** - Hexadecimal column values in a table, smaller font with lower contrast

> | Start    | End       | size      | description                            |
> | -------- | --------- | --------- | -------------------------------------- |
> | `0x0000` | `0x4ff`   | 1.25 KiB  | Bad, small font with low contrast      |
> | `0x0500` | `0x00fff` | 1.25 KiB  | Bad, small font with low contrast      |

Compared to

> | Start     | End      | size      | description                            |
> | -------- | --------- | --------- | -------------------------------------- |
> | 0x7000   | 0x07bff   | 3 KiB     | Good, larger font with higher contrast |
> | 0x7c00   | 0x07dff   | 512 bytes | Good, larger font with higher contrast |

**Bad Usage** - Mix of numbers with plain text numbers and inline code

>- Page size = 4096 (`0x1000`)
>- Bits per page = Page Size * 8 = 4096 * 8 = 32768 (`0x8000`)
>- Max pages per region = Bits per page = 32768 (`0x8000`) (includes bitmask page)
>- Max region size = Max pages per region * Page size = 32768 * 4096 = 134217728
>  (`0x8000000`) = 128 MiB

**Bad Usage** - Numbers in formula without the operators

> Bits per page = Page Size * `8` = `4096` * `8` = `32768` (`0x8000`)

If you're going to quote a formula, include all operators and numbers in a
single quote. Exceptions can be made where impractical.

**Good Usage** - Formula including numbers and operators

> Bits per page = `Page Size * 8` = `4096 * 8` = `32768 (0x8000)`

**Bad Usage** - Quote entire line

> `Bits per page = Page Size * 8 = 4096 * 8 = 32768 (0x8000)`

If a quote spans the entire line, try to use a code block instead. Code blocks
use a larger font with higher contrast between letters and background.

> ```
> Bits per page = Page Size * 8 = 4096 * 8 = 32768 (0x8000)
> ```

## Note and ToDo

An alternative to alerts ia **Note** and **ToDo** (can also be upper case
**NOTE** and **TODO**). These are bold at the start of a paragraph followed by
non-bold text. **Note** is typically used to communicate details that may be
unintuitive or not obvious, but are not important enough to ues an alert.
**ToDo** is typically used as a note or reminder to the writer for future
content and communicates to the reader that a section may be incomplete or
inaccurate.

When used with all caps, **TODO** can also be highlighted by vscode using the
[TODO Highlights](https://marketplace.visualstudio.com/items?itemName=wayou.vscode-todo-highlight)
extension.

## Alerts / Callouts / Admonitions

Alerts can be useful to call attention to some information or visually break up
sections of text. To retain meaning and effectiveness at capturing the readers
attention, alerts should be uses sparingly and with short content.

**Note** In vscode, alerts can have an altered title by following the type `[!...]` with
some text. This fails to render in GitHub so should be avoided.

> [!NOTE]
> Note block

> [!TIP]
> Tip block

> [!IMPORTANT]
> Important block

> [!WARNING]
> Warning block

> [!CAUTION]
> Caution block
