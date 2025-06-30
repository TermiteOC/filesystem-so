#include "filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT 256

void terminal_interface(Directory* root) {
    Directory* current = root;
    char input[MAX_INPUT];

    while (1) {
        printf("> ");
        if (!fgets(input, sizeof(input), stdin)) break;

        // Remover \n
        input[strcspn(input, "\n")] = 0;

        char* cmd = strtok(input, " ");
        if (!cmd) continue;

        if (strcmp(cmd, "ls") == 0) {
            list_directory_contents(current);
        } else if (strcmp(cmd, "cd") == 0) {
            char* dir_name = strtok(NULL, " ");
            if (dir_name) {
                change_directory(&current, dir_name);
            } else {
                printf("Uso: cd <nome_diretorio>\n");
            }
        } else if (strcmp(cmd, "mkdir") == 0) {
            char* name = strtok(NULL, " ");
            if (name) {
                TreeNode* dir = create_directory(name);
                btree_insert(current->tree, dir);
            } else {
                printf("Uso: mkdir <nome_diretorio>\n");
            }
        } else if (strcmp(cmd, "touch") == 0) {
            char* name = strtok(NULL, " ");
            char* content = strtok(NULL, "");
            if (name && content) {
                if (strlen(content) > 1048576) { // 1MB = 1048576 bytes
                    printf("Erro: conteúdo excede o limite de 1MB.\n");
                } else {
                    TreeNode* file = create_txt_file(name, content);
                    if (file) {
                        btree_insert(current->tree, file);
                    }
                }
            } else {
                printf("Uso: touch <nome_arquivo> <conteudo>\n");
            }
        } else if (strcmp(cmd, "rm") == 0) {
            char* name = strtok(NULL, " ");
            if (name) {
                delete_txt_file(current->tree, name);
            } else {
                printf("Uso: rm <nome_arquivo>\n");
            }
        } else if (strcmp(cmd, "rmdir") == 0) {
            char* name = strtok(NULL, " ");
            if (name) {
                delete_directory(current->tree, name);
            } else {
                printf("Uso: rmdir <nome_diretorio>\n");
            }
        } else if (strcmp(cmd, "exit") == 0) {
            break;
        } else {
            printf("Comando desconhecido: %s\n", cmd);
        }
    }
}

int main() {
    Directory* root = get_root_directory();

    printf("\n--- Modo interativo ---\n");
    terminal_interface(root);

    FILE* img = fopen("fs.img", "w");
    if (img) {
        fprintf(img, "ROOT\n");
        save_img(img, root, 1, "│   ");
        fclose(img);
        printf("\nSistema de arquivos salvo em fs.img\n");
    } else {
        perror("Erro ao criar fs.img");
    }

    return 0;
}
