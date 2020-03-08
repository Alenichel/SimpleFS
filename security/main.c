
//
//  main.c
//  SimpleFS
//
//  Created by Alessandro Nichelini on 21/07/2017.
//  Copyright © 2017 Alessandro Nichelini. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAXWIDTH 1024
#define MAXDEPTH 255
#define MINIMUM_BUFFER_SIZE 1024
#define HASH_TABLE_SIZE 1031
#define HASH_ERROR_CODE HASH_TABLE_SIZE + 1
#define HASHING_PRIME_NUMBER 214463
////
// MARK: - FUNCTION DECLARATION
////
int find_symbol(char *string, char symbol);
void fix_sybol(char *string, char symbol);
void clean_string(char * string);

struct node;
typedef struct node Node;
typedef Node *NodePtr;
void free_node(NodePtr node);
void delete_node(NodePtr father, unsigned int hash_key);

struct result;
typedef struct result Result;
typedef Result *ResultPtr;
void tree_insert(ResultPtr root, ResultPtr new_brother);
ResultPtr tree_minimum(ResultPtr root);
ResultPtr tree_search(ResultPtr root, char *path);
void transplant(ResultPtr root, ResultPtr u, ResultPtr v);
void inorder_tree_walk( ResultPtr root);

NodePtr solvePath(NodePtr root, char *path );
unsigned int hashing_handler(NodePtr father, char *path);

void create(NodePtr root, char *path, char type);
void delete(NodePtr roo, char *path);
void target_recursive(NodePtr root ,char *path);
void delete_recursive(NodePtr father, unsigned int hash_key);
void read_file(NodePtr root, char *path);
void write_file(NodePtr root, char *path, char *content);
void launch_find(NodePtr root, char *resource_name);
void find(NodePtr node, ResultPtr results, char *resource_name, char *path);

NodePtr global_tombstone;
NodePtr global_shortcut;
NodePtr global_root;
char * global_last_path;

////
// MARK: - STRUCT
////
struct node {
    char type;
    char *name;
    char *data;
    struct node **children;
    unsigned int children_counter;
};

struct result{
    char *path;
    struct result * father;
    struct result * left_brother;
    struct result * right_brother;
};

void free_node(NodePtr node){
    //free(node->name);
    free(node->data);
    //if (node->children != NULL) free(node->children);
    free(node);
}

void delete_node(NodePtr father, unsigned int hash_key){
    free_node(father->children[hash_key]);//libero la memoria associata al nodo
    father->children[hash_key]= global_tombstone;
    father->children_counter--; //decremento il contatore dei figli nel padre
}

////
// MARK: - HASH
////
unsigned int myhash (void* key, unsigned long len){
    unsigned int hash = 0;
    unsigned char* k_copy = key;
    unsigned int i;
    
    for (i = 0; i < len; i++)
        hash += k_copy[i] * HASHING_PRIME_NUMBER^i; //predispongo per fare diverse prove sulla bontà del numero primo scelto
    return hash%HASH_TABLE_SIZE; //predispongo per fare il programma completamente scalibile in base alla dimensione della tabella di hash voluta
}

