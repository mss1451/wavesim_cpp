EXE = wavesim_cpp
SRCDIR = ../source
OBJDIR = ./object
SRC = $(wildcard $(SRCDIR)/*.c*)
OBJ := $(SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJ := $(OBJ:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
LIB = `pkg-config --libs gtkmm-3.0` -lz -lpthread
INC = `pkg-config --cflags gtkmm-3.0`

all: $(OBJDIR) $(OBJ)
	g++ -o $(EXE) $(OBJ) $(LIB)

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	g++ -g2 -std=c++11 -Wall -c $< -o $@ $(INC)
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	gcc -g2 -Wall -c $< -o $@ $(INC)
	
.PHONY: all clean
	
clean:
	rm -rf $(OBJDIR) $(EXE)
