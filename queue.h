/* structure to hold list of fileNames */

struct qnode {
    char   fileName[256];
    struct qnode *next;
};

struct qnode *addq(struct qnode *, char *, int *);
struct qnode *delq(struct qnode *);
void qprint(struct qnode *);

