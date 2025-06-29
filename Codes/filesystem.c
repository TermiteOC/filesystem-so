#include "filesystem.h"

// Função auxiliar para criar um novo nó da árvore B
static BTreeNode* btree_create_node(int leaf) {
    BTreeNode* node = malloc(sizeof(BTreeNode));
    node->num_keys = 0;
    node->leaf = leaf;
    for (int i = 0; i < 2 * BTREE_ORDER; i++) {
        node->children[i] = NULL;
    }
    for (int i = 0; i < 2 * BTREE_ORDER - 1; i++) {
        node->keys[i] = NULL;
    }
    return node;
}

BTree* btree_create() {
    BTree* tree = malloc(sizeof(BTree));
    tree->root = btree_create_node(1); // raiz folha inicialmente
    return tree;
}

// Função auxiliar para procurar índice onde inserir/buscar nome em nó (ordenado alfabeticamente)
static int find_key_index(BTreeNode* node, const char* name) {
    int idx = 0;
    while (idx < node->num_keys && strcmp(node->keys[idx]->name, name) < 0) {
        idx++;
    }
    return idx;
}

// Busca recursiva na árvore B
TreeNode* btree_search_node(BTreeNode* node, const char* name) {
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
        return btree_search_node(node->children[i], name);
    }
}

TreeNode* btree_search(BTree* tree, const char* name) {
    if (!tree || !tree->root) return NULL;
    return btree_search_node(tree->root, name);
}

// Divide filho cheio y de x na posição i
static void btree_split_child(BTreeNode* x, int i, BTreeNode* y) {
    BTreeNode* z = btree_create_node(y->leaf);
    z->num_keys = BTREE_ORDER - 1;

    // Copiar últimas chaves de y para z
    for (int j = 0; j < BTREE_ORDER - 1; j++) {
        z->keys[j] = y->keys[j + BTREE_ORDER];
    }

    // Se não for folha, copiar filhos também
    if (!y->leaf) {
        for (int j = 0; j < BTREE_ORDER; j++) {
            z->children[j] = y->children[j + BTREE_ORDER];
        }
    }

    y->num_keys = BTREE_ORDER - 1;

    // Mover filhos de x para abrir espaço
    for (int j = x->num_keys; j >= i + 1; j--) {
        x->children[j + 1] = x->children[j];
    }
    x->children[i + 1] = z;

    // Mover chaves de x para abrir espaço
    for (int j = x->num_keys - 1; j >= i; j--) {
        x->keys[j + 1] = x->keys[j];
    }
    // Promover chave do meio de y para x
    x->keys[i] = y->keys[BTREE_ORDER - 1];
    x->num_keys++;
}

// Inserção em nó que não está cheio
static void btree_insert_nonfull(BTreeNode* node, TreeNode* new_node) {
    int i = node->num_keys - 1;

    if (node->leaf) {
        // Inserir em nó folha (chaves ordenadas alfabeticamente)
        while (i >= 0 && strcmp(new_node->name, node->keys[i]->name) < 0) {
            node->keys[i + 1] = node->keys[i];
            i--;
        }
        node->keys[i + 1] = new_node;
        node->num_keys++;
    } else {
        // Nó interno, descer para o filho correto
        while (i >= 0 && strcmp(new_node->name, node->keys[i]->name) < 0) {
            i--;
        }
        i++;

        // Se filho cheio, dividir
        if (node->children[i]->num_keys == 2 * BTREE_ORDER - 1) {
            btree_split_child(node, i, node->children[i]);
            if (strcmp(new_node->name, node->keys[i]->name) > 0) {
                i++;
            }
        }
        btree_insert_nonfull(node->children[i], new_node);
    }
}

void btree_insert(BTree* tree, TreeNode* node) {
    if (!tree || !node) return;

    // Checar se nome já existe (único por diretório)
    if (btree_search(tree, node->name)) {
        printf("Erro: nome '%s' já existe no diretório.\n", node->name);
        return;
    }

    BTreeNode* r = tree->root;
    if (r->num_keys == 2 * BTREE_ORDER - 1) {
        // Raiz cheia, criar nova raiz
        BTreeNode* s = btree_create_node(0);
        tree->root = s;
        s->children[0] = r;
        btree_split_child(s, 0, r);
        btree_insert_nonfull(s, node);
    } else {
        btree_insert_nonfull(r, node);
    }
}

