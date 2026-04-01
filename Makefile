# Compiler yang digunakan
CC = g++

# Flag kompilasi:
# -std=c++17 → pakai standar C++ 2017
# -Wall      → tampilkan semua warning
FLAGS = -std=c++17 -Wall

SRC_DIR = src
BIN_DIR = bin
TARGET = $(BIN_DIR)/lexer	

# Target default yang dijalankan saat ketik 'make'
all: $(TARGET)


# Cara membuat program lexer
# Compile main.cpp dan lexer.cpp menjadi satu executable
$(TARGET): $(SRC_DIR)/main.cpp $(SRC_DIR)/lexer.cpp | $(BIN_DIR)
	$(CC) $(FLAGS) $(SRC_DIR)/main.cpp $(SRC_DIR)/lexer.cpp -o $(TARGET)


$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Hapus file hasil compile
# Dijalankan dengan 'make clean'
clean:
	rm -f $(TARGET)