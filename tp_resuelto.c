// tp2 - tecnologia digital ii - tetris con memoria dinamica
// integrantes: Micaela Majlis, Esther Meta Phinhassoff, Felicitas Robin, Delfina Mengochea
//
// uso:  ./tetris           corre los tests y despues el juego
//       ./tetris --test    corre solo los tests (valgrind --leak-check=full ./tetris --test)

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// constantes del juego
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 800
#define GRID_ROWS 20
#define GRID_COLS 10
#define CELL_SIZE 35
#define OFFSET_X 75
#define OFFSET_Y 50
#define TICK_RATE 30

const int TETROMINOS[7][4][4][4] = {
    { {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}, {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}}, {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}} },
    { {{2,0,0,0},{2,2,2,0},{0,0,0,0},{0,0,0,0}}, {{0,2,2,0},{0,2,0,0},{0,2,0,0},{0,0,0,0}}, {{0,0,0,0},{2,2,2,0},{0,0,2,0},{0,0,0,0}}, {{0,2,0,0},{0,2,0,0},{2,2,0,0},{0,0,0,0}} },
    { {{0,0,3,0},{3,3,3,0},{0,0,0,0},{0,0,0,0}}, {{0,3,0,0},{0,3,0,0},{0,3,3,0},{0,0,0,0}}, {{0,0,0,0},{3,3,3,0},{3,0,0,0},{0,0,0,0}}, {{3,3,0,0},{0,3,0,0},{0,3,0,0},{0,0,0,0}} },
    { {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}} },
    { {{0,5,5,0},{5,5,0,0},{0,0,0,0},{0,0,0,0}}, {{0,5,0,0},{0,5,5,0},{0,0,5,0},{0,0,0,0}}, {{0,0,0,0},{0,5,5,0},{5,5,0,0},{0,0,0,0}}, {{5,0,0,0},{5,5,0,0},{0,5,0,0},{0,0,0,0}} },
    { {{0,6,0,0},{6,6,6,0},{0,0,0,0},{0,0,0,0}}, {{0,6,0,0},{0,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,0,0,0},{6,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,6,0,0},{6,6,0,0},{0,6,0,0},{0,0,0,0}} },
    { {{7,7,0,0},{0,7,7,0},{0,0,0,0},{0,0,0,0}}, {{0,0,7,0},{0,7,7,0},{0,7,0,0},{0,0,0,0}}, {{0,0,0,0},{7,7,0,0},{0,7,7,0},{0,0,0,0}}, {{0,7,0,0},{7,7,0,0},{7,0,0,0},{0,0,0,0}} }
};

SDL_Color colors[8] = {
    {30, 30, 30, 255}, {0, 255, 255, 255}, {0, 0, 255, 255}, {255, 165, 0, 255},
    {255, 255, 0, 255}, {0, 255, 0, 255}, {128, 0, 128, 255}, {255, 0, 0, 255}
};

// estructuras de datos

// 1. nodo para la fila del tablero (lista enlazada)
typedef struct RowNode {
    int cells[GRID_COLS]; 
    struct RowNode* next; 
} RowNode;

// 2. nodo para la cola de próximas piezas (queue)
typedef struct PieceQueueNode {
    int type; 
    struct PieceQueueNode* next;
} PieceQueueNode;

// 3. nodo para el sistema de partículas
typedef struct ParticleNode {
    float x, y;
    float vel_x, vel_y;
    int color;
    int life_timer;
    struct ParticleNode* next;
} ParticleNode;

typedef struct ActivePiece {
    int col, row;
    int type;      
    int rotation;  
    int active;
} ActivePiece;

typedef struct GameBoard {
    RowNode* first_row;             
    PieceQueueNode* queue_head;     
    PieceQueueNode* queue_tail;     
    ParticleNode* first_particle;   
    ActivePiece current_piece;
    int score;
    int tick_timer;
} GameBoard;

// variables globales
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
GameBoard* game_board = NULL;

// ejercicio 1: strings (no usar string.h)

// cuenta los caracteres hasta el '\0' (sin contarlo). la usamos en las tres funciones.
static int largo(char* s) {
    int n = 0;
    while (s[n] != '\0') n++;
    return n;
}

