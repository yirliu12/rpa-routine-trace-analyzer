/* Program to evaluate candidate routines for Robotic Process Automation.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* #define's -----------------------------------------------------------------*/

#define ASIZE 26
#define TRUE 1
#define FALSE 0
#define NONE (-1)
#define END '#'
#define CH_NL '\n'
#define COLON ':'

/* type definitions ----------------------------------------------------------*/

typedef struct action action_t;
struct action {
    char name;                 // action name
    signed char pre[ASIZE];    // precondition: -1 unspecified, 0 false, 1 true
    signed char effect[ASIZE]; // effect: -1 unchanged, 0 false, 1 true
};

typedef struct step step_t;
struct step {
    action_t *action;          // action performed at this step
    step_t   *next;            // next step in the trace
};

typedef struct routine routine_t;
struct routine {
    char *name;                // candidate routine name
    signed char effect[ASIZE]; // cumulative effect
    routine_t *next;           // next candidate routine
};

typedef struct {
    char *data;                // dynamically allocated line contents
    size_t len;                // number of characters in the line
} line_t;

/* function prototypes -------------------------------------------------------*/

static void die(void);
static char *copy_string(const char *s);
static line_t read_line(void);
static void set_unset(signed char a[ASIZE]);
static void parse_letters(const char *s, signed char out[ASIZE], int value);
static action_t *parse_action(const char *line);
static void apply_action(const action_t *a, int state[ASIZE]);
static int preconditions_hold(const action_t *a, const int state[ASIZE]);
static void append_step(step_t **head, step_t **tail, action_t *a);
static void append_routine(routine_t **head, routine_t **tail, routine_t *r);
static routine_t *make_routine(const char *name, action_t *by_name[ASIZE]);
static int same_effect(const signed char a[ASIZE], const signed char b[ASIZE]);
static void print_state(char label, const int state[ASIZE]);
static void print_delimiter(void);
static void print_routines_stage1(routine_t *routines, step_t **steps, size_t nsteps);
static int matches_stage2(const routine_t *r, const int baseline[ASIZE],
                          const int after[ASIZE], const signed char segment_effect[ASIZE],
                          const unsigned char touched[ASIZE],
                          const unsigned char diverged[ASIZE],
                          const unsigned char observed[ASIZE]);
static void print_routines_stage2(routine_t *routines, step_t **steps,
                                  int (*states)[ASIZE], size_t nsteps);
static void free_routines(routine_t *r);
static void free_steps(step_t *s);

/* where it all happens ------------------------------------------------------*/

