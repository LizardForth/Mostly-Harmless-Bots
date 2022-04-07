Marvin The Paranoid Android
============

A discord bot that interprets user commands as FORTH code. 

---
## Technical Details
 - Written horribly in C, using the [Concord API](https://cogmasters.github.io/concord/)
 - Powered by FICL, despite being called Forth *Inspired* Command Language
   its very powerful and easy to use. 
 - Concord apparently natively pthreads on message recived which is nice.
 - IDK speed comparisons but it almost seems like it interprets forth faster
   than discord's api response time because the largest delay I notice is in
   sending messages.
 - It's pretty freaking fast though. 
 
---
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

---
## Setup
 1. Clone this repo 
 2. Run `make`
 3. Put your token in a file called token.txt in the base dir
 4. Run bot

