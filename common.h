#ifndef COMMON_H
#define COMMON_H

typedef enum
{
   STATE_LOBBY,
   STATE_BIO,
   STATE_CHALLENGED,
   STATE_CHALLENGING,
   STATE_INGAME,
   STATE_SPECTATE
} ClientState;

#endif /* COMMON_H */
