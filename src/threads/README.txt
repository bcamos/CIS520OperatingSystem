In the src/tests/threads folder the following was changed:

	In tests.c: In the tests struct I added a new entry for the mega test 
	
	In tests.h: I inserted the line "extern test_func test_alarm_mega;

	In Make.tests: I added a void method signature for the mega alarm. It was the same method signature as the
		multiple alarm but the 7 was changed to a 70

In the pintos/src/threads folder I added the alarm-mega.ck file		
