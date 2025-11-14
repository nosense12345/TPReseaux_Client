#include "simple_ui.h"
#include "server2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char chat_messages[MAX_CHAT_MESSAGES][MAX_MESSAGE_LENGTH];
static int message_count = 0;
static char current_board_display[BUF_SIZE * 5] = "";

void ui_init(void) {
    // No special initialization needed for printf-based UI
}

void ui_cleanup(void) {
    // No special cleanup needed for printf-based UI
}

void ui_clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void ui_draw_board(const char *board_display) {
    strncpy(current_board_display, board_display, sizeof(current_board_display) - 1);
    current_board_display[sizeof(current_board_display) - 1] = '\0';

}

void ui_add_message(const char *message) {
    if (message_count < MAX_CHAT_MESSAGES) {
        strncpy(chat_messages[message_count], message, MAX_MESSAGE_LENGTH - 1);
        chat_messages[message_count][MAX_MESSAGE_LENGTH - 1] = '\0';
        message_count++;
    } else {
        // Shift messages up to make space for new one
        for (int i = 0; i < MAX_CHAT_MESSAGES - 1; i++) {
            strncpy(chat_messages[i], chat_messages[i+1], MAX_MESSAGE_LENGTH - 1);
            chat_messages[i][MAX_MESSAGE_LENGTH - 1] = '\0';
        }
        strncpy(chat_messages[MAX_CHAT_MESSAGES - 1], message, MAX_MESSAGE_LENGTH - 1);
        chat_messages[MAX_CHAT_MESSAGES - 1][MAX_MESSAGE_LENGTH - 1] = '\0';
    }
}

void ui_get_input(char *buffer, int size) {
    printf("> ");
    fgets(buffer, size, stdin);
    // Remove newline character
    buffer[strcspn(buffer, "\n")] = 0;
}

void ui_redraw_all(ClientState currentState) {
    ui_clear_screen();

    // Board Box
    printf("========================================\n");
    printf("|                BOARD                 |\n");
    printf("========================================\n");
    printf("%s\n", current_board_display);
    printf("========================================\n");

    // Chat/Output Box
    printf("========================================\n");
    printf("|                 CHAT                 |\n");
    printf("========================================\n");

    switch (currentState) {
        case STATE_LOBBY:
            printf("Available commands: /list, /bio, /viewbio [user], /challenge [user], /addfriend [user], /removefriend [user], /friends, /chat [user] [message], /spectate [user], /clearchat\n");
            break;
        case STATE_BIO:
            printf("Available commands: /endbio, /clearbio\n");
            break;
        case STATE_CHALLENGED:
            printf("Available commands: /accept [challenger], /refuse [challenger]\n");
            break;
        case STATE_CHALLENGING:
            printf("Available commands: /cancelchallenge\n");
            break;
        case STATE_INGAME:
            printf("Available commands: /move [pit], /quitgame\n");
            break;
        case STATE_SPECTATE:
            printf("Available commands: /stopspectate\n");
            break;
    }
    printf("----------------------------------------\n");

    for (int i = 0; i < message_count; i++) {
        printf("%s\n", chat_messages[i]);
    }
    printf("========================================\n");

    // Input Box
    printf("========================================\n");
    printf("|                INPUT                 |\n");
    printf("========================================\n");
    // Input prompt is handled by ui_get_input
}

void ui_clear_chat(void) {
    message_count = 0;
    ui_redraw_all(STATE_LOBBY); // Redraw with default state
}
