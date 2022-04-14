Deep Thought
============

A discord bot that interprets user commands as FORTH code. 

## Technical Details
 - Written horribly in C, using the [Concord API](https://cogmasters.github.io/concord/)
 - Powered by FICL, despite being called Forth *Inspired* Command Language
   its very powerful and easy to use. 
 - Concord apparently natively pthreads on message recived which is nice.
 - IDK speed comparisons but it almost seems like it interprets forth faster
   than discord's api response time because the largest delay I notice is in
   sending messages.
 - It's pretty freaking fast though. 
 
## Features
 - Distinction between usecases, allowing admins to interpret code, and
   users to interpret safer code without certain function calls(WIP, I have
   the skeleton code there) 
 - Command mode(WIP), just gives te output.
 - VM saftey, each vm is kept sepperate, and doesnt have access to words
   removed by `FORGET`(untested, I believe that ANS compliance requires
   this)
 - Helpful debugging advice as well as tips
 - Beautified output. (I do want to make it prettier)

## Setup
 1. Clone this repo 
 2. Run `make`
 3. Put your token in a file called token.txt in the base dir
 4. Run bot

## Credits
 The FORTH implementation used by this bot is FICL which can be found
[here](http://ficl.sourceforge.net/). FICL is licensed as follows:

Copyright (c) 1997-2001 John Sadler (john_sadler@alum.mit.edu)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

