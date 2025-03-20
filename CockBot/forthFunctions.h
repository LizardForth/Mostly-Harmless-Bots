#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include "concord/discord.h"
#include "ficl/ficl.h"

void disPin(ficlVm *forth_vm);

void disRestart(ficlVm *forth_vm);

void disLoadScript(ficlVm *forth_vm);

void disUptime(ficlVm *forth_vm);

void disOhio(ficlVm *forth_vm);

void disRand(ficlVm *forth_vm);

void disUnOhio(ficlVm *forth_vm);

void disKick(ficlVm *forth_vm);

void disBan(ficlVm *forth_vm);

void disDelBan(ficlVm *forth_vm);

#endif
