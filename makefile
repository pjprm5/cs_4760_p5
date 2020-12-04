CFLAGS += -lpthread -lrt
all: oss user_proc
clean:
	/bin/rm -f *.o $(TARGET) oss user_proc
