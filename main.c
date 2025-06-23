// main.c - WordStar 4.0 Clone for Windows Console
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Constants
#define VERSION "1.0"
#define MAX_LINE_LENGTH 256
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define STATUS_LINE 0
#define MENU_LINE (SCREEN_HEIGHT - 1)
#define EDIT_START 1
#define EDIT_END (SCREEN_HEIGHT - 2)
#define TAB_WIDTH 8

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
    STATE_BLOCK_MARKED
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
} Document;

// Block marking
typedef struct {
    Line *start_line;
    Line *end_line;
    int start_col;
    int end_col;
    int active;
} Block;

// Editor structure
typedef struct {
    Document doc;
    Block block;
    EditorState state;
    int top_line;
    int screen_col;
    HANDLE hConsoleIn;
    HANDLE hConsoleOut;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    char status_msg[256];
    int insert_mode;
    int word_wrap;
    int auto_indent;
} Editor;

// Function prototypes
void init_editor(Editor *ed);
void cleanup_editor(Editor *ed);
void init_console(Editor *ed);
void restore_console(Editor *ed);
void draw_screen(Editor *ed);
void draw_status_line(Editor *ed);
void draw_menu_line(Editor *ed);
void draw_text_area(Editor *ed);
void set_cursor_pos(Editor *ed, int x, int y);
void process_key(Editor *ed, KEY_EVENT_RECORD *key);
void handle_normal_key(Editor *ed, KEY_EVENT_RECORD *key);
void handle_ctrl_k(Editor *ed, KEY_EVENT_RECORD *key);
void handle_ctrl_q(Editor *ed, KEY_EVENT_RECORD *key);
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
void delete_line(Editor *ed);
void delete_word_right(Editor *ed);
void new_line(Editor *ed);
void save_file(Editor *ed);
void load_file(Editor *ed, const char *filename);
void mark_block_begin(Editor *ed);
void mark_block_end(Editor *ed);
void copy_block(Editor *ed);
void move_block(Editor *ed);
void delete_block(Editor *ed);
void write_block(Editor *ed);
void update_status(Editor *ed, const char *msg);

