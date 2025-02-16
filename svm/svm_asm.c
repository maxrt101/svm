/** ========================================================================= *
 *
 * @file svm_asm.c
 * @date 01-11-2024
 * @author Maksym Tkachuk <max.r.tkachuk@gmail.com>
 *
 *  ========================================================================= */

/* Includes ================================================================= */
#include "svm_asm.h"
#include "svm_util.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Defines ================================================================== */
/* Macros =================================================================== */
/* Exposed macros =========================================================== */
/* Enums ==================================================================== */
typedef enum {
  SVM_ASM_ARGC_NONE = 0,
  SVM_ASM_ARGC_ALL,
  SVM_ASM_ARGC_REG_ONLY,
  SVM_ASM_ARGC_IMM_ONLY,
} svm_asm_arg_constraint_t;

/* Types ==================================================================== */
typedef struct {
  uint8_t arg_count;
  svm_asm_arg_constraint_t arg1_restrict;
  svm_asm_arg_constraint_t arg2_restrict;
} svm_asm_opcode_meta_t;

/* Variables ================================================================ */
svm_asm_opcode_meta_t opcode_meta[] = {
    [OP_NOP] = {0, SVM_ASM_ARGC_NONE, SVM_ASM_ARGC_NONE},
    [OP_END] = {0, SVM_ASM_ARGC_NONE, SVM_ASM_ARGC_NONE},
    [OP_MOV] = {2, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_ALL},
    [OP_ADD] = {2, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_ALL},
    [OP_SUB] = {2, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_ALL},
    [OP_MUL] = {2, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_ALL},
    [OP_DIV] = {2, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_ALL},
    [OP_AND] = {2, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_ALL},
    [OP_OR]  = {2, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_ALL},
    [OP_XOR] = {2, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_ALL},
    [OP_SHL] = {2, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_ALL},
    [OP_SHR] = {2, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_ALL},
    [OP_CMP] = {2, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_ALL},
    [OP_CLF] = {0, SVM_ASM_ARGC_NONE, SVM_ASM_ARGC_NONE},
    [OP_JMP] = {1, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_NONE},
    [OP_INV] = {1, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_NONE},
    [OP_RET] = {0, SVM_ASM_ARGC_NONE, SVM_ASM_ARGC_NONE},
    [OP_SYS] = {1, SVM_ASM_ARGC_ALL,  SVM_ASM_ARGC_NONE},
};

/* Private functions ======================================================== */
static const char * svm_asm_constraint2errstr(svm_asm_arg_constraint_t constraint) {
  switch (constraint) {
    case SVM_ASM_ARGC_NONE:     return "must be empty";
    case SVM_ASM_ARGC_ALL:      return "can be anything, but not empty";
    case SVM_ASM_ARGC_REG_ONLY: return "can only be a register";
    case SVM_ASM_ARGC_IMM_ONLY: return "can only be immediate";
    default:
      return "<INVALID CONSTRAINT>";
  }
}

static bool svm_asm_check_constraint(svm_asm_arg_constraint_t constraint, svm_arg_type_t arg) {
  // TODO: Refactor
  if (arg == ARG_NONE && constraint == SVM_ASM_ARGC_NONE) {
    return true;
  } else if (arg == ARG_IMM && constraint == SVM_ASM_ARGC_IMM_ONLY) {
    return true;
  } else if (arg >= ARG_R0 && arg <= ARG_R15 && constraint == SVM_ASM_ARGC_REG_ONLY) {
    return true;
  } else if (arg != ARG_NONE && constraint == SVM_ASM_ARGC_ALL) {
    return true;
  } else {
    return false;
  }
}

static void svm_asm_push_i32(svm_asm_t * ctx, int32_t i32) {
  SVM_ASSERT_RETURN(ctx);

  if (ctx->code.size + 1 >= ctx->code.capacity) {
    ctx->code.capacity += 32;
    SVM_REALLOC_CHECK(ctx->code.buffer, ctx->code.capacity * sizeof(typeof(ctx->code.buffer[0])));
  }
  ctx->code.buffer[ctx->code.size++] = i32;
}

static void svm_asm_add_label(svm_asm_t * ctx, const char * name, int32_t location) {
  SVM_ASSERT_RETURN(ctx && name);

  if (ctx->labels.size + 1 >= ctx->labels.capacity) {
    ctx->labels.capacity += 8;
    SVM_REALLOC_CHECK(ctx->labels.buffer, ctx->labels.capacity * sizeof(ctx->labels.buffer[0]));
  }
  strcpy(ctx->labels.buffer[ctx->labels.size].name, name);
  ctx->labels.buffer[ctx->labels.size].location = location;
  ctx->labels.size++;
}

