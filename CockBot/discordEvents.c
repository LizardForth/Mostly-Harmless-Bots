#include <assert.h>
#include <concord/discord.h>
#include <concord/log.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cockbot.h"

void disOnReady(struct discord *bot_client, const struct discord_ready *event) {
  const struct discord_user *dis_bot = discord_get_self(bot_client);
  log_info("Logged in as %s!", dis_bot->username);
  struct discord_activity dis_activities[] = {
      {
          .name = BOT_STATUS,
          .type = DISCORD_ACTIVITY_GAME,
          .details = BOT_DETAILS,

      },
  };

  struct discord_presence_update dis_status = {
      .activities =
          &(struct discord_activities){
              .size = sizeof(dis_activities) / sizeof *dis_activities,
              .array = dis_activities,
          },
      .status = "online",
      .afk = false,
      .since = discord_timestamp(bot_client),
  };
  discord_set_presence(bot_client, &dis_status);
}

// As of right now this only counts the number of correct emotes on reaction.
void disOnReactionAdd(struct discord *bot_client, const struct discord_message_reaction_add *event) {
  u64snowflake dis_chanId = event->channel_id;
  u64snowflake dis_msgId = event->message_id;

  if (event->emoji->id != POLLO_ID) {
    return;
  }
  struct discord_message dis_msg;
  struct discord_ret_message dis_retMessage = {.sync = &dis_msg};
  discord_get_channel_message(bot_client, dis_chanId, dis_msgId,
                              &dis_retMessage);
  log_info("%s", dis_msg.content);
  for (int i = 0; i < dis_msg.reactions->size; i++) {
    if (dis_msg.reactions->array[i].count > 0 &&
        dis_msg.reactions->array[i].emoji->id == POLLO_ID) {
      log_info("Pollo Count: %d", dis_msg.reactions->array[i].count);
    }
  }
}
