#include <concord/discord.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ficl.h"

// long int admin = 187673891018244096;

// Auth Testing
long int admin = 0;

ficlSystem *fth_system;

struct forth_args {
  char *buffer;
  char *command;
  int fth_rc;
};

pthread_mutex_t forthMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t forthCond = PTHREAD_COND_INITIALIZER;

void on_ready(struct discord *client) {
  const struct discord_user *bot = discord_get_self(client);
  log_info("Logged in as %s!", bot->username);
  struct discord_activity activities[] = {
      {
          .name = "with FORTH",
          .type = DISCORD_ACTIVITY_GAME,
          .details = "AAAAH",

      },
  };

  struct discord_presence_update status = {
      .activities =
          &(struct discord_activities){
              .size = sizeof(activities) / sizeof *activities,
              .array = activities,
          },
      .status = "online",
      .afk = false,
      .since = discord_timestamp(client),
  };
  discord_set_presence(client, &status);
}

int ends_in_string(char str[], char substr[]) {
  int len_str = strlen(str);
  int len_substr = strlen(substr);

  bool ends = false;

  if (len_substr <= len_str) {
    for (int i = 0; i < len_substr; i++) {
      if (str[i + len_str - len_substr] != substr[i]) {
        ends = false;
        break;
      }
      ends = true;
    }
  }
  if (ends) {
    return 1;
  } else {
    return 0;
  }
}

void accessErrorEmbed(struct discord *client, const struct discord_message *msg,
                      char *forth_in) {
  struct discord_embed embeds[] = {
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
      client, msg->channel_id,
      &(struct discord_create_message){
          .allowed_mentions =
              &(struct discord_allowed_mention){
                  .replied_user = false,
              },
          .message_reference =
              &(struct discord_message_reference){
                  .message_id = msg->id,
                  .channel_id = msg->channel_id,
                  .guild_id = msg->guild_id,
              },
          .embeds =
              &(struct discord_embeds){.size = sizeof(embeds) / sizeof *embeds,
                                       .array = embeds}},
      NULL);
}

int getPrefix(struct discord *client, const struct discord_message *msg,
              char *forth_in) {
  int pfx = 0;
  char *access = (char *)malloc(strlen(forth_in) + 9);

  snprintf(access, strlen(forth_in) + 9, "``` %s ```", forth_in);
  if (ends_in_string(msg->content, "!FTH") ||
      ends_in_string(msg->content, "!ADM") ||
      ends_in_string(msg->content, "!CMD")) {
    if (ends_in_string(msg->content, "!FTH")) {
      pfx = 3;
    } else if (ends_in_string(msg->content, "!ADM") ||
               ends_in_string(msg->content, "!CMD")) {
      if (msg->author->id != admin) {
        accessErrorEmbed(client, msg, access);
        return 0;
      }
      if (ends_in_string(msg->content, "!ADM")) {
        pfx = 3;
      } else {
        pfx = 2;
      }
    }
  }
  return pfx;
}

void cmdEmbed(struct discord *client, const struct discord_message *msg,
              char *forth_in, char *forth_out, int fth_rc) {
  if (fth_rc != -257) {
    char output_title[32];
    snprintf(output_title, 32, "Command Error: %d", fth_rc);
    struct discord_embed embeds[] = {
        {
            .title = output_title,
            .color = 16077157,
            .description = forth_out,
        },
    };
    struct discord_create_message params = {
        .message_reference =
            &(struct discord_message_reference){
                .message_id = msg->id,
                .channel_id = msg->channel_id,
                .guild_id = msg->guild_id,
            },
        .allowed_mentions =
            &(struct discord_allowed_mention){
                .replied_user = false,
            },

        .embeds =
            &(struct discord_embeds){
                .size = sizeof(embeds) / sizeof *embeds,
                .array = embeds,
            },
    };
    discord_create_message(client, msg->channel_id, &params, NULL);
  } else {
    struct discord_embed embeds[] = {
        {
            .title = "Command Output",
            .color = 4835913,
            .description = forth_out,
        },
    };

    struct discord_create_message params = {
        .message_reference =
            &(struct discord_message_reference){
                .message_id = msg->id,
                .channel_id = msg->channel_id,
                .guild_id = msg->guild_id,
            },
        .allowed_mentions =
            &(struct discord_allowed_mention){
                .replied_user = false,
            },

        .embeds =
            &(struct discord_embeds){
                .size = sizeof(embeds) / sizeof *embeds,
                .array = embeds,
            },
    };
    discord_create_message(client, msg->channel_id, &params, NULL);
  }
}

