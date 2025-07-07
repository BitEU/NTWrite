// main.c - WordStar 4.0 Clone for Windows Console
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

// Constants
#define VERSION "4.0"
#define MAX_LINE_LENGTH 256
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define STATUS_LINE 0
#define RULER_LINE 1
#define MENU_LINE (SCREEN_HEIGHT - 1)
#define EDIT_START 2
#define EDIT_END (SCREEN_HEIGHT - 2)
#define TAB_WIDTH 8
#define MAX_MARKERS 10
#define FIND_BUFFER_SIZE 80
#define REPLACE_BUFFER_SIZE 80

// Key codes
#define CTRL_A 0x01
#define CTRL_B 0x02
#define CTRL_C 0x03
#define CTRL_D 0x04
#define CTRL_E 0x05
#define CTRL_F 0x06
#define CTRL_G 0x07
#define CTRL_H 0x08
#define CTRL_I 0x09
#define CTRL_J 0x0A
#define CTRL_K 0x0B
#define CTRL_L 0x0C
#define CTRL_M 0x0D
#define CTRL_N 0x0E
#define CTRL_O 0x0F
#define CTRL_P 0x10
#define CTRL_Q 0x11
#define CTRL_R 0x12
#define CTRL_S 0x13
#define CTRL_T 0x14
#define CTRL_U 0x15
#define CTRL_V 0x16
#define CTRL_W 0x17
#define CTRL_X 0x18
#define CTRL_Y 0x19
#define CTRL_Z 0x1A

// Editor states
typedef enum {
    STATE_NORMAL,
    STATE_CTRL_K,
    STATE_CTRL_Q,
    STATE_CTRL_O,
    STATE_CTRL_P,
    STATE_FIND,
    STATE_REPLACE,
    STATE_GOTO_LINE,
    STATE_SAVE_AS
} EditorState;

// Text line structure
typedef struct Line {
    char *text;
    int length;
    int capacity;
    struct Line *next;
    struct Line *prev;
} Line;

// Document structure
typedef struct {
    Line *first_line;
    Line *current_line;
    int cursor_x;
    int line_count;
    int modified;
    char filename[MAX_PATH];
    // Place markers
    Line *markers[MAX_MARKERS];
    int marker_x[MAX_MARKERS];
} Document;

// Block marking
typedef struct {
    Line *start_line;
    Line *end_line;
    int start_col;
    int end_col;
    int active;
    int column_mode;  // For column blocks
} Block;

// Find/Replace
typedef struct {
    char find_text[FIND_BUFFER_SIZE];
    char replace_text[REPLACE_BUFFER_SIZE];
    int case_sensitive;
    int whole_words;
    int backwards;
    int global_replace;
} FindReplace;

// Format settings
typedef struct {
    int right_margin;
    int left_margin;
    int paragraph_margin;
    int tab_width;
    int word_wrap;
    int justify;
    int hyphenation;
    int line_spacing;
} Format;

// Editor structure
typedef struct {
    Document doc;
    Block block;
    FindReplace find;
    Format format;
    EditorState state;
    int top_line;
    int screen_col;
    HANDLE hConsoleIn;
    HANDLE hConsoleOut;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    char status_msg[256];
    char input_buffer[256];
    int input_pos;
    int insert_mode;
    int show_ruler;
    int auto_indent;
    // Clipboard for block operations
    Line *clipboard;
    int clipboard_lines;
} Editor;

// Function prototypes
void init_editor(Editor *ed);
void cleanup_editor(Editor *ed);
void init_console(Editor *ed);
void restore_console(Editor *ed);
void draw_screen(Editor *ed);
void draw_status_line(Editor *ed);
void draw_ruler_line(Editor *ed);
void draw_menu_line(Editor *ed);
void draw_text_area(Editor *ed);
void set_cursor_pos(Editor *ed, int x, int y);
void write_at(Editor *ed, int x, int y, const char *text, WORD attr);
void process_key(Editor *ed, KEY_EVENT_RECORD *key);
void handle_normal_key(Editor *ed, KEY_EVENT_RECORD *key);
void handle_ctrl_k(Editor *ed, KEY_EVENT_RECORD *key);
void handle_ctrl_q(Editor *ed, KEY_EVENT_RECORD *key);
void handle_ctrl_o(Editor *ed, KEY_EVENT_RECORD *key);
void handle_ctrl_p(Editor *ed, KEY_EVENT_RECORD *key);
void handle_input_state(Editor *ed, KEY_EVENT_RECORD *key);
Line *create_line(void);
void free_line(Line *line);
void insert_char(Editor *ed, char ch);
void delete_char(Editor *ed);
void backspace_char(Editor *ed);
void move_cursor_left(Editor *ed);
void move_cursor_right(Editor *ed);
void move_cursor_up(Editor *ed);
void move_cursor_down(Editor *ed);
void move_word_left(Editor *ed);
void move_word_right(Editor *ed);
void move_line_start(Editor *ed);
void move_line_end(Editor *ed);
void move_page_up(Editor *ed);
void move_page_down(Editor *ed);
void move_doc_start(Editor *ed);
void move_doc_end(Editor *ed);
void scroll_up(Editor *ed);
void scroll_down(Editor *ed);
void delete_line(Editor *ed);
void delete_word_right(Editor *ed);
void delete_to_eol(Editor *ed);
void new_line(Editor *ed);
void save_file(Editor *ed);
void save_file_as(Editor *ed, const char *filename);
void load_file(Editor *ed, const char *filename);
void mark_block_begin(Editor *ed);
void mark_block_end(Editor *ed);
void copy_block(Editor *ed);
void move_block(Editor *ed);
void delete_block(Editor *ed);
void write_block(Editor *ed, const char *filename);
void read_block(Editor *ed, const char *filename);
void hide_block(Editor *ed);
int is_line_in_block(Editor *ed, Line *line);
void update_status(Editor *ed, const char *msg);
void find_text(Editor *ed);
void find_next(Editor *ed);
void replace_text(Editor *ed);
void goto_line(Editor *ed, int line_num);
void set_marker(Editor *ed, int marker);
void goto_marker(Editor *ed, int marker);
void reform_paragraph(Editor *ed);
void center_line(Editor *ed);
int get_line_number(Editor *ed, Line *line);
void clear_clipboard(Editor *ed);
Line *duplicate_lines(Line *start, Line *end, int *count);
void delete_lines(Editor *ed, Line *start, Line *end);
void insert_lines(Editor *ed, Line *lines, int count);

// Initialize editor
void init_editor(Editor *ed) {
    memset(ed, 0, sizeof(Editor));
    ed->doc.first_line = create_line();
    ed->doc.current_line = ed->doc.first_line;
    ed->doc.line_count = 1;
    ed->insert_mode = 1;
    ed->show_ruler = 1;
    ed->state = STATE_NORMAL;
    strcpy(ed->doc.filename, "UNTITLED.TXT");
    
    // Initialize format settings
    ed->format.right_margin = 65;
    ed->format.left_margin = 1;
    ed->format.paragraph_margin = 1;
    ed->format.tab_width = TAB_WIDTH;
    ed->format.word_wrap = 1;
    ed->format.justify = 0;
    ed->format.line_spacing = 1;
    
    // Initialize markers
    for (int i = 0; i < MAX_MARKERS; i++) {
        ed->doc.markers[i] = NULL;
        ed->doc.marker_x[i] = 0;
    }
    
    init_console(ed);
}

