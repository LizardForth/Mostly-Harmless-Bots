#include <concord/discord.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "ficl.h"

long int admin = 187673891018244096;

// Auth Testing
// long int admin = 0;

ficlSystem *fth_system;

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

int getPrefix(struct discord *client, const struct discord_message *msg) {
  int pfx = 0;
  if (ends_in_string(msg->content, "!FTH") ||
      ends_in_string(msg->content, "!ADM") ||
      ends_in_string(msg->content, "!CMD")) {
    if (ends_in_string(msg->content, "!FTH")) {
      pfx = 3;
    } else if (ends_in_string(msg->content, "!ADM") ||
               ends_in_string(msg->content, "!CMD")) {
      if (msg->author->id != admin) {
        struct discord_embed embed = {
            .color = 16077157,
        };

        discord_embed_set_title(&embed, "Warning");
        discord_embed_set_description(&embed, "Your are not authorized");

        struct discord_create_message params = {
            .embeds =
                &(struct discord_embeds){
                    .size = 1,
                    .array = &embed,
                },
        };

        discord_create_message(client, msg->channel_id, &params, NULL);

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
  struct discord_embed output_embed = {};
  if (fth_rc != -257) {
    output_embed.color = 16077157;
    discord_embed_set_title(&output_embed, "Command Error");
    discord_embed_set_description(&output_embed, forth_out);
  } else {
    output_embed.color = 4835913;
    discord_embed_set_title(&output_embed, "Command Out");
    discord_embed_set_description(&output_embed, forth_out);
  }
  struct discord_create_message params = {
      .embeds =
          &(struct discord_embeds){
              .size = 1,
              .array = &output_embed,
          },
  };
  discord_create_message(client, msg->channel_id, &params, NULL);
  discord_embed_cleanup(&output_embed);
}

void errEmbed(struct discord *client, const struct discord_message *msg,
              char *forth_in, char *forth_out, int fth_rc) {
  bool debuggable = false;
  struct discord_embed embed = {
      .color = 16077157,
  };

  struct discord_embed debug_embed = {
      .color = 16077157,
  };

  discord_embed_set_title(&embed, "Forth Bot Error");

  discord_embed_add_field(&embed, "Input Code:", forth_in, false);

  switch (fth_rc) {
  case -256:
    discord_embed_add_field(&embed, "Error Explanation:",
                            "Your code exited the inner interpreter loop",
                            false);
    break;
  case -258:
    discord_embed_add_field(&embed, "Error Explanation:",
                            "One of your words needs more text to "
                            "execute, try re-running it.",
                            false);
    break;
  case -260:
    discord_embed_add_field(
        &embed, "Error Explanation:",
        "Your code has encountered an error check output for info.", false);
    if (strstr(forth_in, "not found")) {
      discord_embed_set_title(&debug_embed, "Forth Bot: Attempted Debug");
      discord_embed_set_description(
          &debug_embed,
          "Warning the info in here is a best guess it may or may not be "
          "correct. The debugger gurantees help but not helpfulness.");
      discord_embed_add_field(&debug_embed, "Detected Issue:",
                              "Your code contains a word which is not in "
                              "the current forth dictionary",
                              false);
      discord_embed_add_field(
          &debug_embed, "Proposed solutions:",
          "1) Make sure the word is valid\n2) Check your function "
          "definitions\n3) Check dictionary modifier words such as "
          "``CREATE``\n4) yell at it",
          false);
      debuggable = true;
    } else if (strstr(forth_in, "Error: data stack underflow")) {
      discord_embed_set_title(&debug_embed, "Forth Bot: Attempted Debug");
      discord_embed_set_description(
          &debug_embed,
          "Warning the info in here is a best guess it may or may not be "
          "correct. The debugger gurantees help but not helpfulness.");
      discord_embed_add_field(
          &debug_embed, "Detected Issue:",
          "One of your words drew too much data from the data stack", false);
      discord_embed_add_field(
          &debug_embed, "Proposed Solutions:",
          "1) Check your most recently added code\n2) Use the ``.s`` "
          "word. it will show you the data stack without clearing it\n3) "
          "check the order in which data is recieved, FORTH is reverse "
          "polish\n4) Cry",
          false);
      debuggable = true;
    } else if (strstr(forth_in, "FICL_VM_STATE_COMPILE")) {
      discord_embed_set_title(&debug_embed, "Forth Bot: Attempted Debug");
      discord_embed_set_description(
          &debug_embed,
          "Warning the info in here is a best guess it may or may not be "
          "correct. The debugger gurantees help but not helpfulness.");
      discord_embed_add_field(
          &debug_embed, "Detected Issue:",
          "You've used a word which only works in the compiler", false);
      discord_embed_add_field(
          &debug_embed, "Proposed Solutions:",
          "1) This word can only be compiled\n2) Make sure it is defined "
          "in a funtion like so: ``: FUNCTION-NAME CODE ;``\n3) Make "
          "sure you aren't changing the compiler state to interpret\n4) "
          "Check for words that change compilers behaviour such as "
          "``CREATE LITERAL IMMEDIATE``\n5) Cry",
          false);
      discord_embed_add_field(
          &debug_embed, "Friendly Tip:",
          " Forth differentiates between words when it compiles them and "
          "when they are interpreted. Some words have different "
          "definitions at compile time. If you wish to run a word as "
          "interpreted surrond it with ``[ ]``. Forth is "
          "semi-unique(especially in comparison with modern languages) "
          "in its handeling of the compiler, understanding this is the "
          "basis of higher level FORTH.",
          false);
      discord_embed_add_field(
          &debug_embed, "Friendly Tip:",
          "If you write a function in forth it is standard to use the "
          "following notation to describe its output ``( x -- x )``. "
          "Where x designates data, and the left designates data "
          "retrieved from the stack, and the right shows the data that "
          "will be pushed on to the stack. ",
          false);
      discord_embed_add_field(
          &debug_embed, "Friendly Tip:",
          "If you want to know more about compiler behaviour and "
          "modifying it, "
          "https://www.forth.com/starting-forth/"
          "11-forth-compiler-defining-words/ is a good resource.",
          false);
      debuggable = true;
    } else {
      debuggable = false;
    }
    break;
  case -259:
    discord_embed_add_field(
        &embed, "Error Explanation:", "Your code  attempted to exit.", false);
    break;
  case -261:
    discord_embed_add_field(&embed, "Error Explanation:",
                            "Your code reached a debugger breakpoint.", false);
    break;
  case -1:
    discord_embed_add_field(&embed, "Error Explanation",
                            "Your code encountered an error that made it "
                            "abort. Check the output",
                            false);
    break;
  case -2:
    discord_embed_add_field(
        &embed, "Error Explanation",
        "Your code encountered an error that made it abort\". Check the "
        "output. Note this is different than the normal abort.",
        false);
    break;
  case -56:
    discord_embed_add_field(&embed, "Error Explanation",
                            "Your code encountered an error that made it "
                            "quit. Check the output",
                            false);
    break;
  default:
    break;
  }
  discord_embed_add_field(&embed, "Error Output:", forth_out, false);
  if (debuggable == false) {
    struct discord_create_message params = {
        .embeds =
            &(struct discord_embeds){
                .size = 1,
                .array = &embed,
            },
    };
    discord_create_message(client, msg->channel_id, &params, NULL);
    discord_embed_cleanup(&embed);
  } else {
    struct discord_create_message params = {
        .embeds =
            &(struct discord_embeds){
                .size = 1,
                .array = &embed,
            },
    };
    struct discord_create_message params2 = {
        .embeds =
            &(struct discord_embeds){
                .size = 1,
                .array = &debug_embed,
            },
    };
    discord_create_message(client, msg->channel_id, &params, NULL);
    discord_embed_cleanup(&embed);
    discord_create_message(client, msg->channel_id, &params2, NULL);
    discord_embed_cleanup(&debug_embed);
  }
}

void regEmbed(struct discord *client, const struct discord_message *msg,
              char *forth_in, char *forth_out) {
  struct discord_embed embed = {
      .color = 4835913,
  };

  discord_embed_set_title(&embed, "Forth Bot");

  discord_embed_add_field(&embed, "Input Code:", forth_in, false);
  discord_embed_add_field(&embed, "Output:", forth_out, false);

  struct discord_create_message params = {
      .embeds =
          &(struct discord_embeds){
              .size = 1,
              .array = &embed,
          },
  };
  discord_create_message(client, msg->channel_id, &params, NULL);
  discord_embed_cleanup(&embed);
}

void on_message(struct discord *client, const struct discord_message *msg) {
  ficlVm *vm;

  char buffer[10000] = {0};
  char forth_out[10009];

  int pfx = getPrefix(client, msg);
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

  char *command = malloc(strlen(msg->content) + 9);

  strcpy(command, msg->content);

  int fth_rc;

  vm = ficlSystemCreateVm(fth_system);

  command[strlen(command) - 4] = '\0';

  char *command_old = malloc(strlen(command) + 9);
  char *forth_in = malloc(strlen(command) + 9);

  strcpy(command_old, command);

  log_info("Prefix number: %d", pfx);

  if (pfx == 3 || pfx == 2) {
    char *addon = ": TEST .\" TEST\" ; ";
    char *prep = strdup(command);
    sprintf(command, "%s %s", addon, prep);
    free(prep);
  }

  log_info("Parsed: %s", command);

  fflush(stdout);
  freopen("/dev/null", "a", stdout);
  setbuf(stdout, buffer);

  fth_rc = ficlVmEvaluate(vm, command);

  fflush(stdout);
  freopen("/dev/tty", "a", stdout);

  log_info("Exit Code: %d", fth_rc);

  sprintf(forth_out, "``` %s ```", buffer);
  sprintf(forth_in, "``` %s ```", command_old);

  log_info("Output: %s", buffer);
  if (pfx == 2) {
    cmdEmbed(client, msg, forth_in, forth_out, fth_rc);
  } else if (fth_rc != -257) {
    errEmbed(client, msg, forth_in, forth_out, fth_rc);
  } else {
    regEmbed(client, msg, forth_in, forth_out);
  }
  free(command);
  free(command_old);
  free(forth_in);
}

int main(void) {
  char token[100];
  FILE *fptr;

  fth_system = ficlSystemCreate(NULL);
  ficlSystemCompileExtras(fth_system);

  fptr = fopen("token.txt", "r");
  fscanf(fptr, "%[^\n]", token);
  struct discord *client = discord_init(token);
  discord_set_on_ready(client, &on_ready);
  discord_set_on_message_create(client, &on_message);
  discord_run(client);
}
