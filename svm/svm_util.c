/** ========================================================================= *
 *
 * @file svm_util.c
 * @date 01-11-2024
 * @author Maksym Tkachuk <max.r.tkachuk@gmail.com>
 *
 *  ========================================================================= */

/* Includes ================================================================= */
#include "svm_util.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* Defines ================================================================== */
/* Macros =================================================================== */
/* Exposed macros =========================================================== */
/* Enums ==================================================================== */
/* Types ==================================================================== */
/* Variables ================================================================ */
/* Private functions ======================================================== */
/* Shared functions ========================================================= */
const char * svm_opcode2str(svm_opcode_t op) {
  switch (op) {
    case OP_NOP: return "NOP";
    case OP_END: return "END";
    case OP_MOV: return "MOV";
    case OP_ADD: return "ADD";
    case OP_SUB: return "SUB";
    case OP_MUL: return "MUL";
    case OP_DIV: return "DIV";
    case OP_CMP: return "CMP";
    case OP_CLF: return "CLF";
    case OP_JMP: return "JMP";
    case OP_INV: return "INV";
    case OP_RET: return "RET";
    case OP_SYS: return "SYS";
    case OP_MAX: return "<MAX>";
    default:
      return "<?>";
  }
}

const char * svm_ext2str(svm_ext_t ext, bool dot) {
  switch (ext) {
    case EXT_NONE: return "";
    case EXT_EQ:   return dot ? ".EQ" : "EQ";
    case EXT_NE:   return dot ? ".NE" : "NE";
    case EXT_LT:   return dot ? ".LT" : "LT";
    case EXT_LE:   return dot ? ".LE" : "LE";
    case EXT_GT:   return dot ? ".GT" : "GT";
    case EXT_GE:   return dot ? ".GE" : "GE";
    case EXT_NZ:   return dot ? ".NZ" : "NZ";
    case EXT_Z:    return dot ? ".Z"  : "Z";
    case EXT_MAX:  return "<MAX>";
    default:
      return dot ? ".?" : "?";
  }
}

const char * svm_register2str(svm_register_t reg) {
  switch (reg) {
    case R0:    return "R0";
    case R1:    return "R1";
    case R2:    return "R2";
    case R3:    return "R3";
    case R4:    return "R4";
    case R5:    return "R5";
    case R6:    return "R6";
    case R7:    return "R7";
    case R8:    return "R8";
    case R9:    return "R9";
    case R10:   return "R10";
    case R11:   return "R11";
    case R12:   return "R12";
    case R13:   return "R13";
    case R14:   return "R14";
    case R15:   return "R15";
    case R_MAX: return "<MAX>";
    default:
      return "R?";
  }
}

const char * svm_arg2str(svm_arg_type_t type) {
  switch (type) {
    case ARG_NONE: return "<NONE>";
    case ARG_R0:   return "R0";
    case ARG_R1:   return "R1";
    case ARG_R2:   return "R2";
    case ARG_R3:   return "R3";
    case ARG_R4:   return "R4";
    case ARG_R5:   return "R5";
    case ARG_R6:   return "R6";
    case ARG_R7:   return "R7";
    case ARG_R8:   return "R8";
    case ARG_R9:   return "R9";
    case ARG_R10:  return "R10";
    case ARG_R11:  return "R11";
    case ARG_R12:  return "R12";
    case ARG_R13:  return "R13";
    case ARG_R14:  return "R14";
    case ARG_R15:  return "R15";
    case ARG_IMM:  return "IMM";
    case ARG_MAX:  return "<MAX>";
    default:
      return "<?>";
  }
}

const char * svm_get_arg_str(int32_t * code, uint32_t index, svm_arg_type_t type, bool has_prev_arg) {
  SVM_ASSERT_RETURN(code, NULL);

  static char buffer[16] = {0};

  switch (type) {
    case ARG_NONE: return "";
    // TODO: Use svm_arg2str
    case ARG_R0:   return "R0";
    case ARG_R1:   return "R1";
    case ARG_R2:   return "R2";
    case ARG_R3:   return "R3";
    case ARG_R4:   return "R4";
    case ARG_R5:   return "R5";
    case ARG_R6:   return "R6";
    case ARG_R7:   return "R7";
    case ARG_R8:   return "R8";
    case ARG_R9:   return "R9";
    case ARG_R10:  return "R10";
    case ARG_R11:  return "R11";
    case ARG_R12:  return "R12";
    case ARG_R13:  return "R13";
    case ARG_R14:  return "R14";
    case ARG_R15:  return "R15";
    case ARG_IMM:
      sprintf(buffer, "%d", code[index + (has_prev_arg ? 1 : 0)]);
      return buffer;
    default:
      return "?";
  }
}