int
main(void) {
    int initial[ASIZE] = {0}, state[ASIZE], num_actions = 0;
    action_t *actions[ASIZE] = {0}, *by_name[ASIZE] = {0};
    step_t *head = NULL, *tail = NULL, *s;
    step_t **steps = NULL;
    size_t nsteps = 0, trace_len = 0, valid = 1, i;
    int (*states)[ASIZE] = NULL;
    line_t line;
    routine_t *stage1 = NULL, *stage1tail = NULL;
    routine_t *stage2 = NULL, *stage2tail = NULL;
    int has_stage1 = 0, has_stage2 = 0;

    // read the initial state
    while ((line = read_line()).data != NULL) {
        if (strcmp(line.data, "#") == 0) { free(line.data); break; }
        for (i = 0; i < line.len; i++)
            if (line.data[i] >= 'a' && line.data[i] <= 'z')
                initial[line.data[i] - 'a'] = TRUE;
        free(line.data);
    }

    // read and record action definitions
    while ((line = read_line()).data != NULL) {
        action_t *a;
        if (strcmp(line.data, "#") == 0) { free(line.data); break; }
        a = parse_action(line.data);
        free(line.data);
        if (!a) continue;
        if (a->name < 'A' || a->name > 'Z' || num_actions >= ASIZE) {
            free(a);
            continue;
        }
        actions[num_actions++] = a;
        by_name[a->name - 'A'] = a;
    }

    // read the input trace
    line = read_line();
    if (line.data) {
        trace_len = line.len;
        for (i = 0; i < line.len; i++) {
            action_t *a = (line.data[i] >= 'A' && line.data[i] <= 'Z')
                        ? by_name[line.data[i] - 'A'] : NULL;
            if (a) append_step(&head, &tail, a);
        }
        free(line.data);
    }

    memcpy(state, initial, sizeof(state));
    for (s = head; s; s = s->next) {
        if (!preconditions_hold(s->action, state)) { valid = 0; break; }
        apply_action(s->action, state);
        nsteps++;
    }

    // build an index for scanning the linked trace
    if (nsteps) {
        size_t k = 0;
        steps = malloc(nsteps * sizeof(*steps));
        states = malloc((nsteps + 1) * sizeof(*states));
        if (!steps || !states) die();
        memcpy(state, initial, sizeof(state));
        memcpy(states[0], state, sizeof(state));
        for (s = head; s && k < nsteps; s = s->next) {
            if (!preconditions_hold(s->action, state)) break;
            steps[k] = s;
            apply_action(s->action, state);
            memcpy(states[k + 1], state, sizeof(state));
            k++;
        }
    }

    printf("==STAGE 0===============================\n");
    printf("Number of distinct actions: %d\n", num_actions);
    printf("Length of the input trace: %zu\n", trace_len);
    printf("Trace status: %s\n", valid ? "valid" : "invalid");
    print_delimiter();
    puts("  abcdefghijklmnopqrstuvwxyz");
    print_state('>', initial);
    for (i = 0; i < nsteps; i++) print_state(steps[i]->action->name, states[i + 1]);

    // read the candidate routines for each stage
    line = read_line();
    if (line.data && strcmp(line.data, "#") == 0) has_stage1 = 1;
    free(line.data);
    if (has_stage1) {
        while ((line = read_line()).data != NULL) {
            if (strcmp(line.data, "#") == 0) { has_stage2 = 1; free(line.data); break; }
            if (line.len) append_routine(&stage1, &stage1tail, make_routine(line.data, by_name));
            free(line.data);
        }
        print_routines_stage1(stage1, steps, nsteps);
    }
    if (has_stage2) {
        while ((line = read_line()).data != NULL) {
            if (line.len) append_routine(&stage2, &stage2tail, make_routine(line.data, by_name));
            free(line.data);
        }
        print_routines_stage2(stage2, steps, states, nsteps);
    }

    free_routines(stage1);
    free_routines(stage2);
    free_steps(head);
    for (i = 0; i < (size_t)num_actions; i++) free(actions[i]);
    free(steps);
    free(states);
    return 0;
}

/* function definitions -----------------------------------------------------*/

/* report a memory allocation failure */
static void die(void) {
    fputs("Memory allocation failed.\n", stderr);
    exit(EXIT_FAILURE);
}

/* make a dynamically allocated copy of a null-terminated string */
static char *copy_string(const char *s) {
    size_t n = strlen(s) + 1;
    char *copy = malloc(n);
    if (!copy) die();
    memcpy(copy, s, n);
    return copy;
}

/* read one arbitrarily long line and remove the line ending */
static line_t read_line(void) {
    size_t cap = 64, n = 0;
    char *s = malloc(cap);
    int c;
    if (!s) die();
    while ((c = getchar()) != EOF && c != CH_NL) {
        if (n + 1 >= cap) {
            char *p;
            cap *= 2;
            p = realloc(s, cap);
            if (!p) { free(s); die(); }
            s = p;
        }
        s[n++] = (char)c;
    }
    if (c == EOF && n == 0) { free(s); return (line_t){NULL, 0}; }
    if (n && s[n - 1] == '\r') n--;
    s[n] = '\0';
    return (line_t){s, n};
}