// Initialize console
void init_console(Editor *ed) {
    DWORD mode;
    
    ed->hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
    ed->hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Save current console state
    GetConsoleScreenBufferInfo(ed->hConsoleOut, &ed->csbi);
    
    // Set console mode for raw input
    GetConsoleMode(ed->hConsoleIn, &mode);
    mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
    mode |= ENABLE_WINDOW_INPUT;
    SetConsoleMode(ed->hConsoleIn, mode);
    
    // Set console title
    SetConsoleTitle("WordStar 4.0 Clone");
    
    // Clear screen
    system("cls");
    
    // Show cursor
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(ed->hConsoleOut, &cursorInfo);
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(ed->hConsoleOut, &cursorInfo);
}

// Restore console
void restore_console(Editor *ed) {
    // Restore console mode
    SetConsoleMode(ed->hConsoleIn, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
    
    // Clear screen and reset cursor
    system("cls");
    set_cursor_pos(ed, 0, 0);
}

// Create new line
Line *create_line(void) {
    Line *line = (Line *)malloc(sizeof(Line));
    line->capacity = 256;
    line->text = (char *)malloc(line->capacity);
    line->text[0] = '\0';
    line->length = 0;
    line->next = NULL;
    line->prev = NULL;
    return line;
}

// Free line
void free_line(Line *line) {
    if (line) {
        free(line->text);
        free(line);
    }
}

// Set cursor position
void set_cursor_pos(Editor *ed, int x, int y) {
    COORD pos = {x, y};
    SetConsoleCursorPosition(ed->hConsoleOut, pos);
}

// Write text at position with attributes
void write_at(Editor *ed, int x, int y, const char *text, WORD attr) {
    COORD pos = {x, y};
    DWORD written;
    int len = strlen(text);
    
    WriteConsoleOutputCharacterA(ed->hConsoleOut, text, len, pos, &written);
    if (attr) {
        WORD *attrs = (WORD *)malloc(len * sizeof(WORD));
        for (int i = 0; i < len; i++) attrs[i] = attr;
        WriteConsoleOutputAttribute(ed->hConsoleOut, attrs, len, pos, &written);
        free(attrs);
    }
}

// Draw screen
void draw_screen(Editor *ed) {
    draw_status_line(ed);
    if (ed->show_ruler) draw_ruler_line(ed);
    draw_text_area(ed);
    draw_menu_line(ed);
    
    // Position cursor
    int screen_y = 0;
    Line *line = ed->doc.first_line;
    for (int i = 0; i < ed->top_line && line; i++) {
        line = line->next;
    }
    
    Line *curr = line;
    while (curr && curr != ed->doc.current_line && screen_y < (EDIT_END - EDIT_START)) {
        curr = curr->next;
        screen_y++;
    }
    
    if (curr == ed->doc.current_line) {
        set_cursor_pos(ed, ed->doc.cursor_x - ed->screen_col, EDIT_START + screen_y);
    }
}

// Draw status line
void draw_status_line(Editor *ed) {
    char status[SCREEN_WIDTH + 1];
    int line_num = get_line_number(ed, ed->doc.current_line);
    
    snprintf(status, sizeof(status), " %s %s  Line %d Col %d  %s%s%s",
             ed->doc.filename,
             ed->doc.modified ? "*" : " ",
             line_num,
             ed->doc.cursor_x + 1,
             ed->insert_mode ? "Insert" : "Overtype",
             ed->format.word_wrap ? " Wrap" : "",
             ed->block.active ? " Block" : "");
    
    // Pad with spaces
    int len = strlen(status);
    for (int i = len; i < SCREEN_WIDTH; i++) {
        status[i] = ' ';
    }
    status[SCREEN_WIDTH] = '\0';
    
    // Draw with inverse video
    write_at(ed, 0, STATUS_LINE, status, 
             BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);
}

// Draw ruler line
void draw_ruler_line(Editor *ed) {
    char ruler[SCREEN_WIDTH + 1];
    
    // Build ruler with tab stops and margins
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        int col = i + ed->screen_col + 1;
        if (col == ed->format.left_margin) {
            ruler[i] = 'L';
        } else if (col == ed->format.right_margin) {
            ruler[i] = 'R';
        } else if (col == ed->format.paragraph_margin) {
            ruler[i] = 'P';
        } else if ((col - 1) % ed->format.tab_width == 0) {
            ruler[i] = '!';
        } else if ((col - 1) % 10 == 0) {
            ruler[i] = '0' + ((col - 1) / 10) % 10;
        } else {
            ruler[i] = '-';
        }
    }
    ruler[SCREEN_WIDTH] = '\0';
    
    write_at(ed, 0, RULER_LINE, ruler, 0);
}