void errEmbed(struct discord *client, const struct discord_message *msg,
              char *forth_in, char *forth_out, int fth_rc, char *buffer) {
  struct discord_embed embeds[3] = {0};
  embeds[0].color = 16077157;
  embeds[1].color = 16077157;
  embeds[2].color = 16077157;
  discord_embed_set_title(&embeds[0], "Forth Bot Error:");

  discord_embed_add_field(&embeds[0], "Input Code:", forth_in, false);

  switch (fth_rc) {
  case -256:
    discord_embed_add_field(&embeds[1], "Error Explanation:",
                            "Your code exited the inner interpreter loop",
                            false);
    break;
  case -258:
    discord_embed_add_field(&embeds[1], "Error Explanation:",
                            "One of your words needs more text to "
                            "execute, try re-running it.",
                            false);
    break;
  case -260:
    discord_embed_add_field(
        &embeds[1], "Error Explanation:",
        "Your code has encountered an error check output for info.", false);
    if (strstr(buffer, "not found")) {
      discord_embed_set_title(&embeds[2], "Forth Bot: Attempted Debug");
      discord_embed_set_description(
          &embeds[2],
          "Warning the info in here is a best guess it may or may not be "
          "correct. The debugger gurantees help but not helpfulness.");
      discord_embed_add_field(&embeds[2], "Detected Issue:",
                              "Your code contains a word which is not in "
                              "the current forth dictionary",
                              false);
      discord_embed_add_field(
          &embeds[2], "Proposed solutions:",
          "1) Make sure the word is valid\n2) Check your function "
          "definitions\n3) Check dictionary modifier words such as "
          "``CREATE``\n4) yell at it",
          false);
    } else if (strstr(buffer, "Error: data stack underflow")) {
      discord_embed_set_title(&embeds[2], "Forth Bot: Attempted Debug");
      discord_embed_set_description(
          &embeds[2],
          "Warning the info in here is a best guess it may or may not be "
          "correct. The debugger gurantees help but not helpfulness.");
      discord_embed_add_field(
          &embeds[2], "Detected Issue:",
          "One of your words drew too much data from the data stack", false);
      discord_embed_add_field(
          &embeds[2], "Proposed Solutions:",
          "1) Check your most recently added code\n2) Use the ``.s`` "
          "word. it will show you the data stack without clearing it\n3) "
          "check the order in which data is recieved, FORTH is reverse "
          "polish\n4) Cry",
          false);
    } else if (strstr(buffer, "FICL_VM_STATE_COMPILE")) {
      discord_embed_set_title(&embeds[2], "Forth Bot: Attempted Debug");
      discord_embed_set_description(
          &embeds[2],
          "Warning the info in here is a best guess it may or may not be "
          "correct. The debugger gurantees help but not helpfulness.");
      discord_embed_add_field(
          &embeds[2], "Detected Issue:",
          "You've used a word which only works in the compiler", false);
      discord_embed_add_field(
          &embeds[2], "Proposed Solutions:",
          "1) This word can only be compiled\n2) Make sure it is defined "
          "in a funtion like so: ``: FUNCTION-NAME CODE ;``\n3) Make "
          "sure you aren't changing the compiler state to interpret\n4) "
          "Check for words that change compilers behaviour such as "
          "``CREATE LITERAL IMMEDIATE``\n5) Cry",
          false);
      discord_embed_add_field(
          &embeds[2], "Friendly Tip:",
          " Forth differentiates between words when it compiles them and "
          "when they are interpreted. Some words have different "
          "definitions at compile time. If you wish to run a word as "
          "interpreted surrond it with ``[ ]``. Forth is "
          "semi-unique(especially in comparison with modern languages) "
          "in its handeling of the compiler, understanding this is the "
          "basis of higher level FORTH.",
          false);
      discord_embed_add_field(
          &embeds[2], "Friendly Tip:",
          "If you write a function in forth it is standard to use the "
          "following notation to describe its output ``( x -- x )``. "
          "Where x designates data, and the left designates data "
          "retrieved from the stack, and the right shows the data that "
          "will be pushed on to the stack. ",
          false);
      discord_embed_add_field(
          &embeds[2], "Friendly Tip:",
          "If you want to know more about compiler behaviour and "
          "modifying it, "
          "https://www.forth.com/starting-forth/"
          "11-forth-compiler-defining-words/ is a good resource.",
          false);
    }
    break;
  case -259:
    discord_embed_add_field(&embeds[1], "Error Explanation:",
                            "Your code  attempted to exit.", false);
    break;
  case -261:
    discord_embed_add_field(&embeds[1], "Error Explanation:",
                            "Your code reached a debugger breakpoint.", false);
    break;
  case -1:
    discord_embed_add_field(&embeds[1], "Error Explanation",
                            "Your code encountered an error that made it "
                            "abort. Check the output",
                            false);
    break;
  case -2:
    discord_embed_add_field(
        &embeds[1], "Error Explanation",
        "Your code encountered an error that made it abort\". Check the "
        "output. Note this is different than the normal abort.",
        false);
    break;
  case -56:
    discord_embed_add_field(&embeds[1], "Error Explanation",
                            "Your code encountered an error that made it "
                            "quit. Check the output",
                            false);
    break;
  default:
    break;
  }
  discord_embed_add_field(&embeds[1], "Error Output:", forth_out, false);
  discord_create_message(
      client, msg->channel_id,
      &(struct discord_create_message){
          .allowed_mentions =
              &(struct discord_allowed_mention){
                  .replied_user = false,
              },
          .message_reference =
              &(struct discord_message_reference){
                  .message_id = msg->id,
                  .channel_id = msg->channel_id,
                  .guild_id = msg->guild_id,
              },

          .embeds =
              &(struct discord_embeds){.size = sizeof(embeds) / sizeof *embeds,
                                       .array = embeds}},
      NULL);
  discord_embed_cleanup(&embeds[0]);
  discord_embed_cleanup(&embeds[1]);
  discord_embed_cleanup(&embeds[2]);
}

