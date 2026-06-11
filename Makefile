# --- Configuration ---
CC      = gcc
CFLAGS  = -Wall -Wextra -O2
LIBS    = -lm -lSDL3

GLSLC   = glslc
XXD     = xxd

# Paths
SHADER_DIR = shaders
SRC_DIR    = testbed

# --- File Definitions ---

# Source Shaders
SHADERS_FRAG = $(SHADER_DIR)/default.frag
SHADERS_VERT = $(SHADER_DIR)/default.vert

# Intermediate SPIR-V files
SPV_FRAG = $(SHADERS_FRAG:.frag=.frag.spv)
SPV_VERT = $(SHADERS_VERT:.vert=.vert.spv)

# Final Headers (Now inside the shaders/ directory)
HEADER_FRAG = $(SHADER_DIR)/frag.h
HEADER_VERT = $(SHADER_DIR)/vert.h

# Main Program
TARGET   = main
MAIN_SRC = $(SRC_DIR)/main.c

# --- Build Rules ---

all: $(HEADER_FRAG) $(HEADER_VERT) $(TARGET)

$(TARGET): $(MAIN_SRC) $(HEADER_FRAG) $(HEADER_VERT)
	$(CC) $(CFLAGS) -o $@ $(MAIN_SRC) $(LIBS)

# --- Shader Pipeline ---

# Pattern rule: Convert SPIR-V to C Header and place in SHADER_DIR
# $< is the input (.spv), $@ is the output (.h)
$(SHADER_DIR)/%.h: $(SHADER_DIR)/%.spv
	$(XXD) -i $< > $@

# Pattern rule: Compile GLSL to SPIR-V
%.spv: %
	$(GLSLC) $< -o $@

# --- Utilities ---

shaders: $(HEADER_FRAG) $(HEADER_VERT)

clean:
	rm -f $(TARGET) $(SHADER_DIR)/*.h $(SHADER_DIR)/*.spv

.PHONY: all clean shaders
