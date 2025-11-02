# Multithreaded_DNS
A multi-threaded DNS resolver using a shared circular queue.

Used the pthreads library to create threads and ensure any shared data is synchronized.
A shared circular queue is used to store domain names and retrieve their IP address.
This project was done as an assignment for my Design and Analysis of Operating Systems class.
That being said, I created everything from scratch other than the util.c and util.h file because my teacher decided that we didn't have enough time in the semester to do it ourselves.

How to run:
  1. Compile the program through make
  2. run "./multi-lookup <# requester threads> <#resolver threads> <requester \file> < resolver \file> <input files (can use regex expressions)>
