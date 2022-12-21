Deep Thought
============

A discord bot that interprets user commands as FORTH code, making it reasonably scriptable.

## Technical Details
 - Written somewhat poorly in C, using the [Concord API](https://cogmasters.github.io/concord/).
 - Powered by FICL. FICL is mostly ANS-FORTH compatible despite being called Forth *Inspired* Command Language.
 - Reasonably fast. (fast enough that it can trigger rate limiting)
 
## Features
 - Distinction between usecases, allowing admins to interpret code, and
   users to interpret safer code without certain function calls(WIP, I have
   the skeleton code there) 
 - VM saftey, each vm is instanced per command. 
 - Occasionally helpful debugging advice as well as tips.
 - An amount of customization(This could be improved significantly)
## Setup
 1. Clone this repo. 
 2. Edit the file `cockbot.h`. This includes most of the hardcoded features and it is very important you set these up.
 3. Run `make` in this directory.
 4. cd into the `bot` subdirectory.
 5. Copy `config_default.json` to `config.json`.
 6. Add your token to `config.json`.

## Credits
The FORTH implementation used by this bot is FICL which can be found [here](http://ficl.sourceforge.net/). 
FICL is licensed as follows:

Copyright (c) 1997-2001 John Sadler (john_sadler@alum.mit.edu)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