char* strDuplicate(char* src) {
    int n = largo(src);
    // n + 1 para que entre el '\0' final
    char* copia = (char*)malloc(sizeof(char) * (n + 1));
    // copiamos hasta n inclusive, asi el terminador viaja con el resto
    for (int i = 0; i <= n; i++) copia[i] = src[i];
    return copia;
}

int strCompare(char* s1, char* s2) {
    int i = 0;
    // avanzamos mientras sean iguales y no se termine ninguno (si uno termina, el otro tambien o difiere)
    while (s1[i] != '\0' && s1[i] == s2[i]) i++;
    // casteamos a unsigned char como hace strcmp, si no los caracteres con tilde (negativos) quedarian "menores"
    unsigned char a = (unsigned char)s1[i];
    unsigned char b = (unsigned char)s2[i];
    // convencion inversa a strcmp: 1 si s1 < s2, -1 si s1 > s2
    // un prefijo cae en el caso a == '\0' < b, asi que "casa" < "casas" da 1
    if (a == b) return 0;
    return (a < b) ? 1 : -1;
}

char* strConcatenate(char* src1, char* src2) {
    int n1 = largo(src1);
    int n2 = largo(src2);
    char* res = (char*)malloc(sizeof(char) * (n1 + n2 + 1));
    for (int i = 0; i < n1; i++) res[i] = src1[i];
    for (int i = 0; i <= n2; i++) res[n1 + i] = src2[i];   // <= para copiar el '\0'
    free(src1);
    free(src2);
    return res;
}

// ejercicio 2: funciones a implementar (el tp)

// reserva una fila con todas las celdas en 0. devuelve NULL si malloc falla.
static RowNode* crearFilaVacia() {
    RowNode* fila = (RowNode*)malloc(sizeof(RowNode));
    if (fila == NULL) return NULL;
    for (int c = 0; c < GRID_COLS; c++) fila->cells[c] = 0;
    fila->next = NULL;
    return fila;
}

// gameBoardNew la usa para limpiar si un malloc falla, y se define mas abajo
void gameBoardDelete(GameBoard* board);

GameBoard* gameBoardNew() {
    GameBoard* board = (GameBoard*)malloc(sizeof(GameBoard));
    if (board == NULL) return NULL;

    board->first_row = NULL;
    board->queue_head = NULL;
    board->queue_tail = NULL;
    board->first_particle = NULL;
    board->current_piece.col = 0;
    board->current_piece.row = 0;
    board->current_piece.type = 0;
    board->current_piece.rotation = 0;
    board->current_piece.active = 0;
    board->score = 0;
    board->tick_timer = 0;

    // armamos la lista insertando siempre en la cabeza: asi no hace falta
    // guardar un puntero a la cola y la lista parcial queda siempre bien formada,
    // entonces si un malloc falla en el medio alcanza con borrar lo que ya hay.
    for (int i = 0; i < GRID_ROWS; i++) {
        RowNode* fila = crearFilaVacia();
        if (fila == NULL) {
            gameBoardDelete(board);
            return NULL;
        }
        fila->next = board->first_row;
        board->first_row = fila;
    }
    return board;
}

void gameBoardDelete(GameBoard* board) {
    if (board == NULL) return;

    // en los tres recorridos nos guardamos el next antes del free, despues no se puede leer
    RowNode* fila = board->first_row;
    while (fila != NULL) {
        RowNode* sig = fila->next;
        free(fila);
        fila = sig;
    }

    PieceQueueNode* pieza = board->queue_head;
    while (pieza != NULL) {
        PieceQueueNode* sig = pieza->next;
        free(pieza);
        pieza = sig;
    }

    ParticleNode* part = board->first_particle;
    while (part != NULL) {
        ParticleNode* sig = part->next;
        free(part);
        part = sig;
    }

    free(board);
}

