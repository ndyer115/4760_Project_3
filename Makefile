TARGET1 = worker
TARGET2 = oss

all:  $(TARGET1) $(TARGET2)

$(TARGET1):
	g++ -std=c++11 -g worker.cpp -o worker.out
 
$(TARGET2):
	g++ -std=c++11 -g oss.cpp -o oss.out

clean:
	/bin/rm -f *.out $(TARGET1) $(TARGET2)