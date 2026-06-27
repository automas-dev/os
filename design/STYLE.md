# Style Guide for Design Files

<!-- Remember to update code example in ## Table of Contents -->
<!-- Table of Contents only links to level 2 headers -->
\[ [Table of Contents](#table-of-contents) \]
\[ [Inline Code](#inline-code) \]

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
```

One drawback is the need for manual updates, however these headers are not
frequently updated for well established documents.

## Inline Code

<!-- Why it's good -->
Inline code can be used to display potentially ambiguous characters (eg. lower
and upper case L, letter O and number 0) and to distinguish text from it's
surroundings like hexadecimal numbers (eg. `0x123`).

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

- Page size = 4096 (`0x1000`)
- Bits per page = Page Size * 8 = 4096 * 8 = 32768 (`0x8000`)
- Max pages per region = Bits per page = 32768 (`0x8000`) (includes bitmask page)
- Max region size = Max pages per region * Page size = 32768 * 4096 = 134217728
  (`0x8000000`) = 128 MiB

**Bad Usage** - Numbers in formula without the operators

Bits per page = Page Size * `8` = `4096` * `8` = `32768` (`0x8000`)

If you're going to quote a formula, include all operators and numbers in a
single quote. Exceptions can be made where impractical.

**Good Usage** - Formula including numbers and operators

Bits per page = `Page Size * 8` = `4096 * 8` = `32768 (0x8000)`

**Bad Usage** - Quote entire line

`Bits per page = Page Size * 8 = 4096 * 8 = 32768 (0x8000)`

If a quote spans the entire line, try to use a code block instead. Code blocks
use a larger font with higher contrast between letters and background.

> ```
> Bits per page = Page Size * 8 = 4096 * 8 = 32768 (0x8000)
> ```
