#include <assert.h>
#include <concord/discord.h>
#include <concord/log.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cockbot.h"
#include "ficl/ficl.h"

void forthUse(ficlVm *forth_vm){
    FILE *block_fs;
    int  string_length = ficlStackPopInteger(forth_vm->dataStack);
    void *string_address = (void *)ficlStackPopPointer(forth_vm->dataStack);
    char *block_fname = (char *)malloc(string_length + 1);
    memcpy(block_fname, string_address, string_length);
    
    if (string_length <= 0 ) {
        ficlVmTextOut(forth_vm, "ERROR: Can't load a file with a nonexistent name.\n");
        ficlVmThrow(forth_vm, FICL_VM_STATUS_QUIT);
        return;
    } else if (string_length > 8 ) {
        ficlVmTextOut(forth_vm, "ERROR: Can't use blocks with names longer than 8 Charters.\n");
        ficlVmThrow(forth_vm, FICL_VM_STATUS_QUIT);     
        return;
    }

    log_info("Pointer 1: %p", block_fname);
    log_info("Pointer 2: %p", string_address);
    memcpy(block_fname, string_address, string_length);
    //char *string_tmp = strdup(block_fname);
    //strcpy(block_fname, "fth_scripts/");
    //strcat(block_fname, string_tmp);
    //strcat(block_fname, ".fb");
    //free(string_tmp);
    if ((block_fs = fopen(block_fname, "r+")) == NULL) {
        block_fs = fopen(block_fname, "w+");
        ficlFile *block_ff = (ficlFile *)malloc(sizeof(ficlFile));
        strcpy(block_ff->filename, block_fname);
        block_ff->f = block_fs;
        ficlStackPushPointer(forth_vm->dataStack, block_ff);
    } else {
        ficlFile *block_ff = (ficlFile *)malloc(sizeof(ficlFile));
        strcpy(block_ff->filename, block_fname);
        block_ff->f = block_fs;
        ficlStackPushPointer(forth_vm->dataStack, block_ff);
    }
    
    log_info("Loaded: %s", block_fname);
    free(block_fname);
}
void forthBlock(ficlVm *forth_vm) {
    int block_number = ficlStackPopInteger(forth_vm->dataStack);
    ficlFile *block_ff = (ficlFile *)ficlStackPopPointer(forth_vm->dataStack);
    char buffer_line[66];
    int block_line = 0;
    char c;
    for (c = getc(block_ff->f); block_line != 16*block_number; c = getc(block_ff->f)) {
        if (c == '\n') {
            block_line = block_line + 1;
            log_info("Newline: %d", block_line);
        }   
    } 
    fseek(block_ff->f, -1L, SEEK_CUR); 
    for (int block_row = 0; block_row < 16; ++block_row) {
        fgets(buffer_line, 64, block_ff->f);
        char *string_tmp = strdup(buffer_line);
        sprintf(buffer_line, "%.64s", buffer_line);
        log_info(buffer_line);
    }               
}