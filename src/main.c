#include "sdh_arena.h"

#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"
#define STB_DS_IMPLEMENTATION
#include "main.h"
#include "stb_ds.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

char *token2string(MemoryArena *arena, long token) {
  switch (token) {
  case CLEX_id:
    return "CLEX_id";
  case CLEX_eq:
    return "CLEX_eq";
  case CLEX_noteq:
    return "CLEX_noteq";
  case CLEX_lesseq:
    return "CLEX_lesseq";
  case CLEX_greatereq:
    return "CLEX_greatereq";
  case CLEX_andand:
    return "CLEX_andand";
  case CLEX_oror:
    return "CLEX_oror";
  case CLEX_shl:
    return "CLEX_shl";
  case CLEX_shr:
    return "CLEX_shr";
  case CLEX_plusplus:
    return "CLEX_plusplus";
  case CLEX_minusminus:
    return "CLEX_minusminus";
  case CLEX_arrow:
    return "CLEX_arrow";
  case CLEX_andeq:
    return "CLEX_andeq";
  case CLEX_oreq:
    return "CLEX_oreq";
  case CLEX_xoreq:
    return "CLEX_xoreq";
  case CLEX_pluseq:
    return "CLEX_pluseq";
  case CLEX_minuseq:
    return "CLEX_minuseq";
  case CLEX_muleq:
    return "CLEX_muleq";
  case CLEX_diveq:
    return "CLEX_diveq";
  case CLEX_modeq:
    return "CLEX_modeq";
  case CLEX_shleq:
    return "CLEX_shleq";
  case CLEX_shreq:
    return "CLEX_shreq";
  case CLEX_eqarrow:
    return "CLEX_eqarrow";
  case CLEX_dqstring:
    return "CLEX_dqstring";
  case CLEX_sqstring:
    return "CLEX_sqstring";
  case CLEX_charlit:
    return "CLEX_charlit";
  case CLEX_intlit:
    return "CLEX_intlit";
  case CLEX_floatlit:
    return "CLEX_floatlit";
  default:
    if (token >= 0 && token < 256) {
      char *s = push_size(arena, 2, DEFAULT_ALIGNMENT);
      sprintf(s, "%c", (char)token);
      return s;
    } else {
      printf("<<<UNKNOWN TOKEN %ld >>>\n", token);
      exit(1);
    }
    break;
  }
}

TokenInfo get_token(MemoryArena *arena, stb_lexer *lexer, bool consume) {
  TokenInfo info = {0};
  char *parse_point = lexer->parse_point;
  info.result = stb_c_lexer_get_token(lexer);
  if (!consume) {
    lexer->parse_point = parse_point;
  }

  long token = lexer->token;
  info.token = token;

  switch (token) {
  case '-':
  case '+': {
    info.is_binary_operator = true;
    info.precedence = 1;
    info.op = token;
  } break;
  case '/':
  case '*': {
    info.is_binary_operator = true;
    info.precedence = 2;
    info.op = token;
  } break;
  case CLEX_id: {
    info.is_binary_operator = false;
    info.string_value = push_string(arena, lexer->string);
  } break;
  default: {
    info.is_binary_operator = false;
    info.int_value = token;
  } break;
  }

  return info;
}

#define expect_token(arena, lexer, consume, token)                             \
  _expect_token(arena, lexer, consume, token, __FILE__, __LINE__)

TokenInfo _expect_token(MemoryArena *arena, stb_lexer *lexer, bool consume,
                        long token, char *file, int line) {
  TokenInfo info = get_token(arena, lexer, consume);
  if (info.result == 0) {
    char *token_str = token2string(arena, token);
    printf("Expected token %s but found EOF instead (%s:%d)\n", token_str, file,
           line);
    exit(1);
  }
  if (info.token != token) {
    char *token_str = token2string(arena, token);
    char *cur_token_str = token2string(arena, info.token);
    stb_lex_location loc;
    stb_c_lexer_get_location(lexer, lexer->where_firstchar, &loc);
    printf("%d:%d - Expected token %s but found %s instead (%s:%d)\n",
           loc.line_number, loc.line_offset, token_str, cur_token_str, file,
           line);
    exit(1);
  }
  return info;
}

