# GCC

**Preprocessor**

```bash
gcc -E main.c -o main.i
```

**Compilation**

```bash
gcc -S main.i -o main.s
```

**Assembler**

```bash
as main.s -o main.o
```

**Linking**

```bash
gcc -v -o main main.o
```

# Makefile

## Rule

Cấu trúc:

```bash
target ... : prerequisites ...
    recipe
    ...
    ...
```

- target: Action hoặc file mà make tạo ra.
- prerequisites: File mà target phụ thuộc hay còn gọi là dependence.
- recipe: Lệnh shell để tạo target từ prerequisites.

## Dependency-based build system

Make là một công cụ xây dựng phụ thuộc (dependency-based build system), và hoạt động theo nguyên tắc:

> Một target chỉ được build lại nếu ít nhất một trong các dependency của nó mới hơn target đó.

Khi Make chạy, nó sẽ:
- Kiểm tra tồn tại của target (file đầu ra).
- Nếu không tồn tại, Make chạy command để build.
- Nếu tồn tại, nó so sánh thời gian sửa đổi (timestamp) giữa target và các dependency.
- Nếu bất kỳ dependency nào mới hơn, Make cho rằng target đã cũ → rebuild.
- Nếu không có gì mới hơn → Make bỏ qua, không làm lại.

👉 Có thể sử dụng đặc tính này để chỉ biên dịch những file source thay đổi. Ví dụ:

```bash
SRC := main.c uart.c
OBJ := $(SRC:.c=.o)

%.o: %.c
	$(CC) -c $< -o $@

app.elf: $(OBJ)
	$(CC) $^ -o $@
```

👉 Make sẽ chỉ build `uart.o` nếu `uart.c` (hoặc dependency liên quan như `uart.h`)  bị thay đổi.

**💡 Nhưng làm sao để biết .h có thay đổi?**

Ta cần tạo dependency file (`.d`) để Make biết `.o` phụ thuộc vào `.h`. Ví dụ:

```bash
SRC := main.c uart.c
OBJ := $(SRC:.c=.o)

%.o: %.c
	$(CC) -MMD -MP -c $< -o $@

app.elf: $(OBJ)
	$(CC) $^ -o $@

-include $(OBJ:.o=.d)
```

👉 Với Makefile này:

- Nếu `main.c`, `uart.c` hoặc bất kỳ `.h` mà chúng `#include` bị thay đổi, make sẽ chỉ biên dịch lại những file đó.
- Những file không thay đổi thì make bỏ qua.

## Variable

**Automatic Variables**

Automatic Variables là các biến đặc biệt đại diện cho mà make tự gán giá trị đúng trong rule như target, prerequisites hay file nguồn đang được xử lý.

=>  Giúp viết makefile ngắn gọn, tổng quát và không cần hardcode tên file.

| Symbol | Meaning                             |
| ------ | ----------------------------------- |
| `$@`   | Tên của targe hiện tại              |
| `$^`   | Tất cả các prerequisites            |
| `$+`   | Giống `$^` nhưng giữ lại thứ tự và phần tử trùng lặp |
| `$<`   | Tên của prerequisites đầu tiên      |

**Variable VPATH**

Biến `VPATH` được dùng để chỉ định danh sách các thư mục mà make sẽ tìm kiếm nếu không tìm thấy file nằm trong thư mục hiện tại.

> `VPATH` được dùng cho mọi file nếu không được tìm thấy trong thư mục hiện tai.

Cú pháp:

```bash
VPATH = dir1:dir2:dir3
```

- Các thư mục cách nhau bằng dấu :

Ví dụ: Giả sử file foo.c không tồn tại trong folder hiện tại nhưng được tìm thấy trong folder src

```bash
VPATH = src

foo.o: foo.c
	$(CC) -c $< -o $@
```

=> Dù `foo.c` nằm trong thư mục src, ta chỉ cần ghi `foo.c` mà không cần `src/foo.c`, vì make sẽ tìm trong `VPATH`.

## Wildcard Character

Wildcard Character được dùng để thay thế nhiều tên tệp theo một pattern nhất định.

Một số wildcard character phổ biến:

