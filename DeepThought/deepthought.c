#include <assert.h>
#include <concord/discord.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ficl.h"

ficlSystem *forth_system;

struct forth_runnerArgs {
  char *forth_out;
  char *forth_in;
  int forth_rc;
  int bot_cmd;
  struct discord_message *dis_msg;
  struct discord *bot_client;
};

pthread_mutex_t forth_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t forth_done = PTHREAD_COND_INITIALIZER;

// Simple bot restart if you run the script it will pull from git and autoupdate
static void disRestart(ficlVm *forth_vm) { raise(SIGINT); }

// Nice example code that shows args and explains the actual forth discord
static void disPin(ficlVm *forth_vm) {
  const struct discord_message *dis_msg;
  struct discord *bot_client;
  ficlDictionary *forth_dict = ficlVmGetDictionary(forth_vm);
  // FICL Words can't be normal strings since forth uses counted strings.
  ficlString tempString1;
  ficlString tempString2;
  // A nice little undocumented string converter for forth
  FICL_STRING_SET_FROM_CSTRING(tempString1, "dis_msg");
  FICL_STRING_SET_FROM_CSTRING(tempString2, "bot_client");
  // Execute constants cotaining pointers to our message and client structs
  ficlVmExecuteWord(forth_vm, ficlDictionaryLookup(forth_dict, tempString2));
  ficlVmExecuteWord(forth_vm, ficlDictionaryLookup(forth_dict, tempString1));
  // explicitly grab them as pointers and assign structs to them
  dis_msg = ficlStackPopPointer(forth_vm->dataStack);
  bot_client = ficlStackPopPointer(forth_vm->dataStack);
  // all discord centric code and the like can be done now grabbing from stack
  discord_pin_message(bot_client, dis_msg->channel_id,
                      ficlStackPopInteger(forth_vm->dataStack), NULL);
}

static void disMuteCb(struct discord *bot_client,
                      struct discord_timer *dis_timer) {
  log_info("Unmuting: %lu", dis_timer->data);
  discord_remove_guild_member_role(bot_client, 953769673634246757,
                                   dis_timer->data, 965854189517406238, NULL);
}
static void disMute(ficlVm *forth_vm) {
  const struct discord_message *dis_msg;
  struct discord *bot_client;
  ficlDictionary *forth_dict = ficlVmGetDictionary(forth_vm);
  ficlString tempString1;
  ficlString tempString2;
  FICL_STRING_SET_FROM_CSTRING(tempString1, "dis_msg");
  FICL_STRING_SET_FROM_CSTRING(tempString2, "bot_client");
  ficlVmExecuteWord(forth_vm, ficlDictionaryLookup(forth_dict, tempString2));
  ficlVmExecuteWord(forth_vm, ficlDictionaryLookup(forth_dict, tempString1));
  dis_msg = ficlStackPopPointer(forth_vm->dataStack);
  bot_client = ficlStackPopPointer(forth_vm->dataStack);
  uint64_t *dis_userid = ficlStackPopInteger(forth_vm->dataStack);
  log_info("Muting: %lu", dis_userid);
  discord_add_guild_member_role(bot_client, 953769673634246757, dis_userid,
                                965854189517406238, NULL);
  discord_timer(bot_client, disMuteCb, dis_userid,
                ficlStackPopInteger(forth_vm->dataStack));
}
// Nice example for basic FICL words in C
static void disSpecs(ficlVm *forth_vm) {
  // This function is just going to be hard coded I can't be bothered
#ifdef __linux__
  ficlVmTextOut(forth_vm, "OS: Linux");
#elif __NetBSD__
  ficlVmTextOut(forth_vm, "OS: NetBSD\n");
  ficlVmTextOut(forth_vm, "RAM: 4GB\n");
  ficlVmTextOut(forth_vm, "CPU: BCM2711\n");
  ficlVmTextOut(forth_vm, "SYS: Raspberry Pi 4\n");
#else
  ficlVmTextOut(forth_vm, "OS: Other");
#endif
}