void gameBoardAddParticle(GameBoard* board, float x, float y, int color) {
    ParticleNode* nueva = (ParticleNode*)malloc(sizeof(ParticleNode));

    nueva->x = x;
    nueva->y = y;
    // mismos rangos que el prototipo: vel_x en [-5, 5), vel_y en [-7, -2)
    nueva->vel_x = ((rand() % 100) / 10.0f) - 5.0f;
    nueva->vel_y = ((rand() % 50) / -10.0f) - 2.0f;
    nueva->color = color;
    nueva->life_timer = 30 + (rand() % 30);

    // insercion en la cabeza: o(1), y el orden de las particulas no importa
    nueva->next = board->first_particle;
    board->first_particle = nueva;
}

int gameBoardCheckAndClearLines(GameBoard* board) {
    int eliminadas = 0;
    int indice = 0;   // posicion original de la fila que estamos mirando
    RowNode* prev = NULL;
    RowNode* curr = board->first_row;

    while (curr != NULL) {
        int llena = 1;
        for (int c = 0; c < GRID_COLS && llena; c++) {
            if (curr->cells[c] == 0) llena = 0;
        }

        if (!llena) {
            prev = curr;
            curr = curr->next;
            indice++;
            continue;
        }

        // pedimos la fila de reemplazo antes de tocar nada: si falla, dejamos el tablero
        // como esta (20 filas) en vez de quedarnos con 19.
        RowNode* vacia = crearFilaVacia();
        if (vacia == NULL) break;

        // una particula por celda. la y usa el indice original: al sacar una fila y
        // meter otra arriba, las filas de abajo no cambian de posicion, asi que sigue valiendo.
        for (int c = 0; c < GRID_COLS; c++) {
            gameBoardAddParticle(board,
                                 OFFSET_X + c * CELL_SIZE + CELL_SIZE / 2,
                                 OFFSET_Y + indice * CELL_SIZE + CELL_SIZE / 2,
                                 curr->cells[c]);
        }

        // desenlazar: si es la primera, se mueve first_row; si no, el anterior salta a curr->next.
        RowNode* a_borrar = curr;
        curr = curr->next;
        if (prev == NULL) board->first_row = curr;
        else prev->next = curr;
        free(a_borrar);

        // la fila nueva va a la cabeza. ya actualizamos first_row arriba, asi que
        // tambien anda cuando la que se borro era la primera.
        vacia->next = board->first_row;
        board->first_row = vacia;

        // si borramos la primera, prev seguia en NULL pero ahora la cabeza es la fila nueva:
        // la dejamos como prev, si no el proximo desenlace pisaria first_row y perderiamos la fila nueva.
        // si prev no era NULL no hay que tocarlo, sigue siendo el nodo anterior a curr.
        if (prev == NULL) prev = vacia;

        eliminadas++;
        indice++;
    }
    return eliminadas;
}

void gameBoardEnqueuePiece(GameBoard* board, int type) {
    PieceQueueNode* nodo = (PieceQueueNode*)malloc(sizeof(PieceQueueNode));
    nodo->type = type;
    nodo->next = NULL;

    // cola vacia: el nodo es a la vez head y tail. si no, se cuelga detras del tail.
    if (board->queue_tail == NULL) {
        board->queue_head = nodo;
    } else {
        board->queue_tail->next = nodo;
    }
    board->queue_tail = nodo;
}

int gameBoardDequeuePiece(GameBoard* board, int* out_type) {
    if (board->queue_head == NULL) return 0;

    PieceQueueNode* nodo = board->queue_head;
    *out_type = nodo->type;
    board->queue_head = nodo->next;
    // si era el ultimo, tail quedaria apuntando a memoria liberada: lo bajamos a NULL
    if (board->queue_head == NULL) board->queue_tail = NULL;
    free(nodo);
    return 1;
}

// logica de dibujado y colisiones (no modificar)

RowNode* getRowAt(GameBoard* board, int index) {
    RowNode* curr = board->first_row;
    for(int i = 0; i < index && curr != NULL; i++) curr = curr->next;
    return curr;
}

int canMove(GameBoard* board, int target_col, int target_row, int target_rot) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (TETROMINOS[board->current_piece.type][target_rot][r][c] != 0) {
                int board_col = target_col + c;
                int board_row = target_row + r;
                if (board_col < 0 || board_col >= GRID_COLS || board_row >= GRID_ROWS) return 0;
                if (board_row >= 0) {
                    RowNode* row_node = getRowAt(board, board_row);
                    if (row_node && row_node->cells[board_col] != 0) return 0;
                }
            }
        }
    }
    return 1;
}

