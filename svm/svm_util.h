/** ========================================================================= *
 *
 * @file svm_util.h
 * @date 01-11-2024
 * @author Maksym Tkachuk <max.r.tkachuk@gmail.com>
 *
 *  ========================================================================= */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ================================================================= */
#include "svm.h"

/* Defines ================================================================== */
/* Macros =================================================================== */
/**
 * Concatenates 2 tokens
 */
#define SVM_CAT_RAW(a, b) a ## b

/**
 * Provides necessary indirection step for token concatenation
 */
#define SVM_CAT(a, b) SVM_CAT_RAW(a, b)

/**
 * Assert macro, if expr if false, return __VA_ARGS__
 */
#define SVM_ASSERT_RETURN(expr, ...) \
  do {                               \
    if (!(expr)) {                   \
      return __VA_ARGS__;            \
    }                                \
  } while (0)

/**
 * Implementation for SVM_ERROR_CHECK_RETURN
 */
#define SVM_ERROR_CHECK_RETURN_IMPL(err_var, err_type, err_ok, err_expr)  \
  do {                                                                    \
    err_type err_var = err_expr;                                          \
    if (err_var != err_ok) {                                              \
      return err_var;                                                     \
    }                                                                     \
  } while (0)

/**
 * Checks if err_expr is not SVM_OK, if not, return the error
 */
#define SVM_ERROR_CHECK_RETURN(err_expr) \
  SVM_ERROR_CHECK_RETURN_IMPL(SVM_CAT(__err, __LINE__), svm_error_t, SVM_OK, err_expr)

/**
 * Checks if err_expr is not SVM_ASM_OK, if not, return the error
 */
#define SVM_ASM_ERROR_CHECK_RETURN(err_expr) \
  SVM_ERROR_CHECK_RETURN_IMPL(SVM_CAT(__err, __LINE__), svm_asm_error_t, SVM_ASM_OK, err_expr)

/**
 * Implementation of SVM_REALLOC_CHECK
 */
#define SVM_REALLOC_CHECK_IMPL(new_buf_var, buffer, new_size)             \
  do {                                                                    \
    void * new_buf_var = realloc(buffer, new_size);                       \
    if (!new_buf_var) {                                                   \
      printf("FATAL: Allocation failed (%s:%d)", __FUNCTION__, __LINE__); \
      abort();                                                            \
    }                                                                     \
    buffer = new_buf_var;                                                 \
  } while (0)

/**
 * Provides allocation error check for realloc
 */
#define SVM_REALLOC_CHECK(buffer, new_size) \
  SVM_REALLOC_CHECK_IMPL(SVM_CAT(__buf, __LINE__), buffer, new_size)

/* Enums ==================================================================== */
/* Types ==================================================================== */
/* Variables ================================================================ */
/* Shared functions ========================================================= */
/**
 * Converts opcode to string
 *
 * @param op Opcode
 */
const char * svm_opcode2str(svm_opcode_t op);

/**
 * Converts extension to string
 *
 * @param ext Instruction extension
 * @param dot If true - return ext string with '.' prefix
 */
const char * svm_ext2str(svm_ext_t ext, bool dot);

/**
 * Register to string
 *
 * @param reg Register
 */
const char * svm_register2str(svm_register_t reg);

/**
 * Argument type to string
 *
 * @param type Argument type
 */
const char * svm_arg2str(svm_arg_type_t type);

/**
 * Returns argument value in string
 *
 * Returns register name, if register, of value, if immediate
 *
 * @param code Instruction buffer
 * @param index Index in instruction buffer
 * @param type Argument type
 * @param has_prev_arg To know if previous argument is present
 */
const char * svm_get_arg_str(int32_t * code, uint32_t index, svm_arg_type_t type, bool has_prev_arg);

/**
 * Argument to register conversion (if argument is register)
 *
 * @param arg Argument type
 */
svm_register_t svm_arg_to_reg(svm_arg_type_t arg);

/**
 * Converts string to opcode value
 *
 * @param str String containing opcode
 */
svm_opcode_t svm_str2opcode(const char * str);

/**
 * Converts string to argument type
 *
 * @param str String containing argument
 */
svm_arg_type_t svm_str2arg(const char * str);

/**
 * Converts string to extension type
 *
 * @param str String containing instruction extension
 */
svm_ext_t svm_str2ext(const char * str);

/**
 * Converts string to int32_t
 *
 * @param str String containing integer literal
 * @param value Value to put the result in
 *
 * @retval true if conversion was successful
 */
bool svm_to_int32(const char * str, int32_t * value);

#ifdef __cplusplus
}
#endif