void regEmbed(struct discord *client, const struct discord_message *msg,
              char *forth_in, char *forth_out) {
  struct discord_embed embeds[] = {
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
      client, msg->channel_id,
      &(struct discord_create_message){
          .allowed_mentions =
              &(struct discord_allowed_mention){
                  .replied_user = false,
              },
          .message_reference =
              &(struct discord_message_reference){
                  .message_id = msg->id,
                  .channel_id = msg->channel_id,
                  .guild_id = msg->guild_id,
              },
          .embeds =
              &(struct discord_embeds){.size = sizeof(embeds) / sizeof *embeds,
                                       .array = embeds}},
      NULL);
}

void timeoutEmbed(struct discord *client, const struct discord_message *msg,
                  char *forth_in) {
  struct discord_embed embeds[] = {
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
      client, msg->channel_id,
      &(struct discord_create_message){
          .allowed_mentions =
              &(struct discord_allowed_mention){
                  .replied_user = false,
              },
          .message_reference =
              &(struct discord_message_reference){
                  .message_id = msg->id,
                  .channel_id = msg->channel_id,
                  .guild_id = msg->guild_id,
              },
          .embeds =
              &(struct discord_embeds){.size = sizeof(embeds) / sizeof *embeds,
                                       .array = embeds}},
      NULL);
}

