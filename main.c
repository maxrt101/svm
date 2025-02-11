/** ========================================================================= *
 *
 * @file main.c
 * @date 12-02-2025
 * @author Maksym Tkachuk <max.r.tkachuk@gmail.com>
 *
 *  ========================================================================= */

/* Includes ================================================================= */
#include "svm/svm_asm.h"
#include "svm/svm_util.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>

/* Defines ================================================================== */
#ifndef SVM_ASM_MAX_CYCLES
#define SVM_ASM_MAX_CYCLES 128
#endif

#define SVM_ASM_HEX_PRINT_WORDS_IN_LINE 4
#define SVM_ASM_DEVICES                 4
#define SVM_ASM_USE_COLOR               1

#define SVM_ASM_CHAR_BLOCK              "█"
#define SVM_ASM_COLOR_1                 "\e[0;37m"
#define SVM_ASM_COLOR_0                 "\e[0;30m"
#define SVM_ASM_COLOR_RESET             "\e[0m"

/* Macros =================================================================== */
/* Exposed macros =========================================================== */
/* Enums ==================================================================== */
typedef enum {
  SVM_CMD_UNKNOWN = 0,
  SVM_CMD_HELP,
  SVM_CMD_ASM,
  SVM_CMD_RUN,
} svm_cmd_t;

/* Types ==================================================================== */
typedef uint8_t screen_t[8][8 * SVM_ASM_DEVICES];

/* Variables ================================================================ */
/* Private functions ======================================================== */
static void screen_init(screen_t * screen) {
  SVM_ASSERT_RETURN(screen);

  memset(screen, 0, sizeof(*screen));
}

static void screen_set(screen_t * screen, int32_t x, int32_t y, bool value) {
  SVM_ASSERT_RETURN(screen);

  if (x > 8 * SVM_ASM_DEVICES) {
    printf("Screen overflow (x=%d)\n", x);
    return;
  }

  if (y > 8) {
    printf("Screen overflow (y=%d)\n", y);
    return;
  }

  (*screen)[y][x] = value;
}

static void screen_out(screen_t * screen) {
  SVM_ASSERT_RETURN(screen);

  for (uint16_t y = 0; y < 8; ++y) {
    for (uint16_t x = 0; x < 8 * SVM_ASM_DEVICES; ++x) {
#if SVM_ASM_USE_COLOR
      printf(
          "%s%s%s%s ",
          (*screen)[y][x] ? SVM_ASM_COLOR_1 : SVM_ASM_COLOR_0,
          SVM_ASM_CHAR_BLOCK,
          SVM_ASM_CHAR_BLOCK,
          SVM_ASM_COLOR_RESET
      );
#else
      printf("%c ", (*screen)[y][x] ? '1' : '0');
#endif
    }
    printf("\n");
  }
}

static svm_cmd_t svm_asm_parse_cmd(const char * cmd) {
  SVM_ASSERT_RETURN(cmd, SVM_CMD_UNKNOWN);

  if (!strcmp(cmd, "help")) {
    return SVM_CMD_HELP;
  } else if (!strcmp(cmd, "asm")) {
    return SVM_CMD_ASM;
  } else if (!strcmp(cmd, "run")) {
    return SVM_CMD_RUN;
  } else {
    return SVM_CMD_UNKNOWN;
  }
}

/* Shared functions ========================================================= */
void svm_port_sys(void * ctx, int32_t (*registers)[R_MAX], int32_t syscall_num) {
  printf("sys %d\n", syscall_num);
  switch (syscall_num) {
    case 1:
      usleep((*registers)[R0] * 1000);
      break;

    case 2:
      screen_set((screen_t *) ctx, (*registers)[R0], (*registers)[R1], (*registers)[R2]);
      break;

    case 3:
      screen_out((screen_t *) ctx);
      break;

    default:
      break;

  }
}

int main(int argc, char ** argv) {
  svm_cmd_t cmd = SVM_CMD_HELP;

  if (argc == 3) {
    cmd = svm_asm_parse_cmd(argv[1]);
  }

  switch (cmd) {
    case SVM_CMD_HELP:
      printf(
          "SVM - Small Virtual Machine\n"
          "Usage: %s [help|asm|run] FILE\n"
          "  help - Prints this message\n"
          "  asm  - Assembles provided file\n"
          "         and outputs hex to stdout\n"
          "  run  - Assembles and runs file\n"
          "", argv[0]
      );
      return 1;

    case SVM_CMD_ASM:
    case SVM_CMD_RUN: {
      svm_asm_t ctx;
      svm_asm_error_t res = svm_asm_file(&ctx, argv[2]);

      if (res) {
        return res;
      }

      if (cmd == SVM_CMD_ASM) {
        printf("Bytecode:\n");
        for (uint32_t i = 0; i < ctx.code.size; ++i) {
          printf("0x%08x, ", ctx.code.buffer[i]);
          if (i && (i+1) % SVM_ASM_HEX_PRINT_WORDS_IN_LINE == 0) {
            printf("\n");
          }
        }
        printf("\n");
      } else {
        screen_t screen;
        screen_init(&screen);

        svm_t vm;
        svm_init(&vm, &screen);
        svm_load(&vm, ctx.code.buffer, ctx.code.size);

        printf("Execution:\n");

        uint32_t cycles = 0;
        while (vm.flags.running) {
          if (SVM_ASM_MAX_CYCLES != 0 && cycles >= SVM_ASM_MAX_CYCLES) {
            printf("Max cycles reached (%d)\n", SVM_ASM_MAX_CYCLES);
            return SVM_ERR;
          }
          svm_error_t err = svm_cycle(&vm);
          if (err != SVM_OK) {
            return err;
          }
          cycles++;
        }

        printf("Execution ended. Took %d cycles\n", cycles);
      }

      svm_asm_free(&ctx);

      break;
    }

    default:
      printf("Unknown command %s!\n", argv[1]);
      return 1;
  }

  return 0;
}
