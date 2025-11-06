#ifndef SIMPLE_UI_H
#define SIMPLE_UI_H

#include "common.h"

#define MAX_CHAT_MESSAGES 20
#define MAX_MESSAGE_LENGTH 256

void ui_init(void);
void ui_cleanup(void);
void ui_clear_screen(void);
void ui_draw_board(const char *board_display);
void ui_add_message(const char *message);
void ui_get_input(char *buffer, int size);
void ui_redraw_all(ClientState currentState);
void ui_clear_chat(void);

#endif // SIMPLE_UI_H
