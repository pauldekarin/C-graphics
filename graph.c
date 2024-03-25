#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <locale.h>

#define EPS 1e-10
#define VAR_SYMBOL 'x'
#define GRAPH_SYMBOL "█"
#define MAP_SYMBOL "░"
#define ORDINATE_SYMBOL "│"
#define ABSCISSA_SYMBOL "─"
#define CENTER_SYMBOL "┼"
#define WIDTH 40
#define HEIGHT 20
#define ORDINATE (HEIGHT/2)
#define ABSCISSA (WIDTH/2)
#define STEP .05

#define XMAX WIDTH/2
#define XMIN -WIDTH/2
#define YMAX HEIGHT/2
#define YMIN -HEIGHT/2

typedef struct s_point {
    double x;
    double y;
} point;

typedef struct s_operator {
    char symbol;
    double code;
    int priority;
} operator;

typedef struct s_cell {
    double value;
    int type;
} cell;

enum CELL { OPERATOR, VALUE, VARIABLE };
enum ACTIONS {
    SUM,
    MULTIPLY,
    DIVIDE,
    SUBSTRACTION,
    OPEN_BRACKET,
    CLOSE_BRACKET,
    UNARY_MINUS,
    SIN,
    COS,
    TAN,
    SQRT,
    LN,
    CTG
};

#define STACK(type)                                                                              \
    struct stack_##type {                                                                        \
        type item;                                                                               \
        struct stack_##type *foot;                                                               \
    };                                                                                           \
    struct stack_##type *stack_init_##type(type *item) {                                         \
        struct stack_##type *stack = (struct stack_##type *)malloc(sizeof(struct stack_##type)); \
        if (stack != NULL) {                                                                     \
            stack->item = *item;                                                                 \
            stack->foot = NULL;                                                                  \
        }                                                                                        \
        return stack;                                                                            \
    }                                                                                            \
    void stack_push_##type(struct stack_##type **stack, type *item) {                            \
        if (*stack == NULL) {                                                                    \
            *stack = stack_init_##type(item);                                                    \
        } else {                                                                                 \
            struct stack_##type *foot = *stack;                                                  \
            *stack = stack_init_##type(item);                                                    \
            (*stack)->foot = foot;                                                               \
        }                                                                                        \
    }                                                                                            \
    void stack_pop_##type(struct stack_##type **stack) {                                         \
        if (stack == NULL) return;                                                               \
        struct stack_##type *head = *stack;                                                      \
        *stack = (*stack)->foot;                                                                 \
        free(head);                                                                              \
    }                                                                                            \
    int stack_size_##type(const struct stack_##type *stack) {                                    \
        int size = 0;                                                                            \
        for (; stack != NULL; stack = stack->foot, size++)                                       \
            ;                                                                                    \
        return size;                                                                             \
    }