// Draw menu line
void draw_menu_line(Editor *ed) {
    const char *menu;
    
    switch (ed->state) {
        case STATE_CTRL_K:
            menu = " ^KB Begin ^KK End ^KC Copy ^KV Move ^KY Delete ^KW Write ^KR Read ^KH Hide ";
            break;
        case STATE_CTRL_Q:
            menu = " ^QF Find ^QA Replace ^QR BegFile ^QC EndFile ^QY DelEOL ^QL RestoreLine ";
            break;
        case STATE_CTRL_O:
            menu = " ^OL LeftMarg ^OR RightMarg ^OP ParaMarg ^OW WordWrap ^OJ Justify ^OC Center ";
            break;
        case STATE_CTRL_P:
            menu = " ^PB Bold ^PS Underline ^PD Double ^PV Subscript ^PT Superscript ^PQ Return ";
            break;
        case STATE_FIND:
            menu = " Enter search text (^P for special chars, ESC to cancel) ";
            break;
        case STATE_REPLACE:
            menu = " Enter replacement text (ESC to cancel) ";
            break;
        case STATE_GOTO_LINE:
            menu = " Enter line number: ";
            break;
        case STATE_SAVE_AS:
            menu = " Enter filename: ";
            break;
        default:
            menu = " ^J Help ^KD Save ^KX Exit ^QF Find ^KB Block ^OW Wrap ^B Reform ^N Insert ";
            break;
    }
    
    char menu_line[SCREEN_WIDTH + 1];
    snprintf(menu_line, sizeof(menu_line), "%s", menu);
    
    // Pad with spaces
    int len = strlen(menu_line);
    for (int i = len; i < SCREEN_WIDTH; i++) {
        menu_line[i] = ' ';
    }
    menu_line[SCREEN_WIDTH] = '\0';
    
    write_at(ed, 0, MENU_LINE, menu_line, 0);
    
    // Show input buffer if in input state
    if (ed->state >= STATE_FIND && ed->state <= STATE_SAVE_AS) {
        char input_display[SCREEN_WIDTH - 20];
        snprintf(input_display, sizeof(input_display), "%s", ed->input_buffer);
        write_at(ed, strlen(menu) + 1, MENU_LINE, input_display, 
                 FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
}

// Draw text area
void draw_text_area(Editor *ed) {
    Line *line = ed->doc.first_line;
    
    // Skip to top line
    for (int i = 0; i < ed->top_line && line; i++) {
        line = line->next;
    }
    
    // Draw visible lines
    for (int y = EDIT_START; y <= EDIT_END; y++) {
        set_cursor_pos(ed, 0, y);
        
        if (line) {
            // Check if line is in block
            int in_block = is_line_in_block(ed, line);
            WORD attr = in_block ? (BACKGROUND_BLUE | FOREGROUND_INTENSITY | 
                                   FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE) : 0;
            
            // Draw line with horizontal scrolling
            char display_line[SCREEN_WIDTH + 1];
            int start = ed->screen_col;
            
            for (int i = 0; i < SCREEN_WIDTH; i++) {
                if (start + i < line->length) {
                    display_line[i] = line->text[start + i];
                } else {
                    display_line[i] = ' ';
                }
            }
            display_line[SCREEN_WIDTH] = '\0';
            
            if (attr) {
                write_at(ed, 0, y, display_line, attr);
            } else {
                printf("%s", display_line);
            }
            
            line = line->next;
        } else {
            // Empty line
            for (int i = 0; i < SCREEN_WIDTH; i++) {
                putchar(' ');
            }
        }
    }
}

// Check if line is in block
int is_line_in_block(Editor *ed, Line *line) {
    if (!ed->block.active) return 0;
    
    Line *curr = ed->block.start_line;
    while (curr) {
        if (curr == line) return 1;
        if (curr == ed->block.end_line) break;
        curr = curr->next;
    }
    
    return 0;
}

// Get line number
int get_line_number(Editor *ed, Line *target) {
    int num = 1;
    Line *line = ed->doc.first_line;
    
    while (line && line != target) {
        line = line->next;
        num++;
    }
    
    return num;
}

// Insert character
void insert_char(Editor *ed, char ch) {
    Line *line = ed->doc.current_line;
    
    // Expand line if needed
    if (line->length >= line->capacity - 1) {
        line->capacity *= 2;
        line->text = (char *)realloc(line->text, line->capacity);
    }
    
    if (ed->insert_mode) {
        // Insert mode - shift characters right
        memmove(&line->text[ed->doc.cursor_x + 1], &line->text[ed->doc.cursor_x], 
                line->length - ed->doc.cursor_x + 1);
        line->text[ed->doc.cursor_x] = ch;
        line->length++;
    } else {
        // Overwrite mode
        if (ed->doc.cursor_x >= line->length) {
            line->length = ed->doc.cursor_x + 1;
            line->text[line->length] = '\0';
        }
        line->text[ed->doc.cursor_x] = ch;
    }
    
    ed->doc.cursor_x++;
    ed->doc.modified = 1;
    
    // Word wrap if enabled
    if (ed->format.word_wrap && ed->doc.cursor_x > ed->format.right_margin) {
        // Find last space before margin
        int wrap_pos = ed->format.right_margin;
        while (wrap_pos > ed->format.left_margin && line->text[wrap_pos] != ' ') {
            wrap_pos--;
        }
        
        if (wrap_pos > ed->format.left_margin) {
            // Save cursor position relative to wrap
            int cursor_offset = ed->doc.cursor_x - wrap_pos - 1;
            
            // Move to wrap position
            ed->doc.cursor_x = wrap_pos;
            new_line(ed);
            
            // Move wrapped text
            Line *next = ed->doc.current_line;
            char *wrapped = &line->text[wrap_pos + 1];
            int wrapped_len = line->length - wrap_pos - 1;
            
            if (wrapped_len > 0) {
                memmove(&next->text[ed->format.left_margin - 1], wrapped, wrapped_len);
                next->length = ed->format.left_margin - 1 + wrapped_len;
                next->text[next->length] = '\0';
                line->length = wrap_pos;
                line->text[line->length] = '\0';
            }
            
            // Position cursor
            ed->doc.cursor_x = ed->format.left_margin - 1 + cursor_offset;
        }
    }
}

// Delete character
void delete_char(Editor *ed) {
    Line *line = ed->doc.current_line;
    
    if (ed->doc.cursor_x < line->length) {
        memmove(&line->text[ed->doc.cursor_x], &line->text[ed->doc.cursor_x + 1], 
                line->length - ed->doc.cursor_x);
        line->length--;
        ed->doc.modified = 1;
    } else if (line->next) {
        // Join with next line
        Line *next = line->next;
        int new_length = line->length + next->length;
        
        if (new_length >= line->capacity) {
            line->capacity = new_length + 256;
            line->text = (char *)realloc(line->text, line->capacity);
        }
        
        strcpy(&line->text[line->length], next->text);
        line->length = new_length;
        
        line->next = next->next;
        if (next->next) {
            next->next->prev = line;
        }
        
        free_line(next);
        ed->doc.line_count--;
        ed->doc.modified = 1;
    }
}

// Backspace
void backspace_char(Editor *ed) {
    if (ed->doc.cursor_x > 0) {
        ed->doc.cursor_x--;
        delete_char(ed);
    } else if (ed->doc.current_line->prev) {
        // Join with previous line
        Line *prev = ed->doc.current_line->prev;
        ed->doc.cursor_x = prev->length;
        ed->doc.current_line = prev;
        delete_char(ed);
    }
}

// Cursor movement functions
void move_cursor_left(Editor *ed) {
    if (ed->doc.cursor_x > 0) {
        ed->doc.cursor_x--;
    } else if (ed->doc.current_line->prev) {
        ed->doc.current_line = ed->doc.current_line->prev;
        ed->doc.cursor_x = ed->doc.current_line->length;
    }
}

void move_cursor_right(Editor *ed) {
    if (ed->doc.cursor_x < ed->doc.current_line->length) {
        ed->doc.cursor_x++;
    } else if (ed->doc.current_line->next) {
        ed->doc.current_line = ed->doc.current_line->next;
        ed->doc.cursor_x = 0;
    }
}

void move_cursor_up(Editor *ed) {
    if (ed->doc.current_line->prev) {
        ed->doc.current_line = ed->doc.current_line->prev;
        if (ed->doc.cursor_x > ed->doc.current_line->length) {
            ed->doc.cursor_x = ed->doc.current_line->length;
        }
    }
}

void move_cursor_down(Editor *ed) {
    if (ed->doc.current_line->next) {
        ed->doc.current_line = ed->doc.current_line->next;
        if (ed->doc.cursor_x > ed->doc.current_line->length) {
            ed->doc.cursor_x = ed->doc.current_line->length;
        }
    }
}

// Move by word
void move_word_left(Editor *ed) {
    // Skip current word
    while (ed->doc.cursor_x > 0 && !isspace(ed->doc.current_line->text[ed->doc.cursor_x - 1])) {
        ed->doc.cursor_x--;
    }
    // Skip spaces
    while (ed->doc.cursor_x > 0 && isspace(ed->doc.current_line->text[ed->doc.cursor_x - 1])) {
        ed->doc.cursor_x--;
    }
}

void move_word_right(Editor *ed) {
    Line *line = ed->doc.current_line;
    // Skip current word
    while (ed->doc.cursor_x < line->length && !isspace(line->text[ed->doc.cursor_x])) {
        ed->doc.cursor_x++;
    }
    // Skip spaces
    while (ed->doc.cursor_x < line->length && isspace(line->text[ed->doc.cursor_x])) {
        ed->doc.cursor_x++;
    }
}

// Line movement
void move_line_start(Editor *ed) {
    ed->doc.cursor_x = 0;
}

void move_line_end(Editor *ed) {
    ed->doc.cursor_x = ed->doc.current_line->length;
}

// Scrolling
void scroll_up(Editor *ed) {
    if (ed->doc.current_line->prev) {
        ed->doc.current_line = ed->doc.current_line->prev;
        if (ed->top_line > 0) ed->top_line--;
    }
}

void scroll_down(Editor *ed) {
    if (ed->doc.current_line->next) {
        ed->doc.current_line = ed->doc.current_line->next;
        ed->top_line++;
    }
}

// Page movement
void move_page_up(Editor *ed) {
    int lines = EDIT_END - EDIT_START;
    for (int i = 0; i < lines && ed->doc.current_line->prev; i++) {
        ed->doc.current_line = ed->doc.current_line->prev;
    }
    if (ed->top_line >= lines) {
        ed->top_line -= lines;
    } else {
        ed->top_line = 0;
    }
}

void move_page_down(Editor *ed) {
    int lines = EDIT_END - EDIT_START;
    for (int i = 0; i < lines && ed->doc.current_line->next; i++) {
        ed->doc.current_line = ed->doc.current_line->next;
    }
    ed->top_line += lines;
}

// Document movement
void move_doc_start(Editor *ed) {
    ed->doc.current_line = ed->doc.first_line;
    ed->doc.cursor_x = 0;
    ed->top_line = 0;
}

void move_doc_end(Editor *ed) {
    while (ed->doc.current_line->next) {
        ed->doc.current_line = ed->doc.current_line->next;
    }
    ed->doc.cursor_x = ed->doc.current_line->length;
}

// New line
void new_line(Editor *ed) {
    Line *new = create_line();
    Line *curr = ed->doc.current_line;
    
    // Split current line
    if (ed->doc.cursor_x < curr->length) {
        strcpy(new->text, &curr->text[ed->doc.cursor_x]);
        new->length = strlen(new->text);
        curr->text[ed->doc.cursor_x] = '\0';
        curr->length = ed->doc.cursor_x;
    }
    
    // Auto-indent
    if (ed->auto_indent && ed->doc.cursor_x == 0) {
        int indent = 0;
        while (indent < curr->length && (curr->text[indent] == ' ' || curr->text[indent] == '\t')) {
            indent++;
        }
        if (indent > 0 && indent < new->capacity) {
            memmove(&new->text[indent], new->text, new->length + 1);
            memcpy(new->text, curr->text, indent);
            new->length += indent;
        }
    }
    
    // Insert new line
    new->next = curr->next;
    new->prev = curr;
    if (curr->next) {
        curr->next->prev = new;
    }
    curr->next = new;
    
    ed->doc.current_line = new;
    ed->doc.cursor_x = (ed->auto_indent && ed->doc.cursor_x == 0) ? 
                        (new->length - strlen(&curr->text[ed->doc.cursor_x])) : 0;
    ed->doc.line_count++;
    ed->doc.modified = 1;
}

// Delete line
void delete_line(Editor *ed) {
    Line *line = ed->doc.current_line;
    
    if (ed->doc.line_count == 1) {
        // Just clear the line
        line->text[0] = '\0';
        line->length = 0;
        ed->doc.cursor_x = 0;
    } else {
        // Remove line from list
        if (line->prev) {
            line->prev->next = line->next;
        } else {
            ed->doc.first_line = line->next;
        }
        
        if (line->next) {
            line->next->prev = line->prev;
            ed->doc.current_line = line->next;
        } else {
            ed->doc.current_line = line->prev;
        }
        
        free_line(line);
        ed->doc.line_count--;
        
        if (ed->doc.cursor_x > ed->doc.current_line->length) {
            ed->doc.cursor_x = ed->doc.current_line->length;
        }
    }
    
    ed->doc.modified = 1;
}

// Delete word right
void delete_word_right(Editor *ed) {
    Line *line = ed->doc.current_line;
    int start = ed->doc.cursor_x;
    
    // Skip to end of current word
    while (ed->doc.cursor_x < line->length && !isspace(line->text[ed->doc.cursor_x])) {
        ed->doc.cursor_x++;
    }
    // Include trailing spaces
    while (ed->doc.cursor_x < line->length && isspace(line->text[ed->doc.cursor_x])) {
        ed->doc.cursor_x++;
    }
    
    // Delete the word
    if (ed->doc.cursor_x > start) {
        memmove(&line->text[start], &line->text[ed->doc.cursor_x], 
                line->length - ed->doc.cursor_x + 1);
        line->length -= (ed->doc.cursor_x - start);
        ed->doc.cursor_x = start;
        ed->doc.modified = 1;
    }
}

// Delete to end of line
void delete_to_eol(Editor *ed) {
    Line *line = ed->doc.current_line;
    if (ed->doc.cursor_x < line->length) {
        line->text[ed->doc.cursor_x] = '\0';
        line->length = ed->doc.cursor_x;
        ed->doc.modified = 1;
    }
}

// Save file
void save_file(Editor *ed) {
    FILE *fp = fopen(ed->doc.filename, "w");
    if (!fp) {
        update_status(ed, "Error: Cannot save file");
        return;
    }
    
    Line *line = ed->doc.first_line;
    while (line) {
        fprintf(fp, "%s", line->text);
        if (line->next) {
            fprintf(fp, "\r\n");
        }
        line = line->next;
    }
    
    fclose(fp);
    ed->doc.modified = 0;
    update_status(ed, "File saved");
}

// Save file as
void save_file_as(Editor *ed, const char *filename) {
    strcpy(ed->doc.filename, filename);
    save_file(ed);
}

// Load file
void load_file(Editor *ed, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        update_status(ed, "New file");
        strcpy(ed->doc.filename, filename);
        return;
    }
    
    // Clear existing document
    Line *line = ed->doc.first_line;
    while (line) {
        Line *next = line->next;
        free_line(line);
        line = next;
    }
    
    ed->doc.first_line = create_line();
    ed->doc.current_line = ed->doc.first_line;
    ed->doc.line_count = 1;
    ed->doc.cursor_x = 0;
    
    strcpy(ed->doc.filename, filename);
    
    // Read file
    char buffer[1024];
    Line *curr = ed->doc.first_line;
    int first_line = 1;
    
    while (fgets(buffer, sizeof(buffer), fp)) {
        // Remove newline
        int len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[--len] = '\0';
        }
        if (len > 0 && buffer[len-1] == '\r') {
            buffer[--len] = '\0';
        }
        
        if (!first_line) {
            // Create new line
            Line *new = create_line();
            new->prev = curr;
            curr->next = new;
            curr = new;
            ed->doc.line_count++;
        }
        
        // Store in current line
        if (curr->capacity <= len) {
            curr->capacity = len + 256;
            curr->text = (char *)realloc(curr->text, curr->capacity);
        }
        strcpy(curr->text, buffer);
        curr->length = len;
        
        first_line = 0;
    }
    
    fclose(fp);
    ed->doc.modified = 0;
    update_status(ed, "File loaded");
}

