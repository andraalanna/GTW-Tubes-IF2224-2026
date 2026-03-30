# Compiler yang digunakan
CC = g++

# Flag kompilasi:
# -std=c++17 → pakai standar C++ 2017
# -Wall      → tampilkan semua warning
FLAGS = -std=c++17 -Wall

# Target default yang dijalankan saat ketik 'make'
all: lexer

# Cara membuat program lexer
# Compile main.cpp dan lexer.cpp menjadi satu executable
lexer: src/main.cpp src/lexer.cpp
	$(CC) $(FLAGS) src/main.cpp src/lexer.cpp -o lexer

# Hapus file hasil compile
# Dijalankan dengan 'make clean'
clean:
	rm -f lexer