#define LIST(type)                                                                                      \
    struct list_##type {                                                                                \
        type item;                                                                                      \
        struct list_##type *head;                                                                       \
        struct list_##type *foot;                                                                       \
    };                                                                                                  \
    struct list_##type *list_init_##type(type *item) {                                                  \
        struct list_##type *node = (struct list_##type *)malloc(sizeof(struct list_##type));            \
        if (node != NULL) {                                                                             \
            node->item = *item;                                                                         \
            node->head = NULL;                                                                          \
            node->foot = NULL;                                                                          \
        }                                                                                               \
        return node;                                                                                    \
    }                                                                                                   \
    void list_push_front_##type(struct list_##type **root, type *item) {                                \
        if (*root == NULL) {                                                                            \
            *root = list_init_##type(item);                                                             \
        } else {                                                                                        \
            struct list_##type *save = *root;                                                           \
            for (; (*root)->head != NULL; *root = (*root)->head)                                        \
                ;                                                                                       \
            (*root)->head = list_init_##type(item);                                                     \
            (*root)->head->foot = *root;                                                                \
            *root = save;                                                                               \
        }                                                                                               \
    }                                                                                                   \
    struct list_##type *list_get_##type(struct list_##type *node, int offset) {                         \
        if (offset > 0) {                                                                               \
            for (int i = 0; i < offset && node != NULL; i++, node = node->head)                         \
                ;                                                                                       \
        } else if (offset < 0) {                                                                        \
            for (int i = 0; i > offset && node != NULL; i--, node = node->foot)                         \
                ;                                                                                       \
        }                                                                                               \
        return node;                                                                                    \
    }                                                                                                   \
    void list_remove_##type(struct list_##type *node, struct list_##type **root) {                      \
        if (node == NULL || *root == NULL) return;                                                      \
        if (node == *root) {                                                                            \
            if ((*root)->head == NULL) {                                                                \
                *root = NULL;                                                                           \
            } else {                                                                                    \
                (*root) = (*root)->head;                                                                \
                (*root)->foot = NULL;                                                                   \
            }                                                                                           \
        } else {                                                                                        \
            if (node->head != NULL && node->foot != NULL) {                                             \
                node->foot->head = node->head;                                                          \
                node->head->foot = node->foot;                                                          \
            } else if (node->head != NULL && node->foot == NULL) {                                      \
                node->head->foot = NULL;                                                                \
            } else if (node->head == NULL && node->foot != NULL) {                                      \
                node->foot->head = NULL;                                                                \
            }                                                                                           \
        }                                                                                               \
        free(node);                                                                                     \
    }                                                                                                   \
    void list_destroy_##type(struct list_##type **root) {                                               \
        if (*root == NULL) return;                                                                      \
        for (; (*root)->head != NULL; list_remove_##type(*root, root))                                  \
            ;                                                                                           \
        free(*root);                                                                                    \
        *root = NULL;                                                                                   \
    }                                                                                                   \
    void list_output_##type(struct list_##type *root) {                                                 \
        for (; root != NULL; root = root->head) {                                                       \
            printf("\033[35m");                                                                         \
            if (root->foot == NULL) {                                                                   \
                printf("NULL");                                                                         \
            } else {                                                                                    \
                output_##type(root->item);                                                              \
            }                                                                                           \
            printf("\033[0m");                                                                          \
            printf(" <- ");                                                                             \
            printf("\033[36m");                                                                         \
            output_##type(root->item);                                                                  \
            printf("\033[0m");                                                                          \
            printf(" -> ");                                                                             \
            printf("\033[34m");                                                                         \
            if (root->head == NULL) {                                                                   \
                printf("NULL");                                                                         \
            } else {                                                                                    \
                output_##type(root->item);                                                              \
            }                                                                                           \
            printf("\033[0m");                                                                          \
            printf("\n");                                                                               \
        }                                                                                               \
    }                                                                                                   \
    struct list_##type *list_duplicate_##type(struct list_##type *src) {                                \
        struct list_##type *dst = list_init_##type(&(src->item));                                       \
        for (src = src->head; src != NULL; src = src->head) list_push_front_##type(&dst, &(src->item)); \
        return dst;                                                                                     \
    }                                                                                                   \
    size_t list_size_##type(struct list_##type *root) {                                                 \
        size_t size = 0;                                                                                \
        for (; root != NULL; size++, root = root->head)                                                 \
            ;                                                                                           \
        return size;                                                                                    \
    }                                                                                                   \
    struct list_##type *list_find_##type(struct list_##type *root, type *item) {                        \
        for (; root != NULL && !equal_##type(&(root->item), item); root = root->head)                   \
            ;                                                                                           \
        return root;                                                                                    \
    }

int equal_cell(cell *l_cell, cell *r_cell);
int equal_point(point *l_point, point *r_point);

void output_point(point p_point);
void output_double(double x);
void output_cell(cell c);

LIST(cell)
LIST(point)
STACK(operator)

int is_varialbe(char ch);
int is_digit(char ch);
int is_operator(char ch);
int is_function(char **ptr);

operator from_char_to_operator(char ch);
operator from_function_to_operator(int type);
cell from_double_to_cell(double x, int type);

