
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
#include <stdlib.h>
#include <string.h>

/* Defines ================================================================== */
/* Macros =================================================================== */
/* Exposed macros =========================================================== */
/* Enums ==================================================================== */
/* Types ==================================================================== */
/* Variables ================================================================ */
/* Private functions ======================================================== */
static bool svm_is_arg_register(svm_arg_type_t type) {
  return type > ARG_NONE && type < ARG_IMM;
}

static int32_t svm_get_arg_value(svm_t * vm, svm_arg_type_t type) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  if (svm_is_arg_register(type)) {
    svm_register_t reg = svm_arg_to_reg(type);
    return reg < R_MAX ? vm->task.current->registers[reg] : 0;
  } else if (type == ARG_IMM) {
    return vm->code->buffer[vm->task.current->pc++];
  } else {
    // TODO: Signal error
    return 0;
  }
}

static bool svm_check_condition(svm_t * vm, svm_ext_t ext) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  switch (ext) {
    case EXT_NONE: return true;
    case EXT_EQ:   return vm->task.current->flags.eq;
    case EXT_NE:   return vm->task.current->flags.ne;
    case EXT_LT:   return vm->task.current->flags.lt;
    case EXT_LE:   return vm->task.current->flags.le;
    case EXT_GT:   return vm->task.current->flags.gt;
    case EXT_GE:   return vm->task.current->flags.ge;
    case EXT_NZ:   return vm->task.current->flags.nz;
    case EXT_Z:    return vm->task.current->flags.z;
    default:
      return false;
  }
}

static void svm_check_value_set_nz_z_flags(svm_t * vm, int32_t value) {
  SVM_ASSERT_RETURN(vm);

  if (value) {
    vm->task.current->flags.nz = true;
  } else {
    vm->task.current->flags.z = true;
  }
}

static int svm_set_flag_by_ext(svm_t * vm, svm_ext_t ext, bool value) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  switch (ext) {
    case EXT_NONE:
      vm->task.current->flags.eq = value;
      vm->task.current->flags.ne = value;
      vm->task.current->flags.lt = value;
      vm->task.current->flags.le = value;
      vm->task.current->flags.gt = value;
      vm->task.current->flags.ge = value;
      vm->task.current->flags.nz = value;
      vm->task.current->flags.z = value;
      break;

    case EXT_EQ:
      vm->task.current->flags.eq = value;
      break;

    case EXT_NE:
      vm->task.current->flags.ne = value;
      break;

    case EXT_LT:
      vm->task.current->flags.lt = value;
      break;

    case EXT_LE:
      vm->task.current->flags.le = value;
      break;

    case EXT_GT:
      vm->task.current->flags.gt = value;
      break;

    case EXT_GE:
      vm->task.current->flags.ge = value;
      break;

    case EXT_NZ:
      vm->task.current->flags.nz = value;
      break;

    case EXT_Z:
      vm->task.current->flags.z = value;
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

svm_error_t svm_deinit(svm_t * vm) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  return svm_unload(vm);
}

svm_error_t svm_load(svm_t * vm, svm_code_t * code) {
  SVM_ASSERT_RETURN(vm && code, SVM_ERR_NULL);

  vm->code = code;

  int32_t registers[R_MAX] = {0};
  SVM_ERROR_CHECK_RETURN(svm_task_create(vm, 0, &registers));

  vm->flags.running = true;

  // Perform first switch to set task->current
  return svm_task_switch(vm);
}

svm_error_t svm_unload(svm_t * vm) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  // TODO: release all allocated resources

  return SVM_OK;
}

svm_error_t svm_cycle(svm_t * vm) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  if (!vm->flags.running) {
    return SVM_ERR_NOT_RUNNING;
  }

  if (vm->task.current->pc >= vm->code->size) {
    vm->flags.running = false;
    return SVM_ERR_CODE_OVERFLOW;
  }

  svm_instruction_t * instruction = (svm_instruction_t *) &vm->code->buffer[vm->task.current->pc++];

