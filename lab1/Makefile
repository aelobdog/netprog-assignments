load_balancer: load_balancer.c
	gcc -o $@ load_balancer.c

run: load_balancer.c
	# running the program using default values for filename, number of procs
	gcc -o load_balancer load_balancer.c
	./load_balancer input.txt 4

clean:
	rm load_balancer