int consume_token(stb_lexer *lexer) { return stb_c_lexer_get_token(lexer); }

Expr *parse_primary(MemoryArena *arena, stb_lexer *lexer) {
  TokenInfo info = get_token(arena, lexer, true);
  assert(info.is_binary_operator == false);
  Expr *expr = push_size(arena, sizeof(Expr), DEFAULT_ALIGNMENT);

  if (info.token == CLEX_intlit) {
    expr->type = ExprType_NUMBER;
    expr->int_value = lexer->int_number;
  } else if (info.token == CLEX_dqstring) {
    expr->type = ExprType_STRING;
    expr->string_value = push_string(arena, lexer->string);
  } else if (info.token == CLEX_id) {
    expr->type = ExprType_ID;
    expr->string_value = push_string(arena, lexer->string);
  } else {
    printf("Cannot parse tokens of type %s\n", token2string(arena, info.token));
    exit(1);
  }
  expr->rhs = NULL;
  expr->lhs = NULL;
  return expr;
}

Expr *parse_expression_1(MemoryArena *arena, stb_lexer *lexer, Expr *lhs,
                         int min_precedence) {
  TokenInfo lookahead = get_token(arena, lexer, false);
  if (lookahead.result == 0) {
    return lhs;
  }

  while (lookahead.is_binary_operator &&
         lookahead.precedence >= min_precedence) {
    TokenInfo op = lookahead;
    assert(consume_token(lexer) != 0);
    Expr *rhs = parse_primary(arena, lexer);
    lookahead = get_token(arena, lexer, false);
    while (lookahead.result != 0 && lookahead.is_binary_operator &&
           lookahead.precedence > op.precedence) {
      rhs = parse_expression_1(
          arena, lexer, rhs,
          op.precedence + (lookahead.precedence > op.precedence ? 1 : 0));

      lookahead = get_token(arena, lexer, false);
    }
    Expr *expr = push_size(arena, sizeof(Expr), DEFAULT_ALIGNMENT);
    expr->type = ExprType_BINOP;
    expr->op = op.op;
    expr->rhs = rhs;
    expr->lhs = lhs;
    lhs = expr;
  }

  return lhs;
}

Expr *parse_expression(MemoryArena *arena, stb_lexer *lexer) {
  return parse_expression_1(arena, lexer, parse_primary(arena, lexer), 0);
}

void compile_expr(FILE *out, Expr *expr) {
  switch (expr->type) {
  case ExprType_BINOP: {
    fprintf(out, "(%c ", expr->op);
    compile_expr(out, expr->lhs);
    fprintf(out, " ");
    compile_expr(out, expr->rhs);
    fprintf(out, ")");
  } break;
  case ExprType_NUMBER: {
    fprintf(out, "%d", expr->int_value);
  } break;
  case ExprType_STRING: {
    fprintf(out, "\"%s\"", expr->string_value);
  } break;
  case ExprType_ID: {
    fprintf(out, "%s", expr->string_value);
  } break;
  }
}

void compile_rtrn(FILE *out, StatementReturn *rtrn) {
  fprintf(out, "\n");
  compile_expr(out, rtrn->expr);
}

void compile_funcall(FILE *out, StatementFuncCall *funcall) {
  fprintf(out, "(%s ", funcall->name);
  compile_expr(out, funcall->arg);
  fprintf(out, ")");
}

void compile_assignement(FILE *out, StatementAssignement *assign) {
  fprintf(out, "(define %s ", assign->name);
  compile_expr(out, assign->expr);
  fprintf(out, ")\n");
}

