# GTW-Tubes-IF2224 — Arion Compiler: Milestone 1 (Lexical Analysis)

> IF2224 Teori Bahasa Formal dan Otomata — Institut Teknologi Bandung 2026

---

## Identitas Kelompok

**Kelompok 01 — Kelas 01**

| Nama | NIM |
|------|-----|
| Muhammad Jordan Ferimeison | 13524047 |
| Arina Azka | 13524049 |
| Hakam Avicena Mustain | 13524075 |
| Yavie Azka Putra Araly | 13524077 |
| Angelina Andra Alanna | 13524079 |

---

## Deskripsi Program

Program ini merupakan implementasi **Lexical Analyzer (Lexer)** untuk bahasa pemrograman **Arion** sebagai bagian dari Milestone 1 Tugas Besar IF2224. Lexer membaca source code bahasa Arion dalam format `.txt`, memproses karakter satu per satu menggunakan **Deterministic Finite Automata (DFA)**, dan menghasilkan daftar token dalam format `.txt`.

### Token yang Didukung

| Kategori | Token |
|----------|-------|
| Konstanta | `intcon`, `realcon`, `charcon`, `string` |
| Operator Aritmatika | `plus`, `minus`, `times`, `idiv`, `rdiv`, `imod` |
| Operator Logika | `notsy`, `andsy`, `orsy` |
| Operator Perbandingan | `eql`, `neq`, `gtr`, `geq`, `lss`, `leq` |
| Tanda Baca | `comma`, `semicolon`, `period`, `colon`, `becomes`, `lparent`, `rparent`, `lbrack`, `rbrack` |
| Keyword | `constsy`, `typesy`, `varsy`, `functionsy`, `proceduresy`, `arraysy`, `recordsy`, `programsy`, `beginsy`, `endsy`, `ifsy`, `thensy`, `elsesy`, `casesy`, `ofsy`, `whilesy`, `dosy`, `forsy`, `tosy`, `downtosy`, `repeatsy`, `untilsy` |
| Identifier | `ident` |
| Komentar | `comment` |

### Fitur Utama

- Membaca source code karakter per karakter sesuai prinsip DFA
- Mendukung keyword **case-insensitive** (`BEGIN` == `begin` == `bEgIn`)
- Mendukung escape character `''` di dalam string dan charcon
- Mendeteksi `charcon` (1 karakter) dan `string` (0 atau 2+ karakter) secara otomatis
- Mendukung komentar `{ }` dan `(* *)` termasuk multiline
- Menangani error tanpa crash — program tetap lanjut ke token berikutnya
- Mendukung bilangan real (`realcon`) dengan format `digit.digit`

---

## Requirements

- **Compiler**: `g++` dengan standar C++17 atau lebih baru
- **Build Tool**: `make`
- **OS**: Linux / macOS / Windows (dengan MinGW atau WSL)

---

## Cara Instalasi dan Penggunaan Program

### 1. Clone Repository

```bash
git clone https://github.com/<username>/GTW-Tubes-IF2224-2026.git
cd GTW-Tubes-IF2224-2026
```

### 2. Build Program

```bash
make
```

Binary akan tersimpan di folder `bin/`:

```
bin/lexer
```

### 3. Jalankan Program

```bash
./bin/lexer <input_file> <output_file>
```

**Contoh:**

```bash
./bin/lexer test/milestone-1/test-case-1.txt output.txt
```

### 4. Lihat Output

```bash
cat output.txt
```

### 5. Clean Build

```bash
make clean
```

---

## Struktur Repository

```
GTW-Tubes-IF2224-2026/
├── bin/
│   └── lexer                  # Binary hasil kompilasi
├── doc/
│   └── Laporan-1-GTW.pdf      # Laporan Milestone 1
├── src/
│   ├── dfa.h                  # Definisi enum State DFA
│   ├── lexer.cpp              # Implementasi lexical analyzer
│   ├── lexer.h                # Blueprint class Lexer
│   ├── main.cpp               # Entry point program
│   └── token.h                # Definisi struct Token
├── test/
│   └── milestone-1/
│       ├── test-case-1.txt    # Input kasus uji 1
│       ├── test-case-2.txt    # Input kasus uji 2
│       ├── test-case-3.txt    # Input kasus uji 3
│       ├── test-case-4.txt    # Input kasus uji 4
│       └── test-case-5.txt    # Input kasus uji 5
|       └── # kasus uji seterusnya
├── Makefile
└── README.md
```

---

## Format Input dan Output

### Input (`.txt`)

Source code bahasa pemrograman Arion, contoh:

```
program Hello;
var
  a : integer;
begin
  a := 5;
end.
```

### Output (`.txt`)

Daftar token satu per baris. Token yang memiliki nilai ditampilkan beserta nilainya dalam kurung:

```
programsy
ident (Hello)
semicolon
varsy
ident (a)
colon
ident (integer)
semicolon
beginsy
ident (a)
becomes
intcon (5)
semicolon
endsy
period
```

---

## Pembagian Tugas

| Nama | NIM | Tugas |
|------|-----|-------|
| Muhammad Jordan Ferimeison | 13524047 | `readIdentOrKeyword()`, `classifyKeyword()`, ` tokenize()`, integrasi |
| Arina Azka | 13524049 | `readPunctuation()` |
| Hakam Avicena Mustain | 13524075 | `readComment()`, `readString()`|
| Yavie Azka Putra Araly | 13524077 | `readOperator()` |
| Angelina Andra Alanna | 13524079 | ` readNumber()`, `Lexer()`, `peek()`, `advance()` |