void lockPiece(GameBoard* board) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][r][c];
            if (val != 0) {
                RowNode* row_node = getRowAt(board, board->current_piece.row + r);
                if (row_node) row_node->cells[board->current_piece.col + c] = val;
            }
        }
    }
    board->current_piece.active = 0;
    board->score += gameBoardCheckAndClearLines(board) * 100;
}

// devuelve 0 si la pieza nueva no entra (game over), 1 en otro caso.
int spawnNextPiece(GameBoard* board) {
    int type;
    if (gameBoardDequeuePiece(board, &type)) {
        board->current_piece.type = type;
        board->current_piece.rotation = 0;
        board->current_piece.col = GRID_COLS / 2 - 2;
        board->current_piece.row = 0;
        board->current_piece.active = 1;
        gameBoardEnqueuePiece(board, rand() % 7);
        if (!canMove(board, board->current_piece.col, board->current_piece.row, board->current_piece.rotation)) {
            printf("GAME OVER. Puntaje final: %d\n", board->score);
            board->current_piece.active = 0;
            return 0;   // main sale del loop y libera tablero y SDL
        }
    }
    return 1;
}

// devuelve 0 cuando termina la partida.
int gameBoardUpdate(GameBoard* board) {
    if (!board) return 0;
    if (!board->current_piece.active && !spawnNextPiece(board)) return 0;

    board->tick_timer++;
    if (board->tick_timer >= TICK_RATE) {
        board->tick_timer = 0;
        if (canMove(board, board->current_piece.col, board->current_piece.row + 1, board->current_piece.rotation)) {
            board->current_piece.row++;
        } else {
            lockPiece(board);
        }
    }

    ParticleNode* curr_part = board->first_particle;
    ParticleNode* prev_part = NULL;
    while (curr_part) {
        curr_part->x += curr_part->vel_x;
        curr_part->y += curr_part->vel_y;
        curr_part->vel_y += 0.5f;
        curr_part->life_timer--;

        if (curr_part->life_timer <= 0) {
            ParticleNode* to_delete = curr_part;
            if (prev_part == NULL) {
                board->first_particle = curr_part->next;
                curr_part = board->first_particle;
            } else {
                prev_part->next = curr_part->next;
                curr_part = prev_part->next;
            }
            free(to_delete);
        } else {
            prev_part = curr_part;
            curr_part = curr_part->next;
        }
    }
    return 1;
}

void gameBoardDraw(GameBoard* board) {
    if (!board) return;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect board_rect = {OFFSET_X - 2, OFFSET_Y - 2, GRID_COLS * CELL_SIZE + 4, GRID_ROWS * CELL_SIZE + 4};
    SDL_RenderDrawRect(renderer, &board_rect);

    RowNode* current_row = board->first_row;
    int r = 0;
    while (current_row) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (current_row->cells[c] > 0) {
                SDL_Color col = colors[current_row->cells[c]];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_Rect cell_rect = {OFFSET_X + c * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
                SDL_RenderFillRect(renderer, &cell_rect);
            }
        }
        current_row = current_row->next;
        r++;
    }

    if (board->current_piece.active) {
        for(int pr = 0; pr < 4; pr++) {
            for(int pc = 0; pc < 4; pc++) {
                int cell_val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][pr][pc];
                if (cell_val != 0) {
                    SDL_Color col = colors[cell_val];
                    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                    SDL_Rect p_rect = {OFFSET_X + (board->current_piece.col + pc) * CELL_SIZE, 
                                       OFFSET_Y + (board->current_piece.row + pr) * CELL_SIZE, 
                                       CELL_SIZE - 1, CELL_SIZE - 1};
                    SDL_RenderFillRect(renderer, &p_rect);
                }
            }
        }
    }

    ParticleNode* curr_part = board->first_particle;
    while (curr_part) {
        SDL_Color col = colors[curr_part->color];
        int alpha = curr_part->life_timer * 8;   // se satura: SDL recibe un Uint8
        if (alpha > 255) alpha = 255;
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, alpha);
        SDL_Rect part_rect = {(int)curr_part->x, (int)curr_part->y, 6, 6};
        SDL_RenderFillRect(renderer, &part_rect);
        curr_part = curr_part->next;
    }
    SDL_RenderPresent(renderer);
}