#if USE_SVM_DEBUG_CYCLE
  printf(
      "%04x | %-3s%-3s %-10s %-10s\n",
      vm->task.current->pc - 1,
      svm_opcode2str(instruction->op),
      svm_ext2str(instruction->ext, true),
      svm_get_arg_str(vm->code->buffer, vm->task.current->pc, instruction->arg1, false),
      svm_get_arg_str(vm->code->buffer, vm->task.current->pc, instruction->arg2, instruction->arg1 == ARG_IMM)
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

      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      svm_register_t reg = svm_arg_to_reg(instruction->arg1);
      SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
      vm->task.current->registers[reg] = arg2;
      svm_check_value_set_nz_z_flags(vm, vm->task.current->registers[reg]);

      break;
    }

    case OP_PUSH: {
      int32_t arg1 = 0;

      // If arg1 is immediate, load it's value from code buffer
      if (instruction->arg1 == ARG_IMM) {
        arg1 = svm_get_arg_value(vm, instruction->arg1);
      }

      // If condition flag isn't set - return
      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      // If arg1 is immediate, push single value to stack
      if (instruction->arg1 == ARG_IMM) {
        if (vm->task.current->sp + 1 >= vm->task.current->stack.size) {
          return SVM_ERR_STK_OVERFLOW;
        }
        vm->task.current->stack.buffer[vm->task.current->sp++] = arg1;
        break;
      }

      // If there is no arg2, only need to push 1 register
      if (instruction->arg2 == ARG_NONE) {
        register_t reg = svm_arg_to_reg(instruction->arg1);

        if (vm->task.current->sp + 1 >= vm->task.current->stack.size) {
          return SVM_ERR_STK_OVERFLOW;
        }

        vm->task.current->stack.buffer[vm->task.current->sp++] = vm->task.current->registers[reg];
        break;
      }

      // Push range (arg1-arg2)
      register_t from = svm_arg_to_reg(instruction->arg1), to = svm_arg_to_reg(instruction->arg2);
      SVM_ASSERT_RETURN(from < to, SVM_ERR_PUSH_ARG_BAD_ORDER);

      if (vm->task.current->sp + to - from >= vm->task.current->stack.size) {
        return SVM_ERR_STK_OVERFLOW;
      }

      for (register_t r = from; r <= to; r++) {
        vm->task.current->stack.buffer[vm->task.current->sp++] = vm->task.current->registers[r];
      }

      break;
    }

    case OP_POP: {
      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      if (svm_is_arg_register(instruction->arg1) && instruction->arg2 == ARG_NONE) {
        if (vm->task.current->sp < 1) {
          return SVM_ERR_STK_UNDERFLOW;
        }

        register_t reg = svm_arg_to_reg(instruction->arg1);

        vm->task.current->registers[reg] = vm->task.current->stack.buffer[vm->task.current->sp--];
        break;
      }

      SVM_ASSERT_RETURN(svm_is_arg_register(instruction->arg1), SVM_ERR_ARG_NOT_REG);
      SVM_ASSERT_RETURN(svm_is_arg_register(instruction->arg2), SVM_ERR_ARG_NOT_REG);

      register_t from = svm_arg_to_reg(instruction->arg1), to = svm_arg_to_reg(instruction->arg2);
      SVM_ASSERT_RETURN(from < to, SVM_ERR_PUSH_ARG_BAD_ORDER);

      if (vm->task.current->sp < to - from) {
        return SVM_ERR_STK_UNDERFLOW;
      }

      for (register_t r = to; r >= from ; r--) {
        vm->task.current->registers[r] = vm->task.current->stack.buffer[--vm->task.current->sp];
      }

      break;
    }

    case OP_ADD: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);

      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      svm_register_t reg = svm_arg_to_reg(instruction->arg1);
      SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
      vm->task.current->registers[reg] += arg2;
      svm_check_value_set_nz_z_flags(vm, vm->task.current->registers[reg]);

      break;
    }

    case OP_SUB: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);

      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      svm_register_t reg = svm_arg_to_reg(instruction->arg1);
      SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
      vm->task.current->registers[reg] -= arg2;
      svm_check_value_set_nz_z_flags(vm, vm->task.current->registers[reg]);

      break;
    }

    case OP_MUL: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);

      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      svm_register_t reg = svm_arg_to_reg(instruction->arg1);
      SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
      vm->task.current->registers[reg] *= arg2;
      svm_check_value_set_nz_z_flags(vm, vm->task.current->registers[reg]);

      break;
    }

    case OP_DIV: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);

      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      svm_register_t reg = svm_arg_to_reg(instruction->arg1);
      SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
      vm->task.current->registers[reg] /= arg2;
      svm_check_value_set_nz_z_flags(vm, vm->task.current->registers[reg]);

      break;
    }

    case OP_AND: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);

      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      svm_register_t reg = svm_arg_to_reg(instruction->arg1);
      SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
      vm->task.current->registers[reg] &= arg2;
      svm_check_value_set_nz_z_flags(vm, vm->task.current->registers[reg]);

      break;
    }

    case OP_OR: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);

      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      svm_register_t reg = svm_arg_to_reg(instruction->arg1);
      SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
      vm->task.current->registers[reg] |= arg2;
      svm_check_value_set_nz_z_flags(vm, vm->task.current->registers[reg]);

      break;
    }

    case OP_XOR: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);

      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      svm_register_t reg = svm_arg_to_reg(instruction->arg1);
      SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
      vm->task.current->registers[reg] ^= arg2;
      svm_check_value_set_nz_z_flags(vm, vm->task.current->registers[reg]);

      break;
    }

    case OP_SHL: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);

      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      svm_register_t reg = svm_arg_to_reg(instruction->arg1);
      SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
      vm->task.current->registers[reg] <<= arg2;
      svm_check_value_set_nz_z_flags(vm, vm->task.current->registers[reg]);

      break;
    }

    case OP_SHR: {
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);

      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      svm_register_t reg = svm_arg_to_reg(instruction->arg1);
      SVM_ASSERT_RETURN(reg < R_MAX, SVM_ERR_ARG_NOT_REG);
      vm->task.current->registers[reg] >>= arg2;
      svm_check_value_set_nz_z_flags(vm, vm->task.current->registers[reg]);

      break;
    }

    case OP_CMP: {
      // TODO: svm_set_flag_by_ext(vm, EXT_NONE, false);
      int32_t arg1 = svm_get_arg_value(vm, instruction->arg1);
      int32_t arg2 = svm_get_arg_value(vm, instruction->arg2);

      if (arg1 == arg2) {
        vm->task.current->flags.eq = true;
      }

      if (arg1 != arg2) {
        vm->task.current->flags.ne = true;
      }

      if (arg1 > arg2) {
        vm->task.current->flags.gt = true;
      }

      if (arg1 >= arg2) {
        vm->task.current->flags.ge = true;
      }

      if (arg1 < arg2) {
        vm->task.current->flags.lt = true;
      }

      if (arg1 <= arg2) {
        vm->task.current->flags.le = true;
      }

      break;
    }

    case OP_CLF: {
      svm_set_flag_by_ext(vm, instruction->ext, false);
      break;
    }

    case OP_JMP: {
      int32_t arg1 = svm_get_arg_value(vm, instruction->arg1);

      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      if (arg1 < vm->code->size) {
        vm->task.current->pc = arg1;
      } else {
        return SVM_ERR_JMP_OVERFLOW;
      }

      break;
    }

    case OP_INV: {
      int32_t arg1 = svm_get_arg_value(vm, instruction->arg1);

      if (!svm_check_condition(vm, instruction->ext)) {
        break;
      }

      if (vm->task.current->rpc + 1 >= vm->task.current->call_stack.size) {
        return SVM_ERR_CALL_STK_OVERFLOW;
      }

      vm->task.current->call_stack.buffer[vm->task.current->rpc++] = vm->task.current->pc;

      if (arg1 < vm->code->size) {
        vm->task.current->pc = arg1;
      } else {
        return SVM_ERR_JMP_OVERFLOW;
      }

      break;
    }

    case OP_RET: {
      if (vm->task.current->rpc > 0) {
        vm->task.current->pc = vm->task.current->call_stack.buffer[--vm->task.current->rpc];
      } else {
        return SVM_ERR_CALL_STK_UNDERFLOW;
      }
      break;
    }

    case OP_SYS: {
      svm_sys_handler(vm->ctx, &vm->task.current->registers, svm_get_arg_value(vm, instruction->arg1));
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
        vm->task.current->registers[reg]
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

svm_error_t svm_task_init(svm_t * vm, svm_task_t * task, uint32_t pc, int32_t (*registers)[R_MAX]) {
  SVM_ASSERT_RETURN(vm && task && registers, SVM_ERR_NULL);

  memset(task, 0, sizeof(*task));

  task->pc = pc;
  memcpy(task->registers, registers, sizeof(*registers));

  task->call_stack.size = vm->code->meta.call_stack_size ? vm->code->meta.call_stack_size : SVM_CALL_STACK_INIT_SIZE;
  SVM_REALLOC_CHECK(task->call_stack.buffer, task->call_stack.size);

  task->stack.size = vm->code->meta.stack_size ? vm->code->meta.stack_size : SVM_STACK_INIT_SIZE;
  SVM_REALLOC_CHECK(task->stack.buffer, task->stack.size);

  return SVM_OK;
}

svm_error_t svm_task_deinit(svm_task_t * task) {
  SVM_ASSERT_RETURN(task, SVM_ERR_NULL);

  if (task->call_stack.buffer) {
    svm_free(task->call_stack.buffer);
  }

  if (task->stack.buffer) {
    svm_free(task->stack.buffer);
  }

  return SVM_OK;
}

svm_error_t svm_task_create(svm_t * vm, uint32_t pc, int32_t (*registers)[R_MAX]) {
  SVM_ASSERT_RETURN(vm && registers, SVM_ERR_NULL);

  svm_task_t * task = svm_malloc(sizeof(svm_task_t));

  SVM_ASSERT_RETURN(task, SVM_ERR_BAD_ALLOC);

  SVM_ERROR_CHECK_RETURN(svm_task_init(vm, task, pc, registers));

  if (vm->task.head) {
    svm_task_t * tmp = vm->task.head;

    while (tmp->next) {
      tmp = tmp->next;
    }

    tmp->next = task;
  } else {
    vm->task.head = task;
  }

  return SVM_OK;
}

svm_error_t svm_task_remove(svm_t * vm, svm_task_t * task) {
  SVM_ASSERT_RETURN(vm && task, SVM_ERR_NULL);

  if (vm->task.head == task) {
    vm->task.head = vm->task.head->next;
  } else {
    svm_task_t * tmp = vm->task.head->next;

    while (tmp) {
      if (task->next == task) {
        break;
      }
      tmp = tmp->next;
    }

    SVM_ASSERT_RETURN(tmp, SVM_ERR_TASK_NOT_FOUND);

    tmp->next = task->next;
  }

  return svm_task_deinit(task);
}

svm_error_t svm_task_switch(svm_t * vm) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  SVM_ASSERT_RETURN(!vm->flags.task_switch_block, SVM_ERR_TASK_SWITCH_BLOCKED);

  if (!vm->task.current) {
    vm->task.current = vm->task.head;
    return SVM_OK;
  }

  vm->task.current = vm->task.current->next;

  if (!vm->task.current) {
    vm->task.current = vm->task.head;
  }

  return SVM_OK;
}

svm_error_t svm_task_block(svm_t * vm, bool block) {
  SVM_ASSERT_RETURN(vm, SVM_ERR_NULL);

  vm->flags.task_switch_block = block;

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

__WEAK void * svm_malloc(size_t size) {
  return malloc(size);
}

__WEAK void svm_free(void * buffer) {
  free(buffer);
}

__WEAK void * svm_realloc(void * buffer, size_t size) {
  return realloc(buffer, size);
}

__WEAK void svm_sys_handler(void * ctx, int32_t (*registers)[R_MAX], int32_t syscall_num) {
  // Does nothing
}