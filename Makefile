CC		:= gcc
CCFLAGS	:= -Wall -Wextra -std=c17 -pthread

OPFLAGS	:= -O3 -flto -DNO_VALIDATE
DBFLAGS	:= -g -DDEBUG

OBJ		:= obj
TARGET	:= bin

SRC		:= util
LIB		:= lib

OBJ_IFACE	:= $(OBJ)/iface.o
OBJ_SETREC	:= $(OBJ)/setRec.o
OBJ_EXPAND	:= $(OBJ)/expand.o
OBJ_REDUCE	:= $(OBJ)/reduce.o
OBJ_TESTS	:= $(OBJ)/tests.o

SRC_BASEUP	:= $(SRC)/baseUp.c
SRC_TOPDOWN	:= $(SRC)/topDown.c
SRC_EVAL	:= $(SRC)/evaluate.c
SRC_CREATE	:= $(SRC)/create.c

DEP_UTIL	:= $(OBJ_IFACE) $(OBJ_SETREC)
DEP_BASEUP	:= $(OBJ_EXPAND)
DEP_TOPDOWN	:= $(OBJ_REDUCE) $(OBJ_TESTS)
DEP_EVAL	:=
DEP_CREATE	:=

BASEUP		:= $(TARGET)/baseUp
TOPDOWN		:= $(TARGET)/topDown
EVAL		:= $(TARGET)/eval
CREATE		:= $(TARGET)/create

UTILS		:= $(BASEUP) $(TOPDOWN) $(EVAL) $(CREATE)

.PHONY: all out debug clean utils dirs

all: out

out: CCFLAGS += $(OPFLAGS)
out: utils

debug: CCFLAGS += $(DBFLAGS)
debug: utils

utils: dirs $(UTILS)

dirs:
	mkdir -p $(OBJ) $(TARGET)

clean:
	rm -r $(OBJ) $(TARGET)

$(OBJ)/%.o: $(LIB)/%.c
	$(CC) $(CCFLAGS) -c $< -o $@

$(BASEUP): $(DEP_BASEUP) $(SRC_BASEUP)
$(TOPDOWN): $(DEP_TOPDOWN) $(SRC_TOPDOWN)
$(EVAL): $(DEP_EVAL) $(SRC_EVAL)
$(CREATE): $(DEP_CREATE) $(SRC_CREATE)

$(UTILS): $(DEP_UTIL)
	$(CC) $(CCFLAGS) $^ -o $@
