# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

GRALLOCTEST = gralloctest
SOURCES += gralloctest.c

CCFLAGS += -g -O2 -Wall -fPIE
LIBS    += -lhardware -lsync -lcutils -pie

OBJS =  $(foreach source, $(SOURCES), $(addsuffix .o, $(basename $(source))))

OBJECTS = $(addprefix $(TARGET_DIR), $(notdir $(OBJS)))
BINARY = $(addprefix $(TARGET_DIR), $(GRALLOCTEST))

.PHONY: all clean

all: $(BINARY)

$(BINARY): $(OBJECTS)

clean:
	$(RM) $(BINARY)
	$(RM) $(OBJECTS)

$(BINARY):
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

$(TARGET_DIR)%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@ -MMD
