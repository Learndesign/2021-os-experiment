
extern unsigned char _elf___test_test_shell_elf[];
int _length___test_test_shell_elf;
extern unsigned char _elf___test_test_fs_elf[];
int _length___test_test_fs_elf;
typedef struct ElfFile {
  char *file_name;
  unsigned char* file_content;
  int* file_length;
} ElfFile;

#define ELF_FILE_NUM 2
extern ElfFile elf_files[2];
extern int get_elf_file(const char *file_name, unsigned char **binary, int *length);
