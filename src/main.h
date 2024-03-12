#ifndef __SDH_MAIN
#define __SDH_MAIN

#include <stdbool.h>

enum ExprType {
  ExprType_BINOP,
  ExprType_NUMBER,
  ExprType_STRING,
  ExprType_ID,
};

struct Expr;

struct Expr {
  enum ExprType type;
  char op;
  int int_value;
  char* string_value;
  struct Expr *rhs;
  struct Expr *lhs;
};

typedef struct Expr Expr;

typedef struct {
  int result;
  long token;

  char op;
  bool is_binary_operator;
  int precedence;

  int int_value;
  char *string_value;
} TokenInfo;

enum StatementType { StatementType_FuncCall, StatementType_Return, StatementType_Assignement };

typedef struct {
  char* name;
  Expr* arg;
} StatementFuncCall;

typedef struct {
  Expr* expr;
} StatementReturn;

typedef struct {
  char* name;
  Expr* expr;
} StatementAssignement;

typedef struct {
  enum StatementType type;
  StatementFuncCall funcall;
  StatementReturn rtrn;
  StatementAssignement assign;
} Statement;

typedef struct {
  // TODO parameters
  char *return_type;
  char *name;
  Statement *statements;
} Function;

#endif
