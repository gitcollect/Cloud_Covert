CC = gcc
SENDER = sender
RECEIVER = receiver
RM = rm -rf

all: $(SENDER) $(RECEIVER)

$(SENDER): $(SENDER).c
	$(CC) -o $(SENDER) $(SENDER).c

$(RECEIVER): $(RECEIVER).c
	$(CC) -o $(RECEIVER) $(RECEIVER).c

clean:
	$(RM) $(SENDER) $(RECEIVER)