// Block operations
void mark_block_begin(Editor *ed) {
    ed->block.start_line = ed->doc.current_line;
    ed->block.start_col = ed->doc.cursor_x;
    if (!ed->block.active || ed->block.end_line == NULL) {
        ed->block.end_line = ed->doc.current_line;
        ed->block.end_col = ed->doc.cursor_x;
    }
    ed->block.active = 1;
    update_status(ed, "Block begin marked");
}

void mark_block_end(Editor *ed) {
    ed->block.end_line = ed->doc.current_line;
    ed->block.end_col = ed->doc.cursor_x;
    if (!ed->block.active || ed->block.start_line == NULL) {
        ed->block.start_line = ed->doc.current_line;
        ed->block.start_col = ed->doc.cursor_x;
    }
    ed->block.active = 1;
    update_status(ed, "Block end marked");
}

void hide_block(Editor *ed) {
    ed->block.active = 0;
    update_status(ed, "Block hidden");
}

// Clear clipboard
void clear_clipboard(Editor *ed) {
    Line *line = ed->clipboard;
    while (line) {
        Line *next = line->next;
        free_line(line);
        line = next;
    }
    ed->clipboard = NULL;
    ed->clipboard_lines = 0;
}

// Duplicate lines
Line *duplicate_lines(Line *start, Line *end, int *count) {
    Line *new_start = NULL;
    Line *new_prev = NULL;
    Line *curr = start;
    *count = 0;
    
    while (curr) {
        Line *new = create_line();
        if (new->capacity <= curr->length) {
            new->capacity = curr->length + 1;
            new->text = (char *)realloc(new->text, new->capacity);
        }
        strcpy(new->text, curr->text);
        new->length = curr->length;
        
        if (new_prev) {
            new_prev->next = new;
            new->prev = new_prev;
        } else {
            new_start = new;
        }
        new_prev = new;
        (*count)++;
        
        if (curr == end) break;
        curr = curr->next;
    }
    
    return new_start;
}