int to_pole(struct list_cell **list_cells, struct stack_operator **stack_operators, operator* in_operator);
struct list_cell *convert_to_pole(char *equation, const size_t len);
int convert_from_pole(struct list_cell *list_cells, double variable, double *keeper);

int compare_operators(operator* src, operator* cmp);

char *remove_spaces(char *string, size_t *len);
char *scan_equation(size_t *len);

void normalize(struct list_point *points, double l_bound, double t_bound, double r_bound, double b_bound);
struct list_point *calculate(struct list_cell *pole);
void clear();
void draw_graph(struct list_point *points, int x0, int y0, int width, int height);
void output_buffer(double *buffer, size_t len);
void print_log(char *equation, struct list_cell *pole, struct list_point *points);
void stack_output_operator(struct stack_operator *stack);
int is_valid_equation(char *buffer);
void hide_cursor();
void show_cursor();
void realtime_input();
void scale(struct list_point *points, int scale);
void welcome_view();

int main() {
    setlocale(LC_ALL, "");
#ifdef DEBUG

    // hide_cursor();
    clear();
    welcome_view();
    realtime_input();
    // show_cursor();
#elif RELEASE
    clear();
    size_t len;
    char *equation = NULL;
    struct list_cell *pole = NULL;
    struct list_point *points = NULL;
    if ((equation = scan_equation(&len)) != NULL && (pole = convert_to_pole(equation, len)) != NULL &&
        (points = calculate(pole)) != NULL) {
#ifdef LOG
        print_log(equation, pole, points);
#endif
        normalize(points, 0, 0, WIDTH, HEIGHT);
        draw_graph(points, 0, ORDINATE, WIDTH, HEIGHT);

    } else {
        printf("То ли программа не работает - То ли меня пытаются сломать...\n");
    }

    if (points != NULL) list_destroy_point(&points);
    if (equation != NULL) free(equation);
    if (pole != NULL) list_destroy_cell(&pole);

#endif
    clear();
    return 0;
}
void welcome_view(){
    printf(
        "\033[33m"
        "  ───▄▀▀▀▄▄▄▄▄▄▄▀▀▀▄───\n"
        "  ───█▒▒░░░░░░░░░▒▒█───\n"
        "  ────█░░█░░░░░█░░█────\n"
        "  ─▄▄──█░░░▀█▀░░░█──▄▄─\n"
        "  █░░█─▀▄░░░░░░░▄▀─█░░█\n"
        "  █▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀█\n"
        "  █░░╦─╦╔╗╦─╔╗╔╗╔╦╗╔╗░░█\n"
        "  █░░║║║╠─║─║─║║║║║╠─░░█\n"
        "  █░░╚╩╝╚╝╚╝╚╝╚╝╩─╩╚╝░░█\n"
        "  █▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄█\n"
        "\n"
        "\033[0m"
        "type EQUATION to see REVALUATION\n"
    );
    fflush(stdout);
}
void hide_cursor(){
    printf("\e[?25l");
}
static int run = 1;
int is_valid_equation(char *buffer){
    for (char *ptr = buffer; buffer != NULL &&  *ptr ; ptr++) {
        if (!is_digit(*ptr) && !is_operator(*ptr) && !is_function(&ptr) && !is_varialbe(*ptr) &&
            *ptr != '.') {
            buffer = NULL;
        }
    }
    return buffer != NULL;
}
void scale(struct list_point *points, int scale){
    for(struct list_point *ptr = points ; ptr != NULL ; ptr = ptr->head){
        points->item.y *= scale;
        printf("%lf ", points->item.y);
    }
}


