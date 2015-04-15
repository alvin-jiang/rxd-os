/**
 *
 * @file: keyboard.h
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-04-14
 *
 */


#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

/* keyboard scan codes: set 1 */
#define NR_KEYS     128

// normal keys: with make/break, with press repeating
#define KEY_A       0x1E
#define KEY_B       0x30
#define KEY_C       0x2E
#define KEY_D       0x20
#define KEY_E       0x12
#define KEY_F       0x21
#define KEY_G       0x22
#define KEY_H       0x23
#define KEY_I       0x17
#define KEY_J       0x24
#define KEY_K       0x25
#define KEY_L       0x26
#define KEY_M       0x32
#define KEY_N       0x31
#define KEY_O       0x18
#define KEY_P       0x19
#define KEY_Q       0x10
#define KEY_R       0x13
#define KEY_S       0x1F
#define KEY_T       0x14
#define KEY_U       0x16
#define KEY_V       0x2F
#define KEY_W       0x11
#define KEY_X       0x2D
#define KEY_Y       0x15
#define KEY_Z       0x2C
#define KEY_0       0x0B
#define KEY_1       0x02
#define KEY_2       0x03
#define KEY_3       0x04
#define KEY_4       0x05
#define KEY_5       0x06
#define KEY_6       0x07
#define KEY_7       0x08
#define KEY_8       0x09
#define KEY_9       0x0A
#define KEY_F1      0x3B
#define KEY_F2      0x3C
#define KEY_F3      0x3D
#define KEY_F4      0x3E
#define KEY_F5      0x3F
#define KEY_F6      0x40
#define KEY_F7      0x41
#define KEY_F8      0x42
#define KEY_F9      0x43
#define KEY_F10     0x44
#define KEY_F11     0x57
#define KEY_F12     0x58

#define KEY_ESC     0x01
#define KEY_ENTER   0x1C
#define KEY_TAB     0x0F
#define KEY_SPACE   0x39

#define KEY_GRAVE           0x29    // '`'
#define KEY_MINUS           0x0C    // '-'
#define KEY_EQUALS          0x0D    // '='
#define KEY_LEFT_BRACKET    0x1A    // '['
#define KEY_RIGHT_BRACKET   0x1B    // ']'
#define KEY_BACKSLASH       0x2B    // '\'
#define KEY_SEMICOLON       0x27    // ';'
#define KEY_SINGLE_QUOTE    0x28    // '''
#define KEY_COMMA           0x33    // ','
#define KEY_PERIOD          0x34    // '.'
#define KEY_SLASH           0x35    // '/'

// control keys: with make/break, no press repeating
#define KEY_ALT_L   0x38
#define KEY_SHIFT_L 0x2A
#define KEY_CTRL_L  0x1D
#define KEY_CAPS_LOCK       0x3A
#define KEY_NUM_LOCK        0x45
//#define KEY_SCROLL_LOCK   0x46

// special keys: scan code start with E0/E1
#define KEY_INSERT
#define KEY_DELETE
#define KEY_HOME
#define KEY_END
#define KEY_PAGE_UP
#define KEY_PAGE_DOWN
#define KEY_UP
#define KEY_DOWN
#define KEY_LEFT
#define KEY_RIGHT

//#define KEY_PRINT_SCREEN
//#define KEY_PAUSE

// keypad keys
#define KEY_PAD_0
#define KEY_PAD_1
#define KEY_PAD_2
#define KEY_PAD_3
#define KEY_PAD_4
#define KEY_PAD_5
#define KEY_PAD_6
#define KEY_PAD_7
#define KEY_PAD_8
#define KEY_PAD_9
#define KEY_PAD_DIVIDE
#define KEY_PAD_MULTIPLY
#define KEY_PAD_SUBTRACT
#define KEY_PAD_ADD
#define KEY_PAD_ENTER
#define KEY_PAD_DECIMAL

// control flag
#define KB_F_CTRL           0x0001
#define KB_F_ALT            0x0002
#define KB_F_SHIFT          0x0004
#define KB_F_E0             0x0008
// #define KB_F_CAPS_LOCK
// #define KB_F_NUM

int kb_keycode (int scan_code);
unsigned char kb_ascii (int scan_code, int kb_flag);

#endif

