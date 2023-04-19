# method 1:
# app: TaskQueue.o ThreadPool.o main.o
#	g++ TaskQueue.o ThreadPool.o main.o -g -o app

#TaskQueue.o:TaskQueue.cc
#	g++ -c TaskQueue.cc -lpthread

#ThreadPool.o:ThreadPool.cc
#	g++ -c ThreadPool.cc -lpthread

#main.o:main.cc
#	g++ -c main.cc -lpthread

# method 2:
#app: TaskQueue.cc ThreadPool.cc main.cc
#	g++ TaskQueue.cc ThreadPool.cc main.cc -lpthread -g -o app

# method 3:
#app: TaskQueue.cc ThreadPool.cc main.cc
#	g++ $^ -lpthread -g -o $@

# method 4:
target=app
# find all .cc file
src=$(wildcard *.cc)
# replace the subfix from .cc to .o
obj=$(patsubst *.cc, *.o, $(src))
$(target):$(obj)
	g++ $(obj) -o $(target)

%.o:%.cc
	g++ -c $^ -lpthread

# avoid checking the timestamp
.PHONY:clean
clean:
	rm $(target)
