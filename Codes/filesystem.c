#include "filesystem.h"

BTree* btree_create() {
    BTree* tree = malloc(sizeof(BTree));
    tree->root = NULL;
    return tree;
}

TreeNode* create_txt_file(const char* name, const char* content) {
    File* file = malloc(sizeof(File));
    file->name = strdup(name);
    file->content = strdup(content);
    file->size = strlen(content);

    TreeNode* node = malloc(sizeof(TreeNode));
    node->name = strdup(name);
    node->type = FILE_TYPE;
    node->data.file = file;
    return node;
}

TreeNode* create_directory(const char* name) {
    Directory* dir = malloc(sizeof(Directory));
    dir->tree = btree_create();

    TreeNode* node = malloc(sizeof(TreeNode));
    node->name = strdup(name);
    node->type = DIRECTORY_TYPE;
    node->data.directory = dir;
    return node;
}

// Função auxiliar para criar novo nó de árvore B
static BTreeNode* btree_create_node(int leaf) {
    BTreeNode* node = malloc(sizeof(BTreeNode));
    node->num_keys = 0;
    node->leaf = leaf;
    for (int i = 0; i < 2 * BTREE_ORDER; i++) {
        node->children[i] = NULL;
    }
    return node;
}

// Busca por nome na árvore B (recursiva)
static TreeNode* btree_search_recursive(BTreeNode* node, const char* name) {
    if (node == NULL) return NULL;
    int i = 0;
    while (i < node->num_keys && strcmp(name, node->keys[i]->name) > 0) {
        i++;
    }
    if (i < node->num_keys && strcmp(name, node->keys[i]->name) == 0) {
        return node->keys[i];
    }
    if (node->leaf) {
        return NULL;
    } else {
        return btree_search_recursive(node->children[i], name);
    }
}

TreeNode* btree_search(BTree* tree, const char* name) {
    printf("Buscando: %s\n", name);
    return btree_search_recursive(tree->root, name);
}

// Divide o filho y de x na posição i
static void btree_split_child(BTreeNode* x, int i, BTreeNode* y) {
    BTreeNode* z = btree_create_node(y->leaf);
    z->num_keys = BTREE_ORDER - 1;

    for (int j = 0; j < BTREE_ORDER - 1; j++) {
        z->keys[j] = y->keys[j + BTREE_ORDER];
    }
    if (!y->leaf) {
        for (int j = 0; j < BTREE_ORDER; j++) {
            z->children[j] = y->children[j + BTREE_ORDER];
        }
    }

    y->num_keys = BTREE_ORDER - 1;

    for (int j = x->num_keys; j >= i + 1; j--) {
        x->children[j + 1] = x->children[j];
    }
    x->children[i + 1] = z;

    for (int j = x->num_keys - 1; j >= i; j--) {
        x->keys[j + 1] = x->keys[j];
    }
    x->keys[i] = y->keys[BTREE_ORDER - 1];
    x->num_keys++;
}

// Inserção não cheia
static void btree_insert_nonfull(BTreeNode* node, TreeNode* k) {
    int i = node->num_keys - 1;

    if (node->leaf) {
        while (i >= 0 && strcmp(k->name, node->keys[i]->name) < 0) {
            node->keys[i + 1] = node->keys[i];
            i--;
        }
        node->keys[i + 1] = k;
        node->num_keys++;
    } else {
        while (i >= 0 && strcmp(k->name, node->keys[i]->name) < 0) {
            i--;
        }
        i++;
        if (node->children[i]->num_keys == 2 * BTREE_ORDER - 1) {
            btree_split_child(node, i, node->children[i]);
            if (strcmp(k->name, node->keys[i]->name) > 0) {
                i++;
            }
        }
        btree_insert_nonfull(node->children[i], k);
    }
}

void btree_insert(BTree* tree, TreeNode* k) {
    printf("Inserindo: %s\n", k->name);

    if (btree_search(tree, k->name)) {
        printf("Erro: nome '%s' ja existe no diretorio.\n", k->name);
        return;
    }

    if (tree->root == NULL) {
        tree->root = btree_create_node(1);
        tree->root->keys[0] = k;
        tree->root->num_keys = 1;
    } else {
        if (tree->root->num_keys == 2 * BTREE_ORDER - 1) {
            BTreeNode* s = btree_create_node(0);
            s->children[0] = tree->root;
            btree_split_child(s, 0, tree->root);
            tree->root = s;
        }
        btree_insert_nonfull(tree->root, k);
    }
}

// Função auxiliar para liberar TreeNode
static void free_treenode(TreeNode* node) {
    if (node->type == FILE_TYPE) {
        free(node->data.file->name);
        free(node->data.file->content);
        free(node->data.file);
    } else {
        free(node->data.directory->tree);
        free(node->data.directory);
    }
    free(node->name);
    free(node);
}

// Exclusão de chave da raiz (simples)
void btree_delete(BTree* tree, const char* name) {
    printf("Removendo: %s\n", name);

    if (!tree || !tree->root) return;
    BTreeNode* node = tree->root;
    for (int i = 0; i < node->num_keys; i++) {
        if (strcmp(node->keys[i]->name, name) == 0) {
            free_treenode(node->keys[i]);
            for (int j = i; j < node->num_keys - 1; j++) {
                node->keys[j] = node->keys[j + 1];
            }
            node->num_keys--;
            return;
        }
    }
    printf("'%s' nao encontrado para remocao.\n", name);
}

static void btree_traverse_recursive(BTreeNode* node) {
    if (!node) return;
    int i;
    for (i = 0; i < node->num_keys; i++) {
        if (!node->leaf) {
            btree_traverse_recursive(node->children[i]);
        }
        printf("- %s\n", node->keys[i]->name);
    }
    if (!node->leaf) {
        btree_traverse_recursive(node->children[i]);
    }
}

void btree_traverse(BTree* tree) {
    printf("[Exemplo] arquivo.txt\n");

    btree_traverse_recursive(tree->root);
}

void delete_txt_file(BTree* tree, const char* name) {
    TreeNode* node = btree_search(tree, name);
    if (!node || node->type != FILE_TYPE) {
        printf("Arquivo '%s' nao encontrado.\n", name);
        return;
    }
    btree_delete(tree, name);
    printf("Arquivo '%s' deletado\n", name);
}

void delete_directory(BTree* tree, const char* name) {
    TreeNode* node = btree_search(tree, name);
    if (!node || node->type != DIRECTORY_TYPE) {
        printf("Diretorio '%s' nao encontrado.\n", name);
        return;
    }
    if (node->data.directory->tree->root != NULL && node->data.directory->tree->root->num_keys > 0) {
        printf("Diretorio '%s' nao esta vazio.\n", name);
        return;
    }
    btree_delete(tree, name);
    printf("Diretório '%s' deletado\n", name);
}

Directory* get_root_directory() {
    Directory* root = malloc(sizeof(Directory));
    root->tree = btree_create();
    return root;
}

void change_directory(Directory** current, const char* path) {
    printf("Mudando para o diretório: %s\n", path);

    TreeNode* node = btree_search((*current)->tree, path);
    if (!node || node->type != DIRECTORY_TYPE) {
        printf("Diretorio '%s' nao encontrado.\n", path);
        return;
    }
    *current = node->data.directory;
    printf("Diretorio atual: %s\n", path);
}

void list_directory_contents(Directory* dir) {
    printf("Conteúdo do diretório:\n");
    btree_traverse(dir->tree);
}