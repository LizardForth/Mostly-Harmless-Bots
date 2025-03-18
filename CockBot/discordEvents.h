#ifndef EVENTS_H_INCLUDED
#define EVENTS_H_INCLUDED

#include <discord.h>

void disOnReady(struct discord *bot_client, const struct discord_ready *event);

void disOnReactionAdd(struct discord *bot_client, const struct discord_message_reaction_add *event);

#endif