// Copy block
void copy_block(Editor *ed) {
    if (!ed->block.active) {
        update_status(ed, "No block marked");
        return;
    }
    
    clear_clipboard(ed);
    ed->clipboard = duplicate_lines(ed->block.start_line, ed->block.end_line, 
                                   &ed->clipboard_lines);
    update_status(ed, "Block copied to clipboard");
}

// Insert lines at current position
void insert_lines(Editor *ed, Line *lines, int count) {
    if (!lines || count == 0) return;
    
    Line *curr = ed->doc.current_line;
    Line *last = lines;
    
    // Find last line in clipboard
    while (last->next) {
        last = last->next;
    }
    
    // Insert after current line
    last->next = curr->next;
    if (curr->next) {
        curr->next->prev = last;
    }
    
    lines->prev = curr;
    curr->next = lines;
    
    ed->doc.line_count += count;
    ed->doc.modified = 1;
}

// Move block
void move_block(Editor *ed) {
    if (!ed->block.active) {
        update_status(ed, "No block marked");
        return;
    }
    
    // Copy to clipboard
    copy_block(ed);
    
    // Delete original
    delete_block(ed);
    
    // Insert at current position
    if (ed->clipboard) {
        Line *dup_clip = duplicate_lines(ed->clipboard, NULL, &ed->clipboard_lines);
        insert_lines(ed, dup_clip, ed->clipboard_lines);
        update_status(ed, "Block moved");
    }
}

// Delete block
void delete_block(Editor *ed) {
    if (!ed->block.active) {
        update_status(ed, "No block marked");
        return;
    }
    
    Line *start = ed->block.start_line;
    Line *end = ed->block.end_line;
    
    // Connect previous to next
    if (start->prev) {
        start->prev->next = end->next;
    } else {
        ed->doc.first_line = end->next;
    }
    
    if (end->next) {
        end->next->prev = start->prev;
    }
    
    // Count deleted lines
    int deleted = 0;
    Line *curr = start;
    while (curr) {
        Line *next = curr->next;
        free_line(curr);
        deleted++;
        if (curr == end) break;
        curr = next;
    }
    
    ed->doc.line_count -= deleted;
    ed->doc.current_line = end->next ? end->next : 
                           (start->prev ? start->prev : ed->doc.first_line);
    ed->doc.cursor_x = 0;
    
    ed->block.active = 0;
    ed->doc.modified = 1;
    update_status(ed, "Block deleted");
}

// Write block to file
void write_block(Editor *ed, const char *filename) {
    if (!ed->block.active) {
        update_status(ed, "No block marked");
        return;
    }
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        update_status(ed, "Error: Cannot write block");
        return;
    }
    
    Line *curr = ed->block.start_line;
    while (curr) {
        fprintf(fp, "%s\r\n", curr->text);
        if (curr == ed->block.end_line) break;
        curr = curr->next;
    }
    
    fclose(fp);
    update_status(ed, "Block written to file");
}

// Read block from file
void read_block(Editor *ed, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        update_status(ed, "Error: Cannot read file");
        return;
    }
    
    Line *first = NULL;
    Line *prev = NULL;
    int count = 0;
    char buffer[1024];
    
    while (fgets(buffer, sizeof(buffer), fp)) {
        Line *new = create_line();
        
        // Remove newline
        int len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') buffer[--len] = '\0';
        if (len > 0 && buffer[len-1] == '\r') buffer[--len] = '\0';
        
        if (new->capacity <= len) {
            new->capacity = len + 256;
            new->text = (char *)realloc(new->text, new->capacity);
        }
        strcpy(new->text, buffer);
        new->length = len;
        
        if (prev) {
            prev->next = new;
            new->prev = prev;
        } else {
            first = new;
        }
        prev = new;
        count++;
    }
    
    fclose(fp);
    
    if (first) {
        insert_lines(ed, first, count);
        update_status(ed, "Block read from file");
    }
}

// Find text
void find_text(Editor *ed) {
    if (strlen(ed->find.find_text) == 0) {
        update_status(ed, "No search text");
        return;
    }
    
    Line *start_line = ed->doc.current_line;
    int start_pos = ed->doc.cursor_x + 1;
    Line *line = start_line;
    
    // Search forward
    while (line) {
        char *found = strstr(&line->text[start_pos], ed->find.find_text);
        if (found) {
            ed->doc.current_line = line;
            ed->doc.cursor_x = found - line->text;
            update_status(ed, "Found");
            return;
        }
        
        line = line->next;
        start_pos = 0;
    }
    
    // Wrap around
    line = ed->doc.first_line;
    while (line != start_line) {
        char *found = strstr(line->text, ed->find.find_text);
        if (found) {
            ed->doc.current_line = line;
            ed->doc.cursor_x = found - line->text;
            update_status(ed, "Found (wrapped)");
            return;
        }
        line = line->next;
    }
    
    update_status(ed, "Not found");
}

