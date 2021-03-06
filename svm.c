#include <stdio.h>
#include <stdlib.h>
// stack will have fixed size
#define STACK_SIZE 100

typedef struct
{
    int *locals; // local scoped data
    int *code;   // array od byte codes to be executed
    int *stack;  // virtual stack
    int pc;      // program counter (aka. IP - instruction pointer)
    int sp;      // stack pointer
    int fp;      // frame pointer (for local scope)
} VM;

enum
{
    ADD_I32 = 1,   // int add
    SUB_I32 = 2,   // int sub
    MUL_I32 = 3,   // int mul
    LT_I32 = 4,    // int less than
    EQ_I32 = 5,    // int equal
    JMP = 6,       // branch
    JMPT = 7,      // branch if true
    JMPF = 8,      // branch if false
    CONST_I32 = 9, // push constant integer
    LOAD = 10,     // load from local
    GLOAD = 11,    // load from global
    STORE = 12,    // store in local
    GSTORE = 13,   // store in global memory
    PRINT = 14,    // print value on top of the stack
    POP = 15,      // throw away top of the stack
    HALT = 16,     // stop program
    CALL = 17,     // call procedure
    RET = 18       // return from procedure
};

VM *newVM(int *code, // pointer to table containing a bytecode to be executed
          int pc,    // address of instruction to be invoked as first one - entrypoint/main func
          int datasize)
{ // total locals size required to perform a program operations
    VM *vm = (VM *)malloc(sizeof(VM));
    vm->code = code;
    vm->pc = pc;
    vm->fp = 0;
    vm->sp = -1;
    vm->locals = (int *)malloc(sizeof(int) * datasize);
    vm->stack = (int *)malloc(sizeof(int) * STACK_SIZE);

    return vm;
}

void delVM(VM *vm)
{
    free(vm->locals);
    free(vm->stack);
    free(vm);
}

#define PUSH(vm, v) vm->stack[++vm->sp] = v // push value on top of the stack
#define POP(vm) vm->stack[vm->sp--]         // pop value from top of the stack
#define NCODE(vm) vm->code[vm->pc++]        // get next bytecode