| Ký tự |                   Ý nghĩa                  |                      Ví dụ                           |
|-------|--------------------------------------------|------------------------------------------------------|
|   *   | Khớp với bất kỳ chuỗi nào, kể cả chuỗi rỗng| `*.c` sẽ khớp `main.c`, `syscalls.c`                 |
|   ?   | Khớp với một ký tự bất kỳ                  | `a?.txt` khớp `a1.txt`, `ab.txt`, không khớp abc.txt |
| [...] | Khớp với một ký tự nằm trong tập chỉ định  | `file[12].c` khớp file1.c, file2.c                   |

Wildcard character sẽ không hợp lệ nếu được sử dụng như sau:

```bash
objects = *.o
```

=> Lúc này, giá trị của biến `objects` sẽ là chuỗi '*.o'. Tuy nhiên, nếu sử dụng giá trị này của objects ở target hoặc prerequisite thì Wildcard character sẽ xảy ra tại đây.

Để có thể thực hiện Wildcard character khi định nghĩa variable, ta sử dụng:

```bash
objects := $(wildcard *.o)
```

Ta có thể thay đổi list các file source C thành list các file object bằng cách thay thế '.c' thành '.o' như sau:

```bash
$(patsubst %.c,%.o,$(wildcard *.c))
```

## String Function

**$(subst from,to,text)**

Thực hiện thay thế text.

Ví dụ:

```bash
$(subst ee,EE,feet on the street)
```

=> tạo ra giá trị ‘fEEt on the strEEt’.

**$(patsubst pattern,replacement,text)**

Tìm các word được phân cách bằng space trong text khớp với **pattern** và thực hiện thay thế chung bằng **replacement**.

```bash
$(patsubst %.c,%.o,x.c.c bar.c)
```

=> tạo ra giá trị ‘x.c.o bar.o’.

**Substitution references**

Đây là cách đơn giản hơn tương tự với hàm patsubst. Cấu trúc:

```bash
$(var:pattern=replacement)
```

tương đương với

```bash
$(patsubst pattern,replacement,$(var))
```

Ví dụ, ta có một list các file object:

```bash
objects = foo.o bar.o baz.o
```

Để có list các file source tương ứng, ta chỉ cần viết:

```bash
$(objects:.o=.c)
```

thay vì sử dụng dạng nguyên thủy:

```bash
$(patsubst %.o,%.c,$(objects))
```

https://www.gnu.org/software/make/manual/make.html#Text-Functions

## Foreach Functions

Cú pháp của hàm foreach:

```bash
$(foreach var,list,text)
```

Duyệt lần lượt từng từ trong `list`, sau đó gán cho `var` và `text` sẽ được mở rộng.

Ví dụ biến `files` chứa danh sách tất cả các file trong các thư mục `dirs`:

```bash
dirs := a b c d 
files := $(foreach dir,$(dirs),$(wildcard $(dir)/*))
```

- Lần lặp lại đầu tiên thấy giá trị 'a' => '$(wildcard a/*)'
- Lần lặp lại thứ hai thấy giá trị 'b' => '$(wildcard b/*)'
- Lần lặp lại đầu tiên thấy giá trị 'c' => '$(wildcard c/*)'

=> Kết quả của ví dụ này sẽ là:

```bash
files := $(wildcard a/*)  $(wildcard b/*) $(wildcard c/*)  $(wildcard d/*)
```

https://www.gnu.org/software/make/manual/make.html#Foreach-Function

## Direction

**include**

Direction **include** sẽ tạm dừng đọc makefile hiện tại và đọc các makefile khác trước khi tiếp tục.

```bash
include filenames…
```

Nếu muốn make bỏ qua một makefile không tồn tại hoặc không thể tạo, không có thông bảo lỗi thì có thể sử dụng câu lệnh sau. Lệnh này hoạt động giống như lệnh trên, ngoại trừ việc nó không có cảnh báo lỗi nếu không có tồn tại một makefile.

```bash
-include filenames
```

**vpath**

Khác với biến `VPATH`, direction `vpath` được dùng để make tìm kiếm các file cụ thể khớp với pattern trong các folder thay vì áp dụng cho mọi file như `VPATH`.

Trong GNU Make, directive vpath có 3 hình thức sử dụng, tùy theo mục đích:

**1. Đặt thư mục tìm kiếm cho một pattern**

```bash
vpath <pattern> <directories>
```

- `<pattern>`: kiểu tên file muốn áp dụng (ví dụ: %.c, config.h, main.c).
- `<directories>`: danh sách thư mục để tìm file khớp với pattern, cách nhau bằng khoảng trắng.

Ví dụ:

```bash
vpath %.c $(SRC_DIRS)
```