//funzione che gestisce le collisioni.
unsigned int conflicts_handler(unsigned int conflicted_key, unsigned int counter){
    return (conflicted_key + counter*(1 + conflicted_key%(HASH_TABLE_SIZE-1)))%HASH_TABLE_SIZE;
}
////
// MARK: - TOOLBOOX
////
int find_symbol(char *string, char symbol){
    for(int i = 0; *string; string++, i++)
        if( *string == symbol ) return i;
    return 0;
}
void fix_symbol(char *string, char symbol, char substitute){
    for(; *string; string++)
        if( *string ==  symbol)
            *string = substitute;
}
unsigned int count_symbol(char *string, char symbol){
    unsigned int i = 0;
    for(; *string; string++)
        if( *string == symbol ) i++;
    return i;
}
unsigned int last_symbol(char *string, char symbol){
    unsigned int n = count_symbol(string, symbol);
    for(unsigned int i = 0; *string; string++, i++){
        if(*string == symbol)
            n--;
        if (n == 0)
            return i;
    }
    return 0;
}
void clean_string(char * string){
    for(; *string; string++) *string = '\0';
}
unsigned long max(unsigned int len1, unsigned long len2){
    if(len1 >len2 ) return len1;
    return len2;
}
////
// MARK: - BINARY TREE FUNCTIONS (per la find)
////
/* Implemento un albero binario per gestire i risultati della find e stamparli tramite una visita in-order*/
void tree_insert(ResultPtr root, ResultPtr new_brother){
    ResultPtr y = NULL, x = root;
    while( x != NULL ){
        y = x;
        if( strcmp(new_brother->path, x->path) < 0)
            x = x->left_brother;
        else x = x->right_brother;
    }
    new_brother->father = y;
    if (y == NULL)
        root = new_brother;
    else if ( strcmp(new_brother->path, y->path) < 0 )
        y->left_brother = new_brother;
    else y->right_brother = new_brother;
}

ResultPtr tree_minimum(ResultPtr node){
    while(node->left_brother != NULL)
        node = node->left_brother;
    return node;
}

void transplant(ResultPtr root, ResultPtr first, ResultPtr second){
    if(first->father == NULL) root = second;
    else if (first == first->father->left_brother) first->father->left_brother = second;
    else first->father->right_brother = second;
    if(second != NULL) second->father = first->father;
}

void inorder_tree_walk( ResultPtr root){
    if(root != NULL){
        inorder_tree_walk(root->left_brother);
        printf("ok %s\n", root->path); //stampo su stdout il path trovato
        inorder_tree_walk(root->right_brother);
    }
}

////
// MARK: - FILE SYSTEM FUNCTIONS
////
NodePtr solvePath(NodePtr root, char *path ){
    unsigned int index = 0, hash_key = 0, counter = 0, slash_counter = count_symbol(&path[1], '/');
    unsigned int conflicts_counter = 0, alternative_key = 0;
    char tmp[256] = {'\0'};
    NodePtr hash_elected_node = NULL;
    
    //nel caso in cui la profondità del percorso ecceda quella prevista dai limiti del FS, sicuramente non avrò un puntatore alla risorsa cercata e ritorno quindi NULL
    if(slash_counter > MAXDEPTH) return NULL;
    
    //scorro il path dividendo l'input in base alla posizione degli slash.
    //per ogni nome del percorso, mi sposto nella rispettiva posizione delle tabelle hash associate
    do{
        path++;
        index = find_symbol(path, '/'); //trovo il successivo '/' per trovare la fine della cartella
        clean_string(tmp);
        if( index != 0 ) //nel caso ve ne siano, copio la stringa, fino a quel carattere, in tmp
            strncpy(tmp, path, index);
        else //nel caso invece che non ve ne siano, copio tutta la stringa in tmp
            strcpy(tmp, path);
        path = path + index;
        
        hash_key = myhash(tmp, strlen(tmp)); //calcolo la funzione di hash sul nome del percorso
        hash_elected_node = root->children[hash_key]; //mi posiziono sull'elemento scelto
        
        //se sono arrivato all'ultimo nome oltre lo slash, vuol dire che sono riuscito a percorrere tutto il path in maniera corretta e che posso quindi ritorno il puntatore all'ultimo padre
        if( slash_counter - counter == 0 ){
            return root;
        }//fine dell'else sulla casistica sulla gerarchia della risorsa
        
        else{//se invece non mi trovo sull'ultimo nome e quindi devo entrare in una directory
            //nel caso in cui la cella trovata applicando la hash sia vuota, vuol dire che il path non è valido
            if (hash_elected_node == NULL ){
                return NULL;
            }
            //nel caso in cui la cella trovata applicando la hash sia piena del giusto nodo di tipo directory, continuo a scorrere
            else if( strcmp(hash_elected_node->name, tmp) == 0 && hash_elected_node->type == 'd' ){
                root = hash_elected_node;
                counter++; //aggiorno il counter
            }
            //nel caso in cui la cella trovata applicando la hash sia piena di un nodo con il nome corretto ma di tipo file, il path è ovviamente invalido.
            else if( strcmp(hash_elected_node->name, tmp) == 0 && hash_elected_node->type == 'f'){
                return NULL;
            }
            //negli altri casi: ossia nel caso in cui la cella trovata applicando la hash sia piena di un nodo direcotory con un nome diverso da quello ricercato, vuol dire che si sta verificando un conflitto
            else{
                //continuo ad applicare la sequenza di probing fin quando non trovo il nodo giusto
                while ( strcmp(hash_elected_node->name, tmp) != 0){
                    conflicts_counter++; //aggiorno il contatore dei conflitti
                    alternative_key = conflicts_handler(hash_key, conflicts_counter); //calcolo nuovamente la key
                    hash_elected_node = root->children[alternative_key]; //riassegno in base alla nuova key
                    if (hash_elected_node == NULL) return NULL;
                } //fine del while del conflitto
                root = root->children[alternative_key];
                counter++;
                conflicts_counter = 0;
            }//fine dell'else sulla casistica dello scorrimento del path
        }//fine dell'else sulla casistica sulla gerarchia della risorsa
    } while (index != 0); //fine del do-while concernente ciascuna risorsa
    return NULL;
}//fine della funzione