void realtime_input(){
    fd_set readfds;
    char buffer[512] = "\0";
    memset(buffer, 0, 512);   
    struct list_cell *pole = NULL;
    struct list_point *points = NULL;

    while(run){
        FD_ZERO(&readfds);
        FD_SET(fileno(stdin), &readfds);
        system("/bin/stty raw");
        select(fileno(stdin) + 1, &readfds, NULL, NULL, NULL);
        system("/bin/stty cooked");
        if(FD_ISSET(fileno(stdin), &readfds)){
            clear();
            system("/bin/stty raw");
            int i_key = getc(stdin);
            if(i_key == 127 && strlen(buffer) > 0){ //Backspace
                buffer[strlen(buffer) - 1] = '\0';
            }else if(i_key > 33 && i_key < 127){
                buffer[strlen(buffer)] = i_key;
                buffer[strlen(buffer) + 1] = '\0';

            }else if(i_key == 21){
                memset(buffer, 0, 512);
                *buffer = '\0';
            }else if(i_key == 27){
                run = 0;
            }
            system("/bin/stty cooked");
    
            printf("> y=%s\n\n", buffer);
            if(strcmp(buffer, "exit") == 0) {
                run = 0;
            }else{
                if(
                    is_valid_equation(buffer) &&
                    (pole = convert_to_pole(buffer, strlen(buffer))) != NULL &&
                    (points = calculate(pole)) != NULL
                ){
                    // normalize(points, 0,0, WIDTH, HEIGHT);
                    draw_graph(points, ABSCISSA, ORDINATE, WIDTH, HEIGHT);
                    // print_log(buffer, pole, points);
                }else{
                    draw_graph(NULL, ABSCISSA,ORDINATE, WIDTH, HEIGHT);
                }
                
            }
            putchar('\n');
            printf("\033[33mEsc - To Exit\033[0m\n");
            // printf("Key: %d", i_key);
            fflush(stdout);
        }
    }
    list_destroy_cell(&pole);
    list_destroy_point(&points);
}
int equal_cell(cell *l_cell, cell *r_cell) {
    return (l_cell->value == r_cell->value && l_cell->type == r_cell->type);
}
int equal_point(point *l_point, point *r_point) {
    return ((int)round(l_point->x) == (int)round(r_point->x) &&
            (int)round(l_point->y) == (int)round(r_point->y));
}

void output_point(point p_point) { printf("X: %.2lf -- Y: %.2lf", p_point.x, p_point.y); }
void output_double(double x) { printf("%lf", x); }
void output_cell(cell c) {
    if (c.type == OPERATOR) {
        printf("|type: operator action: ");
        if (c.value == SUM) {
            printf("summarize");
        } else if (c.value == DIVIDE) {
            printf("divide");
        } else if (c.value == MULTIPLY) {
            printf("multiply");
        } else if (c.value == SUBSTRACTION) {
            printf("substract");
        } else if (c.value == UNARY_MINUS) {
            printf("unary minus");
        } else if (c.value == SIN) {
            printf("sin");
        } else if (c.value == COS) {
            printf("cos");
        } else if (c.value == TAN) {
            printf("tan");
        } else if (c.value == CTG) {
            printf("ctg");
        } else if (c.value == LN) {
            printf("ln");
        } else if (c.value == SQRT) {
            printf("sqrt");
        }
        printf("|");
    } else if (c.type == VALUE) {
        printf("|type: value  data: %lf|", c.value);
    } else {
        printf("|type: variable symbol: %c|", VAR_SYMBOL);
    }
}

cell from_double_to_cell(double x, int type) {
    cell r_cell = {.type = type, .value = x};
    return r_cell;
}
void clear() { 
    system("@cls|clear");
    printf("\e[1;1H\e[2J");
     }
