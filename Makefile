CC = gcc
CFLAGS = -Wall -pthread

TARGET = main
SRC = main.c
SCRIPT = change_freq.sh

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS)

run: $(TARGET)
	chmod +x $(SCRIPT)
	./$(TARGET) & \
	./$(SCRIPT) & \
	sleep 300; \
	pkill $(TARGET)

clean:
	rm -f $(TARGET) time_and_interval.txt