unsigned int hashing_handler(NodePtr father, char *path){
    unsigned int last_slash = last_symbol(path, '/'), hash_key = 0;
    unsigned int alternative_key = 0, conflicts_counter = 0;
    
    char resource_name[256];
    strcpy(resource_name, &path[last_slash+1]);
    
    if (father == NULL) return HASH_ERROR_CODE;
    
    hash_key = myhash(resource_name, strlen(resource_name)); //calcolo la funzione di hash sul nome del percorso
    NodePtr hash_elected_node = father->children[hash_key]; //mi posiziono sull'elemento scelto
    
    //nel caso la risorsa non esiste
    if( hash_elected_node == NULL) return HASH_ERROR_CODE;
    
    //nel caso non ci siano conflitti
    if( strcmp(hash_elected_node->name, resource_name) == 0 ) return hash_key;
    
    else{
        while (hash_elected_node != NULL){
            if( strcmp(hash_elected_node->name, resource_name) == 0 ) return alternative_key;
            conflicts_counter++;
            alternative_key = conflicts_handler(hash_key, conflicts_counter);
            hash_elected_node = father->children[alternative_key];
        }//fine del while
        return HASH_ERROR_CODE;
    }//fine dell'else
}//fine della funzione

void create(NodePtr root, char *path, char type){
    NodePtr father, hash_elected_node, new_node;
    
    if (root == global_root) father = solvePath(root, path);
    else father = root;
    
    unsigned int last_slash = last_symbol(path, '/'), hash_key;
    char resource_name[256];
    strcpy(resource_name, &path[last_slash+1]);
    
    
    //nel caso in cui il percorso desiderato non esiste o non sia valido
    if( father == NULL ){
        printf("%s\n", "no");
        return;
    }
    
    hash_key = myhash(resource_name, strlen(resource_name)); //calcolo la funzione di hash sul nome del percorso
    hash_elected_node = father->children[hash_key]; //mi posiziono sull'elemento scelto
    unsigned int alternative_key = hash_key, conflicts_counter = 0;
    
    
    //nel caso in cui il nodo in cui sto creando la risorsa abbia raggiunto il numero massimo di risorse previsto dal FS
    if( father->children_counter == MAXWIDTH ){
        printf("%s\n", "no");
        return;
    }
    
    //inizializzo nuovo nodo da aggiungere nella struttura
    //nel caso che sia una dirctory. Alloco tutto lo spazio insieme per ridurre le chiamate a calloc che impattano in modo significativo il tempo di esecuzione.
    if (type == 'd'){
        NodePtr *hash;
        new_node = (Node *)calloc(1, sizeof(Node) + (strlen(resource_name)+1)*sizeof(char) + HASH_TABLE_SIZE*sizeof(NodePtr));
        new_node->name = (char*)(new_node +1);
        hash = (NodePtr *)(new_node->name + (strlen(resource_name)+1));
        new_node->children = hash;
    }
    
    else{
        new_node = calloc(1, sizeof(Node) + (strlen(resource_name)+1)*sizeof(char));
        //new_node = calloc(1, sizeof(Node));
        if (new_node == NULL){
            printf("%s\n", "no");
            return;
        }
        new_node->name = (char*)(new_node +1);
        //new_node->name = calloc(strlen(resource_name)+1, sizeof(char));
    }
    
    new_node->type = type;
    new_node->children_counter = 0;
    strcpy(new_node->name, resource_name);
    
    //if (type == 'd') new_node->children = calloc(HASH_TABLE_SIZE, sizeof(NodePtr));
    
    //nel caso in cui la funzione di hash punti ad una casella vuota o ad una tombstone, la riempio con il nuovo nodo
    if (hash_elected_node == NULL || hash_elected_node == global_tombstone){
        father->children[hash_key] = new_node;
        printf("%s\n", "ok");
    }
    
    //altrimenti devo gestire il conflitto e trovare una location alternativa adatta
    else{
        //se provo a creare una risorsa con lo stesso nome di una già esistente, mi fermo.
        if( strcmp(hash_elected_node->name, resource_name) == 0 ){
            printf("%s\n", "no");
            //free_node(new_node);
            return;
        }
        //altrimenti passo alla gestione del conflitto
        while(hash_elected_node != NULL){
            conflicts_counter++;
            alternative_key = conflicts_handler(hash_key, conflicts_counter);
            hash_elected_node = father->children[alternative_key];
        }
        father->children[alternative_key] = new_node;
        printf("%s\n", "ok");
    }
    father->children_counter++; //aumento il contatore dei figli del padre
    if (type == 'd') global_shortcut = new_node;
}