void show_cursor() {printf("\e[?25h");}
operator from_char_to_operator(char ch) {
    operator r_operator = {.symbol = ch};
    if (ch == '+') {
        r_operator.code = SUM;
        r_operator.priority = 1;
    } else if (ch == '-') {
        r_operator.code = SUBSTRACTION;
        r_operator.priority = 1;
    } else if (ch == '*') {
        r_operator.code = MULTIPLY;
        r_operator.priority = 2;
    } else if (ch == '/') {
        r_operator.code = DIVIDE;
        r_operator.priority = 2;
    } else if (ch == '(') {
        r_operator.code = OPEN_BRACKET;
        r_operator.priority = 0;
    } else if (ch == ')') {
        r_operator.code = CLOSE_BRACKET;
        r_operator.priority = 0;
    }
    return r_operator;
}
operator from_function_to_operator(int type) {
    operator r_operator = {.priority = 3, .code = type};
    if (type == SIN) {
        r_operator.symbol = 'S';
    } else if (type == CTG) {
        r_operator.symbol = 'G';
    } else if (type == COS) {
        r_operator.symbol = 'C';
    } else if (type == TAN) {
        r_operator.symbol = 'T';
    } else if (type == SQRT) {
        r_operator.symbol = 'Q';
    } else if (type == LN) {
        r_operator.symbol = 'L';
    }
    return r_operator;
}

int compare_operators(operator* src, operator* cmp) { return src->priority > cmp->priority; }

int is_varialbe(char ch) { return ch == VAR_SYMBOL; }
int is_digit(char ch) { return (int)ch >= 48 && (int)ch <= 57; }

int is_operator(char ch) { return (((int)ch >= 40 && (int)ch <= 43) || (int)ch == 45 || (int)ch == 47); }
int is_function(char **ptr) {
    int s = 0;
    if (strstr(*ptr, "sin") == *ptr) {
        s = SIN;
        (*ptr) += 2;
    } else if (strstr(*ptr, "cos") == *ptr) {
        s = COS;
        (*ptr) += 2;
    } else if (strstr(*ptr, "tan") == *ptr) {
        s = TAN;
        (*ptr) += 2;
    } else if (strstr(*ptr, "ctg") == *ptr) {
        s = CTG;
        (*ptr) += 2;
    } else if (strstr(*ptr, "sqrt") == *ptr) {
        s = SQRT;
        (*ptr) += 3;
    } else if (strstr(*ptr, "ln") == *ptr) {
        s = LN;
        (*ptr) += 1;
    }
    return s;
}

char *remove_spaces(char *string, size_t *len) {
    size_t sf_len = 0;
    char *str_space_free = (char *)malloc(sizeof(char) * (*len + 1));
    if (str_space_free != NULL) {
        for (size_t i = 0; i < *len; i++) {
            if (string[i] != ' ' && string[i] != '\t') {
                str_space_free[sf_len] = string[i];
                sf_len++;
            }
        }
        if (sf_len != *len) str_space_free = (char *)realloc(str_space_free, sizeof(char) * sf_len);
    }
    free(string);
    *len = sf_len;
    return str_space_free;
}

int to_pole(struct list_cell **list_cells, struct stack_operator **stack_operators, operator* in_operator) {
    int f_status = 1;
    if (in_operator->symbol == '(') {
        stack_push_operator(stack_operators, in_operator);
    } else if (in_operator->symbol == ')') {
        for (; *stack_operators != NULL && (*stack_operators)->item.symbol != '(';
             stack_pop_operator(stack_operators)) {
            cell tmp = from_double_to_cell((*stack_operators)->item.code, OPERATOR);
            list_push_front_cell(list_cells, &tmp);
        }
        if (*stack_operators == NULL)
            f_status = 0;
        else
            stack_pop_operator(stack_operators);
    } else {
        for (; *stack_operators != NULL && !compare_operators(in_operator, &((*stack_operators)->item));
             stack_pop_operator(stack_operators)) {
            cell tmp = from_double_to_cell((*stack_operators)->item.code, OPERATOR);
            list_push_front_cell(list_cells, &tmp);
        }
        stack_push_operator(stack_operators, in_operator);
    }
    return f_status;
}

