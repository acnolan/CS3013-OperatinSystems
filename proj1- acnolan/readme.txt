==========================================================================
doit.cpp
==========================================================================
By: Andrew Nolan

--------------------------------------------------------------------------
Files Included
--------------------------------------------------------------------------
doit.cpp - the code for the doit program
foo.txt - a random txt file to run commmands on
makefile - the makefile for the program
readme.txt - the file you are reading now
testruns.txt - file that contains the example test runs for doit.cpp

--------------------------------------------------------------------------
Description
--------------------------------------------------------------------------
doit.cpp is my submission for project one
for CS 3013-Operating Systems. The program
allows you to call Linux commands and displays
system statistics about each run. It can be
run in one line mode by calling: "./doit command argument1 arugument2"
or whatever number of arguments are needed 
for the process. Calling it in shell mode
can be done with a simple "./doit". Shell mode
functions like the normal Linux shell and can
be used to run commands. Tasks can be set to
run in the background by adding an & as the last
parameter.

--------------------------------------------------------------------------
Mini-Shell vs. Linux Shell comparison
--------------------------------------------------------------------------
This is the comparison for part 3 of the assignment.
The mini-shell is very similar to the linux shell. 
Differences being the Linux shell likely handles
background tasks better. Another difference is my
shell automatically outputs statistics. The linux
command prompt also handles errors better. It gives
more detailed error messages. It also doesn't mess
up how it looks, when the background processes return
in the mini-shell it sometimes displays incorrectly.
But as far as I can tell the mini-shell can do
pretty much anything the Linux shell can do, including
running doit.cpp

--------------------------------------------------------------------------
How to run
--------------------------------------------------------------------------
To run inline mode "./doit command parameters"
To run in shell mode "./doit"
Make sure to call "make" to run the make file 
before trying to run the program.

--------------------------------------------------------------------------
Special Commands (only in shell mode)
--------------------------------------------------------------------------
cd - changes directory 
exit- exits shell (cannot exit when background tasks are running, ctrl+d ends the program either way)
set prompt = - sets the prompt to your new prompt
jobs - lists the active background jobs

--------------------------------------------------------------------------
Special Thanks
--------------------------------------------------------------------------
Thanks to all the SA's and TA's who had office 
hours Tuesday, Wednesday, and Thursday. I
spent more time in the zoolab this past week
than I did all year last year.