svm_register_t svm_arg_to_reg(svm_arg_type_t arg) {
  switch (arg) {
    case ARG_R0:  return R0;
    case ARG_R1:  return R1;
    case ARG_R2:  return R2;
    case ARG_R3:  return R3;
    case ARG_R4:  return R4;
    case ARG_R5:  return R5;
    case ARG_R6:  return R6;
    case ARG_R7:  return R7;
    case ARG_R8:  return R8;
    case ARG_R9:  return R9;
    case ARG_R10: return R10;
    case ARG_R11: return R11;
    case ARG_R12: return R12;
    case ARG_R13: return R13;
    case ARG_R14: return R14;
    case ARG_R15: return R15;
    default:
      return R_MAX;
  }
}

svm_opcode_t svm_str2opcode(const char * str) {
  SVM_ASSERT_RETURN(str, OP_MAX);

  if (!strcmp(str, "nop")) {
    return OP_NOP;
  } else if (!strcmp(str, "end")) {
    return OP_END;
  } else if (!strcmp(str, "mov")) {
    return OP_MOV;
  } else if (!strcmp(str, "add")) {
    return OP_ADD;
  } else if (!strcmp(str, "sub")) {
    return OP_SUB;
  } else if (!strcmp(str, "mul")) {
    return OP_MUL;
  } else if (!strcmp(str, "div")) {
    return OP_DIV;
  } else if (!strcmp(str, "cmp")) {
    return OP_CMP;
  } else if (!strcmp(str, "clf")) {
    return OP_CLF;
  } else if (!strcmp(str, "jmp")) {
    return OP_JMP;
  } else if (!strcmp(str, "inv")) {
    return OP_INV;
  } else if (!strcmp(str, "ret")) {
    return OP_RET;
  } else if (!strcmp(str, "sys")) {
    return OP_SYS;
  } else {
    return OP_MAX;
  }
}

svm_arg_type_t svm_str2arg(const char * str) {
  SVM_ASSERT_RETURN(str, ARG_MAX);

  if (!strcmp(str, "r0")) {
    return ARG_R0;
  } else if (!strcmp(str, "r1")) {
    return ARG_R1;
  } else if (!strcmp(str, "r2")) {
    return ARG_R2;
  } else if (!strcmp(str, "r3")) {
    return ARG_R3;
  } else if (!strcmp(str, "r4")) {
    return ARG_R4;
  } else if (!strcmp(str, "r5")) {
    return ARG_R5;
  } else if (!strcmp(str, "r6")) {
    return ARG_R6;
  } else if (!strcmp(str, "r7")) {
    return ARG_R7;
  } else if (!strcmp(str, "r8")) {
    return ARG_R8;
  } else if (!strcmp(str, "r9")) {
    return ARG_R9;
  } else if (!strcmp(str, "r10")) {
    return ARG_R10;
  } else if (!strcmp(str, "r11")) {
    return ARG_R11;
  } else if (!strcmp(str, "r12")) {
    return ARG_R12;
  } else if (!strcmp(str, "r13")) {
    return ARG_R13;
  } else if (!strcmp(str, "r14")) {
    return ARG_R14;
  } else if (!strcmp(str, "r15")) {
    return ARG_R15;
  } else {
    return ARG_IMM;
  }
}

svm_ext_t svm_str2ext(const char * str) {
  SVM_ASSERT_RETURN(str, EXT_MAX);

  if (!strcmp(str, "eq")) {
    return EXT_EQ;
  } else if (!strcmp(str, "ne")) {
    return EXT_NE;
  } else if (!strcmp(str, "lt")) {
    return EXT_LT;
  } else if (!strcmp(str, "le")) {
    return EXT_LE;
  } else if (!strcmp(str, "gt")) {
    return EXT_GT;
  } else if (!strcmp(str, "ge")) {
    return EXT_GE;
  } else if (!strcmp(str, "nz")) {
    return EXT_NZ;
  } else if (!strcmp(str, "z")) {
    return EXT_Z;
  } else {
    return EXT_NONE;
  }
}

bool svm_to_int32(const char * str, int32_t * value) {
  SVM_ASSERT_RETURN(str && value, false);

  *value = 0;

  uint32_t size = strlen(str);
  uint32_t index = 0;
  int32_t  base = 10;

  if (size > 2) {
    if (str[index] == '0') {
      if (str[index+1] == 'x') {
        index += 2;
        base = 16;
      } else if (str[index+1] == 'b') {
        index += 2;
        base = 2;
      }
    }
  }

  for (; index < size; ++index) {
    int32_t digit = 0;

    if (str[index] >= '0' && str[index] < '9') {
      digit = str[index] - '0';
    } else if (str[index] >= 'a' && str[index] <= 'f') {
      digit = str[index] - 'a';
    } else if (str[index] >= 'A' && str[index] <= 'F') {
      if (isupper(str[index])) {
        digit = str[index] - 'A';
      }
    } else {
      return false;
    }

    *value = *value * base + digit;
  }

  return true;
}
