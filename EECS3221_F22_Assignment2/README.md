1. Copy the files "New_Alarm_Cond.c", and "errors.h" into your
   own directory.

2. To compile the program "New_Alarm_Cond.c", use the following command:

      cc New_Alarm_cond.c -D_POSIX_PTHREAD_SEMANTICS -lpthread

3. Type "a.out" to run the executable code.

4. At the prompt "alarm>", type in the number of seconds at which
   the alarm should expire, followed by the message number, then the text of the message.
   For example:
   	alarm> 2 Message(2) Have fun
5. To remove an alarm request from the program, type 'Cancel: then the message number'
	For example:
	alarm> Cancel: Message(2)

  (To exit from the program, type Ctrl-d or Ctrl-c)