void delete(NodePtr root, char *path){
    NodePtr father = solvePath(root, path);
    unsigned int hash_key = hashing_handler(father, path);
    
    //verifico che la funzione chiamata non ritorni un codice di errore
    if (hash_key == HASH_ERROR_CODE){
        printf("%s\n", "no");
        return;
    }
    
    NodePtr hash_elected_node = father->children[hash_key];
    
    //nel caso la risorsa sia una cartella e abbia dei fisgli
    if( hash_elected_node ->children_counter != 0 ){
        printf("%s\n", "no");
        return;
    }//fine dell'if
    
    delete_node(father, hash_key);
    printf("%s\n", "ok");
}//fine di delete


void write_file(NodePtr root, char *path, char *content){
    NodePtr father = solvePath(root, path);
    unsigned int hash_key = hashing_handler(father, path);
    
    //verifico che la funzione chiamata non ritorni un codice di errore
    if (hash_key == HASH_ERROR_CODE){
        printf("%s\n", "no");
        return;
    }
    
    NodePtr hash_elected_node = father->children[hash_key];
    
    //nel caso la risorsa sia una cartella
    if( hash_elected_node ->type != 'f' ){
        printf("%s\n", "no");
        return;
    }
    
    //nel caso non ci sia contenuto da scrivere
    if( *content == '\0') {
        if (hash_elected_node->data != NULL) free(hash_elected_node->data);
        hash_elected_node->data = NULL;
    } //fine dell'if
    
    //nel caso in cui non era mai stato precedentemente scritto su data
    else if( hash_elected_node->data == NULL ){
        hash_elected_node->data = calloc(strlen(content)+1, sizeof(char));
        if (hash_elected_node->data == NULL){
            printf("%s\n", "no");
            return;
            //exit(69);
        }
        strcpy(hash_elected_node->data, content);
    } //fine dell'else_if
    
    //negli altri casi (ossi quando era già stato scritto in passato)
    else{
        hash_elected_node->data = realloc(hash_elected_node->data, strlen(content)+1);
        if (hash_elected_node->data == NULL){
            printf("%s\n", "no");
            return;
        }
        strcpy(hash_elected_node->data, content);
    } //fine dell'else
    unsigned long len = strlen(content);
    printf("%s %lu\n", "ok", len);
}//fine della funzione