static int32_t svm_asm_find_label(svm_asm_t * ctx, const char * name) {
  SVM_ASSERT_RETURN(ctx && name, -1);

  for (uint32_t i = 0; i < ctx->labels.size; ++i) {
    if (!strcmp(name, ctx->labels.buffer[i].name)) {
      return ctx->labels.buffer[i].location;
    }
  }
  return -1;
}

static void svm_asm_add_patch_label(svm_asm_t * ctx, const char * name, int32_t location) {
  SVM_ASSERT_RETURN(ctx && name);

  if (ctx->patches.size + 1 >= ctx->patches.capacity) {
    ctx->patches.capacity += 8;
    SVM_REALLOC_CHECK(ctx->patches.buffer, ctx->patches.capacity);
  }
  strcpy(ctx->patches.buffer[ctx->patches.size].name, name);
  ctx->patches.buffer[ctx->patches.size].location = location;
  ctx->patches.size++;
}

static svm_asm_error_t svm_asm_patch_labels(svm_asm_t * ctx) {
  SVM_ASSERT_RETURN(ctx, SVM_ASM_ERR_NULL);

  for (int32_t i = 0; i < ctx->patches.size; ++i) {
    printf("try patch %s at 0x%x\n",
           ctx->patches.buffer[i].name, ctx->patches.buffer[i].location);
    int32_t value = svm_asm_find_label(ctx, ctx->patches.buffer[i].name);
    if (value == -1) {
      printf(
          "Undefined label '%s' referenced at 0x%x\n",
          ctx->patches.buffer[i].name,
          ctx->patches.buffer[i].location
      );
      return SVM_ASM_ERR_UNDEFINED_LABEL;
    }
    printf("patch %s at 0x%x with 0x%x\n",
           ctx->patches.buffer[i].name, ctx->patches.buffer[i].location, value);
    ctx->code.buffer[ctx->patches.buffer[i].location] = value;
  }

  return SVM_ASM_OK;
}

static char * svm_asm_next_token(char ** source, char * source_end) {
  SVM_ASSERT_RETURN(source && *source && source_end, NULL);

  if (*source >= source_end) {
    return NULL;
  }

  while (**source == ' ' || **source == '\n') {
    (*source)++;
  }

  if (**source == '#') {
    while (**source != '\n' || **source != '\0') {
      (*source)++;
    }
  }

  char * token = *source;

  while (**source != ' ' && **source != '\n' && **source != '.' && **source != '\0') {
    (*source)++;
  }

  **source = '\0';
  (*source)++;

#if USE_SVM_ASM_PRINT_TOKENS
  printf("token '%s'\n", token);
#endif

  return token;
}

static void svm_asm_rollback_token(char * token, char ** source) {
  SVM_ASSERT_RETURN(token && source && *source);

  (*source)[-1] = ' ';
  *source = token;

#if USE_SVM_ASM_PRINT_TOKENS
  printf("token rollback\n");
#endif
}

/* Shared functions ========================================================= */
svm_asm_error_t svm_asm_init(svm_asm_t * ctx) {
  SVM_ASSERT_RETURN(ctx, SVM_ASM_ERR_NULL);

  memset(ctx, 0, sizeof(*ctx));

  ctx->code.capacity = 32;
  ctx->code.buffer = svm_malloc(ctx->code.capacity * sizeof(typeof(ctx->code.buffer[0])));

  if (!ctx->code.buffer) {
    printf("Failed to allocate buffer for bytecode\n");
    return SVM_ASM_ERR_BAD_ALLOC;
  }

  ctx->labels.capacity = 8;
  ctx->labels.buffer = svm_malloc(ctx->labels.capacity);

  if (!ctx->labels.buffer) {
    printf("Failed to allocate buffer for labels\n");
    return SVM_ASM_ERR_BAD_ALLOC;
  }

  return SVM_ASM_OK;
}

svm_asm_error_t svm_asm_free(svm_asm_t * ctx) {
  if (ctx->code.buffer) {
    svm_free(ctx->code.buffer);
  }

  if (ctx->labels.buffer) {
    svm_free(ctx->labels.buffer);
  }

  if (ctx->patches.buffer) {
    svm_free(ctx->patches.buffer);
  }

  return SVM_ASM_OK;
}

