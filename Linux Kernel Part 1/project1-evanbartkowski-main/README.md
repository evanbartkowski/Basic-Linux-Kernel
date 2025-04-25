[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/OHwIpFF9)
# Project 1
Answer these questions please:

Project Overview:
-----------------------------------
What is the project about? Provide a brief overview that includes the project's name, purpose, and target audience.

This project aims to create a very rudimentary version of a linux shell coded
in C. It will act as a command line interpreter for user commands and 
provides core functionalities. Some functionalities include 
command parsing, execution, /proc. Its essentially a simplified version 
of other shells such as bash. This project caters 
towards new operation system learners.

Solution Approach: Create a basic linux/unix shell
------------------------------------------------------------

How did you approach the problem? Outline the steps or phases in your project development process?

To create a new linux shell first step is to download the template provided.
Next is to fill in and create the makefile. Next is to one by one design
the functionalities in the main C file.

Challenges and Solutions?
---------------------------
What challenges did you encounter while completeing your project, and how did you overcome them?

There are many challenges such as ruining the shell and having to hope and 
pray you made a snapshot to go back too. Another challenge is when
dynamically allocating memory to make sure there are no memory leaks.
Valgrind is very helpful for solving this.


LLM Disclosure
--------------
Are there any external resources or references that might be helpful?


Testing and Validation
-----------------------
Testing Methodology: How did you test your project? Discuss any unit tests, integration tests, or user acceptance testing conducted.
Are there any specific commands or scripts that should be known?

Testing project is done through running program and using test programs or
renacting the users point of view and inputting into the shell.

Example session
---------------
Provide examples on running your project

gcc -o simple_shell simple_shell.c -wall
or just use 'make' to make the program
'./simple_shell' in order to run the shell 

to see history just type 'history' while in the simpleshell
you can also use '/proc/' to check things
