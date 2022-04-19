#include <concord/discord.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
void disOnReady(struct discord *bot_client) {
  const struct discord_user *dis_bot = discord_get_self(bot_client);
  log_info("Logged in as %s! Running update checker", dis_bot->username);
}
void disExitTimerCb(struct discord *bot_client,
                    struct discord_timer *dis_timer) {
  log_info("Killing Updater Bot");
  discord_shutdown(bot_client);
}

void bot_updateRunner(struct discord *bot_client) {
  struct discord_embed embed = {.color = 4835913,
                                .timestamp = discord_timestamp(bot_client),
                                .title = "Deepthought is updating"};
  struct discord_create_message params = {
      .embeds =
          &(struct discord_embeds){
              .size = 1,
              .array = &embed,
          },
  };
  discord_create_message(bot_client, 966071069255565353, &params, NULL);
  remove("update.log");
  system("git pull");
#ifdef __linux__
  system("make clean");
  int status = system("make -j$(nproc) &> update.log");
#elif __NetBSD__
  system("make -f Makefile.bsd -j4 &> update.log");
  int status = system("make -f Makefile.bsd -j4 &> update.log");
#endif
  log_info("%d", status);
  if (status == 0) {
    embed.title = "Deep thought compiled succesfully";
    embed.description = "See attached log";
    struct discord_attachment attachment_arr[] = {
        {.filename = "update.log"},
    };
    struct discord_create_message params = {
        .embeds =
            &(struct discord_embeds){
                .size = 1,
                .array = &embed,
            },
    };
    discord_create_message(bot_client, 966071069255565353, &params, NULL);
    params.attachments = &(struct discord_attachments){
        .size = 1,
        .array = attachment_arr,
    };
    params.embeds->size = 0;
    discord_create_message(bot_client, 966071069255565353, &params, NULL);
  } else {
    embed.title = "Deepthought failed to compile see attached log";
    embed.description = "See attached log";
    embed.color = 16077157;
    struct discord_attachment attachment_arr[] = {
        {.filename = "update.log"},
    };
    struct discord_create_message params = {
        .embeds =
            &(struct discord_embeds){
                .size = 1,
                .array = &embed,
            },
    };
    discord_create_message(bot_client, 966071069255565353, &params, NULL);
    params.attachments = &(struct discord_attachments){
        .size = 1,
        .array = attachment_arr,
    };
    params.embeds->size = 0;
    discord_create_message(bot_client, 966071069255565353, &params, NULL);
  }
  discord_timer(bot_client, disExitTimerCb, NULL, 5000);
}

int main(void) {
  char bot_token[100];
  FILE *tokenFile;

  ccord_global_init();

  tokenFile = fopen("token.txt", "r");
  fscanf(tokenFile, "%[^\n]", bot_token);

  struct discord *bot_client = discord_init(bot_token);
  discord_set_on_ready(bot_client, &disOnReady);
  bot_updateRunner(bot_client);
  discord_run(bot_client);
  discord_cleanup(bot_client);
  ccord_global_cleanup();
}