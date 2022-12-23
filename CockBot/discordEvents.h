#ifndef EVENTS_H_INCLUDED
#define EVENTS_H_INCLUDED

#include <concord/discord.h>

void disOnReady(struct discord *bot_client);

void disOnReactionAdd(struct discord *bot_client, u64snowflake dis_userId, u64snowflake dis_chanId, u64snowflake dis_msgId, u64snowflake dis_guildId, const struct discord_guild_member *dis_member, const struct discord_emoji *dis_emoji);

#endif
