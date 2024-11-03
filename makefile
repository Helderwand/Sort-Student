all : clean compile run

compile : 
	@gcc -o student_grade HW1.c
run : 
	./student_grade
clean :
	@rm -f *.o
	@rm -f student_grade
	
	
