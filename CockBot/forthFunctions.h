#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include "ficl/ficl.h"
#include "concord/discord.h"

static void disPin(ficlVm *forth_vm);

static void disRestart(ficlVm *forth_vm);

static void disLoadScript(ficlVm *forth_vm);

static void disMuteCb(struct discord *bot_client, struct discord_timer *dis_timer);

static void disMute(ficlVm *forth_vm);

static void disKick(ficlVm *forth_vm);

static void disBan(ficlVm *forth_vm);

static void disDelBan(ficlVm *forth_vm); 

#endif
