CC = gcc
CFLAGS = -O3 -Wall
LDFLAGS = -lm

TARGET = ag_ftth_hibrido
SRCS = ag_ftth_hibrido.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(TARGET).exe *.o
