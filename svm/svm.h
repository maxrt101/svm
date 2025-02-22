
/** ========================================================================= *
 *
 * @file svm.h
 * @date 01-11-2024
 * @author Maksym Tkachuk <max.r.tkachuk@gmail.com>
 *
 *  ========================================================================= */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ================================================================= */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Defines ================================================================== */
/**
 * Provides definition for __PACKED if not available
 *
 * @note Defined here, instead of svm_util.h, because svm_util.h depends on
 *       this header
 */
#ifndef __PACKED
#define __PACKED __attribute__((packed))
#endif

/**
 * Provides definition for __WEAK if not available
 *
 * @note Defined here, instead of svm_util.h, because svm_util.h depends on
 *       this header
 */
#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

/**
 * Provides definition for call stack initial size, if not provided
 */
#ifndef SVM_CALL_STACK_INIT_SIZE
#define SVM_CALL_STACK_INIT_SIZE 8
#endif

/**
 * Provides definition for stack initial size, if not provided
 */
#ifndef SVM_STACK_INIT_SIZE
#define SVM_STACK_INIT_SIZE 32
#endif

/**
 * Provides definition for max tasks initial size, if not provided
 */
#ifndef SVM_MAX_TASKS
#define SVM_MAX_TASKS 4
#endif

/* Macros =================================================================== */
/* Enums ==================================================================== */
/**
 * Registers
 *
 * SVM has 16 general purpose registers (r0-r15) all can be used by user code,
 * none have a special usage
 */
typedef enum {
  R0 = 0,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  R14,
  R15,

  R_MAX,
} svm_register_t;

/**
 * Opcodes
 */
typedef enum {
  OP_NOP = 0, /** NoOperation - does nothing */

  OP_END,     /** Terminates execution */
  OP_MOV,     /** Move value to register / register to register */
  OP_PUSH,    /** Push specified register range (or immediate value) to stack */
  OP_POP,     /** Pop register range from stack */

  OP_ADD,     /** Adds 2 operands, result stored in first */
  OP_SUB,     /** Subtracts 2 operands, result stored in first */
  OP_MUL,     /** Multiplies 2 operands, result stored in first */
  OP_DIV,     /** Divides 2 operands, result stored in first */
  OP_AND,     /** Performs binary AND */
  OP_OR,      /** Performs binary OR */
  OP_XOR,     /** Performs binary XOR */
  OP_SHL,     /** Performs left shift */
  OP_SHR,     /** Performs right shift */

  OP_CMP,     /** Compare values, sets appropriate flags for conditional execution */
  OP_CLF,     /** Clears specified flag, or all */

  OP_JMP,     /** Jump to address */
  OP_INV,     /** Invoke. Jump to address, and push PC to call stack */
  OP_RET,     /** Restore most recent PC from call stack */

  OP_SYS,     /** System Call (handled by svm_port_sys) */

  OP_MAX      /** Special marker to get count of instructions */
} svm_opcode_t;

/**
 * Instruction extensions
 */
typedef enum {
  EXT_NONE = 0, /** No extension flag */

  EXT_EQ,       /** Executes instruction only if EQ flag is set */
  EXT_NE,       /** Executes instruction only if NE flag is set */
  EXT_LT,       /** Executes instruction only if LT flag is set */
  EXT_LE,       /** Executes instruction only if LE flag is set */
  EXT_GT,       /** Executes instruction only if GT flag is set */
  EXT_GE,       /** Executes instruction only if GE flag is set */
  EXT_NZ,       /** Executes instruction only if NZ flag is set */
  EXT_Z,        /** Executes instruction only if Z flag is set */

  EXT_MAX       /** Special marker to get count of extension flags */
} svm_ext_t;

/**
 * Arguments
 */
typedef enum {
  ARG_NONE = 0, /** No argument is present */
  ARG_R0,
  ARG_R1,
  ARG_R2,
  ARG_R3,
  ARG_R4,
  ARG_R5,
  ARG_R6,
  ARG_R7,
  ARG_R8,
  ARG_R9,
  ARG_R10,
  ARG_R11,
  ARG_R12,
  ARG_R13,
  ARG_R14,
  ARG_R15,
  ARG_IMM,      /** Immediate value is present in next i32 */

  ARG_MAX       /** Special marker to get count of args */
} svm_arg_type_t;

/**
 * SVM Core Error Codes
 */
