CC		:= gcc
CCFLAGS	:= -Wall -Wextra

OPFLAGS	:= -O2 -flto
DBFLAGS	:= -g -DDEBUG

OBJ		:= obj
TARGET	:= bin

SRC		:= util
LIB		:= lib

OBJ_IFACE	:= $(OBJ)/iface.o
OBJ_SETREC	:= $(OBJ)/setRec.o
OBJ_EX_SUP	:= $(OBJ)/supers.o
OBJ_EX_MUT	:= $(OBJ)/mutate.o
OBJ_NULTEST	:= $(OBJ)/nulTest.o

SRC_GEN		:= $(SRC)/generation.c
SRC_WEED	:= $(SRC)/weed.c
SRC_EVAL	:= $(SRC)/evaluate.c
SRC_CREATE	:= $(SRC)/create.c

DEP_UTIL	:= $(OBJ_IFACE) $(OBJ_SETREC)
DEP_GEN		:= $(OBJ_EX_SUP) $(OBJ_EX_MUT)
DEP_WEED	:= $(OBJ_NULTEST)
DEP_EVAL	:=
DEP_CREATE	:=

GEN			:= $(TARGET)/gen
WEED		:= $(TARGET)/weed
EVAL		:= $(TARGET)/eval
CREATE		:= $(TARGET)/create

UTILS		:= $(GEN) $(WEED) $(EVAL) $(CREATE)

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

$(GEN): $(DEP_GEN) $(SRC_GEN)
$(WEED): $(DEP_WEED) $(SRC_WEED)
$(EVAL): $(DEP_EVAL) $(SRC_EVAL)
$(CREATE): $(DEP_CREATE) $(SRC_CREATE)

$(UTILS): $(DEP_UTIL)
	$(CC) $(CCFLAGS) $^ -o $@
