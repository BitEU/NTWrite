# WordStar 4.0 Clone for Windows Console

A faithful recreation of the classic WordStar 4.0 word processor for the Windows command prompt.

## Features Implemented

### Basic Editing
- Insert/Overtype modes (^V to toggle)
- Character, word, line, and block operations
- Auto-indent support
- Tab support with configurable tab width

### Cursor Movement
- **Character**: Arrow keys or ^S/^D/^E/^X
- **Word**: ^A (left), ^F (right)
- **Line**: Home/End or ^QS/^QD
- **Page**: PgUp/PgDn or ^R/^C
- **Document**: ^QR (top), ^QC (bottom)
- **Scrolling**: ^W (up), ^Z (down)

### Block Operations (^K Menu)
- **^KB**: Mark block begin
- **^KK**: Mark block end
- **^KC**: Copy block
- **^KV**: Move block
- **^KY**: Delete block
- **^KH**: Hide block markers
- **^KW**: Write block to file
- **^KR**: Read file at cursor
- **^K0-9**: Set markers 0-9

### Quick Movement (^Q Menu)
- **^QF**: Find text
- **^QA**: Find and replace
- **^QR**: Go to beginning of file
- **^QC**: Go to end of file
- **^QB**: Go to block begin
- **^QK**: Go to block end
- **^QY**: Delete to end of line
- **^QI**: Go to line number
- **^Q0-9**: Go to markers 0-9

### Formatting (^O Menu)
- **^OL**: Set left margin at cursor
- **^OR**: Set right margin at cursor
- **^OP**: Set paragraph margin at cursor
- **^OW**: Toggle word wrap
- **^OJ**: Toggle justify
- **^OC**: Center current line
- **^OT**: Toggle ruler display
- **^OF**: Toggle auto-indent

### Other Commands
- **^B**: Reform paragraph
- **^G**: Delete character
- **^H**: Backspace
- **^T**: Delete word right
- **^Y**: Delete entire line
- **^N**: Insert blank line
- **^L**: Repeat find
- **^J**: Help (brief)

### File Operations
- **^KS**: Save file
- **^KD**: Save file (done)
- **^KX**: Save and exit
- **^KQ**: Quit without saving

## Key Differences from Original

### Visual Enhancements
- Color-coded status line
- Block highlighting in blue
- Optional ruler line showing margins and tab stops
- Real-time status messages

### Modern Conveniences
- Long filename support
- Larger file capacity
- Standard Windows key mappings work alongside WordStar commands
- Mouse support (not implemented yet)

## Building

Requires Visual Studio Build Tools or Visual Studio with C++ support.

```cmd
nmake
```

Or use the provided makecmd.txt:
```cmd
cmd /c < makecmd.txt
```

## Usage

```cmd
wordstar [filename]
```

If no filename is specified, starts with a new file called UNTITLED.TXT.

## Technical Details

- Pure C implementation
- Uses Windows Console API directly
- No external dependencies
- No .NET Framework required
- Runs on any Windows 10 or later system

## Memory Model

- Dynamic line allocation
- No fixed line length limits
- Efficient memory usage for large files
- Automatic buffer expansion

## Known Limitations

1. **Undo/Redo**: Not implemented (^QL restore line is a placeholder)
2. **Print formatting**: ^P commands insert markers but don't affect display
3. **Mail merge**: Not implemented
4. **Spell check**: Not implemented
5. **Column blocks**: Line blocks only
6. **Macros**: Not implemented
7. **Multiple windows**: Single window only

## Future Enhancements

- Multi-level undo/redo
- Column block mode
- Search/replace with regex
- Multiple buffers/windows
- Macro recording and playback
- Printer support
- Configuration file support
- Syntax highlighting
- UTF-8 support

## Keyboard Reference

### Control Key Combinations

| Key | Function | Key | Function |
|-----|----------|-----|----------|
| ^A | Word left | ^N | Insert line |
| ^B | Reform paragraph | ^O | Format menu |
| ^C | Page down | ^P | Print menu |
| ^D | Cursor right | ^Q | Quick menu |
| ^E | Cursor up | ^R | Page up |
| ^F | Word right | ^S | Cursor left |
| ^G | Delete char | ^T | Delete word |
| ^H | Backspace | ^U | (Unused) |
| ^I | Tab | ^V | Insert toggle |
| ^J | Help | ^W | Scroll up |
| ^K | Block menu | ^X | Cursor down |
| ^L | Find next | ^Y | Delete line |
| ^M | Enter | ^Z | Scroll down |

## License

GPL v3 - See LICENSE file

## Acknowledgments

Based on the original WordStar 4.0 by MicroPro International Corporation.