// Find next
void find_next(Editor *ed) {
    find_text(ed);
}

// Replace text
void replace_text(Editor *ed) {
    if (strlen(ed->find.find_text) == 0) {
        update_status(ed, "No search text");
        return;
    }
    
    Line *line = ed->doc.current_line;
    
    // Check if we're at a match
    if (ed->doc.cursor_x + strlen(ed->find.find_text) <= line->length &&
        strncmp(&line->text[ed->doc.cursor_x], ed->find.find_text, 
                strlen(ed->find.find_text)) == 0) {
        
        // Delete old text
        int find_len = strlen(ed->find.find_text);
        int replace_len = strlen(ed->find.replace_text);
        
        if (replace_len != find_len) {
            // Adjust line length
            int new_length = line->length - find_len + replace_len;
            if (new_length >= line->capacity) {
                line->capacity = new_length + 256;
                line->text = (char *)realloc(line->text, line->capacity);
            }
            
            // Move text
            memmove(&line->text[ed->doc.cursor_x + replace_len],
                    &line->text[ed->doc.cursor_x + find_len],
                    line->length - ed->doc.cursor_x - find_len + 1);
            line->length = new_length;
        }
        
        // Insert replacement
        memcpy(&line->text[ed->doc.cursor_x], ed->find.replace_text, replace_len);
        ed->doc.modified = 1;
        
        // Move cursor past replacement
        ed->doc.cursor_x += replace_len;
    }
    
    // Find next occurrence
    find_next(ed);
}

// Go to line
void goto_line(Editor *ed, int line_num) {
    if (line_num < 1) line_num = 1;
    
    ed->doc.current_line = ed->doc.first_line;
    for (int i = 1; i < line_num && ed->doc.current_line->next; i++) {
        ed->doc.current_line = ed->doc.current_line->next;
    }
    
    ed->doc.cursor_x = 0;
    
    // Adjust top line for visibility
    int visible_lines = EDIT_END - EDIT_START + 1;
    ed->top_line = line_num - visible_lines / 2;
    if (ed->top_line < 0) ed->top_line = 0;
}

// Set marker
void set_marker(Editor *ed, int marker) {
    if (marker >= 0 && marker < MAX_MARKERS) {
        ed->doc.markers[marker] = ed->doc.current_line;
        ed->doc.marker_x[marker] = ed->doc.cursor_x;
        char msg[32];
        snprintf(msg, sizeof(msg), "Marker %d set", marker);
        update_status(ed, msg);
    }
}

// Go to marker
void goto_marker(Editor *ed, int marker) {
    if (marker >= 0 && marker < MAX_MARKERS && ed->doc.markers[marker]) {
        ed->doc.current_line = ed->doc.markers[marker];
        ed->doc.cursor_x = ed->doc.marker_x[marker];
        char msg[32];
        snprintf(msg, sizeof(msg), "At marker %d", marker);
        update_status(ed, msg);
    } else {
        update_status(ed, "Marker not set");
    }
}

// Reform paragraph
void reform_paragraph(Editor *ed) {
    // Find paragraph boundaries
    Line *start = ed->doc.current_line;
    Line *end = ed->doc.current_line;
    
    // Find start of paragraph
    while (start->prev && start->prev->length > 0) {
        start = start->prev;
    }
    
    // Find end of paragraph
    while (end->next && end->next->length > 0) {
        end = end->next;
    }
    
    // Collect all text
    int total_len = 0;
    Line *line = start;
    while (line) {
        total_len += line->length + 1;
        if (line == end) break;
        line = line->next;
    }
    
    char *para_text = (char *)malloc(total_len);
    para_text[0] = '\0';
    
    line = start;
    while (line) {
        strcat(para_text, line->text);
        if (line != end) strcat(para_text, " ");
        if (line == end) break;
        line = line->next;
    }
    
    // Delete old lines (except first)
    line = start->next;
    while (line && line != end->next) {
        Line *next = line->next;
        
        if (line->prev) line->prev->next = line->next;
        if (line->next) line->next->prev = line->prev;
        
        free_line(line);
        ed->doc.line_count--;
        line = next;
    }
    
    // Reform into new lines
    start->next = end->next;
    if (end->next) end->next->prev = start;
    
    char *word = strtok(para_text, " ");
    start->text[0] = '\0';
    start->length = 0;
    
    // Add initial indent
    for (int i = 0; i < ed->format.paragraph_margin - 1; i++) {
        start->text[start->length++] = ' ';
    }
    
    Line *curr = start;
    
    while (word) {
        int word_len = strlen(word);
        
        if (curr->length > ed->format.left_margin - 1 && 
            curr->length + word_len + 1 > ed->format.right_margin) {
            // Create new line
            Line *new = create_line();
            new->prev = curr;
            new->next = curr->next;
            if (curr->next) curr->next->prev = new;
            curr->next = new;
            curr = new;
            ed->doc.line_count++;
            
            // Add left margin
            for (int i = 0; i < ed->format.left_margin - 1; i++) {
                curr->text[curr->length++] = ' ';
            }
        }
        
        if (curr->length > ed->format.left_margin - 1) {
            curr->text[curr->length++] = ' ';
        }
        
        strcpy(&curr->text[curr->length], word);
        curr->length += word_len;
        curr->text[curr->length] = '\0';
        
        word = strtok(NULL, " ");
    }
    
    free(para_text);
    ed->doc.modified = 1;
    update_status(ed, "Paragraph reformed");
}

// Center line
void center_line(Editor *ed) {
    Line *line = ed->doc.current_line;
    
    // Remove leading/trailing spaces
    int start = 0;
    while (start < line->length && isspace(line->text[start])) start++;
    
    int end = line->length - 1;
    while (end > start && isspace(line->text[end])) end--;
    
    if (start <= end) {
        int text_len = end - start + 1;
        int margin = (ed->format.right_margin - ed->format.left_margin - text_len) / 2;
        if (margin < 0) margin = 0;
        
        // Build centered line
        char *new_text = (char *)malloc(line->capacity);
        int pos = 0;
        
        // Add left margin and centering spaces
        for (int i = 0; i < ed->format.left_margin - 1 + margin; i++) {
            new_text[pos++] = ' ';
        }
        
        // Add text
        memcpy(&new_text[pos], &line->text[start], text_len);
        pos += text_len;
        new_text[pos] = '\0';
        
        // Replace line text
        strcpy(line->text, new_text);
        line->length = pos;
        free(new_text);
        
        ed->doc.modified = 1;
        update_status(ed, "Line centered");
    }
}

// Update status message
void update_status(Editor *ed, const char *msg) {
    strncpy(ed->status_msg, msg, sizeof(ed->status_msg) - 1);
    ed->status_msg[sizeof(ed->status_msg) - 1] = '\0';
}