int convert_from_pole(struct list_cell *list_cells, double variable, double *keeper) {
    int f_status = 1;
    struct list_cell *list_cell_dupl = list_duplicate_cell(list_cells);

    for (struct list_cell *ptr = list_cell_dupl; ptr != NULL && f_status; ptr = ptr->head) {
        if (ptr->item.type == VARIABLE) {
            ptr->item.value = variable;
        } else if (ptr->item.type == OPERATOR) {
            struct list_cell *f_elem = list_get_cell(ptr, -2);
            struct list_cell *s_elem = list_get_cell(ptr, -1);

            int action = ptr->item.value;
            if (action == SUM && f_elem != NULL && s_elem != NULL) {
                ptr->item.value = f_elem->item.value + s_elem->item.value;
            } else if (action == UNARY_MINUS && s_elem != NULL) {
                ptr->item.value = -s_elem->item.value;
            } else if (action == SUBSTRACTION && s_elem != NULL && f_elem != NULL) {
                ptr->item.value = f_elem->item.value - s_elem->item.value;
            } else if (action == MULTIPLY && s_elem != NULL && f_elem != NULL) {
                ptr->item.value = f_elem->item.value * s_elem->item.value;
            } else if (action == DIVIDE && s_elem != NULL && f_elem != NULL) {
                if (fabs(s_elem->item.value) > EPS)
                    ptr->item.value = f_elem->item.value / s_elem->item.value;
                else
                    ptr->item.value = HEIGHT;
            } else if (action == SIN && s_elem != NULL) {
                ptr->item.value = sin(s_elem->item.value);
            } else if (action == COS && s_elem != NULL) {
                ptr->item.value = cos(s_elem->item.value);
            } else if (action == TAN && s_elem != NULL) {
                ptr->item.value = sin(s_elem->item.value) / cos(s_elem->item.value);
            } else if (action == CTG && s_elem != NULL) {
                ptr->item.value = cos(s_elem->item.value) / sin(s_elem->item.value);
            } else if (action == SQRT && s_elem != NULL) {
                ptr->item.value = sqrt(s_elem->item.value);
            } else if (action == LN && s_elem != NULL) {
                ptr->item.value = log(s_elem->item.value);
            } else {
                f_status = 0;
            }
            ptr->item.type = VALUE;
            list_remove_cell(f_elem, &list_cell_dupl);
            list_remove_cell(s_elem, &list_cell_dupl);
        }
    }
    *keeper = list_cell_dupl->item.value;
    if (list_size_cell(list_cell_dupl) != 1) {
        f_status = 0;
    }
    list_destroy_cell(&list_cell_dupl);
    return f_status;
}
char *scan_equation(size_t *len) {
    char *string = NULL;

    if (getline(&string, len, stdin)) {
        string = remove_spaces(string, len);
        for (char *ptr = string; string != NULL && *ptr != '\n' && *ptr != '\0'; ptr++) {
            if (!is_digit(*ptr) && !is_operator(*ptr) && !is_function(&ptr) && !is_varialbe(*ptr) &&
                *ptr != '.') {
                free(string);
                string = NULL;
            }
        }
    }

    return string;
}
struct list_cell *convert_to_pole(char *equation, const size_t len) {
    char *ptr = equation;
    struct list_cell *list_cells = NULL;
    struct stack_operator *stack_operators = NULL;
    int func_type;
    cell variable = {.type = VARIABLE};

    for (; (size_t)(ptr - equation) < len && *ptr != '\0' && *ptr != '\n' && *ptr != EOF; ptr++) {
        if (is_varialbe(*ptr)) {
            list_push_front_cell(&list_cells, &variable);
        } else if (is_operator(*ptr)) {
            operator c_operator = from_char_to_operator(*ptr);
            if (*ptr == '-' && (*(ptr - 1) == '(' || ptr - equation == 0)) {
                c_operator.code = UNARY_MINUS;
            }
            to_pole(&list_cells, &stack_operators, &c_operator);

        } else if (is_digit(*ptr) || *ptr == '.') {
            cell ch_cell;
            ch_cell.type = VALUE;
            size_t dots = 0;
            sscanf(ptr, "%lf", &(ch_cell.value));
            list_push_front_cell(&list_cells, &ch_cell);
            for (char *j_ptr = ptr + 1;
                 (size_t)(j_ptr - equation) < len && dots <= 1 && (is_digit(*j_ptr) || (*j_ptr == '.'));
                 ptr = j_ptr, j_ptr++) {
                if (*j_ptr == '.') dots++;
            }

        } else if ((func_type = is_function(&ptr))) {
            operator func_operator = from_function_to_operator(func_type);
            to_pole(&list_cells, &stack_operators, &func_operator);
        }
    }
    for (; stack_operators != NULL; stack_pop_operator(&stack_operators)) {
        cell tmp = from_double_to_cell(stack_operators->item.code, OPERATOR);
        list_push_front_cell(&list_cells, &tmp);
    }