typedef enum {
  SVM_OK = 0,                   /** OK */
  SVM_ERR,                      /** Generic Error */
  SVM_ERR_NULL,                 /** Something non-nullable is NULL */
  SVM_ERR_BAD_ALLOC,            /** Allocation error */
  SVM_ERR_NOT_RUNNING,          /** Runtime Not Running */
  SVM_ERR_CODE_OVERFLOW,        /** Overflow in code */
  SVM_ERR_ARG_NOT_REG,          /** Expected register */
  SVM_ERR_PUSH_ARG_BAD_ORDER,   /** Higher register is first argument to push */
  SVM_ERR_JMP_OVERFLOW,         /** Overflow in code during jump */
  SVM_ERR_CALL_STK_OVERFLOW,    /** Overflow in call stack */
  SVM_ERR_CALL_STK_UNDERFLOW,   /** Underflow in call stack */
  SVM_ERR_STK_OVERFLOW,         /** Overflow in stack */
  SVM_ERR_STK_UNDERFLOW,        /** Underflow in stack */
  SVM_ERR_TASK_NOT_FOUND,       /** Requested task not found */
  SVM_ERR_TASK_SWITCH_BLOCKED,  /** Task switching requested, but it's blocked externally */
  SVM_ERR_UNKNOWN_INSTRUCTION,  /** Unknown instruction */
} svm_error_t;

/* Types ==================================================================== */
/**
 * Buffer of int32_t
 */
typedef struct {
  int32_t * buffer;
  uint32_t  size;
} svm_i32_buffer_t;

/**
 * Instruction
 *
 * TODO: Optimize size
 */
typedef struct __PACKED {
  svm_opcode_t op     : 8;
  svm_ext_t ext       : 8;
  svm_arg_type_t arg1 : 8;
  svm_arg_type_t arg2 : 8;
} svm_instruction_t;


/**
 * SVM Compiler code block
 */
typedef struct {
  int32_t * buffer;
  uint32_t  size;

  struct {
    uint32_t call_stack_size;
    uint32_t stack_size;
  } meta;
} svm_code_t;

/**
 * Single task (thread) execution context
 */
typedef struct svm_task_t {
  struct svm_task_t * next;     /** Next task (NULL if last) */

  struct __PACKED {
    bool eq       : 1;          /** Equality flag */
    bool ne       : 1;          /** Not Equal flag */
    bool lt       : 1;          /** Less Than flag */
    bool le       : 1;          /** Less Equals flag */
    bool gt       : 1;          /** Greater Than flag */
    bool ge       : 1;          /** Greater Equals flag */
    bool nz       : 1;          /** Not Zero flag */
    bool z        : 1;          /** Zero flag */
  } flags;

  uint32_t pc;                  /** Program Counter (index into code) */
  uint32_t rpc;                 /** Return Program Counter (index into call_stack) */
  uint32_t sp;                  /** Stack pointer (index into stack) */
  int32_t registers[R_MAX];     /** Registers */

  svm_i32_buffer_t stack;       /** Program Stack */
  svm_i32_buffer_t call_stack;  /** Call Stack */
} svm_task_t;

/**
 * SVM Runtime Context
 */
typedef struct {
  struct __PACKED {
    bool running           : 1; /** Is VM running flag */
    bool task_switch_block : 1; /** Block task switching */
  } flags;

  struct {
    svm_task_t * current;
    svm_task_t * head;
  } task;

  svm_code_t * code;            /** Executable code context */

  void * ctx;                   /** User context for svm_sys_port */
} svm_t;

/* Variables ================================================================ */
/* Shared functions ========================================================= */
/**
 * Initialize SVM context
 *
 * @param vm SVM Context
 * @param ctx User context for port functions
 *
 * @retval SVM_OK If operation completed successfully
 * @retval SVM_ERR_NULL If pointer to vm is NULL
 */
svm_error_t svm_init(svm_t * vm, void * ctx);

/**
 * De-initialize SVM context
 *
 * @param vm SVM Context
 *
 * @retval SVM_OK If operation completed successfully
 * TODO: describe retvals
 */
svm_error_t svm_deinit(svm_t * vm);

/**
 * Load instruction buffer into the VM
 *
 * @param vm SVM Context
 * @param code SVM Compiler code context
 *
 * @retval SVM_OK If operation completed successfully
 * @retval SVM_ERR_NULL If pointer to vm or buffer is NULL, or size is 0
 */
svm_error_t svm_load(svm_t * vm, svm_code_t * code);