void *forth_execute(void *input) {
  log_info("Starting forth thread");
  ficlVm *vm;
  vm = ficlSystemCreateVm(fth_system);
  log_info("Recieved: %s", ((struct forth_args *)input)->command);
  fflush(stdout);
  freopen("/dev/null", "a", stdout);
  setbuf(stdout, ((struct forth_args *)input)->buffer);
  ((struct forth_args *)input)->fth_rc = -127;
  ((struct forth_args *)input)->fth_rc =
      ficlVmEvaluate(vm, ((struct forth_args *)input)->command);
  fflush(stdout);
  freopen("/dev/tty", "a", stdout);
  pthread_cond_signal(&forthCond);
  pthread_exit(NULL);
}
void *watchcat(void *input) {
  struct timespec ts;
  int rc = 0;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 10;

  log_info("Starting the watcher");
  pthread_mutex_lock(&forthMutex);
  rc = pthread_cond_timedwait(&forthCond, &forthMutex, &ts);
  if (rc == 0) {
    log_info("Executed Successfully");
  } else {
    log_info("timed out");
    log_info("Killing %d", (pthread_t)input);
    pthread_kill((pthread_t)input, SIGKILL);
  }
  pthread_mutex_unlock(&forthMutex);
  pthread_exit(NULL);
}

void on_message(struct discord *client, const struct discord_message *msg) {

  char buffer[10000] = {0};
  char forth_out[10009];

  int pfx = getPrefix(client, msg, msg->content);
  if (pfx == 0) {
    return;
  }

  if (msg->author->bot)
    return;

  if (strlen(msg->content) > 10002) {
    struct discord_embed embed = {
        .color = 16077157,
    };

    discord_embed_set_title(&embed, "Warning");
    discord_embed_set_description(&embed,
                                  "I will not accept excessively long code");

    struct discord_create_message params = {
        .embeds =
            &(struct discord_embeds){
                .size = 1,
                .array = &embed,
            },
    };

    discord_create_message(client, msg->channel_id, &params, NULL);
    return;
  }

  char *command = (char *)malloc(strlen(msg->content) + 4);

  strncpy(command, msg->content, strlen(msg->content));
  int fth_rc;

  command[strlen(command) - 4] = '\0';

  char *command_old = (char *)malloc(strlen(command) + 1);
  char *forth_in = (char *)malloc(strlen(command) + 9);

  strncpy(command_old, command, strlen(command));

  log_info("Prefix number: %d", pfx);

  if (pfx == 3 || pfx == 2) {
    char *addon = ": TEST .\" TEST\" ; ";
    char *prep = strdup(command);
    command = (char *)realloc(command, strlen(command) + strlen(addon) + 2);
    snprintf(command, strlen(command) + strlen(addon) + 2, "%s %s", addon,
             prep);
    free(prep);
  }

  log_info("Parsed: %s", command);

  struct forth_args *forth_args_in =
      (struct forth_args *)malloc(sizeof(struct forth_args));
  forth_args_in->command = command;
  forth_args_in->buffer = buffer;

  pthread_t tid1;
  pthread_t tid2;
  pthread_create(&tid2, NULL, watchcat, (void *)&tid1);
  pthread_create(&tid1, NULL, forth_execute, (void *)forth_args_in);
  pthread_join(tid2, NULL);

  fth_rc = forth_args_in->fth_rc;

  log_info("Exit Code: %d", fth_rc);
  snprintf(forth_out, strlen(buffer) + 9, "``` %s ```", buffer);
  snprintf(forth_in, strlen(command_old) + 9, "``` %s ```", command_old);

  log_info("Output: %s", buffer);

  if (fth_rc == -127) {
    timeoutEmbed(client, msg, forth_in);
  } else {
    if (pfx == 2) {
      cmdEmbed(client, msg, forth_in, forth_out, fth_rc);
    } else if (fth_rc != -257) {
      errEmbed(client, msg, forth_in, forth_out, fth_rc, buffer);
    } else {
      regEmbed(client, msg, forth_in, forth_out);
    }
  }
  free(command);
  free(command_old);
  free(forth_in);
  free(forth_args_in);
}

int main(void) {
  char token[100];
  FILE *fptr;

  pthread_mutex_init(&forthMutex, NULL);
  pthread_cond_init(&forthCond, NULL);

  fth_system = ficlSystemCreate(NULL);
  ficlSystemCompileExtras(fth_system);

  fptr = fopen("token.txt", "r");
  fscanf(fptr, "%[^\n]", token);
  struct discord *client = discord_init(token);
  discord_set_on_ready(client, &on_ready);
  discord_set_on_message_create(client, &on_message);
  discord_run(client);
  discord_cleanup(client);
  ccord_global_cleanup();
}