svm_asm_error_t svm_asm(svm_asm_t * ctx, char * source) {
  size_t source_size = strlen(source);
  char * source_end = source + source_size;

  while (*source) {
    if (*source == '#') {
      while (*source != '\n' || *source != '\0') {
        source++;
      }
    }
    
    svm_opcode_t op;
    svm_ext_t ext;
    svm_arg_type_t arg1 = ARG_NONE, arg2 = ARG_NONE;
    char * op_str, * ext_str, * arg1_str, * arg2_str;

    SVM_ASSERT_RETURN(op_str = svm_asm_next_token(&source, source_end), SVM_ASM_OK);

    op = svm_str2opcode(op_str);

    if (op == OP_MAX) {
      svm_asm_add_label(ctx, op_str, ctx->code.size);
      continue;
    }

    ext_str = svm_asm_next_token(&source, source_end);
    ext = svm_str2ext(ext_str);

    // TODO: Document
    if (ext != EXT_NONE) {
      if (opcode_meta[op].arg_count > 0) {
        SVM_ASSERT_RETURN(arg1_str = svm_asm_next_token(&source, source_end), SVM_ASM_ERR_EXPECTED_TOKEN);
      }
    } else {
      if (opcode_meta[op].arg_count == 0) {
        svm_asm_rollback_token(ext_str, &source);
      } else {
        arg1_str = ext_str;
      }
    }

    if (opcode_meta[op].arg_count > 0) {
      arg1 = svm_str2arg(arg1_str);
      if (!svm_asm_check_constraint(opcode_meta[op].arg1_restrict, arg1)) {
        printf("First argument to %s %s\n", op_str, svm_asm_constraint2errstr(opcode_meta[op].arg1_restrict));
        return SVM_ASM_ERR_ARG_CONSTRAINT_UNSATISFIED;
      }
    }

    if (opcode_meta[op].arg_count > 1) {
      SVM_ASSERT_RETURN(arg2_str = svm_asm_next_token(&source, source_end), SVM_ASM_ERR_EXPECTED_TOKEN);
      arg2 = svm_str2arg(arg2_str);
      if (!svm_asm_check_constraint(opcode_meta[op].arg2_restrict, arg2)) {
        printf("Second argument to %s %s\n", op_str, svm_asm_constraint2errstr(opcode_meta[op].arg2_restrict));
        return SVM_ASM_ERR_ARG_CONSTRAINT_UNSATISFIED;
      }
    }

    ext  = ext  == EXT_MAX ? EXT_NONE : ext;
    arg1 = arg1 == ARG_MAX ? ARG_NONE : arg1;
    arg2 = arg2 == ARG_MAX ? ARG_NONE : arg2;

    svm_asm_push_i32(ctx, svm_instruction_to_int32(svm_pack_instruction(op, ext, arg1, arg2)));

    if (arg1 == ARG_IMM) {
      int32_t value;
      if (!svm_to_int32(arg1_str, &value)) {
        value = svm_asm_find_label(ctx, arg1_str);
        if (value == -1) {
          svm_asm_add_patch_label(ctx, arg1_str, ctx->code.size);
        }
      }
      svm_asm_push_i32(ctx, value);
    }

    if (arg2 == ARG_IMM) {
      int32_t value;
      if (!svm_to_int32(arg2_str, &value)) {
        value = svm_asm_find_label(ctx, arg2_str);
        if (value == -1) {
          svm_asm_add_patch_label(ctx, arg2_str, ctx->code.size);
        }
      }
      svm_asm_push_i32(ctx, value);
    }
  }

  return SVM_ASM_OK;
}

svm_asm_error_t svm_asm_file(svm_asm_t * ctx, const char * filename) {
  SVM_ASSERT_RETURN(ctx && filename, SVM_ASM_ERR_NULL);

  FILE * file = fopen(filename, "r");

  if (!file) {
    printf("Failed to open %s\n", filename);
    return SVM_ASM_ERR_FILE_OPEN_FAILED;
  }

  fseek(file, 0, SEEK_END);
  uint32_t fsize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char * source = svm_malloc(fsize + 1);
  fread(source, fsize, 1, file);
  fclose(file);

  source[fsize] = 0;

  svm_asm_init(ctx);
  svm_asm_error_t res = svm_asm(ctx, source);

  free(source);

  if (res != SVM_ASM_OK) {
    return res;
  }

  res = svm_asm_patch_labels(ctx);

  if (res != SVM_ASM_OK) {
    return res;
  }

#if USE_SVM_ASM_PRINT_DISASM
  printf("Disassembly:\n");
  svm_disassemble(ctx->code.buffer, ctx->code.size);
#endif

#if USE_SVM_ASM_PRINT_LABELS
  printf("Labels:\n");
  for (int32_t i = 0; i < ctx->labels.size; ++i) {
    printf("0x%04x | '%s'\n", ctx->labels.buffer[i].location, ctx->labels.buffer[i].name);
  }
#endif

  return SVM_ASM_OK;
}