void compile_func(FILE *out, Function *f) {
  fprintf(out, "(define (%s) \n", f->name);
  for (int i = 0; i < arrlen(f->statements); ++i) {
    Statement stmt = f->statements[i];
    fprintf(out, "  ");
    switch (stmt.type) {
    case StatementType_Return: {
      compile_rtrn(out, &stmt.rtrn);
    } break;
    case StatementType_FuncCall: {
      compile_funcall(out, &stmt.funcall);
    } break;
    case StatementType_Assignement: {
      compile_assignement(out, &stmt.assign);
    } break;
    };
  }
  fprintf(out, ")\n");
}

Statement parse_statement(MemoryArena *arena, stb_lexer *lexer) {
  char *id_token = expect_token(arena, lexer, true, CLEX_id).string_value;
  if (strcmp(id_token, "return") == 0) {
    Statement stmt;
    stmt.type = StatementType_Return;
    stmt.rtrn.expr = parse_expression(arena, lexer);
    assert(stmt.rtrn.expr);
    expect_token(arena, lexer, true, ';');
    return stmt;
  }
  TokenInfo next_token = get_token(arena, lexer, false);
  Statement stmt;
  if (next_token.token == '(') {
    stmt.type = StatementType_FuncCall;
    stmt.funcall.name = id_token;
    expect_token(arena, lexer, true, '(');
    Expr *s = parse_expression(arena, lexer);
    assert(s);
    stmt.funcall.arg = s;
    expect_token(arena, lexer, true, ')');
    expect_token(arena, lexer, true, ';');
  } else {
    stmt.type = StatementType_Assignement;
    char *name = expect_token(arena, lexer, true, CLEX_id).string_value;
    expect_token(arena, lexer, true, '=');
    Expr *s = parse_expression(arena, lexer);
    assert(s);
    stmt.assign.name = name;
    stmt.assign.expr = s;
    expect_token(arena, lexer, true, ';');
  }
  return stmt;
}

Function *parse_function(MemoryArena *arena, stb_lexer *lexer) {
  char *return_type = expect_token(arena, lexer, true, CLEX_id).string_value;
  char *name = expect_token(arena, lexer, true, CLEX_id).string_value;
  expect_token(arena, lexer, true, '(');

  // TODO handle args
  while (stb_c_lexer_get_token(lexer) != 0) {
    if (lexer->token == ')') {
      break;
    }
  }
  if (lexer->token != ')') {
    printf("missing ) in function declaration\n");
    exit(1);
  }
  expect_token(arena, lexer, true, '{');

  Statement *statements = NULL;

  while (get_token(arena, lexer, false).token != '}') {
    Statement stmt = parse_statement(arena, lexer);
    arrput(statements, stmt);
  }

  expect_token(arena, lexer, true, '}');

  Function *func = push_size(arena, sizeof(Function), DEFAULT_ALIGNMENT);
  func->name = name;
  func->return_type = return_type;
  func->statements = statements;
  return func;
}

int main(void) {
  size_t memory_size = Megabytes(100);
  void *base_address = (void *)(0);
  // NOTE: MAP_ANONYMOUS content initialized to zero
  void *memory_block = mmap(base_address, memory_size, PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  MemoryArena arena;
  initializeArena(&arena, memory_size, memory_block);

  size_t lexer_storage_size = 1000;
  char *lexer_storage =
      push_size(&arena, lexer_storage_size, DEFAULT_ALIGNMENT);
  stb_lexer lexer;

  char *filename = "test.c";
  struct stat st;
  assert(stat(filename, &st) == 0);

  FILE *input = fopen(filename, "rb");
  assert(input);
  char *text = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fileno(input), 0);
  fclose(input); // TODO: i can do this now right?

  FILE *out = fopen("test.scm", "wb");
  assert(out);

  stb_c_lexer_init(&lexer, text, text + strlen(text), lexer_storage,
                   lexer_storage_size);

  Function *func = parse_function(&arena, &lexer);
  assert(func);

  fprintf(out, "(define printf print)\n");

  compile_func(out, func);

  fprintf(out, "(print (main))");
  fclose(out);
  munmap(text, st.st_size);
  munmap(memory_block, memory_size);
  return 0;
}