// Process key input
void process_key(Editor *ed, KEY_EVENT_RECORD *key) {
    if (!key->bKeyDown) return;
    
    // Handle input states
    if (ed->state >= STATE_FIND && ed->state <= STATE_SAVE_AS) {
        handle_input_state(ed, key);
        return;
    }
    
    switch (ed->state) {
        case STATE_NORMAL:
            handle_normal_key(ed, key);
            break;
        case STATE_CTRL_K:
            handle_ctrl_k(ed, key);
            break;
        case STATE_CTRL_Q:
            handle_ctrl_q(ed, key);
            break;
        case STATE_CTRL_O:
            handle_ctrl_o(ed, key);
            break;
        case STATE_CTRL_P:
            handle_ctrl_p(ed, key);
            break;
    }
}

// Handle input states (find, replace, etc.)
void handle_input_state(Editor *ed, KEY_EVENT_RECORD *key) {
    WORD vk = key->wVirtualKeyCode;
    char ch = key->uChar.AsciiChar;
    
    if (vk == VK_ESCAPE) {
        ed->state = STATE_NORMAL;
        update_status(ed, "Cancelled");
        return;
    }
    
    if (vk == VK_RETURN) {
        switch (ed->state) {
            case STATE_FIND:
                strcpy(ed->find.find_text, ed->input_buffer);
                ed->state = STATE_NORMAL;
                find_text(ed);
                break;
            case STATE_REPLACE:
                strcpy(ed->find.replace_text, ed->input_buffer);
                ed->state = STATE_NORMAL;
                replace_text(ed);
                break;
            case STATE_GOTO_LINE:
                {
                    int line_num = atoi(ed->input_buffer);
                    ed->state = STATE_NORMAL;
                    goto_line(ed, line_num);
                }
                break;
            case STATE_SAVE_AS:
                ed->state = STATE_NORMAL;
                save_file_as(ed, ed->input_buffer);
                break;
        }
        ed->input_buffer[0] = '\0';
        ed->input_pos = 0;
        return;
    }
    
    if (vk == VK_BACK && ed->input_pos > 0) {
        ed->input_pos--;
        ed->input_buffer[ed->input_pos] = '\0';
    } else if (ch >= 32 && ch < 127 && ed->input_pos < sizeof(ed->input_buffer) - 1) {
        ed->input_buffer[ed->input_pos++] = ch;
        ed->input_buffer[ed->input_pos] = '\0';
    }
}

// Handle normal state keys
void handle_normal_key(Editor *ed, KEY_EVENT_RECORD *key) {
    WORD vk = key->wVirtualKeyCode;
    char ch = key->uChar.AsciiChar;
    
    // Check for control keys
    if (key->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
        switch (ch) {
            case CTRL_K:
                ed->state = STATE_CTRL_K;
                break;
            case CTRL_Q:
                ed->state = STATE_CTRL_Q;
                break;
            case CTRL_O:
                ed->state = STATE_CTRL_O;
                break;
            case CTRL_P:
                ed->state = STATE_CTRL_P;
                break;
            case CTRL_S:  // Left
                move_cursor_left(ed);
                break;
            case CTRL_D:  // Right
                move_cursor_right(ed);
                break;
            case CTRL_E:  // Up
                move_cursor_up(ed);
                break;
            case CTRL_X:  // Down
                move_cursor_down(ed);
                break;
            case CTRL_A:  // Word left
                move_word_left(ed);
                break;
            case CTRL_F:  // Word right
                move_word_right(ed);
                break;
            case CTRL_W:  // Scroll up
                scroll_up(ed);
                break;
            case CTRL_Z:  // Scroll down
                scroll_down(ed);
                break;
            case CTRL_R:  // Page up
                move_page_up(ed);
                break;
            case CTRL_C:  // Page down
                move_page_down(ed);
                break;
            case CTRL_G:  // Delete char
                delete_char(ed);
                break;
            case CTRL_H:  // Backspace
                backspace_char(ed);
                break;
            case CTRL_T:  // Delete word right
                delete_word_right(ed);
                break;
            case CTRL_Y:  // Delete line
                delete_line(ed);
                break;
            case CTRL_V:  // Insert mode toggle
                ed->insert_mode = !ed->insert_mode;
                update_status(ed, ed->insert_mode ? "Insert mode" : "Overtype mode");
                break;
            case CTRL_N:  // Insert line
                new_line(ed);
                move_cursor_up(ed);
                break;
            case CTRL_B:  // Reform paragraph
                reform_paragraph(ed);
                break;
            case CTRL_L:  // Find next
                find_next(ed);
                break;
            case CTRL_J:  // Help (not implemented fully)
                update_status(ed, "WordStar 4.0 Clone - Use ^K/^Q/^O/^P menus");
                break;
        }
    } else {
        // Regular keys
        switch (vk) {
            case VK_LEFT:
                move_cursor_left(ed);
                break;
            case VK_RIGHT:
                move_cursor_right(ed);
                break;
            case VK_UP:
                move_cursor_up(ed);
                break;
            case VK_DOWN:
                move_cursor_down(ed);
                break;
            case VK_HOME:
                move_line_start(ed);
                break;
            case VK_END:
                move_line_end(ed);
                break;
            case VK_PRIOR:  // Page Up
                move_page_up(ed);
                break;
            case VK_NEXT:   // Page Down
                move_page_down(ed);
                break;
            case VK_DELETE:
                delete_char(ed);
                break;
            case VK_BACK:
                backspace_char(ed);
                break;
            case VK_RETURN:
                new_line(ed);
                break;
            case VK_TAB:
                // Insert spaces to next tab stop
                do {
                    insert_char(ed, ' ');
                } while (ed->doc.cursor_x % ed->format.tab_width != 0);
                break;
            case VK_INSERT:
                ed->insert_mode = !ed->insert_mode;
                update_status(ed, ed->insert_mode ? "Insert mode" : "Overtype mode");
                break;
            default:
                if (ch >= 32 && ch < 127) {
                    insert_char(ed, ch);
                }
                break;
        }
    }
}

// Handle Ctrl-K menu
void handle_ctrl_k(Editor *ed, KEY_EVENT_RECORD *key) {
    char ch = toupper(key->uChar.AsciiChar);
    
    switch (ch) {
        case 'S':  // Save
            save_file(ed);
            break;
        case 'D':  // Done (save and continue)
            save_file(ed);
            break;
        case 'X':  // Exit
            if (ed->doc.modified) {
                update_status(ed, "Save changes? (Y/N)");
                // Simple prompt - in real implementation would wait for Y/N
                save_file(ed);
            }
            exit(0);
            break;
        case 'Q':  // Quit without save
            exit(0);
            break;
        case 'B':  // Block begin
            mark_block_begin(ed);
            break;
        case 'K':  // Block end
            mark_block_end(ed);
            break;
        case 'C':  // Copy block
            copy_block(ed);
            break;
        case 'V':  // Move block
            move_block(ed);
            break;
        case 'Y':  // Delete block
            delete_block(ed);
            break;
        case 'H':  // Hide block
            hide_block(ed);
            break;
        case 'W':  // Write block
            ed->state = STATE_SAVE_AS;
            ed->input_buffer[0] = '\0';
            ed->input_pos = 0;
            update_status(ed, "Write block to file:");
            return;  // Stay in submenu
        case 'R':  // Read file
            ed->state = STATE_SAVE_AS;
            ed->input_buffer[0] = '\0';
            ed->input_pos = 0;
            update_status(ed, "Read file:");
            return;  // Stay in submenu
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            // Set marker
            set_marker(ed, ch - '0');
            break;
    }
    
    ed->state = STATE_NORMAL;
}