int inicializar() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    window = SDL_CreateWindow("Tetris Base - TP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) return 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) return 0;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return 1;
}

// tests

static int tests_ok = 0;
static int tests_fallidos = 0;

static void chequear(int condicion, char* descripcion) {
    if (condicion) {
        tests_ok++;
    } else {
        tests_fallidos++;
        printf("  FALLO: %s\n", descripcion);
    }
}

// cuenta las filas de la lista (tiene que dar siempre 20)
static int contarFilas(GameBoard* board) {
    int n = 0;
    for (RowNode* f = board->first_row; f != NULL; f = f->next) n++;
    return n;
}

static int contarParticulas(GameBoard* board) {
    int n = 0;
    for (ParticleNode* p = board->first_particle; p != NULL; p = p->next) n++;
    return n;
}

static void llenarFila(GameBoard* board, int indice) {
    RowNode* f = getRowAt(board, indice);
    for (int c = 0; c < GRID_COLS; c++) f->cells[c] = 1;
}

static void testStrDuplicate() {
    char* vacio = strDuplicate("");
    chequear(vacio != NULL && vacio[0] == '\0', "strDuplicate: string vacio");
    free(vacio);

    char* uno = strDuplicate("x");
    chequear(uno != NULL && uno[0] == 'x' && uno[1] == '\0', "strDuplicate: un caracter");
    free(uno);

    char* varios = strDuplicate("Hola, TD2!");
    int igual = varios != NULL;
    char* esperado = "Hola, TD2!";
    for (int i = 0; igual && i <= 10; i++) if (varios[i] != esperado[i]) igual = 0;
    chequear(igual, "strDuplicate: caracteres distintos");
    // tiene que ser una copia, no el mismo puntero
    chequear(varios != esperado, "strDuplicate: devuelve memoria nueva");
    free(varios);
}

static void testStrCompare() {
    chequear(strCompare("", "") == 0, "strCompare: dos vacios");
    chequear(strCompare("a", "a") == 0, "strCompare: 'a' vs 'a'");
    chequear(strCompare("a", "b") == 1, "strCompare: 'a' vs 'b' da 1");
    chequear(strCompare("b", "a") == -1, "strCompare: 'b' vs 'a' da -1");
    chequear(strCompare("casa", "casas") == 1, "strCompare: prefijo es menor");
    chequear(strCompare("casas", "casa") == -1, "strCompare: el mas largo es mayor");
    chequear(strCompare("", "a") == 1, "strCompare: vacio vs 'a'");
    chequear(strCompare("hola", "hola") == 0, "strCompare: iguales largos");
}

static void testStrConcatenate() {
    // los dos argumentos salen de strDuplicate porque strConcatenate los libera
    char* r1 = strConcatenate(strDuplicate(""), strDuplicate("abc"));
    chequear(r1 != NULL && strCompare(r1, "abc") == 0, "strConcatenate: vacio + 'abc'");
    free(r1);

    char* r2 = strConcatenate(strDuplicate("hola "), strDuplicate("mundo"));
    chequear(r2 != NULL && strCompare(r2, "hola mundo") == 0, "strConcatenate: 'hola ' + 'mundo'");
    free(r2);

    char* r3 = strConcatenate(strDuplicate("abc"), strDuplicate(""));
    chequear(r3 != NULL && strCompare(r3, "abc") == 0, "strConcatenate: 'abc' + vacio");
    free(r3);
}

