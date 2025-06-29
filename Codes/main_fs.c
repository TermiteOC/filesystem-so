#include "filesystem.h"

int main() {
    Directory* root = get_root_directory();

    // Criar diretórios
    TreeNode* dirSO = create_directory("SO");
    TreeNode* dirTEST = create_directory("TEST");

    btree_insert(root->tree, dirSO);
    btree_insert(root->tree, dirTEST);

    // Criar arquivos em SO
    TreeNode* file1 = create_txt_file("arquivo1.txt", "Arquivo de teste de SO.");
    TreeNode* file2 = create_txt_file("readme.txt", "Arquivo readme.");

    btree_insert(dirSO->data.directory->tree, file1);
    btree_insert(dirSO->data.directory->tree, file2);

    // Listar conteúdo do root
    printf("--- Conteúdo do diretório ROOT ---\n");
    list_directory_contents(root);

    // Listar conteúdo do SO
    printf("\n--- Conteúdo do diretório SO ---\n");
    list_directory_contents(dirSO->data.directory);

    // Navegar para SO
    Directory* current = root;
    change_directory(&current, "SO");

    // Listar conteúdo do diretório atual (SO)
    printf("\n--- Conteúdo do diretório atual ---\n");
    list_directory_contents(current);

    // Deletar arquivo
    delete_txt_file(current->tree, "arquivo1.txt");

    printf("\n--- Conteúdo do diretório SO após exclusão ---\n");
    list_directory_contents(current);

    // Tentar remover diretório TEST (vazio)
    delete_directory(root->tree, "TEST");

    printf("\n--- Conteúdo do diretório ROOT após remoção de TEST ---\n");
    list_directory_contents(root);

    // Tentar remover diretório SO (não vazio)
    delete_directory(root->tree, "SO");

    return 0;
}