void read_file(NodePtr root, char *path){
    NodePtr father = solvePath(root, path);
    unsigned int hash_key = hashing_handler(father, path);
    
    //verifico che la funzione chiamata non ritorni un codice di errore
    if (hash_key == HASH_ERROR_CODE){
        printf("%s\n", "no");
        return;
    } //fine dell'if
    
    NodePtr hash_elected_node = father->children[hash_key];
    
    //nel caso la risorsa sia una cartella
    if( hash_elected_node ->type != 'f' ){
        printf("%s\n", "no");
        return;
    }//finel dell'if
    
    //stampo il contenuto avendo cura di lasciare lo spazio bianco nel caso in cui
    //il campo data sia vuoto
    if( hash_elected_node->data == NULL) printf("contenuto \n");
    else printf("contenuto %s\n", hash_elected_node->data);
    
}//fine della funzione

void target_recursive(NodePtr root ,char *path){
    NodePtr father = solvePath(root, path);
    unsigned int hash_key = hashing_handler(father, path);
    
    //verifico che la funzione chiamata non ritorni un codice di errore
    if (hash_key == HASH_ERROR_CODE){
        printf("%s\n", "no");
        return;
    } //fine dell'if
    
    NodePtr hash_elected_node = father->children[hash_key];
    
    //nel caso in cui il nodo da cancellare ricorsivamente sia semplicemente un file o una cartella senza figli, chiamo la delete standard
    if (hash_elected_node->children_counter == 0) delete(root, path);
    else{
        delete_recursive(father, hash_key);
    }
    printf("%s\n", "ok");
}

//versione della delete recursive senza struttura ausiliaria
void delete_recursive(NodePtr father, unsigned int hash_key){
    NodePtr node = father->children[hash_key], hash_elected_node = NULL;
    unsigned int counter = 0;
    
    for (unsigned int i = 0; i < HASH_TABLE_SIZE; i++){
        if (counter == node->children_counter) break; //inutile scorrere fino in fondo se ho già eliminato tutti i figli
        hash_elected_node = node->children[i];
        if (hash_elected_node == NULL) continue; //se è vuoto, salto a quello successivo
        if (hash_elected_node->type == 't' ) continue;  //se è una tombstone, salto a quello successivo
        if (hash_elected_node->children_counter == 0){ //se non ha figli, lo posso eliminare direttamente
            delete_node(node, i);
            counter++;
        }
        else{ //altrimenti riapplico la rimozione ricorsiva
            delete_recursive(node, i);
            counter++;
        }
    }//fine del for
    delete_node(father, hash_key);
}//fine della funzione

void launch_find(NodePtr root, char *resource_name) {
    char current_path[256*MAXDEPTH];
    
    ResultPtr results = calloc(1, sizeof(Result));
    find(root, results, resource_name, current_path);
    
    if (results->path == NULL){
        printf("%s\n", "no");
        return;
    }
    inorder_tree_walk(results);
}