/**
 * Unload instructions & tasks from VM
 *
 * @param vm SVM Context
 *
 * @retval SVM_OK If operation completed successfully
 * TODO: describe retvals
 */
svm_error_t svm_unload(svm_t * vm);

/**
 * Run VM for 1 cycle
 *
 * @param vm SVM Context
 *
 * @retval SVM_OK If operation completed successfully
 * @retval SVM_ERR_NULL If pointer to vm is NULL
 * @retval SVM_ERR_NOT_RUNNING If vm was stopped (due to error or end of buffer)
 * @retval SVM_ERR_CODE_OVERFLOW If PC reached of the instruction buffer
 * @retval SVM_ERR_ARG_NOT_REG Argument to current instruction must be a
 *                             register, but it's not
 * @retval SVM_ERR_JMP_OVERFLOW JMP or INV instruction tried to jump outside of
 *                              code buffer bounds
 * @retval SVM_ERR_CALL_STK_OVERFLOW Can't invoke, call stack is full
 * @retval SVM_ERR_CALL_STK_UNDERFLOW Can't return, call stack is empty
 * @retval SVM_ERR_UNKNOWN_INSTRUCTION Unknown instruction
 */
svm_error_t svm_cycle(svm_t * vm);

/**
 * Initialize task context
 *
 * @param vm SVM instance
 * @param task Task instance
 * @param pc PC where task should start it's execution
 * @param registers Registers state that task is expecting at start
 */
svm_error_t svm_task_init(svm_t * vm, svm_task_t * task, uint32_t pc, int32_t (*registers)[R_MAX]);

/**
 * De-Initialize task context and release all resources
 *
 * @param task Task instance
 */
svm_error_t svm_task_deinit(svm_task_t * task);

/**
 * Create task in VM context
 *
 * @param vm SVM instance
 * @param pc PC where task should start it's execution
 * @param registers Registers state that task is expecting at start
 */
svm_error_t svm_task_create(svm_t * vm, uint32_t pc, int32_t (*registers)[R_MAX]);

/**
 * Remove task from VM context
 *
 * @param vm SVM instance
 * @param task Task instance to be removed
 */
svm_error_t svm_task_remove(svm_t * vm, svm_task_t * task);

/**
 * Perform task switch
 *
 * @param vm SVM instance
 */
svm_error_t svm_task_switch(svm_t * vm);

/**
 * Block/Unblock task switching
 *
 * @param vm SVM instance
 * @param block Task switch blocking state
 */
svm_error_t svm_task_block(svm_t * vm, bool block);

/**
 * Packs instruction from it's contents
 *
 * @param op Opcode
 * @param ext Instruction Extension
 * @param arg1 Argument 1 type
 * @param arg2 Argument 2 type
 */
svm_instruction_t svm_pack_instruction(
    svm_opcode_t op,
    svm_ext_t ext,
    svm_arg_type_t arg1,
    svm_arg_type_t arg2
);

/**
 * Packs instruction into int32_t
 *
 * @param instruction Instruction
 */
int32_t svm_instruction_to_int32(svm_instruction_t instruction);

/**
 * Disassembles from buffer to buffer + size
 *
 * @param buffer Instruction buffer
 * @param size Size to disassemble
 */
void svm_disassemble(int32_t * buffer, uint32_t size);

/**
 * Re-definable malloc wrapper
 *
 * @note Calls libc malloc by default
 *
 * @param size Size in bytes of requested buffer
 *
 * @returns void pointer to allocated buffer
 * @retval NULL if allocation failed
 */
void * svm_malloc(size_t size);

/**
 * Re-definable free wrapper
 *
 * @note Calls libc free by default
 *
 * @param buffer Buffer, previously allocated with svm_malloc, to be freed
 */
void svm_free(void * buffer);

/**
  * Re-definable realloc wrapper
 *
 * @note Calls libc realloc by default
 *
 * @param buffer Buffer, previously allocated with svm_malloc, to be reallocated
 * @param size Size in bytes of requested buffer
 *
 * @returns void pointer to allocated buffer
 * @retval NULL if allocation failed
 */
void * svm_realloc(void * buffer, size_t size);

/**
 * Port for syscalls
 *
 * @param ctx User context
 * @param registers Registers array
 * @param syscall_num Syscall number
 */
void svm_sys_handler(void * ctx, int32_t (*registers)[R_MAX], int32_t syscall_num);

#ifdef __cplusplus
}
#endif