// Handle Ctrl-Q menu
void handle_ctrl_q(Editor *ed, KEY_EVENT_RECORD *key) {
    char ch = toupper(key->uChar.AsciiChar);
    
    switch (ch) {
        case 'F':  // Find
            ed->state = STATE_FIND;
            strcpy(ed->input_buffer, ed->find.find_text);
            ed->input_pos = strlen(ed->input_buffer);
            update_status(ed, "Find text:");
            return;  // Stay in submenu
        case 'A':  // Replace
            ed->state = STATE_REPLACE;
            strcpy(ed->input_buffer, ed->find.replace_text);
            ed->input_pos = strlen(ed->input_buffer);
            update_status(ed, "Replace with:");
            return;  // Stay in submenu
        case 'R':  // Beginning of file
            move_doc_start(ed);
            break;
        case 'C':  // End of file
            move_doc_end(ed);
            break;
        case 'B':  // Beginning of block
            if (ed->block.active && ed->block.start_line) {
                ed->doc.current_line = ed->block.start_line;
                ed->doc.cursor_x = ed->block.start_col;
            }
            break;
        case 'K':  // End of block
            if (ed->block.active && ed->block.end_line) {
                ed->doc.current_line = ed->block.end_line;
                ed->doc.cursor_x = ed->block.end_col;
            }
            break;
        case 'Y':  // Delete to end of line
            delete_to_eol(ed);
            break;
        case 'L':  // Restore line (undo - simplified)
            update_status(ed, "Undo not implemented");
            break;
        case 'I':  // Go to line
            ed->state = STATE_GOTO_LINE;
            ed->input_buffer[0] = '\0';
            ed->input_pos = 0;
            update_status(ed, "Go to line number:");
            return;  // Stay in submenu
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            // Go to marker
            goto_marker(ed, ch - '0');
            break;
    }
    
    ed->state = STATE_NORMAL;
}

// Handle Ctrl-O menu (Format)
void handle_ctrl_o(Editor *ed, KEY_EVENT_RECORD *key) {
    char ch = toupper(key->uChar.AsciiChar);
    
    switch (ch) {
        case 'L':  // Set left margin
            ed->format.left_margin = ed->doc.cursor_x + 1;
            update_status(ed, "Left margin set");
            break;
        case 'R':  // Set right margin
            ed->format.right_margin = ed->doc.cursor_x + 1;
            update_status(ed, "Right margin set");
            break;
        case 'P':  // Set paragraph margin
            ed->format.paragraph_margin = ed->doc.cursor_x + 1;
            update_status(ed, "Paragraph margin set");
            break;
        case 'W':  // Toggle word wrap
            ed->format.word_wrap = !ed->format.word_wrap;
            update_status(ed, ed->format.word_wrap ? "Word wrap ON" : "Word wrap OFF");
            break;
        case 'J':  // Toggle justify
            ed->format.justify = !ed->format.justify;
            update_status(ed, ed->format.justify ? "Justify ON" : "Justify OFF");
            break;
        case 'C':  // Center line
            center_line(ed);
            break;
        case 'T':  // Toggle ruler
            ed->show_ruler = !ed->show_ruler;
            update_status(ed, ed->show_ruler ? "Ruler ON" : "Ruler OFF");
            break;
        case 'I':  // Set tab width
            // Simplified - would need input
            ed->format.tab_width = 4;
            update_status(ed, "Tab width set to 4");
            break;
        case 'F':  // Toggle auto-indent
            ed->auto_indent = !ed->auto_indent;
            update_status(ed, ed->auto_indent ? "Auto-indent ON" : "Auto-indent OFF");
            break;
    }
    
    ed->state = STATE_NORMAL;
}

// Handle Ctrl-P menu (Print formatting)
void handle_ctrl_p(Editor *ed, KEY_EVENT_RECORD *key) {
    char ch = toupper(key->uChar.AsciiChar);
    
    // In a real implementation, these would insert formatting codes
    switch (ch) {
        case 'B':  // Bold on/off
            insert_char(ed, 0x02);  // STX character for bold
            update_status(ed, "Bold marker inserted");
            break;
        case 'S':  // Underline on/off
            insert_char(ed, 0x13);  // DC3 character for underline
            update_status(ed, "Underline marker inserted");
            break;
        case 'D':  // Double strike on/off
            update_status(ed, "Double strike marker inserted");
            break;
        case 'V':  // Subscript on/off
            update_status(ed, "Subscript marker inserted");
            break;
        case 'T':  // Superscript on/off
            update_status(ed, "Superscript marker inserted");
            break;
        case 'Q':  // Return to edit
            break;
    }
    
    ed->state = STATE_NORMAL;
}

// Cleanup
void cleanup_editor(Editor *ed) {
    // Free all lines
    Line *line = ed->doc.first_line;
    while (line) {
        Line *next = line->next;
        free_line(line);
        line = next;
    }
    
    // Free clipboard
    clear_clipboard(ed);
    
    restore_console(ed);
}

// Main program
int main(int argc, char *argv[]) {
    Editor editor;
    
    init_editor(&editor);
    
    // Load file if specified
    if (argc > 1) {
        load_file(&editor, argv[1]);
    } else {
        update_status(&editor, "New file - Press ^J for help");
    }
    
    // Main loop
    draw_screen(&editor);
    
    while (1) {
        INPUT_RECORD input;
        DWORD events;
        
        if (ReadConsoleInput(editor.hConsoleIn, &input, 1, &events)) {
            if (input.EventType == KEY_EVENT) {
                process_key(&editor, &input.Event.KeyEvent);
                
                // Adjust scroll position
                int visible_lines = EDIT_END - EDIT_START + 1;
                int current_line = 0;
                Line *line = editor.doc.first_line;
                while (line && line != editor.doc.current_line) {
                    line = line->next;
                    current_line++;
                }
                
                if (current_line < editor.top_line) {
                    editor.top_line = current_line;
                } else if (current_line >= editor.top_line + visible_lines) {
                    editor.top_line = current_line - visible_lines + 1;
                }
                
                // Adjust horizontal scroll
                if (editor.doc.cursor_x < editor.screen_col) {
                    editor.screen_col = editor.doc.cursor_x;
                } else if (editor.doc.cursor_x >= editor.screen_col + SCREEN_WIDTH) {
                    editor.screen_col = editor.doc.cursor_x - SCREEN_WIDTH + 1;
                }
                
                draw_screen(&editor);
            } else if (input.EventType == WINDOW_BUFFER_SIZE_EVENT) {
                // Handle window resize if needed
                draw_screen(&editor);
            }
        }
    }
    
    cleanup_editor(&editor);
    return 0;
}