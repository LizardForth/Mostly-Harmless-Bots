#include <assert.h>
#include <concord/discord.h>
#include <concord/log.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

#include "cockbot.h"
#include "discordEvents.h"
#include "ficl/ficl.h"
#include "forthFunctions.h"

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

int hasPostfix(char *strIn, char *postfix) {
  return (!strcmp(&strIn[strlen(strIn) - 4], postfix));
}

char *strReplace(const char *strIn, const char *strMatch,
                 const char *strReplace) {
  int i;
  int count = 0;
  // Counting the number of times old word
  // occur in the string
  for (i = 0; strIn[i] != '\0'; i++) {
    if (strstr(&strIn[i], strMatch) == &strIn[i]) {

      // Jumping to index after the old word.
      i += strlen(strMatch);
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
       .footer =
           &(struct discord_embed_footer){
               .text = "TESTING TESTING",
               .icon_url = NOACCESS_ICON,
           },
       .fields = &(struct discord_embed_fields){
           .size = 2,
           .array = (struct discord_embed_field[]){
               {.name = "Error Explanation:",
                .value = "Your code accessed a function or did something that "
                         "triggered one of our safety checks. **Please don't "
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

void helpEmbed(struct discord *bot_client,
               const struct discord_message *dis_msg) {
  struct discord_embed dis_embeds[] = {
      {.title = "Cockbot Help:",
       .color = COLOR_SUCCESS,
       .fields =
           &(struct discord_embed_fields){
               .size = 2,
               .array =
                   (struct discord_embed_field[]){

                       {.name = "Forth Starter Guide:",
                        .value = "https://www.forth.com/starting-forth/",
                        .Inline = false},
                       {.name = "Forth Starting Guide:",
                        .value =
                            "All CockBot commands are interpreted in a "
                            "programming language known as forth. This allows "
                            "you to create scriptable commands, and many other "
                            "neat things! In order to invoke CockBot you must "
                            "end your command with one of the invokers.",
                        .Inline = false}}}},

      {.color = COLOR_SUCCESS,
       .title = "Invokers:",
       .footer =
           &(struct discord_embed_footer){
               .text = "TESTING TESTING",
               .icon_url = NOACCESS_ICON,
           },
       .fields = &(struct discord_embed_fields){
           .size = 4,
           .array = (struct discord_embed_field[]){
               {.name = "!fth",
                .value = "Simply executes the command with full output also "
                         "includes an occasionally helpful error checker that "
                         "sometimes gives tips.",
                .Inline = false},
               {.name = "!hlp",
                .value = "Shows this help guide.",
                .Inline = false},
               {.name = "!adm",
                .value = "Same as !FTH but for admins.",
                .Inline = false},
               {.name = "!cmd",
                .value = "Same as !adm but won't return the input or formatted "
                         "output code. It's quite useful for admin scripts.",
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

// command parsing
int botGetCmd(struct discord *bot_client, struct discord_message *dis_msg,
              char *forth_in) {
  char *forth_error = (char *)malloc(strlen(forth_in) + 9);
  snprintf(forth_error, strlen(forth_in) + 9, "``` %s ```", forth_in);

  if (hasPostfix(dis_msg->content, "!fth") ||
      hasPostfix(dis_msg->content, "!FTH")) {
    free(forth_error);
    return 1;
  } else if (hasPostfix(dis_msg->content, "!hlp") ||
             hasPostfix(dis_msg->content, "!HLP")) {
    free(forth_error);
    return 4;
  } else if (hasPostfix(dis_msg->content, "!adm") ||
             hasPostfix(dis_msg->content, "!ADM") ||
             hasPostfix(dis_msg->content, "!cmd") ||
             hasPostfix(dis_msg->content, "!CMD")) {
    for (int i = 0; i < dis_msg->member->roles->size; i++) {
      if (dis_msg->member->roles->array[i] == ADMIN_ROLE) {
        // log_info("Admin is Executing");
        if (hasPostfix(dis_msg->content, "!adm") ||
            hasPostfix(dis_msg->content, "!ADM")) {
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
            .color = COLOR_FAILURE,
            .description = forth_out,
            .footer =
                &(struct discord_embed_footer){
                    .text = "TESTING TESTING",
                    .icon_url = ERROR_ICON,
                },
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
            .color = COLOR_SUCCESS,
            .description = forth_out,
            .footer =
                &(struct discord_embed_footer){
                    .text = "TESTING TESTING",
                    .icon_url = SUCCESS_ICON,
                },
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

void toolongEmbed(struct discord *bot_client,
                  const struct discord_message *dis_msg, char *forth_in,
                  char *forth_outFormatted, int forth_rc, char *forth_out) {
  struct discord_embed dis_embeds[3] = {0};
  dis_embeds[0].color = COLOR_FAILURE;
  dis_embeds[1].color = COLOR_FAILURE;
  dis_embeds[2].color = COLOR_FAILURE;
  discord_embed_set_title(&dis_embeds[0], "Forth Bot Error:");
  discord_embed_add_field(&dis_embeds[0], "Input Code:", forth_in, false);
  discord_embed_add_field(&dis_embeds[1], "Error Explanation:",
                          "The output of the program is too long to be printed",
                          false);
  discord_embed_add_field(
      &dis_embeds[1], "Error Output:", "[More than 1024 characters]", false);
  discord_embed_set_footer(&dis_embeds[2], "TESTING TESTING", ERROR_ICON, "");
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
void errEmbed(struct discord *bot_client, const struct discord_message *dis_msg,
              char *forth_in, char *forth_outFormatted, int forth_rc,
              char *forth_out) {
  struct discord_embed dis_embeds[3] = {0};
  dis_embeds[0].color = COLOR_FAILURE;
  dis_embeds[1].color = COLOR_FAILURE;
  dis_embeds[2].color = COLOR_FAILURE;
  discord_embed_set_title(&dis_embeds[0], "Forth Bot Error:");

  discord_embed_add_field(&dis_embeds[0], "Input Code:", forth_in, false);

  switch (forth_rc) {
  case FICL_VM_STATUS_INNER_EXIT:
    discord_embed_add_field(&dis_embeds[1], "Error Explanation:",
                            "Your code exited the inner interpreter loop",
                            false);
    break;
  case FICL_VM_STATUS_RESTART:
    discord_embed_add_field(&dis_embeds[1], "Error Explanation:",
                            "One of your words needs more text to "
                            "execute, try re-running it.",
                            false);
    break;
  case FICL_VM_STATUS_ERROR_EXIT:
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
  case FICL_VM_STATUS_USER_EXIT:
    discord_embed_add_field(&dis_embeds[1], "Error Explanation:",
                            "Your code  attempted to exit.", false);
    break;
  case FICL_VM_STATUS_BREAK:
    discord_embed_add_field(&dis_embeds[1], "Error Explanation:",
                            "Your code reached a debugger breakpoint.", false);
    break;
  case FICL_VM_STATUS_ABORT:
    discord_embed_add_field(&dis_embeds[1], "Error Explanation",
                            "Your code encountered an error that made it "
                            "abort. Check the output",
                            false);
    break;
  case FICL_VM_STATUS_ABORTQ:
    discord_embed_add_field(
        &dis_embeds[1], "Error Explanation",
        "Your code encountered an error that made it abort\". Check the "
        "output. Note this is different than the normal abort.",
        false);
    break;
  case FICL_VM_STATUS_QUIT:
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
  discord_embed_set_footer(&dis_embeds[2], "TESTING TESTING", ERROR_ICON, "");
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
       .color = COLOR_SUCCESS,
       .fields =
           &(struct discord_embed_fields){
               .size = 1,
               .array = (struct discord_embed_field[]){{.name = "Input Code:",
                                                        .value = forth_in,
                                                        .Inline = false}}}},
      {.color = COLOR_SUCCESS,
       .footer =
           &(struct discord_embed_footer){
               .text = "TESTING TESTING",
               .icon_url = SUCCESS_ICON,
           },
       .fields = &(struct discord_embed_fields){
           .size = 1,
           .array = (struct discord_embed_field[]){
               {.name = "Output:",
                .value = strlen(forth_out) < 1024 ? forth_out
                                                  : "Error: Program too long",
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

void timeoutEmbed(struct discord *bot_client,
                  const struct discord_message *dis_msg, char *forth_in) {
  struct discord_embed dis_embeds[] = {
      {.title = "Forth Bot Error: ",
       .color = COLOR_FAILURE,
       .fields =
           &(struct discord_embed_fields){
               .size = 1,
               .array = (struct discord_embed_field[]){{.name = "Input Code:",
                                                        .value = forth_in,
                                                        .Inline = false}}}},
      {.color = COLOR_FAILURE,
       .footer =
           &(struct discord_embed_footer){
               .text = "TESTING TESTING",
               .icon_url = NOACCESS_ICON,
           },
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
  log_info("Starting Forth");
  log_info("Starting Runner");
  ficlVm *forth_vm;
  forth_vm = ficlSystemCreateVm(forth_system);
  ficlDictionary *forth_dict = ficlVmGetDictionary(forth_vm);

  ficlDictionarySetPrimitive(forth_dict, "uptime", disUptime,
                             FICL_WORD_DEFAULT);
  ficlDictionarySetPrimitive(forth_dict, "rand", disUptime, FICL_WORD_DEFAULT);

  if (((struct forth_runnerArgs *)input)->bot_cmd == 2 ||
      ((struct forth_runnerArgs *)input)->bot_cmd == 3) {
    ficlDictionarySetPrimitive(forth_dict, "restart", disRestart,
                               FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(forth_dict, "pin", disPin, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(forth_dict, "ohio", disOhio, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(forth_dict, "unohio", disUnOhio,
                               FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(forth_dict, "ban", disBan, FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(forth_dict, "delban", disDelBan,
                               FICL_WORD_DEFAULT);
    ficlDictionarySetPrimitive(forth_dict, "kick", disKick, FICL_WORD_DEFAULT);
    ficlDictionarySetConstant(
        forth_dict, "dis_msg",
        (ficlInteger)(((struct forth_runnerArgs *)input)->dis_msg));
    ficlDictionarySetConstant(
        forth_dict, "bot_client",
        (ficlInteger)(((struct forth_runnerArgs *)input)->bot_client));
    ficlDictionarySetPrimitive(forth_dict, "load", disLoadScript,
                               FICL_WORD_DEFAULT);
  }

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
  log_info("Destroying ForthVM");
  ficlVmDestroy(forth_vm);
  pthread_cond_signal(&forth_done);
  pthread_exit(NULL);
}

void *forthWatchCat(void *input) {
  struct timespec watchCatTimer;
  clock_gettime(CLOCK_REALTIME, &watchCatTimer);
  watchCatTimer.tv_sec += WATCHCAT_TIME;

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
  return NULL;
}

void disOnMessage(struct discord *bot_client,
                  const struct discord_message *dis_msg) {

  char forth_out[10000] = {0};
  char forth_outFormatted[10009];

  if (dis_msg->author->bot)
    return;

  int bot_cmd = botGetCmd(bot_client, dis_msg, dis_msg->content);
  if (bot_cmd == 4) {
    helpEmbed(bot_client, dis_msg);
    return;
  }
  if (bot_cmd == 0) {
    return;
  }

  if (strlen(dis_msg->content) > 10002) {
    struct discord_embed dis_embed = {
        .color = COLOR_FAILURE,
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
  forth_in[strlen(dis_msg->content) - 4] = '\0';

  int forth_rc;

  char *forth_inOld = (char *)malloc(strlen(forth_in) + 1);

  strncpy(forth_inOld, forth_in, strlen(forth_in) + 1);
  char *bot_mentionPrep = (char *)malloc(strlen(forth_in));
  char *bot_mentionId = (char *)malloc(strlen(forth_in));

  for (int i = 0; i < dis_msg->mentions->size; i++) {
    char *forth_mentionPrep = strdup(forth_in);
    sprintf(bot_mentionPrep, "<@%lu>", dis_msg->mentions->array[i].id);
    sprintf(bot_mentionId, "%lu", dis_msg->mentions->array[i].id);
    strncpy(forth_in, strReplace(forth_in, bot_mentionPrep, bot_mentionId),
            strlen(forth_in) + 1);
    free(forth_mentionPrep);
  }

  free(bot_mentionId);
  free(bot_mentionPrep);

  log_info("Prefix number: %d", bot_cmd);

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
  snprintf(forth_outFormatted, strlen(forth_out) + 10, "``` %s ```", forth_out);
  snprintf(forth_inFormatted, strlen(forth_inOld) + 10, "``` %s ```",
           forth_inOld);

  log_info("Output: %s", forth_out);

  if (forth_rc == -127) {
    timeoutEmbed(bot_client, dis_msg, forth_inFormatted);
  } else {
    if (bot_cmd == 2) {
      cmdEmbed(bot_client, dis_msg, forth_inFormatted, forth_outFormatted,
               forth_rc);
    } else if (forth_rc != FICL_VM_STATUS_OUT_OF_TEXT) {
      errEmbed(bot_client, dis_msg, forth_inFormatted, forth_outFormatted,
               forth_rc, forth_out);
    } else if (strlen(forth_outFormatted) > 1024) {
      toolongEmbed(bot_client, dis_msg, forth_inFormatted, forth_outFormatted,
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

time_t uptime;
unsigned long long polloemotes[] = {
    982486221261508608,  1167501376696352859, 993305708135186462,
    1035645103156494396, 996806132981055569,  1171908512562020382,
    1064441109297774632, 1168525351350181950, 1171908670829895711,
    970384190547845170,  1351339473543368734, 1171909884233662534,
    1035646207692910612, 1171912292582047847, 973500947328290856,
    1171855900265418784, 1171908410833375328, 970393587256791050,
    964351201984536577,  1171908362347237406, 1173648997697720382,
    1014877227135025252, 1171910117940269137, 970378355696357377,
    1243586956374315098, 1035654365391900744, 1171908591196844225,
    982487353815859241,  1119336172527816756, 958592178026852352,
    1171908737372524546};

int main(void) {
  forth_system = ficlSystemCreate(NULL);
  pthread_mutex_init(&forth_mutex, NULL);
  pthread_cond_init(&forth_done, NULL);

  ccord_global_init();
  struct discord *bot_client = discord_config_init("./config.json");

  discord_add_intents(bot_client, DISCORD_GATEWAY_MESSAGE_CONTENT);
  discord_set_on_ready(bot_client, &disOnReady);
  discord_set_on_message_create(bot_client, &disOnMessage);
  discord_set_on_message_reaction_add(bot_client, &disOnReactionAdd);

  uptime = time(0);
  discord_run(bot_client);

  discord_cleanup(bot_client);
  ccord_global_cleanup();
}
