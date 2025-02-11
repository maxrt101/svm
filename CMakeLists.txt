# SVM v0.1
cmake_minimum_required(VERSION 3.27)

set(CMAKE_C_STANDARD 17)
set(PROJECT_NAME svm)
set(PROJECT_PATH ${CMAKE_CURRENT_LIST_DIR})

project(${PROJECT_NAME} C)

add_executable(${PROJECT_NAME}
        ${PROJECT_PATH}/svm/svm.h
        ${PROJECT_PATH}/svm/svm.c
        ${PROJECT_PATH}/svm/svm_asm.h
        ${PROJECT_PATH}/svm/svm_asm.c
        ${PROJECT_PATH}/svm/svm_util.h
        ${PROJECT_PATH}/svm/svm_util.c
        ${PROJECT_PATH}/main.c
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        USE_SVM_DEBUG_CYCLE=1
        USE_SVM_DEBUG_CYCLE_PRINT_STACK=1
        USE_SVM_DEBUG_CYCLE_PRINT_FLAGS=1
        USE_SVM_ASM_PRINT_TOKENS=1
        USE_SVM_ASM_PRINT_DISASM=1
        USE_SVM_ASM_PRINT_LABELS=1
        SVM_ASM_MAX_CYCLES=10000
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_PATH}
)