// Função auxiliar para encontrar o índice de uma chave em um nó (retorna -1 se não encontrado)
static int btree_find_key(BTreeNode* node, const char* name) {
    for (int i = 0; i < node->num_keys; i++) {
        if (strcmp(node->keys[i]->name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Função auxiliar para remover chave em nó folha
static void btree_remove_from_leaf(BTreeNode* node, int idx) {
    for (int i = idx + 1; i < node->num_keys; i++) {
        node->keys[i - 1] = node->keys[i];
    }
    node->num_keys--;
}

// Função auxiliar para remover chave em nó interno
static void btree_remove_from_nonleaf(BTreeNode* node, int idx);

// Função auxiliar para pegar predecessor de uma chave
static TreeNode* btree_get_pred(BTreeNode* node, int idx) {
    BTreeNode* cur = node->children[idx];
    while (!cur->leaf) {
        cur = cur->children[cur->num_keys];
    }
    return cur->keys[cur->num_keys - 1];
}

// Função auxiliar para pegar sucessor de uma chave
static TreeNode* btree_get_succ(BTreeNode* node, int idx) {
    BTreeNode* cur = node->children[idx + 1];
    while (!cur->leaf) {
        cur = cur->children[0];
    }
    return cur->keys[0];
}

// Função auxiliar para preencher filho com poucas chaves
static void btree_fill(BTreeNode* node, int idx);

// Função auxiliar para emprestar da esquerda
static void btree_borrow_from_prev(BTreeNode* node, int idx);

// Função auxiliar para emprestar da direita
static void btree_borrow_from_next(BTreeNode* node, int idx);

// Função auxiliar para mesclar filho com irmão
static void btree_merge(BTreeNode* node, int idx);

static void btree_remove(BTreeNode* node, const char* name) {
    int idx = btree_find_key(node, name);

    if (idx != -1) {
        // Chave encontrada no nó
        if (node->leaf) {
            // Nó folha
            btree_remove_from_leaf(node, idx);
        } else {
            // Nó interno
            btree_remove_from_nonleaf(node, idx);
        }
    } else {
        // Chave não está nesse nó
        if (node->leaf) {
            // Não está na árvore
            printf("Erro: arquivo ou diretório '%s' não encontrado para remoção.\n", name);
            return;
        }

        // Determinar se chave está presente no filho idx
        int flag = ( (idx == node->num_keys) ? 1 : 0 );

        if (node->children[idx]->num_keys < BTREE_ORDER) {
            btree_fill(node, idx);
        }

        if (flag && idx > node->num_keys) {
            btree_remove(node->children[idx - 1], name);
        } else {
            btree_remove(node->children[idx], name);
        }
    }
}

static void btree_remove_from_nonleaf(BTreeNode* node, int idx) {
    TreeNode* k = node->keys[idx];

    // Se filho anterior tem >= t chaves, pegar predecessor
    if (node->children[idx]->num_keys >= BTREE_ORDER) {
        TreeNode* pred = btree_get_pred(node, idx);
        node->keys[idx] = pred;
        btree_remove(node->children[idx], pred->name);
    }
    // Se filho seguinte tem >= t chaves, pegar sucessor
    else if (node->children[idx + 1]->num_keys >= BTREE_ORDER) {
        TreeNode* succ = btree_get_succ(node, idx);
        node->keys[idx] = succ;
        btree_remove(node->children[idx + 1], succ->name);
    }
    // Mesclar filhos e remover recursivamente
    else {
        btree_merge(node, idx);
        btree_remove(node->children[idx], k->name);
    }
}

static void btree_fill(BTreeNode* node, int idx) {
    if (idx != 0 && node->children[idx - 1]->num_keys >= BTREE_ORDER) {
        btree_borrow_from_prev(node, idx);
    } else if (idx != node->num_keys && node->children[idx + 1]->num_keys >= BTREE_ORDER) {
        btree_borrow_from_next(node, idx);
    } else {
        if (idx != node->num_keys) {
            btree_merge(node, idx);
        } else {
            btree_merge(node, idx - 1);
        }
    }
}

static void btree_borrow_from_prev(BTreeNode* node, int idx) {
    BTreeNode* child = node->children[idx];
    BTreeNode* sibling = node->children[idx - 1];

    // Deslocar chaves e filhos de child para frente
    for (int i = child->num_keys - 1; i >= 0; i--) {
        child->keys[i + 1] = child->keys[i];
    }
    if (!child->leaf) {
        for (int i = child->num_keys; i >= 0; i--) {
            child->children[i + 1] = child->children[i];
        }
    }

    // Pegar chave de pai para child
    child->keys[0] = node->keys[idx - 1];

    if (!child->leaf) {
        child->children[0] = sibling->children[sibling->num_keys];
    }

    // Pegar chave de sibling para pai
    node->keys[idx - 1] = sibling->keys[sibling->num_keys - 1];

    sibling->num_keys--;
    child->num_keys++;
}

static void btree_borrow_from_next(BTreeNode* node, int idx) {
    BTreeNode* child = node->children[idx];
    BTreeNode* sibling = node->children[idx + 1];

    // Chave de pai para child
    child->keys[child->num_keys] = node->keys[idx];

    if (!(child->leaf)) {
        child->children[child->num_keys + 1] = sibling->children[0];
    }

    // Chave de sibling para pai
    node->keys[idx] = sibling->keys[0];

    // Deslocar chaves e filhos de sibling para trás
    for (int i = 1; i < sibling->num_keys; i++) {
        sibling->keys[i - 1] = sibling->keys[i];
    }
    if (!sibling->leaf) {
        for (int i = 1; i <= sibling->num_keys; i++) {
            sibling->children[i - 1] = sibling->children[i];
        }
    }

    sibling->num_keys--;
    child->num_keys++;
}

static void btree_merge(BTreeNode* node, int idx) {
    BTreeNode* child = node->children[idx];
    BTreeNode* sibling = node->children[idx + 1];

    // Pegar chave de pai para child
    child->keys[BTREE_ORDER - 1] = node->keys[idx];

    // Copiar chaves de sibling para child
    for (int i = 0; i < sibling->num_keys; i++) {
        child->keys[i + BTREE_ORDER] = sibling->keys[i];
    }

    // Copiar filhos de sibling para child
    if (!child->leaf) {
        for (int i = 0; i <= sibling->num_keys; i++) {
            child->children[i + BTREE_ORDER] = sibling->children[i];
        }
    }

    child->num_keys += sibling->num_keys + 1;

    // Remover chave de node
    for (int i = idx + 1; i < node->num_keys; i++) {
        node->keys[i - 1] = node->keys[i];
    }
    // Mover filhos para trás
    for (int i = idx + 2; i <= node->num_keys; i++) {
        node->children[i - 1] = node->children[i];
    }

    node->num_keys--;

    free(sibling);
}

void btree_delete(BTree* tree, const char* name) {
    if (!tree || !tree->root) return;

    btree_remove(tree->root, name);

    // Se raiz ficou vazia, atualizar raiz
    if (tree->root->num_keys == 0) {
        BTreeNode* old_root = tree->root;
        if (tree->root->leaf) {
            // Árvore ficou vazia
            free(tree->root);
            tree->root = NULL;
        } else {
            tree->root = tree->root->children[0];
            free(old_root);
        }
    }
}

// Percorre e imprime os nomes de arquivos e pastas em ordem lexicográfica
static void btree_traverse_node(BTreeNode* node, int depth) {
    if (!node) return;

    for (int i = 0; i < node->num_keys; i++) {
        if (!node->leaf) {
            btree_traverse_node(node->children[i], depth + 1);
        }
        for (int j = 0; j < depth; j++) printf("  ");
        if (node->keys[i]->type == DIRECTORY_TYPE) {
            printf("[DIR] %s\n", node->keys[i]->name);
        } else {
            printf("[FILE] %s\n", node->keys[i]->name);
        }
    }
    if (!node->leaf) {
        btree_traverse_node(node->children[node->num_keys], depth + 1);
    }
}

void btree_traverse(BTree* tree) {
    if (!tree) return;
    btree_traverse_node(tree->root, 0);
}

// Remoção de arquivo: verifica se existe e remove da árvore
void delete_txt_file(BTree* tree, const char* name) {
    TreeNode* node = btree_search(tree, name);
    if (!node) {
        printf("Arquivo '%s' não encontrado para remoção.\n", name);
        return;
    }
    if (node->type != FILE_TYPE) {
        printf("'%s' não é um arquivo.\n", name);
        return;
    }
    btree_delete(tree, name);
    printf("Arquivo '%s' removido com sucesso.\n", name);
}

// Remoção de diretório: verifica se vazio e remove
void delete_directory(BTree* tree, const char* name) {
    TreeNode* node = btree_search(tree, name);
    if (!node) {
        printf("Diretório '%s' não encontrado para remoção.\n", name);
        return;
    }
    if (node->type != DIRECTORY_TYPE) {
        printf("'%s' não é um diretório.\n", name);
        return;
    }

    // Verifica se diretório está vazio
    if (node->data.directory->tree->root->num_keys > 0) {
        printf("Diretório '%s' não está vazio e não pode ser removido.\n", name);
        return;
    }

    btree_delete(tree, name);
    printf("Diretório '%s' removido com sucesso.\n", name);
}

// Navegação: muda o diretório atual para o especificado por path (apenas nome do filho ou "..")
void change_directory(Directory** current, const char* path) {
    if (strcmp(path, "..") == 0) {
        // Não tem estrutura para diretório pai no seu modelo, então ignorar
        printf("Comando '..' não suportado (diretório pai não armazenado).\n");
        return;
    }
    TreeNode* node = btree_search((*current)->tree, path);
    if (!node) {
        printf("Diretório '%s' não encontrado.\n", path);
        return;
    }
    if (node->type != DIRECTORY_TYPE) {
        printf("'%s' não é um diretório.\n", path);
        return;
    }
    *current = node->data.directory;
    printf("Mudou para diretório '%s'.\n", path);
}

// Lista o conteúdo do diretório atual (arquivos e subdiretórios)
void list_directory_contents(Directory* dir) {
    if (!dir || !dir->tree) return;
    btree_traverse(dir->tree);
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

Directory* get_root_directory() {
    Directory* root = malloc(sizeof(Directory));
    root->tree = btree_create();
    return root;
}