    return list_cells;
}

void draw_graph(struct list_point *points, int x0, int y0, int width, int height) {
    point p;
    for (int i = 0; i <= height; i++) {
        p.y = -1 * (i - y0);
        for (int j = 0; j <= width; j++) {
            p.x = j - x0;
            if (points != NULL && list_find_point(points, &p) != NULL) {
                printf(GRAPH_SYMBOL);
            } else if(j == x0 && i == y0){
                printf(CENTER_SYMBOL);
            }else if(j == x0){
                printf(ORDINATE_SYMBOL);
            }else if(i == y0){
                printf(ABSCISSA_SYMBOL);
            }else{
                printf(MAP_SYMBOL);
            }
        }
        printf("\n");
    }
}

void normalize(struct list_point *points, double l_bound, double t_bound, double r_bound, double b_bound) {
    double x_min = points->item.x, y_min = points->item.y, x_max = points->item.x, y_max = points->item.y;
    for (struct list_point *ptr = points; ptr != NULL; ptr = ptr->head) {
        if (x_min > ptr->item.x) {
            x_min = ptr->item.x;
        }
        if (x_max < ptr->item.x) {
            x_max = ptr->item.x;
        }
        if (y_min > ptr->item.y) {
            y_min = ptr->item.y;
        }
        if (y_max < ptr->item.y) {
            y_max = ptr->item.y;
        }
    }
    for (struct list_point *ptr = points; ptr != NULL; ptr = ptr->head) {
        if (fabs(x_max - x_min) > EPS)
            ptr->item.x = ptr->item.x / (x_max - x_min) * (r_bound - l_bound) + l_bound;
        if (fabs(y_max - y_min) > EPS)
            ptr->item.y = ptr->item.y / (y_max - y_min) * (b_bound - t_bound) + t_bound;
        // else {
        //     ptr->item.y = ptr->item.y * ((b_bound - t_bound) + t_bound) / 2;
        // }
    }
}

struct list_point *calculate(struct list_cell *pole) {
    struct list_point *points = NULL;
    double keeper;
    point p;
    for (double x = XMIN; x <= XMAX; x += STEP) {
        if (!convert_from_pole(pole, x, &keeper)) {
            list_destroy_point(&points);
            points = NULL;
            break;
        }
        if (keeper >= YMIN && keeper <= YMAX) {
            p.x = x;
            p.y = keeper;
            list_push_front_point(&points, &p);
        }
    }
    return points;
}

void print_log(char *equation, struct list_cell *pole, struct list_point *points) {
    printf("\033[36m--Уравнение\033[0m\n\033[34my=%s\033[0m\n", equation);
    printf("\033[36m--Области\033[0m\n\033[34mАбсцисса: [%.2lf:%.2lf]  -- Ордината: [%.2lf:%.2lf]\033[0m\n\n",
           (double)XMIN, (double)XMAX, (double)YMIN, (double)YMAX);
    printf("\033[36m--Польская Нотация\033[0m\n");
    list_output_cell(pole);
    printf("\n\n");
    printf("\033[36m--Значения Функции\033[0m\n");
    list_output_point(points);
    printf("\033[36m--Количество Точек\033[0m\n");
    printf("\033[34m%zx\033[0m\n\n", list_size_point(points));
}
void stack_output_operator(struct stack_operator *stack) {
    for (; stack != NULL; stack = stack->foot)
        printf("Symbol: %c  Priority: %d\n", stack->item.symbol, stack->item.priority);
}