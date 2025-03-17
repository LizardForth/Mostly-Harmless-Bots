// Guards and includes
// *********************

#ifndef COCKBOT_H_INCLUDED
#define COCKBOT_H_INCLUDED 

#include "ficl/ficl.h"
// *********************
// Global Forth
// *********************

extern ficlSystem *forth_system;

// *********************
// Values
// *********************

#define MUTE_ROLE 1052842619937501194

#define GUILD_ID 953769673634246757

#define BOT_STATUS "with FORTH" // Will display next to name
#define BOT_DETAILS "AAAAH"

#define ADMIN_ROLE 1235943635888111637

#define POLLO_ID 958592178026852352 // Emoji ID are fairly annoying to get.

// Convert regular rgb hex to decimal.
#define COLOR_SUCCESS 4835913  // #49CA49
#define COLOR_FAILURE 16077157 // #F55165

#define WATCHCAT_TIME                                                          \
  10 // If a given command takes longer than the said time in seconds it will
     // be automatically killed!

#define NOACCESS_ICON                                                          \
  "https://cdn.discordapp.com/emojis/1035646207692910612.webp" // Angry Chicken
                                                               // Emote Picture
#define SUCCESS_ICON                                                           \
  "https://cdn.discordapp.com/emojis/970384190547845170.gif" // Squished Chicken
                                                             // Emote Gif
#define ERROR_ICON                                                             \
  "https://cdn.discordapp.com/emojis/1035654365391900744.webp" // Sad Chicken
                                                               // Emote Picture

#define LINE_BUFFERSIZE 512 // Pretty big but I have a habbit of one liners.
#define ADMIN_SCRIPT "fth-scripts/admin.fth"

#endif
