# Compiler yang digunakan
CC = g++

# Flag kompilasi:
# -std=c++17 → pakai standar C++ 2017
# -Wall      → tampilkan semua warning
FLAGS = -std=c++17 -Wall

SRC_DIR = src
BIN_DIR = bin
TARGET = $(BIN_DIR)/program
SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/lexer.cpp $(SRC_DIR)/Parser.cpp

# Target default yang dijalankan saat ketik 'make'
all: $(TARGET)


# Cara membuat program lexer
# Compile main.cpp dan lexer.cpp menjadi satu executable
$(TARGET): $(SOURCES) | $(BIN_DIR)
	$(CC) $(FLAGS) $(SOURCES) -o $(TARGET)


$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Hapus file hasil compile
# Dijalankan dengan 'make clean'
clean:
	rm -f $(TARGET)