Yêu cầu make tìm kiếm các file có tên kết thúc bằng `.c` trong folder `SRC_DIRS` nếu file không được tìm thấy trong folder hiện tại.

**2. Xoá bỏ thư mục tìm kiếm cho một pattern**

```bash
vpath <pattern>
```

- Xoá mọi thiết lập trước đó (nếu có) về nơi tìm kiếm cho pattern này.
- Pattern sẽ không được tra trong bất kỳ thư mục nào ngoài thư mục hiện hành.

**3. Xoá toàn bộ các đường dẫn đã được chỉ định bằng vpath trước đó**

```bash
vpath
```

- Xoá toàn bộ cấu hình vpath đã được thiết lập trước đó.
- Tất cả các pattern sẽ chỉ còn được tìm trong thư mục hiện tại.

## Pattern Rules

Pattern Rules là một tính năng mạnh mẽ giúp viết Makefile ngắn gọn và linh hoạt, bằng cách trừu tượng hoá quy tắc build cho nhiều file có cấu trúc tương tự.

Thay vì viết:

```bash
main.o: main.c
	$(CC) -c main.c -o main.o

utils.o: utils.c
	$(CC) -c utils.c -o utils.o
```

Ta chỉ cần viết:

```bash
%.o: %.c
	$(CC) -c $< -o $@
```

Make sẽ tự áp dụng quy tắc này cho mọi file `.c` tương ứng `.o`.

# Command Arm Toolchain

## Compiler

[Compiler Options] **-o** [Target] [All prerequisite]

**Compiler Options**

**1.** `-mcpu=cortex-m4`

> Chọn kiến trúc CPU đích là ARM Cortex-M4

- Lệnh này yêu cầu compiler sinh mã máy cho Cortex-M4.
- Có ảnh hưởng đến tập lệnh và tối ưu hóa.

**2.** `-mthumb`

> Bật chế độ Thumb instruction set (tập lệnh 16-bit của ARM)

**3.** `-fdata-sections` và `-ffunction-sections`

> Mỗi biến toàn cục và hàm được đặt vào một section riêng biệt trong output object.

- Bình thường: tất cả hàm/biến đi vào `.text`, `.data` chung.
- Với các flag này: mỗi `foo()` ➜ `.text.foo`, mỗi `bar` ➜ `.data.bar`.

👉 Kết hợp với linker flag --gc-sections ➜ loại bỏ mã không dùng ➜ giúp giảm kích thước firmware!

**4.** `-MMD, -MP, -MF"$(@:%.o=%.d)"`

> Dùng để tạo file dependency `.d` tự động

| Flag                  | Ý nghĩa                                                   |
|-----------------------|-----------------------------------------------------------|
| `-MMD`                | Tạo file .d chứa dependency, bỏ qua các system headers    |
| `-MP`                 | Thêm dummy rules để tránh lỗi nếu file .h bị xóa          |
| `-MF"$(@:%.o=%.d)"`   | Đặt tên file .d dựa trên tên file .o                      |

**5.** `-Ox`

> Tối ưu hóa mức độ x khi biên dịch.

**6.** `-I$(INCLUDE_DIR)`

> Thêm đường dẫn tìm kiếm file header vào trình biên dịch.

**7.** `-std=gnu11`

> Chỉ định chuẩn ngôn ngữ C được sử dụng là GNU C 2011

## Linker

[Linker Options] **-o** [Target] [All prerequisite]

**Linker Options**

**1.** `-lc`

> Link thư viện C chuẩn (libc).

**2.** `-lm`

> Link thư viện math, cần khi dùng `sin()`, `sqrt()`, `pow()`.

**3.** `-lnosys`

> Stub cho các syscall chưa định nghĩa để tránh lỗi link khi không có syscalls.c. `-lnosys` thường dùng khi không có OS, nó sẽ link tới `libnosys.a`, trong đó các syscall như `_write`, `_read`,... là hàm rỗng.

**4.** `-L$(LIBDIR)`

> Chỉ định thư mục chứa các thư viện .a, nếu ta cần link các thư viện tự tạo.

**5.** `-T$(LDSCRIPT)`

> Chỉ định linker script (`.ld` file) — là file định nghĩa cách sắp xếp vùng nhớ (FLASH, RAM, v.v.)

**6.** `-Map=[path_to_file.map]`

> Tạo file .map chứa thông tin symbol.