void find(NodePtr node, ResultPtr results, char *resource_name, char *path){
    NodePtr hash_elected_node;
    char *tmp;
    unsigned long path_len = strlen(path);
    unsigned long resource_len = strlen(resource_name);
    unsigned long hash_elected_node_name_len = 0;
    unsigned int counter = 0;
    
    //scorro tutte le caselle della hast_table
    for(unsigned int i = 0; i < HASH_TABLE_SIZE; i++){
        hash_elected_node = node->children[i];
        if (counter >= node->children_counter) break; //controllo che non abbia già analizzato un numero di risorse pari a quelle totali del nodo
        else if (hash_elected_node == NULL) continue; //nel caso sia vuota, salto alla successiva
        else if (hash_elected_node->type == 't') continue; //nel caso contenga una tombstone, è come se fosse vuota
        if (strcmp(hash_elected_node->name, resource_name) == 0){ //se il nome di una risorsa coincide con quello cercato, allora devo salvarlo nell'albero binario
            
            //nel caso in cui stia inserendo il primo elemento non ho bisogno di allocare il nuovo nodo, ma solo
            //di allocare lo spazio per il percorso e linkarlo al primo nodo
            if (results->path == NULL){
                results->path = calloc( path_len + resource_len + 1, sizeof(char));
                //if (results->path == NULL) exit(71);
                strcpy(results->path, path);
                results->path[path_len] = '/';
                strcpy(&results->path[path_len+1], resource_name);
                results->path[strlen(results->path)] = '\0';
            }
            
            //altrimenti alloco il nuovo nodo per intero
            else{
                ResultPtr new_result = calloc(1, sizeof(Result)); //alloco spazio per il nuovo nodo
                //if (new_result == NULL) exit(72);
                new_result->path = calloc( path_len + resource_len + 1, sizeof(char));
                //if (new_result->path == NULL) exit(73);
                strcpy(new_result->path, path);
                new_result->path[path_len] = '/';
                strcpy(&new_result->path[path_len+1], resource_name);
                new_result->path[strlen(new_result->path)] = '\0';
                tree_insert(results, new_result);
            }
            counter++;
        } //fine dell'if
        
        //a prescendire dal fatto che mi sia salvato oppure no il percorso della risorsa, se incontro una nodo che è una cartella devo esplorare anche essa
        if (hash_elected_node->children_counter != 0){
            hash_elected_node_name_len = strlen(hash_elected_node->name);
            tmp = calloc(path_len + hash_elected_node_name_len + 2, sizeof(char)); //creo una variabile temporanea dove salvare il percorso della risorsa
            //if (tmp == NULL) exit(66);
            
            strcpy(tmp,path);
            tmp[path_len] = '/';
            strcpy(&tmp[path_len+1], hash_elected_node->name);
            tmp[path_len + hash_elected_node_name_len + 2] = '\0';
            find(hash_elected_node, results, resource_name, tmp);
            free(tmp);
            counter++;
        }//fine dell'if
    }//fine del for
}//fine della funzione