static void testCola() {
    GameBoard* b = gameBoardNew();
    chequear(b != NULL, "cola: gameBoardNew no devolvio NULL");
    if (b == NULL) return;

    int out = -1;
    chequear(gameBoardDequeuePiece(b, &out) == 0 && out == -1, "cola: pop en cola vacia devuelve 0 y no toca out");

    for (int i = 0; i < 5; i++) gameBoardEnqueuePiece(b, i);
    int orden_ok = 1;
    for (int i = 0; i < 5; i++) {
        out = -1;
        if (!gameBoardDequeuePiece(b, &out) || out != i) orden_ok = 0;
    }
    chequear(orden_ok, "cola: 5 pops salen en orden FIFO");

    out = -1;
    chequear(gameBoardDequeuePiece(b, &out) == 0, "cola: el sexto pop devuelve 0");
    chequear(out == -1, "cola: el sexto pop no modifica out_type");
    chequear(b->queue_head == NULL && b->queue_tail == NULL, "cola: head y tail quedan en NULL");

    gameBoardEnqueuePiece(b, 6);
    chequear(b->queue_head != NULL && b->queue_head == b->queue_tail, "cola: push despues de vaciar, head == tail");
    chequear(gameBoardDequeuePiece(b, &out) == 1 && out == 6, "cola: se puede extraer el push posterior");
    chequear(b->queue_head == NULL && b->queue_tail == NULL, "cola: vuelve a quedar vacia");

    // dejamos piezas sin sacar para que gameBoardDelete tenga que liberarlas
    gameBoardEnqueuePiece(b, 1);
    gameBoardEnqueuePiece(b, 2);
    gameBoardDelete(b);
}

static void testTablero() {
    GameBoard* b = gameBoardNew();
    chequear(b != NULL, "tablero: gameBoardNew no devolvio NULL");
    if (b == NULL) return;

    chequear(contarFilas(b) == GRID_ROWS, "tablero: arranca con 20 filas");
    chequear(b->score == 0 && b->tick_timer == 0 && !b->current_piece.active, "tablero: campos iniciales en 0");
    chequear(b->queue_head == NULL && b->queue_tail == NULL && b->first_particle == NULL, "tablero: punteros iniciales en NULL");

    int vacias = 1;
    for (RowNode* f = b->first_row; f != NULL; f = f->next)
        for (int c = 0; c < GRID_COLS; c++) if (f->cells[c] != 0) vacias = 0;
    chequear(vacias, "tablero: todas las celdas arrancan en 0");

    chequear(gameBoardCheckAndClearLines(b) == 0, "tablero: sin filas llenas no elimina nada");

    // 1) una sola fila llena: la de abajo (indice 19)
    RowNode* vieja_cabeza = b->first_row;
    llenarFila(b, 19);
    chequear(gameBoardCheckAndClearLines(b) == 1, "tablero: fila 19 llena devuelve 1");
    chequear(b->first_row != vieja_cabeza, "tablero: first_row es la fila nueva");
    int cabeza_vacia = 1;
    for (int c = 0; c < GRID_COLS; c++) if (b->first_row->cells[c] != 0) cabeza_vacia = 0;
    chequear(cabeza_vacia, "tablero: la fila nueva esta vacia");
    chequear(contarFilas(b) == GRID_ROWS, "tablero: sigue con 20 filas (1 linea)");
    chequear(contarParticulas(b) == GRID_COLS, "tablero: 10 particulas por la fila eliminada");

    // 2) filas 0 y 1 llenas: la primera de la lista es una de las que se borran
    llenarFila(b, 0);
    llenarFila(b, 1);
    chequear(gameBoardCheckAndClearLines(b) == 2, "tablero: filas 0 y 1 llenas devuelve 2");
    chequear(contarFilas(b) == GRID_ROWS, "tablero: sigue con 20 filas (2 lineas)");
    chequear(contarParticulas(b) == 3 * GRID_COLS, "tablero: 30 particulas acumuladas");

    gameBoardDelete(b);
}

