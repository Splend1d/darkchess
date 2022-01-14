# g++
G_PLUS_PLUS := g++ -std=c++11 -static -O3 -Wall -g

# object file folder
OBJ_FOLDER := bin

# check OS
ifeq ($(OS), Windows_NT)
	EXEC := hw1.exe
	DELETE := del /f $(OBJ_FOLDER)\*.o $(EXEC)
	MAKE_FOLDER := if not exist $(OBJ_FOLDER) mkdir $(OBJ_FOLDER)
else
	EXEC := exec.hw1
	DELETE := rm -f $(OBJ_FOLDER)/*.o $(EXEC)
	MAKE_FOLDER := mkdir -p $(OBJ_FOLDER)
endif

# specify files
MAIN := main
MAIN_FILE := $(MAIN).cpp
MAIN_OBJ := $(OBJ_FOLDER)/$(MAIN).o

AI := MyAI
AI_FILE := $(AI).cpp
AI_HEADER := $(AI).h
AI_OBJ := $(OBJ_FOLDER)/$(AI).o

# command
.PHONY: all
all: hw1

$(AI_OBJ) : $(AI_FILE) $(AI_HEADER) 
	$(MAKE_FOLDER)
	$(G_PLUS_PLUS) -c $(AI_FILE) -o $(AI_OBJ)

$(EXEC) : $(AI_OBJ) $(MAIN_FILE)
	$(MAKE_FOLDER)
	$(G_PLUS_PLUS) -c $(MAIN_FILE) -o $(MAIN_OBJ)
	$(G_PLUS_PLUS) $(AI_OBJ) $(MAIN_OBJ) -o $(EXEC)

# target
hw1 : $(EXEC)

# clean object files and executable file
.PHONY: clean
clean: 
	$(DELETE)