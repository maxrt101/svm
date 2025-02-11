
/** ========================================================================= *
 *
 * @file svm.c
 * @date 01-11-2024
 * @author Maksym Tkachuk <max.r.tkachuk@gmail.com>
 *
 *  ========================================================================= */

/* Includes ================================================================= */
#include "svm.h"
#include "svm_util.h"
#include <stdio.h>
#include <string.h>

/* Defines ================================================================== */
/* Macros =================================================================== */
/* Exposed macros =========================================================== */
/* Enums ==================================================================== */
/* Types ==================================================================== */
/* Variables ================================================================ */
/* Private functions ======================================================== */
static int32_t svm_get_arg_value(svm_t * vm, svm_arg_type_t type) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  if (type > ARG_NONE && type < ARG_IMM) {
    svm_register_t reg = svm_arg_to_reg(type);
    return reg < R_MAX ? vm->registers[reg] : 0;
  } else if (type == ARG_IMM) {
    return vm->code.buffer[vm->pc++];
  } else {
    // TODO: Signal error
    return 0;
  }
}

static bool svm_check_condition(svm_t * vm, svm_ext_t ext) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  switch (ext) {
    case EXT_NONE: return true;
    case EXT_EQ:   return vm->flags.eq;
    case EXT_NE:   return vm->flags.ne;
    case EXT_LT:   return vm->flags.lt;
    case EXT_LE:   return vm->flags.le;
    case EXT_GT:   return vm->flags.gt;
    case EXT_GE:   return vm->flags.ge;
    case EXT_NZ:   return vm->flags.nz;
    case EXT_Z:    return vm->flags.z;
    default:
      return false;
  }
}

static void svm_check_value_set_nz_z_flags(svm_t * vm, int32_t value) {
  SVM_ASSERT_RETURN(vm);

  if (value) {
    vm->flags.nz = true;
  } else {
    vm->flags.z = true;
  }
}

static int svm_set_flag_by_ext(svm_t * vm, svm_ext_t ext, bool value) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  switch (ext) {
    case EXT_NONE:
      vm->flags.eq = value;
      vm->flags.ne = value;
      vm->flags.lt = value;
      vm->flags.le = value;
      vm->flags.gt = value;
      vm->flags.ge = value;
      vm->flags.nz = value;
      vm->flags.z = value;
      break;

    case EXT_EQ:
      vm->flags.eq = value;
      break;

    case EXT_NE:
      vm->flags.ne = value;
      break;

    case EXT_LT:
      vm->flags.lt = value;
      break;

    case EXT_LE:
      vm->flags.le = value;
      break;

    case EXT_GT:
      vm->flags.gt = value;
      break;

    case EXT_GE:
      vm->flags.ge = value;
      break;

    case EXT_NZ:
      vm->flags.nz = value;
      break;

    case EXT_Z:
      vm->flags.z = value;
      break;

    default:
      return 1;
  }

  return 0;
}

/* Shared functions ========================================================= */
svm_instruction_t svm_pack_instruction(
    svm_opcode_t op,
    svm_ext_t ext,
    svm_arg_type_t arg1,
    svm_arg_type_t arg2
) {
  svm_instruction_t instruction = {
      .op = op,
      .ext = ext,
      .arg1 = arg1,
      .arg2 = arg2
  };
  return instruction;
}

int32_t svm_instruction_to_int32(svm_instruction_t instruction) {
  return *(int32_t*) &instruction;
}

svm_error_t svm_init(svm_t * vm, void * ctx) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  memset(vm, 0, sizeof(*vm));

  vm->ctx = ctx;

  return SVM_OK;
}

svm_error_t svm_load(svm_t * vm, int32_t * buffer, uint32_t size) {
  SVM_ASSERT_RETURN(vm && buffer && size, SVM_ERR_NULL);

  vm->code.buffer = buffer;
  vm->code.size = size;
  vm->pc = 0;
  vm->flags.running = true;

  return SVM_OK;
}

svm_error_t svm_cycle(svm_t * vm) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  if (!vm->flags.running) {
    return SVM_ERR_NOT_RUNNING;
  }

  if (vm->pc >= vm->code.size) {
    vm->flags.running = false;
    return SVM_ERR_CODE_OVERFLOW;
  }

  svm_instruction_t * instruction = (svm_instruction_t *) &vm->code.buffer[vm->pc++];

#if USE_SVM_DEBUG_CYCLE
  printf(
      "%04x | %-3s%-3s %-10s %-10s\n",
      vm->pc - 1,
      svm_opcode2str(instruction->op),
      svm_ext2str(instruction->ext, true),
      svm_get_arg_str(vm->code.buffer, vm->pc, instruction->arg1, false),
      svm_get_arg_str(vm->code.buffer, vm->pc, instruction->arg2, instruction->arg1 == ARG_IMM)
  );