// casos extra: lo que hay arriba de una fila eliminada baja, y varias filas no contiguas.
static void testTableroExtra() {
    GameBoard* b = gameBoardNew();
    if (b == NULL) { chequear(0, "extra: gameBoardNew"); return; }

    // marca en la fila 17 y fila 18, y fila 19 llena: despues de limpiar las marcas bajan un lugar
    getRowAt(b, 17)->cells[3] = 5;
    getRowAt(b, 18)->cells[7] = 6;
    llenarFila(b, 19);
    chequear(gameBoardCheckAndClearLines(b) == 1, "extra: limpia la 19");
    chequear(getRowAt(b, 18)->cells[3] == 5, "extra: la marca de la fila 17 bajo a la 18");
    chequear(getRowAt(b, 19)->cells[7] == 6, "extra: la marca de la fila 18 bajo a la 19");
    chequear(contarFilas(b) == GRID_ROWS, "extra: 20 filas tras limpiar la 19");

    // filas no contiguas (5, 9 y 19) en la misma llamada, con una fila incompleta de por medio
    getRowAt(b, 12)->cells[0] = 2;
    llenarFila(b, 5);
    llenarFila(b, 9);
    llenarFila(b, 19);
    chequear(gameBoardCheckAndClearLines(b) == 3, "extra: tres filas no contiguas");
    chequear(contarFilas(b) == GRID_ROWS, "extra: 20 filas tras limpiar tres");
    // la marca estaba en la 12: las filas 5 y 9 (arriba) se reponen arriba y no la mueven,
    // la 19 (abajo) si la empuja un lugar => queda en la 13
    chequear(getRowAt(b, 13)->cells[0] == 2, "extra: la fila incompleta baja un lugar");

    // las 20 filas llenas a la vez
    for (int i = 0; i < GRID_ROWS; i++) llenarFila(b, i);
    chequear(gameBoardCheckAndClearLines(b) == GRID_ROWS, "extra: las 20 filas llenas");
    chequear(contarFilas(b) == GRID_ROWS, "extra: 20 filas tras limpiar todo");

    gameBoardDelete(b);
    gameBoardDelete(NULL);   // tiene que tolerar NULL
    chequear(1, "extra: gameBoardDelete(NULL) no explota");
}

void correrTests() {
    tests_ok = 0;
    tests_fallidos = 0;

    testStrDuplicate();
    testStrCompare();
    testStrConcatenate();
    testCola();
    testTablero();
    testTableroExtra();

    if (tests_fallidos == 0) printf("Tests: %d chequeos OK.\n", tests_ok);
    else printf("Tests: %d OK, %d FALLARON.\n", tests_ok, tests_fallidos);
}

// compara contra "--test" sin depender de string.h ni del ejercicio 1.
int esFlagTest(char* arg) {
    char* flag = "--test";
    int i = 0;
    while (arg[i] != '\0' && arg[i] == flag[i]) i++;
    return arg[i] == '\0' && flag[i] == '\0';
}

int main(int argc, char* args[]) {
    srand(time(NULL));

    // los tests corren antes de SDL. con --test termina aca, asi valgrind solo
    // reporta la memoria del tp (SDL tiene bloques propios que no se liberan).
    correrTests();
    if (argc > 1 && esFlagTest(args[1])) return 0;

    if (!inicializar()) return 1;

    game_board = gameBoardNew();
    if (game_board == NULL) {
        printf("ADVERTENCIA: Implementa gameBoardNew() para iniciar el juego.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    for(int i=0; i<3; i++) gameBoardEnqueuePiece(game_board, rand() % 7);

    SDL_Event e;
    int game_over = 0;

    while (!game_over) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) game_over = 1;
            if (e.type == SDL_KEYDOWN && game_board->current_piece.active) {
                if (e.key.keysym.sym == SDLK_LEFT && canMove(game_board, game_board->current_piece.col - 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col--;
                if (e.key.keysym.sym == SDLK_RIGHT && canMove(game_board, game_board->current_piece.col + 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col++;
                if (e.key.keysym.sym == SDLK_DOWN && canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                if (e.key.keysym.sym == SDLK_UP) {
                    int next_rot = (game_board->current_piece.rotation + 1) % 4;
                    if (canMove(game_board, game_board->current_piece.col, game_board->current_piece.row, next_rot)) game_board->current_piece.rotation = next_rot;
                }
                if (e.key.keysym.sym == SDLK_SPACE) {
                     while(canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                     lockPiece(game_board);
                }
            }
        }
        if (!gameBoardUpdate(game_board)) game_over = 1;
        gameBoardDraw(game_board);
        SDL_Delay(16);
    }
    gameBoardDelete(game_board);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
