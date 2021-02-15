CIS 520: Project 0
Author: Ben Amos

To create the test alarm-mega, i followed the process from the test alarm-multiple,
changing the number of alarms to 70, rather than 7. This required adding a function stub in the
tests.h file, and defining the function within the alarm-wait.c file.
I also added to the tests[] array, which mapped the string alarm-mega to the function test_alarm_mega.
Not entirely sure if necessary, i also added the file alarm-mega.ck which copied from alarm-multiple but changed
the value of the function from 7 to 70 there as well (in addition to the test_alarm_mega function). 