void run(VM *vm)
{
    while (1)
    {
        int opcode = NCODE(vm); // fetch
        int v, addr, offset, a, b, argc, rval;

        switch (opcode)
        { // decode
        case HALT:
            return; // stop the program
        case CONST_I32:
            v = NCODE(vm); // get next value from code ...
            PUSH(vm, v);   // ... and move it on top of the stack
            break;
        case ADD_I32:
            b = POP(vm);
            a = POP(vm);
            PUSH(vm, a + b);
            break;
        case SUB_I32:
            b = POP(vm);
            a = POP(vm);
            PUSH(vm, a - b);
            break;
        case MUL_I32:
            b = POP(vm);
            a = POP(vm);
            PUSH(vm, a * b);
            break;
        case LT_I32:
            b = POP(vm);
            a = POP(vm);
            PUSH(vm, (a < b) ? 1 : 0);
            ;
            break;
        case EQ_I32:
            b = POP(vm);
            a = POP(vm);
            PUSH(vm, (a == b) ? 1 : 0);
            break;
        case JMP:
            vm->pc = NCODE(vm); // unconditionaly jump with program counter to provided address
            break;
        case JMPT:
            addr = NCODE(vm); // get address pointer from code ...
            if (POP(vm))
            {                  // ... pop value from top of the stack, and if it's true ...
                vm->pc = addr; // ... jump with program counter to provided address
            }
            break;
        case JMPF:
            addr = NCODE(vm); // get address pointer from code ...
            if (!POP(vm))
            {                  // ... pop value from top of the stack, and if it's false ...
                vm->pc = addr; // ... jump with program counter to provided address
            }
            break;
        case LOAD:                                // load local value or function arg
            offset = NCODE(vm);                   // get next value from code to identify local variables offset start on the stack
            PUSH(vm, vm->stack[vm->fp + offset]); // ... put on the top of the stack variable stored relatively to frame pointer
            break;
        case STORE:                          // store local value or function arg
            v = POP(vm);                     // get value from top of the stack ...
            offset = NCODE(vm);              // ... get the relative pointer address from code ...
            vm->locals[vm->fp + offset] = v; // ... and store value at address received relatively to frame pointer
            break;

        case GLOAD:
            addr = POP(vm);       // get pointer address from code ...
            v = vm->locals[addr]; // ... load value from memory of the provided addres ...
            PUSH(vm, v);          // ... and put that value on top of the stack
            break;
        case GSTORE:
            v = POP(vm);          // get value from top of the stack ...
            addr = NCODE(vm);     // ... get pointer address from code ...
            vm->locals[addr] = v; // ... and store value at address received
            break;
        case CALL:
            // we expect all args to be on the stack
            addr = NCODE(vm); // get next instruction as an address of procedure jump ...
            argc = NCODE(vm); // ... and next one as number of arguments to load ...
            PUSH(vm, argc);   // ... save num args ...
            PUSH(vm, vm->fp); // ... save function pointer ...
            PUSH(vm, vm->pc); // ... save instruction pointer ...
            vm->fp = vm->sp;  // ... set new frame pointer ...
            vm->pc = addr;    // ... move instruction pointer to target procedure address
            break;
        case RET:
            rval = POP(vm);   // pop return value from top of the stack
            vm->sp = vm->fp;  // ... return from procedure address ...
            vm->pc = POP(vm); // ... restore instruction pointer ...
            vm->fp = POP(vm); // ... restore framepointer ...
            argc = POP(vm);   // ... hom many args procedure has ...
            vm->sp -= argc;   // ... discard all of the args left ...
            PUSH(vm, rval);   // ... leave return value on top of the stack
            break;
        case POP:
            --vm->sp; // throw away value at top of the stack
            break;
        case PRINT:
            v = POP(vm);       // pop value from top of the stack ...
            printf("%d\n", v); // ... and print it
            break;
        default:
            break;
        }
    }
}

int main(int argc, char const *argv[])
{
    const int fib = 0; // address of the fibonacci procedure
    int program[] = {
        // int fib(n) {
        //     if(n == 0) return 0;
        LOAD, -3,     // 0 - load last function argument N
        CONST_I32, 0, // 2 - put 0
        EQ_I32,       // 4 - check equality: N == 0
        JMPF, 10,     // 5 - if they are NOT equal, goto 10
        CONST_I32, 0, // 7 - otherwise put 0
        RET,          // 9 - and return it
        //     if(n < 3) return 1;
        LOAD, -3,     // 10 - load last function argument N
        CONST_I32, 3, // 12 - put 3
        LT_I32,       // 14 - check if 3 is less than N
        JMPF, 20,     // 15 - if 3 is NOT less than N, goto 20
        CONST_I32, 1, // 17 - otherwise put 1
        RET,          // 19 - and return it
        //     else return fib(n-1) + fib(n-2);
        LOAD, -3,     // 20 - load last function argument N
        CONST_I32, 1, // 22 - put 1
        SUB_I32,      // 24 - calculate: N-1, result is on the stack
        CALL, fib, 1, // 25 - call fib function with 1 arg. from the stack
        LOAD, -3,     // 28 - load N again
        CONST_I32, 2, // 30 - put 2
        SUB_I32,      // 32 - calculate: N-2, result is on the stack
        CALL, fib, 1, // 33 - call fib function with 1 arg. from the stack
        ADD_I32,      // 36 - since 2 fibs pushed their ret values on the stack, just add them
        RET,          // 37 - return from procedure
        // entrypoint - main function
        CONST_I32, 6, // 38 - put 6
        CALL, fib, 1, // 40 - call function: fib(arg) where arg = 6;
        PRINT,        // 43 - print result
        HALT          // 44 - stop program
    };

    // initialize virtual machine
    VM *vm = newVM(program, // program to execute
                   38,      // start address of main function
                   0);      // locals to be reserved, fib doesn't require them
    run(vm);
    return 0;
}