void disOnReady(struct discord *bot_client) {
  const struct discord_user *dis_bot = discord_get_self(bot_client);
  log_info("Logged in as %s!", dis_bot->username);
  struct discord_activity dis_activities[] = {
      {
          .name = "with FORTH",
          .type = DISCORD_ACTIVITY_GAME,
          .details = "AAAAH",

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
void disOnReactionAdd(struct discord *bot_client, u64snowflake dis_userId,
                      u64snowflake dis_chanId, u64snowflake dis_msgId,
                      u64snowflake dis_guildId,
                      const struct discord_guild_member *dis_member,
                      const struct discord_emoji *dis_emoji) {
  if (dis_emoji->id != 958592178026852352) {
    return;
  }
  struct discord_message dis_msg;
  struct discord_ret_message dis_retMessage = {.sync = &dis_msg};
  discord_get_channel_message(bot_client, dis_chanId, dis_msgId,
                              &dis_retMessage);
  log_info("%s", dis_msg.content);
  for (int i = 0; i < dis_msg.reactions->size; i++) {
    if (dis_msg.reactions->array[i].count > 0 &&
        dis_msg.reactions->array[i].emoji->id == 958592178026852352) {
      log_info("Pollo Count: %d", dis_msg.reactions->array[i].count);
    }
  }
}

int hasPostfix(char *strIn, char *postfix) {
  if (!strcmp(strrchr(strIn, '\0') - strlen(postfix), postfix)) {
    return 1;
  }
  return 0;
}

char *strReplace(const char *strIn, const char *strMatch,
                 const char *strReplace) {
  int i;
  int count = 0;
  // Counting the number of times old word
  // occur in the string
  for (i = 0; strIn[i] != '\0'; i++) {
    if (strstr(&strIn[i], strMatch) == &strIn[i]) {
      count++;

      // Jumping to index after the old word.
      i += strlen(strMatch);
      -1;
    }
  }

  // Making new string of enough length
  char *strOut =
      (char *)malloc(i + count * (strlen(strReplace) - strlen(strMatch)) + 1);

  i = 0;
  while (*strIn) {
    // compare the substring with the result
    if (strstr(strIn, strMatch) == strIn) {
      strcpy(&strOut[i], strReplace);
      i += strlen(strReplace);
      strIn += strlen(strMatch);
    } else
      strOut[i++] = *strIn++;
  }

  strOut[i] = '\0';
  return strOut;
}

void accessErrorEmbed(struct discord *bot_client,
                      const struct discord_message *dis_msg, char *forth_in) {
  struct discord_embed dis_embeds[] = {
      {.title = "Forth Bot Error: ",
       .color = 16077157,
       .fields =
           &(struct discord_embed_fields){
               .size = 1,
               .array = (struct discord_embed_field[]){{.name = "Input Code:",
                                                        .value = forth_in,
                                                        .Inline = false}}}},
      {.color = 16077157,
       .fields = &(struct discord_embed_fields){
           .size = 2,
           .array = (struct discord_embed_field[]){
               {.name = "Error Explanation:",
                .value = "Your code access a function or did something that "
                         "triggered one of our saftey checks. **Please don't "
                         "do that again**.",
                .Inline = false},
               {.name = "Output:",
                .value = "```Error: your code attempted to do something it "
                         "shouldn't```",
                .Inline = false}}}}};

  discord_create_message(
      bot_client, dis_msg->channel_id,
      &(struct discord_create_message){
          .allowed_mentions =
              &(struct discord_allowed_mention){
                  .replied_user = false,
              },
          .message_reference =
              &(struct discord_message_reference){
                  .message_id = dis_msg->id,
                  .channel_id = dis_msg->channel_id,
                  .guild_id = dis_msg->guild_id,
              },
          .embeds = &(struct discord_embeds){.size = sizeof(dis_embeds) /
                                                     sizeof *dis_embeds,
                                             .array = dis_embeds}},
      NULL);
}

int botGetCmd(struct discord *bot_client, const struct discord_message *dis_msg,
              char *forth_in) {
  char *forth_error = (char *)malloc(strlen(forth_in) + 9);
  snprintf(forth_error, strlen(forth_in) + 9, "``` %s ```", forth_in);

  if (hasPostfix(dis_msg->content, "!FTH") ||
      hasPostfix(dis_msg->content, "!ADM") ||
      hasPostfix(dis_msg->content, "!CMD") ||
      hasPostfix(dis_msg->content, "!fth") ||
      hasPostfix(dis_msg->content, "!adm") ||
      hasPostfix(dis_msg->content, "!cmd")) {
    if (hasPostfix(dis_msg->content, "!FTH") ||
        hasPostfix(dis_msg->content, "!fth")) {
      free(forth_error);
      return 1;
    } else if (hasPostfix(dis_msg->content, "!ADM") ||
               hasPostfix(dis_msg->content, "!CMD") ||
               hasPostfix(dis_msg->content, "!adm") ||
               hasPostfix(dis_msg->content, "!cmd")) {
      for (int i = 0; i < dis_msg->member->roles->size; i++) {
        if (dis_msg->member->roles->array[i] == 953785894656147566) {
          log_info("Admin is Executing");
          if (hasPostfix(dis_msg->content, "!ADM") ||
              hasPostfix(dis_msg->content, "!adm")) {

            free(forth_error);
            return 3;
          } else {
            free(forth_error);
            return 2;
          }
        }
      }
      accessErrorEmbed(bot_client, dis_msg, forth_error);
      free(forth_error);
      return 0;
    }
  }
  free(forth_error);
  return 0;
}

void cmdEmbed(struct discord *bot_client, const struct discord_message *dis_msg,
              char *forth_in, char *forth_out, int forth_rc) {
  if (forth_rc != -257) {
    char dis_embedTitle[32];
    snprintf(dis_embedTitle, 32, "Command Error: %d", forth_rc);
    struct discord_embed dis_embeds[] = {
        {
            .title = dis_embedTitle,
            .color = 16077157,
            .description = forth_out,
        },
    };
    struct discord_create_message dis_params = {
        .message_reference =
            &(struct discord_message_reference){
                .message_id = dis_msg->id,
                .channel_id = dis_msg->channel_id,
                .guild_id = dis_msg->guild_id,
            },
        .allowed_mentions =
            &(struct discord_allowed_mention){
                .replied_user = false,
            },

        .embeds =
            &(struct discord_embeds){
                .size = sizeof(dis_embeds) / sizeof *dis_embeds,
                .array = dis_embeds,
            },
    };
    discord_create_message(bot_client, dis_msg->channel_id, &dis_params, NULL);
  } else {
    struct discord_embed dis_embeds[] = {
        {
            .title = "Command Output",
            .color = 4835913,
            .description = forth_out,
        },
    };

    struct discord_create_message dis_params = {
        .message_reference =
            &(struct discord_message_reference){
                .message_id = dis_msg->id,
                .channel_id = dis_msg->channel_id,
                .guild_id = dis_msg->guild_id,
            },
        .allowed_mentions =
            &(struct discord_allowed_mention){
                .replied_user = false,
            },

        .embeds =
            &(struct discord_embeds){
                .size = sizeof(dis_embeds) / sizeof *dis_embeds,
                .array = dis_embeds,
            },
    };
    discord_create_message(bot_client, dis_msg->channel_id, &dis_params, NULL);
  }
}

void errEmbed(struct discord *bot_client, const struct discord_message *dis_msg,
              char *forth_in, char *forth_outFormatted, int forth_rc,
              char *forth_out) {
  struct discord_embed dis_embeds[3] = {0};
  dis_embeds[0].color = 16077157;
  dis_embeds[1].color = 16077157;
  dis_embeds[2].color = 16077157;
  discord_embed_set_title(&dis_embeds[0], "Forth Bot Error:");

  discord_embed_add_field(&dis_embeds[0], "Input Code:", forth_in, false);

  switch (forth_rc) {
  case -256:
    discord_embed_add_field(&dis_embeds[1], "Error Explanation:",
                            "Your code exited the inner interpreter loop",
                            false);
    break;
  case -258:
    discord_embed_add_field(&dis_embeds[1], "Error Explanation:",
                            "One of your words needs more text to "
                            "execute, try re-running it.",
                            false);
    break;
  case -260:
    discord_embed_add_field(
        &dis_embeds[1], "Error Explanation:",
        "Your code has encountered an error check output for info.", false);
    if (strstr(forth_out, "not found")) {
      discord_embed_set_title(&dis_embeds[2], "Forth Bot: Attempted Debug");
      discord_embed_set_description(
          &dis_embeds[2],
          "Warning the info in here is a best guess it may or may not be "
          "correct. The debugger gurantees help but not helpfulness.");
      discord_embed_add_field(&dis_embeds[2], "Detected Issue:",
                              "Your code contains a word which is not in "
                              "the current forth dictionary",
                              false);
      discord_embed_add_field(
          &dis_embeds[2], "Proposed solutions:",
          "1) Make sure the word is valid\n2) Check your function "
          "definitions\n3) Check dictionary modifier words such as "
          "``CREATE``\n4) yell at it",
          false);
    } else if (strstr(forth_out, "Error: data stack underflow")) {
      discord_embed_set_title(&dis_embeds[2], "Forth Bot: Attempted Debug");
      discord_embed_set_description(
          &dis_embeds[2],
          "Warning the info in here is a best guess it may or may not be "
          "correct. The debugger gurantees help but not helpfulness.");
      discord_embed_add_field(
          &dis_embeds[2], "Detected Issue:",
          "One of your words drew too much data from the data stack", false);
      discord_embed_add_field(
          &dis_embeds[2], "Proposed Solutions:",
          "1) Check your most recently added code\n2) Use the ``.s`` "
          "word. it will show you the data stack without clearing it\n3) "
          "check the order in which data is recieved, FORTH is reverse "
          "polish\n4) Cry",
          false);
    } else if (strstr(forth_out, "FICL_VM_STATE_COMPILE")) {
      discord_embed_set_title(&dis_embeds[2], "Forth Bot: Attempted Debug");
      discord_embed_set_description(
          &dis_embeds[2],
          "Warning the info in here is a best guess it may or may not be "
          "correct. The debugger gurantees help but not helpfulness.");
      discord_embed_add_field(
          &dis_embeds[2], "Detected Issue:",
          "You've used a word which only works in the compiler", false);
      discord_embed_add_field(
          &dis_embeds[2], "Proposed Solutions:",
          "1) This word can only be compiled\n2) Make sure it is defined "
          "in a funtion like so: ``: FUNCTION-NAME CODE ;``\n3) Make "
          "sure you aren't changing the compiler state to interpret\n4) "
          "Check for words that change compilers behaviour such as "
          "``CREATE LITERAL IMMEDIATE``\n5) Cry",
          false);
      discord_embed_add_field(
          &dis_embeds[2], "Friendly Tip:",
          " Forth differentiates between words when it compiles them and "
          "when they are interpreted. Some words have different "
          "definitions at compile time. If you wish to run a word as "
          "interpreted surrond it with ``[ ]``. Forth is "
          "semi-unique(especially in comparison with modern languages) "
          "in its handeling of the compiler, understanding this is the "
          "basis of higher level FORTH.",
          false);
      discord_embed_add_field(
          &dis_embeds[2], "Friendly Tip:",
          "If you write a function in forth it is standard to use the "
          "following notation to describe its output ``( x -- x )``. "
          "Where x designates data, and the left designates data "
          "retrieved from the stack, and the right shows the data that "
          "will be pushed on to the stack. ",
          false);
      discord_embed_add_field(
          &dis_embeds[2], "Friendly Tip:",
          "If you want to know more about compiler behaviour and "
          "modifying it, "
          "https://www.forth.com/starting-forth/"
          "11-forth-compiler-defining-words/ is a good resource.",
          false);
    }
    break;
  case -259:
    discord_embed_add_field(&dis_embeds[1], "Error Explanation:",
                            "Your code  attempted to exit.", false);
    break;
  case -261:
    discord_embed_add_field(&dis_embeds[1], "Error Explanation:",
                            "Your code reached a debugger breakpoint.", false);
    break;
  case -1:
    discord_embed_add_field(&dis_embeds[1], "Error Explanation",
                            "Your code encountered an error that made it "
                            "abort. Check the output",
                            false);
    break;
  case -2:
    discord_embed_add_field(
        &dis_embeds[1], "Error Explanation",
        "Your code encountered an error that made it abort\". Check the "
        "output. Note this is different than the normal abort.",
        false);
    break;
  case -56:
    discord_embed_add_field(&dis_embeds[1], "Error Explanation",
                            "Your code encountered an error that made it "
                            "quit. Check the output",
                            false);
    break;
  default:
    break;
  }
  discord_embed_add_field(&dis_embeds[1], "Error Output:", forth_outFormatted,
                          false);
  discord_create_message(
      bot_client, dis_msg->channel_id,
      &(struct discord_create_message){
          .allowed_mentions =
              &(struct discord_allowed_mention){
                  .replied_user = false,
              },
          .message_reference =
              &(struct discord_message_reference){
                  .message_id = dis_msg->id,
                  .channel_id = dis_msg->channel_id,
                  .guild_id = dis_msg->guild_id,
              },

          .embeds = &(struct discord_embeds){.size = sizeof(dis_embeds) /
                                                     sizeof *dis_embeds,
                                             .array = dis_embeds}},
      NULL);
  discord_embed_cleanup(&dis_embeds[0]);
  discord_embed_cleanup(&dis_embeds[1]);
  discord_embed_cleanup(&dis_embeds[2]);
}

void regEmbed(struct discord *bot_client, const struct discord_message *dis_msg,
              char *forth_in, char *forth_out) {
  struct discord_embed dis_embeds[] = {
      {.title = "Forth Bot:",
       .color = 4835913,
       .fields =
           &(struct discord_embed_fields){
               .size = 1,
               .array = (struct discord_embed_field[]){{.name = "Input Code:",
                                                        .value = forth_in,
                                                        .Inline = false}}}},
      {.color = 4835913,
       .fields = &(struct discord_embed_fields){
           .size = 1,
           .array = (struct discord_embed_field[]){
               {.name = "Output:", .value = forth_out, .Inline = false}}}}};

  discord_create_message(
      bot_client, dis_msg->channel_id,
      &(struct discord_create_message){
          .allowed_mentions =
              &(struct discord_allowed_mention){
                  .replied_user = false,
              },
          .message_reference =
              &(struct discord_message_reference){
                  .message_id = dis_msg->id,
                  .channel_id = dis_msg->channel_id,
                  .guild_id = dis_msg->guild_id,
              },
          .embeds = &(struct discord_embeds){.size = sizeof(dis_embeds) /
                                                     sizeof *dis_embeds,
                                             .array = dis_embeds}},
      NULL);
}

void timeoutEmbed(struct discord *bot_client,
                  const struct discord_message *dis_msg, char *forth_in) {
  struct discord_embed dis_embeds[] = {
      {.title = "Forth Bot Error: ",
       .color = 16077157,
       .fields =
           &(struct discord_embed_fields){
               .size = 1,
               .array = (struct discord_embed_field[]){{.name = "Input Code:",
                                                        .value = forth_in,
                                                        .Inline = false}}}},
      {.color = 16077157,
       .fields = &(struct discord_embed_fields){
           .size = 2,
           .array = (struct discord_embed_field[]){
               {.name = "Error Explanation:",
                .value = "Your code took longer than expected to execute. In "
                         "most cases this is an infinite loop.",
                .Inline = false},
               {.name = "Output:",
                .value = "```Error: your code took too long to execute```",
                .Inline = false}}}}};

  discord_create_message(
      bot_client, dis_msg->channel_id,
      &(struct discord_create_message){
          .allowed_mentions =
              &(struct discord_allowed_mention){
                  .replied_user = false,
              },
          .message_reference =
              &(struct discord_message_reference){
                  .message_id = dis_msg->id,
                  .channel_id = dis_msg->channel_id,
                  .guild_id = dis_msg->guild_id,
              },
          .embeds = &(struct discord_embeds){.size = sizeof(dis_embeds) /
                                                     sizeof *dis_embeds,
                                             .array = dis_embeds}},
      NULL);
}

void *forthRunner(void *input) {
  log_info("Starting Runner");
  ficlVm *forth_vm;
  forth_vm = ficlSystemCreateVm(forth_system);
  ficlDictionary *forth_dict = ficlVmGetDictionary(forth_vm);
  if (((struct forth_runnerArgs *)input)->bot_cmd == 2 ||
      ((struct forth_runnerArgs *)input)->bot_cmd == 3) {
    ficlDictionarySetPrimitive(forth_dict, "restart", disRestart,
                               FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(forth_dict, "pin", disPin, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(forth_dict, "mute", disMute, FICL_WORD_DEFAULT);
    ficlDictionarySetConstant(forth_dict, "dis_msg",
                              ((struct forth_runnerArgs *)input)->dis_msg);
    ficlDictionarySetConstant(forth_dict, "bot_client",
                              ((struct forth_runnerArgs *)input)->bot_client);
  }
  ficlDictionarySetPrimitive(forth_dict, "specs", disSpecs, FICL_WORD_DEFAULT);
  log_info("Recieved: %s", ((struct forth_runnerArgs *)input)->forth_in);
  fflush(stdout);
  freopen("/dev/null", "a", stdout);
  setbuf(stdout, ((struct forth_runnerArgs *)input)->forth_out);
  ((struct forth_runnerArgs *)input)->forth_rc = -127;
  ((struct forth_runnerArgs *)input)->forth_rc =
      ficlVmEvaluate(forth_vm, ((struct forth_runnerArgs *)input)->forth_in);
  if (((struct forth_runnerArgs *)input)->forth_rc == -257) {
    ficlVmTextOut(forth_vm, "ok");
  }
  fflush(stdout);
  freopen("/dev/tty", "a", stdout);
  pthread_cond_signal(&forth_done);
  pthread_exit(NULL);
}

void forthWatchCat(void *input) {
  struct timespec watchCatTimer;
  clock_gettime(CLOCK_REALTIME, &watchCatTimer);
  watchCatTimer.tv_sec += 10;

  log_info("Starting WatchCat");
  pthread_mutex_lock(&forth_mutex);
  if (pthread_cond_timedwait(&forth_done, &forth_mutex, &watchCatTimer) == 0) {
    log_info("Executed Successfully");
  } else {
    log_info("timed out");
    log_info("Killing %d", (pthread_t)input);
    pthread_kill((pthread_t)input, SIGKILL);
  }
  pthread_mutex_unlock(&forth_mutex);
  pthread_exit(NULL);
}

void disOnMessage(struct discord *bot_client,
                  const struct discord_message *dis_msg) {

  char forth_out[10000] = {0};
  char forth_outFormatted[10009];

  if (dis_msg->author->bot)
    return;
  int bot_cmd = botGetCmd(bot_client, dis_msg, dis_msg->content);
  if (bot_cmd == 0) {
    return;
  }

  if (strlen(dis_msg->content) > 10002) {
    struct discord_embed dis_embed = {
        .color = 16077157,
    };

    discord_embed_set_title(&dis_embed, "Warning");
    discord_embed_set_description(&dis_embed,
                                  "I will not accept excessively long code");

    struct discord_create_message dis_params = {
        .embeds =
            &(struct discord_embeds){
                .size = 1,
                .array = &dis_embed,
            },
    };

    discord_create_message(bot_client, dis_msg->channel_id, &dis_params, NULL);
    return;
  }
  char *forth_in = (char *)malloc(strlen(dis_msg->content) - 3);
  strncpy(forth_in, dis_msg->content, strlen(dis_msg->content) - 4);
  forth_in[strlen(dis_msg->content) - 4] = 0;

  int forth_rc;

  char *forth_inOld = (char *)malloc(strlen(forth_in));

  strncpy(forth_inOld, forth_in, strlen(forth_in) + 1);
  char *bot_mentionPrep[32];
  char *bot_mentionId[32];

  for (int i = 0; i < dis_msg->mentions->size; i++) {
    char *forth_mentionPrep = strdup(forth_in);
    sprintf(bot_mentionPrep, "<@%lu>", dis_msg->mentions->array[i].id);
    sprintf(bot_mentionId, "%lu", dis_msg->mentions->array[i].id);
    strncpy(forth_in, strReplace(forth_in, bot_mentionPrep, bot_mentionId),
            strlen(forth_in) + 1);
    free(forth_mentionPrep);
  }

  log_info("Prefix number: %d", bot_cmd);

  if (bot_cmd == 3 || bot_cmd == 2) {
    char *forth_addon = ": TEST .\" TEST\" ; ";
    char *forth_prep = strdup(forth_in);
    forth_in =
        (char *)realloc(forth_in, strlen(forth_in) + strlen(forth_addon) + 2);
    snprintf(forth_in, strlen(forth_in) + strlen(forth_addon) + 1, "%s %s",
             forth_addon, forth_prep);
    free(forth_prep);
  }

  char *forth_inFormatted = (char *)malloc(strlen(forth_inOld) + 9);

  log_info("Parsed: %s", forth_in);

  struct forth_runnerArgs *forth_runnerIn =
      (struct forth_runnerArgs *)malloc(sizeof(struct forth_runnerArgs));
  forth_runnerIn->forth_in = forth_in;
  forth_runnerIn->forth_out = forth_out;
  forth_runnerIn->bot_cmd = bot_cmd;
  forth_runnerIn->dis_msg = dis_msg;
  forth_runnerIn->bot_client = bot_client;

  pthread_t forth_runnerTid;
  pthread_t forth_watchCatTid;
  pthread_create(&forth_watchCatTid, NULL, forthWatchCat,
                 (void *)&forth_runnerTid);
  pthread_create(&forth_runnerTid, NULL, forthRunner, (void *)forth_runnerIn);
  pthread_join(forth_watchCatTid, NULL);

  forth_rc = forth_runnerIn->forth_rc;

  log_info("Exit Code: %d", forth_rc);
  snprintf(forth_outFormatted, strlen(forth_out) + 9, "``` %s ```", forth_out);
  snprintf(forth_inFormatted, strlen(forth_inOld) + 9, "``` %s ```",
           forth_inOld);

  log_info("Output: %s", forth_out);

  if (forth_rc == -127) {
    timeoutEmbed(bot_client, dis_msg, forth_inFormatted);
  } else {
    if (bot_cmd == 2) {
      cmdEmbed(bot_client, dis_msg, forth_inFormatted, forth_outFormatted,
               forth_rc);
    } else if (forth_rc != -257) {
      errEmbed(bot_client, dis_msg, forth_inFormatted, forth_outFormatted,
               forth_rc, forth_out);
    } else {
      regEmbed(bot_client, dis_msg, forth_inFormatted, forth_outFormatted);
    }
  }
  free(forth_in);
  free(forth_inOld);
  free(forth_inFormatted);
  free(forth_runnerIn);
}

int main(void) {
  char bot_token[100];
  FILE *tokenFile;

  pthread_mutex_init(&forth_mutex, NULL);
  pthread_cond_init(&forth_done, NULL);

  ccord_global_init();

  forth_system = ficlSystemCreate(NULL);

  tokenFile = fopen("token.txt", "r");
  fscanf(tokenFile, "%[^\n]", bot_token);
  struct discord *bot_client = discord_init(bot_token);

  discord_set_on_ready(bot_client, &disOnReady);
  discord_set_on_message_create(bot_client, &disOnMessage);
  discord_set_on_message_reaction_add(bot_client, &disOnReactionAdd);

  discord_run(bot_client);

  discord_cleanup(bot_client);
  ccord_global_cleanup();
}
