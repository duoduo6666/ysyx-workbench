#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <elf.h>

#define CONFIG_FTRACE_STACK_SIZE 256

// extern bool exit_status;
extern char* img_file;
extern char* elf_file;
typedef uint32_t word_t;

Elf32_Sym* symbols;
int symbol_num;
char* strtab;
int ftrace_deep = 0;
char* ftrace_stack[CONFIG_FTRACE_STACK_SIZE] = {0};
void init_ftrace() {
    
    if (img_file == NULL && elf_file == NULL) return;
    size_t file_len = strlen(img_file);
    if (elf_file == NULL && strcmp(img_file+file_len-3, "bin") == 0) {
        elf_file = (char*)malloc(file_len+1);
        strcpy(elf_file, img_file);
        elf_file[file_len-3] = 'e';
        elf_file[file_len-2] = 'l';
        elf_file[file_len-1] = 'f';
    }
    // printf("%s\n%s\n", img_file, elf_file);
    assert(elf_file != NULL);

    FILE* fp = fopen(elf_file, "r");
    assert(fp != NULL);

    Elf32_Ehdr elf;
    assert(fread(&elf, sizeof(Elf32_Ehdr), 1, fp) == 1);

    assert(fseek(fp, elf.e_shoff, SEEK_SET) == 0);
    Elf32_Shdr* shdr = (Elf32_Shdr*)malloc(elf.e_shnum * sizeof(Elf32_Shdr));
    assert(shdr != NULL);
    assert(fread(shdr, sizeof(Elf32_Shdr), elf.e_shnum, fp) == elf.e_shnum);

    char* shstrtab = NULL;
    shstrtab = (char*)malloc(shdr[elf.e_shstrndx].sh_size);
    assert(shstrtab != NULL);
    assert(fseek(fp, shdr[elf.e_shstrndx].sh_offset, SEEK_SET) == 0);
    assert(fread(shstrtab, sizeof(char), shdr[elf.e_shstrndx].sh_size, fp) == shdr[elf.e_shstrndx].sh_size);

    Elf32_Shdr* symtab_hdr = NULL;
    strtab = NULL;
    for (int i = 0; i < elf.e_shnum; i++) {
        // Log("[%d] Name: %s Type: %x", i, shstrtab + shdr[i].sh_name, shdr[i].sh_type);
        if (shdr[i].sh_type == SHT_SYMTAB) {
        symtab_hdr = &shdr[i];
        }
        if (strcmp(shstrtab + shdr[i].sh_name, ".strtab") == 0) {
        strtab = (char*)malloc(shdr[i].sh_size);
        assert(fseek(fp, shdr[i].sh_offset, SEEK_SET) == 0);
        assert(fread(strtab, sizeof(char), shdr[i].sh_size, fp) == shdr[i].sh_size);
        }
    }
    assert(symtab_hdr != NULL);

    symbol_num = symtab_hdr->sh_size / sizeof(Elf32_Sym);
    symbols = (Elf32_Sym*)malloc(symtab_hdr->sh_size);
    assert(symbols != NULL);
    assert(fseek(fp, symtab_hdr->sh_offset, SEEK_SET) == 0);
    assert(fread(symbols, sizeof(char), symtab_hdr->sh_size, fp) == symtab_hdr->sh_size);
    for (int i = 0; i < symbol_num; i++) {
        if (ELF32_ST_TYPE(symbols[i].st_info) == STT_FUNC) {
            printf("function [%d] Name %s Value 0x%x Size 0x%x\n", i, strtab + symbols[i].st_name, symbols[i].st_value, symbols[i].st_size);
        }
    }

    fclose(fp);
}

int ftrace_pc_in_func(word_t pc) {
    for (int i = 0; i < symbol_num; i++) {
        if (pc > symbols[i].st_value && pc < symbols[i].st_value + symbols[i].st_size){
        return i;
        }
    }
    return -1;
}

void ftrace_print_deep() {
    for (int i = 0; i < ftrace_deep; i++) {
        printf("  ");
    }
}

int ftrace_search_stack(char* func) {
    for (int i = ftrace_deep - 1; i >= 0; i--) {
        if (strcmp(ftrace_stack[i], func)) {
        return i + 1;
        }
    }
    return -1;
}

void ftrace_check(word_t pc, word_t next_pc) {
    if (img_file == NULL && elf_file == NULL) return;
    for (int i = 0; i < symbol_num; i++) {
        if (ELF32_ST_TYPE(symbols[i].st_info) == STT_FUNC) {
            if (next_pc == symbols[i].st_value) {
                printf("0x%x: ", pc);
                ftrace_print_deep();
                printf("cell[%s@0x%x]\n", strtab + symbols[i].st_name, symbols[i].st_value);
                ftrace_stack[ftrace_deep] = strtab + symbols[i].st_name;
                ftrace_deep++;
                assert(ftrace_deep < CONFIG_FTRACE_STACK_SIZE);
                return;
            }
        }
        }
    for (int i = 0; i < symbol_num; i++) {
        if (ELF32_ST_TYPE(symbols[i].st_info) == STT_FUNC) {
            if (ftrace_pc_in_func(pc) == i && (next_pc > symbols[i].st_value + symbols[i].st_size || next_pc < symbols[i].st_value)) {
                ftrace_deep = ftrace_search_stack(strtab + symbols[i].st_name);
                printf("ftrace_deep %d\n", ftrace_deep);
                assert(ftrace_deep > 0);
                printf("0x%x: ", pc);
                ftrace_print_deep();
                printf("ret[%s@0x%x]\n", strtab + symbols[i].st_name, next_pc);
                break;
            }
        }
    }
}