/** ========================================================================= *
 *
 * @file svm_asm.h
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
#define SVM_LABEL_NAME_MAX 32

/* Macros =================================================================== */
/* Enums ==================================================================== */
/**
 * SVM Asm Error Codes
 */
typedef enum {
  SVM_ASM_OK = 0,                         /** OK */
  SVM_ASM_ERR_NULL,                       /** Something non-nullable is NULL */
  SVM_ASM_ERR_BAD_ALLOC,                  /** Bad Allocation */
  SVM_ASM_ERR_ARG_CONSTRAINT_UNSATISFIED, /** Argument constraint unsatisfied */
  SVM_ASM_ERR_UNDEFINED_LABEL,            /** Undefined label referenced */
  SVM_ASM_ERR_FILE_OPEN_FAILED,           /** Failed to open file */
  SVM_ASM_ERR_EXPECTED_TOKEN,             /** Expected token, but got nothing */
} svm_asm_error_t;

/* Types ==================================================================== */
/**
 * Label, contains name and location
 */
typedef struct {
  char name[SVM_LABEL_NAME_MAX];
  int32_t location;
} svm_asm_label_t;

/**
 * SVM Asm Context
 */
typedef struct {
  struct {
    int32_t * buffer;
    uint32_t capacity;
    int32_t size;
  } code;

  struct {
    svm_asm_label_t * buffer;
    uint32_t capacity;
    uint32_t size;
  } labels;

  struct {
    svm_asm_label_t * buffer;
    uint32_t capacity;
    uint32_t size;
  } patches;
} svm_asm_t;

/* Variables ================================================================ */
/* Shared functions ========================================================= */
/**
 * Initializes SVM ASM Context
 *
 * @param code Context
 */
svm_asm_error_t svm_asm_init(svm_asm_t * code);

/**
 * Free SVM Asm Context
 *
 * @param code Context
 */
svm_asm_error_t svm_asm_free(svm_asm_t * code);

/**
 * Assemble source
 *
 * @note The result will be available in code->code.buffer & code->code.size
 *
 * @param code Context
 * @param source NULL-terminated string containing source code
 *
 * @retval SVM_ASM_OK If assembly was successful
 * @retval SVM_ASM_ERR_EXPECTED_TOKEN If expected next token, but there was none
 * @retval SVM_ASM_ERR_ARG_CONSTRAINT_UNSATISFIED If instruction argument constraint wasn't met
 */
svm_asm_error_t svm_asm(svm_asm_t * code, char * source);

/**
 * Assemble source from file, patches labels, print debug info
 *
 * @note The result will be available in code->code.buffer & code->code.size
 * @note Calls svm_asm
 *
 * @param code Context
 * @param filename NULL-terminated string containing filename to read source
 *                 from
 *
 * @retval SVM_ASM_OK If assembly was successful
 * @retval SVM_ASM_ERR_EXPECTED_TOKEN If expected next token, but there was none
 * @retval SVM_ASM_ERR_ARG_CONSTRAINT_UNSATISFIED If instruction argument constraint wasn't met
 * @retval SVM_ASM_ERR_FILE_OPEN_FAILED If couldn't open file
 */
svm_asm_error_t svm_asm_file(svm_asm_t * ctx, const char * filename);

#ifdef __cplusplus
}
#endif