/* initialize an effect array to unspecified */
static void set_unset(signed char a[ASIZE]) {
    int i;
    for (i = 0; i < ASIZE; i++) a[i] = NONE;
}

/* convert variable letters into state-array entries */
static void parse_letters(const char *s, signed char out[ASIZE], int value) {
    while (*s) {
        if (*s >= 'a' && *s <= 'z') out[*s - 'a'] = (signed char)value;
        s++;
    }
}

/* parse one colon-separated action definition */
static action_t *parse_action(const char *line) {
    char *copy = copy_string(line), *fields[5], *p;
    action_t *a;
    int i;
    if (!copy) die();
    fields[0] = copy;
    for (i = 1; i < 5; i++) {
        p = strchr(fields[i - 1], COLON);
        if (!p) { free(copy); return NULL; }
        *p = '\0';
        fields[i] = p + 1;
    }
    a = malloc(sizeof(*a));
    if (!a) { free(copy); die(); }
    set_unset(a->pre);
    set_unset(a->effect);
    parse_letters(fields[0], a->pre, 1);
    parse_letters(fields[1], a->pre, 0);
    a->name = fields[2][0];
    parse_letters(fields[3], a->effect, 1);
    parse_letters(fields[4], a->effect, 0);
    free(copy);
    return a;
}

/* apply an action effect to the current state */
static void apply_action(const action_t *a, int state[ASIZE]) {
    int i;
    for (i = 0; i < ASIZE; i++)
        if (a->effect[i] != NONE) state[i] = a->effect[i];
}

/* check whether an action is enabled in the current state */
static int preconditions_hold(const action_t *a, const int state[ASIZE]) {
    int i;
    for (i = 0; i < ASIZE; i++)
        if (a->pre[i] != NONE && a->pre[i] != state[i]) return 0;
    return 1;
}

/* insert one action at the tail of the trace list */
static void append_step(step_t **head, step_t **tail, action_t *a) {
    step_t *s = malloc(sizeof(*s));
    if (!s) die();
    s->action = a;
    s->next = NULL;
    if (*tail) (*tail)->next = s;
    else *head = s;
    *tail = s;
}

/* insert one candidate routine at the tail of its list */
static void append_routine(routine_t **head, routine_t **tail, routine_t *r) {
    r->next = NULL;
    if (*tail) (*tail)->next = r;
    else *head = r;
    *tail = r;
}

/* calculate the cumulative effect of a candidate routine */
static routine_t *make_routine(const char *name, action_t *by_name[ASIZE]) {
    routine_t *r = malloc(sizeof(*r));
    size_t i;
    if (!r) die();
    r->name = copy_string(name);
    if (!r->name) die();
    set_unset(r->effect);
    for (i = 0; name[i]; i++) {
        action_t *a = (name[i] >= 'A' && name[i] <= 'Z')
                    ? by_name[name[i] - 'A'] : NULL;
        int v;
        if (!a) continue; /* Input is specified to contain defined action names. */
        for (v = 0; v < ASIZE; v++)
            if (a->effect[v] != NONE) {
                r->effect[v] = a->effect[v];
            }
    }
    return r;
}

/* compare two cumulative effect arrays */
static int same_effect(const signed char a[ASIZE], const signed char b[ASIZE]) {
    int i;
    for (i = 0; i < ASIZE; i++) if (a[i] != b[i]) return 0;
    return 1;
}

/* print one row of the trace state table */
static void print_state(char label, const int state[ASIZE]) {
    int i;
    printf("%c ", label);
    for (i = 0; i < ASIZE; i++) putchar(state[i] ? '1' : '0');
    putchar('\n');
}

/* print the required output delimiter */
static void print_delimiter(void) {
    puts("----------------------------------------");
}