// Initialize editor
void init_editor(Editor *ed) {
    memset(ed, 0, sizeof(Editor));
    ed->doc.first_line = create_line();
    ed->doc.current_line = ed->doc.first_line;
    ed->doc.line_count = 1;
    ed->insert_mode = 1;
    ed->word_wrap = 1;
    ed->state = STATE_NORMAL;
    strcpy(ed->doc.filename, "UNTITLED.TXT");
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
    
    // Clear screen
    system("cls");
    
    // Hide cursor for drawing
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

// Draw screen
void draw_screen(Editor *ed) {
    draw_status_line(ed);
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
    int line_num = 1;
    Line *line = ed->doc.first_line;
    
    while (line && line != ed->doc.current_line) {
        line = line->next;
        line_num++;
    }
      snprintf(status, sizeof(status), " %s %s  Line:%d  Col:%d  %s",
             ed->doc.filename,
             ed->doc.modified ? "*" : " ",
             line_num,
             ed->doc.cursor_x + 1,
             ed->insert_mode ? "Insert" : "Overwrite");
    
    // Pad with spaces
    int len = strlen(status);
    for (int i = len; i < SCREEN_WIDTH; i++) {
        status[i] = ' ';
    }
    status[SCREEN_WIDTH] = '\0';
    
    // Draw with inverse video
    set_cursor_pos(ed, 0, STATUS_LINE);
    WORD attrs = BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED;
    DWORD written;
    WriteConsoleOutputAttribute(ed->hConsoleOut, &attrs, SCREEN_WIDTH, 
                                (COORD){0, STATUS_LINE}, &written);
    printf("%s", status);
}

// Draw menu line
void draw_menu_line(Editor *ed) {
    const char *menu;
    
    switch (ed->state) {
        case STATE_CTRL_K:
            menu = " ^KB Begin ^KK End ^KC Copy ^KV Move ^KY Delete ^KW Write ^KR Read ^KD Done ";
            break;
        case STATE_CTRL_Q:
            menu = " ^QF Find ^QA Replace ^QR Top ^QC End ^QY DelEOL ^QB Block ^QK Block ";
            break;
        default:
            menu = " ^KD Save ^KX Exit ^QF Find ^KB MarkBlk ^KC Copy ^KV Move ^KS Save ^KQ Quit ";
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
    
    set_cursor_pos(ed, 0, MENU_LINE);
    printf("%s", menu_line);
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
            // Draw line with horizontal scrolling
            int start = ed->screen_col;
            int end = start + SCREEN_WIDTH;
            
            for (int i = 0; i < SCREEN_WIDTH; i++) {
                if (start + i < line->length) {
                    putchar(line->text[start + i]);
                } else {
                    putchar(' ');
                }
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
    
    // Insert new line
    new->next = curr->next;
    new->prev = curr;
    if (curr->next) {
        curr->next->prev = new;
    }
    curr->next = new;
    
    ed->doc.current_line = new;
    ed->doc.cursor_x = 0;
    ed->doc.line_count++;
    ed->doc.modified = 1;
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

// Load file
void load_file(Editor *ed, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
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
    
    while (fgets(buffer, sizeof(buffer), fp)) {
        // Remove newline
        int len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[--len] = '\0';
        }
        if (len > 0 && buffer[len-1] == '\r') {
            buffer[--len] = '\0';
        }
        
        // Store in current line
        if (curr->capacity <= len) {
            curr->capacity = len + 256;
            curr->text = (char *)realloc(curr->text, curr->capacity);
        }
        strcpy(curr->text, buffer);
        curr->length = len;
        
        // Create next line if not EOF
        if (!feof(fp)) {
            Line *new = create_line();
            new->prev = curr;
            curr->next = new;
            curr = new;
            ed->doc.line_count++;
        }
    }
    
    fclose(fp);
    ed->doc.modified = 0;
}

// Update status message
void update_status(Editor *ed, const char *msg) {
    strncpy(ed->status_msg, msg, sizeof(ed->status_msg) - 1);
    ed->status_msg[sizeof(ed->status_msg) - 1] = '\0';
}

// Process key input
void process_key(Editor *ed, KEY_EVENT_RECORD *key) {
    if (!key->bKeyDown) return;
    
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
            case CTRL_G:  // Delete char
                delete_char(ed);
                break;
            case CTRL_H:  // Backspace
                backspace_char(ed);
                break;
            case CTRL_V:  // Insert mode toggle
                ed->insert_mode = !ed->insert_mode;
                break;
            case CTRL_Y:  // Delete line
                delete_line(ed);
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
            case VK_DELETE:
                delete_char(ed);
                break;
            case VK_BACK:
                backspace_char(ed);
                break;
            case VK_RETURN:
                new_line(ed);
                break;
            case VK_INSERT:
                ed->insert_mode = !ed->insert_mode;
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
                // TODO: Prompt to save
            }
            exit(0);
            break;
        case 'Q':  // Quit without save
            exit(0);
            break;
    }
    
    ed->state = STATE_NORMAL;
}

// Handle Ctrl-Q menu
void handle_ctrl_q(Editor *ed, KEY_EVENT_RECORD *key) {
    char ch = toupper(key->uChar.AsciiChar);
    
    switch (ch) {
        case 'R':  // Beginning of file
            move_doc_start(ed);
            break;
        case 'C':  // End of file
            move_doc_end(ed);
            break;
    }
    
    ed->state = STATE_NORMAL;
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

// Move to start/end of document
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

// Cleanup
void cleanup_editor(Editor *ed) {
    // Free all lines
    Line *line = ed->doc.first_line;
    while (line) {
        Line *next = line->next;
        free_line(line);
        line = next;
    }
    
    restore_console(ed);
}

// Main program
int main(int argc, char *argv[]) {
    Editor editor;
    
    init_editor(&editor);
    
    // Load file if specified
    if (argc > 1) {
        load_file(&editor, argv[1]);
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
            }
        }
    }
    
    cleanup_editor(&editor);
    return 0;
}