#endif

  switch (instruction->op) {
    case OP_NOP: {
      break;
    }

    case OP_END: {
      vm->flags.running = false;
      break;
    }

    case OP_MOV: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);
      if (svm_check_condition(vm, instruction->ext)) {
        svm_register_t reg = svm_arg_to_reg(instruction->arg1);
        SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
        vm->registers[reg] = arg2;
        svm_check_value_set_nz_z_flags(vm, vm->registers[reg]);
      }
      break;
    }

    case OP_ADD: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);
      if (svm_check_condition(vm, instruction->ext)) {
        svm_register_t reg = svm_arg_to_reg(instruction->arg1);
        SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
        vm->registers[reg] += arg2;
        svm_check_value_set_nz_z_flags(vm, vm->registers[reg]);
      }
      break;
    }

    case OP_SUB: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);
      if (svm_check_condition(vm, instruction->ext)) {
        svm_register_t reg = svm_arg_to_reg(instruction->arg1);
        SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
        vm->registers[reg] -= arg2;
        svm_check_value_set_nz_z_flags(vm, vm->registers[reg]);

      }
      break;
    }

    case OP_MUL: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);
      if (svm_check_condition(vm, instruction->ext)) {
        svm_register_t reg = svm_arg_to_reg(instruction->arg1);
        SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
        vm->registers[reg] *= arg2;
        svm_check_value_set_nz_z_flags(vm, vm->registers[reg]);
      }
      break;
    }

    case OP_DIV: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);
      if (svm_check_condition(vm, instruction->ext)) {
        svm_register_t reg = svm_arg_to_reg(instruction->arg1);
        SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
        vm->registers[reg] /= arg2;
        svm_check_value_set_nz_z_flags(vm, vm->registers[reg]);
      }
      break;
    }

    case OP_CMP: {
      int32_t arg1 = svm_get_arg_value(vm, instruction->arg1);
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);

      if (arg1 == arg2) {
        vm->flags.eq = true;
      }

      if (arg1 != arg2) {
        vm->flags.ne = true;
      }

      if (arg1 > arg2) {
        vm->flags.gt = true;
      }

      if (arg1 >= arg2) {
        vm->flags.ge = true;
      }

      if (arg1 < arg2) {
        vm->flags.lt = true;
      }

      if (arg1 <= arg2) {
        vm->flags.le = true;
      }

      break;
    }

    case OP_CLF: {
      svm_set_flag_by_ext(vm, instruction->ext, false);
      break;
    }

    case OP_JMP: {
      int32_t arg1 = svm_get_arg_value(vm, instruction->arg1);
      if (svm_check_condition(vm, instruction->ext)) {
        if (arg1 < vm->code.size) {
          vm->pc = arg1;
        } else {
          return SVM_ERR_JMP_OVERFLOW;
        }
      }
      break;
    }

    case OP_INV: {
      int32_t arg1 = svm_get_arg_value(vm, instruction->arg1);
      if (svm_check_condition(vm, instruction->ext)) {
        if (vm->rpc + 1 >= SVM_CALL_STACK_SIZE) {
          return SVM_ERR_CALL_STK_OVERFLOW;
        }
        vm->call_stack[vm->rpc++] = vm->pc;
        if (arg1 < vm->code.size) {
          vm->pc = arg1;
        } else {
          return SVM_ERR_JMP_OVERFLOW;
        }
      }
      break;
    }

    case OP_RET: {
      if (vm->rpc > 0) {
        vm->pc = vm->call_stack[--vm->rpc];
      } else {
        return SVM_ERR_CALL_STK_UNDERFLOW;
      }
      break;
    }

    case OP_SYS: {
      svm_port_sys(vm->ctx, &vm->registers, svm_get_arg_value(vm, instruction->arg1));
      break;
    }

    default:
      return SVM_ERR_UNKNOWN_INSTRUCTION;
  }

#if USE_SVM_DEBUG_CYCLE_PRINT_STACK
  for (svm_register_t reg = 0; reg < R_MAX; ++reg) {
    if ((reg) % 2 == 0) {
      printf("     | ");
    }
    printf(
        "%-3s %10d    ",
        svm_register2str(reg),
        vm->registers[reg]
    );
    if (reg+1 != R_MAX && (reg+1) % 2 == 0) {
      printf("\n");
    }
  }
  printf("\n");
#endif

#if USE_SVM_DEBUG_CYCLE_PRINT_FLAGS
  printf("     | ");
  for (svm_ext_t ext = EXT_NONE+1; ext < EXT_MAX; ++ext) {
    printf("%s=%d ", svm_ext2str(ext, false), svm_check_condition(vm, ext));
  }
  printf("\n     |\n");
#endif

  return SVM_OK;
}

void svm_disassemble(int32_t * buffer, uint32_t size) {
  SVM_ASSERT_RETURN(buffer && size);

  uint32_t index = 0;
  while (index < size) {
    svm_instruction_t * instruction = (svm_instruction_t *) &buffer[index++];

    printf(
        "%04x | %-3s%-3s %-10s %-10s\n",
        index - 1,
        svm_opcode2str(instruction->op),
        svm_ext2str(instruction->ext, true),
        svm_get_arg_str(buffer, index, instruction->arg1, false),
        svm_get_arg_str(buffer, index, instruction->arg2, instruction->arg1 == ARG_IMM)
    );

    if (instruction->arg1 == ARG_IMM) {
      index++;
    }

    if (instruction->arg2 == ARG_IMM) {
      index++;
    }
  }
}