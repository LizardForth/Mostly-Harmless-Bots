#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include "ficl/ficl.h"
#include "concord/discord.h"

void disPin(ficlVm *forth_vm);

void disRestart(ficlVm *forth_vm);

void disLoadScript(ficlVm *forth_vm);

void disMuteCb(struct discord *bot_client, struct discord_timer *dis_timer);

void disMute(ficlVm *forth_vm);

void disKick(ficlVm *forth_vm);

void disBan(ficlVm *forth_vm);

void disDelBan(ficlVm *forth_vm); 

#endif