/* find non-overlapping exact-effect matches for Stage 1 */
static void print_routines_stage1(routine_t *routines, step_t **steps,
                                  size_t nsteps) {
    routine_t *r;
    int first = 1;
    puts("==STAGE 1===============================");
    for (r = routines; r; r = r->next) {
        size_t start = 0;
        if (!first) print_delimiter();
        first = 0;
        printf("Candidate routine: %s\n", r->name);

        while (start < nsteps) {
            signed char effect[ASIZE];
            size_t end, v;
            set_unset(effect);
            for (end = start; end < nsteps; end++) {
                for (v = 0; v < ASIZE; v++)
                    if (steps[end]->action->effect[v] != NONE)
                        effect[v] = steps[end]->action->effect[v];
                if (same_effect(effect, r->effect)) {
                    size_t k;
                    printf("%5zu: ", start);
                    for (k = start; k <= end; k++) putchar(steps[k]->action->name);
                    putchar('\n');
                    start = end + 1; /* Keep reported subsequences non-overlapping. */
                    break;
                }
            }
            if (end == nsteps) start++;
        }
    }
}

/* check whether a trace interval matches a Stage 2 candidate */
static int matches_stage2(const routine_t *r, const int baseline[ASIZE],
                          const int after[ASIZE],
                          const signed char segment_effect[ASIZE],
                          const unsigned char touched[ASIZE],
                          const unsigned char diverged[ASIZE],
                          const unsigned char observed[ASIZE]) {
    int v;
    for (v = 0; v < ASIZE; v++) {
        if (r->effect[v] != NONE) {
            if (segment_effect[v] != r->effect[v]) return 0;
        } else {
            if (after[v] != baseline[v]) return 0;
            /* If a write is idempotent, require a matching precondition to
               establish the value the trace relies on. */
            if (touched[v] && !diverged[v] && !observed[v]) return 0;
        }
    }
    return 1;
}

/* find non-overlapping effect and restoration matches for Stage 2 */
static void print_routines_stage2(routine_t *routines, step_t **steps,
                                  int (*states)[ASIZE],
                                  size_t nsteps) {
    routine_t *r;
    int first = 1;
    puts("==STAGE 2===============================");
    for (r = routines; r; r = r->next) {
        size_t start = 0;
        if (!first) print_delimiter();
        first = 0;
        printf("Candidate routine: %s\n", r->name);

        while (start < nsteps) {
            signed char effect[ASIZE];
            int after[ASIZE];
            unsigned char touched[ASIZE] = {0}, diverged[ASIZE] = {0};
            unsigned char observed[ASIZE] = {0};
            size_t end, v;
            memcpy(after, states[start], sizeof(after));
            set_unset(effect);
            for (end = start; end < nsteps; end++) {
                for (v = 0; v < ASIZE; v++) {
                    if (r->effect[v] == NONE &&
                        steps[end]->action->pre[v] != NONE &&
                        steps[end]->action->pre[v] == states[start][v])
                        observed[v] = 1;
                    if (steps[end]->action->effect[v] != NONE) {
                        effect[v] = steps[end]->action->effect[v];
                        if (r->effect[v] == NONE) {
                            touched[v] = 1;
                            if (steps[end]->action->effect[v] != states[start][v])
                                diverged[v] = 1;
                        }
                    }
                }
                apply_action(steps[end]->action, after);
                if (matches_stage2(r, states[start], after, effect,
                                   touched, diverged, observed)) {
                    size_t k;
                    printf("%5zu: ", start);
                    for (k = start; k <= end; k++) putchar(steps[k]->action->name);
                    putchar('\n');
                    start = end + 1;
                    break;
                }
            }
            if (end == nsteps) start++;
        }
    }
    puts("==THE END===============================");
}

/* release all candidate-routine storage */
static void free_routines(routine_t *r) {
    while (r) { routine_t *n = r->next; free(r->name); free(r); r = n; }
}

/* release all linked trace storage */
static void free_steps(step_t *s) {
    while (s) { step_t *n = s->next; free(s); s = n; }
}

/* ta-da-da-daa!!! -----------------------------------------------------------*/