////
// MARK: - MAIN
////
int main(void){
    //Imposto il main in modo che l'input venga analizzato carattere per carattere.
    //Inizializzo un buffer di dimensione variabile che incremento ogni volta che eccede passando alla successiva potenza di due.
    unsigned int buffer_lenght = MINIMUM_BUFFER_SIZE; //la lunghezza iniziale del buffer è di 2
    unsigned int actual_buffer_size = 0;
    char c = getc(stdin);
    char *buffer = calloc(buffer_lenght, sizeof(char));
    buffer[actual_buffer_size] = c;
    
    unsigned int first_space_index, second_space_index;
    char *instruction, *second_parameter, *third_parameter;
    
    //inizializzo global_tombstone
    global_tombstone = calloc(1,sizeof(Node));
    global_tombstone->type = 't';
    global_tombstone->name = calloc(10, sizeof(char));
    strcpy(global_tombstone->name, "tombstone");
    global_tombstone->name[9] = '\0';
    
    //inizializzo root
    NodePtr root = calloc(1, sizeof(Node));
    root->type = 'd';
    root->name = calloc(5, sizeof(char));
    root->children = calloc(HASH_TABLE_SIZE, sizeof(NodePtr));
    strcpy(root->name, "root");
    root->data = NULL;
    
    global_last_path = calloc(256*10, sizeof(char));
    global_root = root;
    
    unsigned int last_slash = 0;
    
    while(1){
        
        while( c != '\n'){ //ciclo che viene ripetuto per ogni riga del file di input
            c = getc(stdin); //prendo il carattere successivo
            if( actual_buffer_size < buffer_lenght - 1) //controllo di non ecceddere il buffer
                actual_buffer_size++;
            else{ //in caso contrario lo espando riallocando lo spazio necessario
                buffer_lenght*=2;
                buffer = realloc(buffer, buffer_lenght);
                //printf("reallocating: %i\n", buffer_lenght); //DA TOGLIERE CAZZO
                actual_buffer_size++;
            }
            buffer[actual_buffer_size] = c; //salvo nel buffer l'effettivo carattere letto
        } //fine del while annidato
        
        if( strcmp(buffer, "exit\n") == 0){ //condizione di uscita dal ciclo e dal programma
            free(buffer);
            exit(0);
        } //chiusura della condizione di uscita
        
        buffer[actual_buffer_size] = ' ';
        
        /////
        //instruction handler
        /////
        first_space_index = find_symbol(buffer, ' '); //trovo l'indice del primo spazio
        instruction = calloc( first_space_index + 1, sizeof(char)); //alloco memoria sufficiente per il nome dell'istruzione
        strncpy(instruction, buffer, first_space_index); // copio la porzione di stringa nello spazio allocato
        instruction[strlen(instruction)] = '\0';
        
        unsigned int i = 1;
        second_space_index = find_symbol(&buffer[first_space_index+i], ' '); //trovo il secondo spazio
        
        if (second_space_index == 0){
            while ( *(&buffer[first_space_index+i]) == ' ') i++; //gestione del caso in cui l'input sia formato male
            second_space_index = find_symbol(&buffer[first_space_index+i], ' '); //ignoro più spazi consecutivi tra i parametri
        }
        
        second_parameter = calloc(second_space_index+i, sizeof(char));//alloco la memoria sufficiente per il /
        strncpy(second_parameter, &buffer[first_space_index+i], second_space_index );
        second_parameter[strlen(second_parameter)] = '\0'; //aggiungo il terminatore
        
        
        if( strcmp(instruction, "create") == 0){
            global_shortcut = NULL;
            create(root, second_parameter,'f');
        }
        
        else if (strcmp(instruction, "create_dir") == 0){
            //ottimizzo per gestire esplosioni in verticale del file system
            last_slash = last_symbol(second_parameter, '/');
            if ( (global_shortcut != NULL)  && (strncmp(global_last_path, second_parameter, max(last_slash-1, strlen(global_last_path))) == 0)){
                create(global_shortcut, &second_parameter[last_slash], 'd');
            }
            else create(root, second_parameter, 'd');
            strcpy(global_last_path, second_parameter);
        }
        
        else if (strcmp(instruction, "delete") == 0){
            global_shortcut = NULL;
            delete(root, second_parameter);
        }
        
        else if (strcmp(instruction, "delete_r") == 0){
            global_shortcut = NULL;
            target_recursive(root, second_parameter);
        }
        
        else if (strcmp(instruction, "find") == 0){
            launch_find(root, second_parameter);
            global_shortcut = NULL;
        }
        
        else if (strcmp(instruction, "read") == 0){
            global_shortcut = NULL;
            read_file(root,second_parameter);
        }
        
        else if( strcmp(instruction, "write") == 0){
            global_shortcut = NULL;
            third_parameter = calloc(strlen(&buffer[first_space_index+second_space_index+i+1]+1), sizeof(char)) ;
            third_parameter = strcpy(third_parameter, &buffer[first_space_index+second_space_index+i+2]);
            fix_symbol(third_parameter, '\"', '\0');
            write_file(root, second_parameter, third_parameter);
            free(third_parameter);
        }
        
        free(instruction);// libero memoria
        free(second_parameter);//libero memoria
        free(buffer); //libero memoria
        buffer_lenght = MINIMUM_BUFFER_SIZE; //ripristino parametri
        actual_buffer_size = 0; //ripristino parametri
        buffer = calloc(buffer_lenght, sizeof(char));
        
        //imposto lettura del prossimo carattere
        c = getc(stdin);
        buffer[actual_buffer_size] = c;
    }//fine del while principale
}//fine del main
