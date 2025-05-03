#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

#define BUFFER_SIZE 100
#define LABEL_ARR_SIZE 100
#define LINE_BUFFER_SIZE 256
#define BASE 0
#define INSTRUCTION_SIZE 16
#define REGISTER_SIZE 3
#define OPCODE_SIZE 4
#define IMMEDIATE_SIZE 6
#define JUMP_ADDRESS_SIZE 12
#define NO_OP 0
#define IMM_MASK 63

#define USE_JUMP_NOPS 0
#define USE_BRANCH_NOPS 0
#define USE_OTHER_NOPS 0
#define USE_LW_NOPS 0

typedef struct LABEL_STRUCT{
    char* labelName;
    u_int32_t address;
} LABEL;
typedef struct INSTRUCTION_STRUCT{
    u_int8_t opcode;
    u_int8_t func;
} INSTRUCTION_ID;

int current = -1;
int labelIndex = 0;
LABEL* labels;
int currentLineNumber = 0;

int max(int one, int two, int three){
    return one > two ? one > three ? one : three : two > three ? two : three;
}

int signExtend6Bit(int num){
    return num << 26 >> 26;
}

void compileError(const char* message){
    printf("\n\033[35mCompile Error on line %d: %s\033[0m\n", currentLineNumber, message);
    exit(1);
}
void trimComma(char word[]){
    size_t len = strlen(word);
    if (len > 0 && word[len - 1] == ',') {
        word[len - 1] = '\0';
    }
}
int resolveImmediate(char im[]){
    int isNegative = im[0] == '-';
    char base = isNegative ? im[2] : im[1];
    int flag = 0;
    if(base == 'x'){
        base = 16;
    }
    else if(base == 'd'){
        base = 10;
    }
    else if(base == 'b'){
        base = 2;
    }
    else {
        base = 10;
        flag = 1;
    }
    int start = flag ? 0 : isNegative ? 3 : 2;
    int value = (int) strtol(im + start, NULL, base);
    if(value > 32767){
        compileError("Immediate value out of range");
    }
    return isNegative && !flag ? value * -1 : value;
}

u_int32_t resolveLabel(char label[]){
    for(int i = 0; i < labelIndex; i++){
        if(strcmp(labels[i].labelName, label) == 0){
            return labels[i].address;
        }
    }
    char errorMessage[BUFFER_SIZE];
    sprintf(errorMessage, "Invalid label \"%s\"", label);
    compileError(errorMessage);
    return -1;
}

void printBinary(u_int32_t number, int bits ){
    for (int i = bits - 1; i >= 0; i--) {
        u_int32_t bit = (number >> i) & 1;
        printf("%d", bit);
    }
}

void writeInstructionsToFile(const u_int16_t *instructions, int size, FILE* file ){
    for(int i = 0; i < size; i++ ){
        for (int j = INSTRUCTION_SIZE - 1; j >= 0; j--) {
            u_int16_t bit = (instructions[i] >> j) & 1;
            fprintf(file,"%d", bit);
        }
        fprintf(file, "\n");
    }
}

void writeVHDLArrayInitializerToFile(const u_int16_t *instructions, int size, FILE* file ){
    for(int i = 0; i < size; i++ ){
        fprintf(file, "%d => \"", i * 2);
        for (int j = INSTRUCTION_SIZE - 1; j >= (INSTRUCTION_SIZE /2); j--) {
            u_int16_t bit = (instructions[i] >> j) & 1;
            fprintf(file,"%d", bit);
        }
        fprintf(file, "\",\n");
        fprintf(file, "%d => \"", (i * 2) + 1);
        for (int j = (INSTRUCTION_SIZE /2) - 1; j >= 0; j--) {
            u_int16_t bit = (instructions[i] >> j) & 1;
            fprintf(file,"%d", bit);
        }
        fprintf(file, "\",\n");
    }
}

 INSTRUCTION_ID getInstructionId(char operation[]){
    if(!strcmp(operation, "add")){
        return (INSTRUCTION_ID){.opcode = 1, .func = 0};
    }
    else if(!strcmp(operation, "inc")){
        return (INSTRUCTION_ID){.opcode = 1, .func = 1};
    }
    else if(!strcmp(operation, "addi")){
        return (INSTRUCTION_ID){.opcode = 15, .func = 0};
    }
    else if(!strcmp(operation, "sub")){
        return (INSTRUCTION_ID){.opcode = 1, .func = 3};
    }
    else if(!strcmp(operation, "and")){
        return (INSTRUCTION_ID){.opcode = 1, .func = 4};
    }
    else if(!strcmp(operation, "or")){
        return (INSTRUCTION_ID){.opcode = 1, .func = 5};;
    }
    else if (!strcmp(operation, "xor")){
        return (INSTRUCTION_ID){.opcode = 1, .func = 6};
    }
    else if (!strcmp(operation, "sll")){
        return (INSTRUCTION_ID){.opcode = 2, .func = 0};
    }
    else if(!strcmp(operation, "dec")){
        return (INSTRUCTION_ID){.opcode = 1, .func = 2};
    }
    else if(!strcmp(operation, "srl")){
        return (INSTRUCTION_ID){.opcode = 3, .func = 0};
    }
    else if(!strcmp(operation, "sra")){
        return (INSTRUCTION_ID){.opcode = 4, .func = 0};
    }
    else if(!strcmp(operation, "beq")){
        return (INSTRUCTION_ID){.opcode = 5, .func = 0};
    }
    else if(!strcmp(operation, "beqz")){
        return (INSTRUCTION_ID){.opcode = 6, .func = 0};
    }
    else if(!strcmp(operation, "ble")){
        return (INSTRUCTION_ID){.opcode = 7, .func = 0};
    }
    else if(!strcmp(operation, "j")){
        return (INSTRUCTION_ID){.opcode = 8, .func = 0};
    }
    else if(!strcmp(operation, "jal")){
        return (INSTRUCTION_ID){.opcode = 9, .func = 0};
    }
    else if(!strcmp(operation, "r")){
        return (INSTRUCTION_ID){.opcode = 12, .func = 0};
    }
    else if(!strcmp(operation, "lw")){
        return (INSTRUCTION_ID){.opcode = 10, .func = 0};
    }
    else if(!strcmp(operation, "sw")){
        return (INSTRUCTION_ID){.opcode = 11, .func = 0};
    }
    else if(!strcmp(operation, "clr")){
        return (INSTRUCTION_ID){.opcode = 13, .func = 0};
    }
    return (INSTRUCTION_ID){.opcode = 127, .func = 0};
}
u_int8_t getInstructionType(u_int8_t opcode){
    if(opcode == 1 || opcode == 12 || opcode == 13){
        return 1;
    }
    else if(opcode == 8 || opcode == 9){
        return 2;
    }
    return 0;
}

u_int8_t getRegisterNumber(char r[]){
    u_int8_t num = 0;
    if(r[0] != '$'){
        char errorMessage[BUFFER_SIZE];
        sprintf(errorMessage, "Expected register, got %s", r);
        compileError(errorMessage);
    }
    num += r[1] - '0';
    return num;
}

int main(int argc, char** argv) {
    assert(argc == 5);
    FILE *file;
    FILE *outFile;
    char *filename = argv[1];
    char *outputFilename = argv[2];
    char *originalInstructionsFilename = argv[3];
    char *instructionsOutputFilename = argv[4];
    char word[BUFFER_SIZE];
    labels = malloc(LABEL_ARR_SIZE * sizeof (LABEL*));
    outFile = fopen(outputFilename, "w");
    file = fopen(filename, "r");
    if(file == NULL){
        printf("Unable to open the file\n");
        return 1;
    }

    char line[LINE_BUFFER_SIZE];
    int currentAddress = -1;
    while (fgets(line, sizeof(line), file) != NULL) {
        if(line[0] == '\n'){
            continue;
        }
        currentAddress++;
        char *colonPointer = strchr(line, ':');

        if (colonPointer != NULL) {
            int labelLen = colonPointer - line;
            int isLabel = 1;
            int labelStartIndex = 0;
            for (int i = 0; i < labelLen; i++) {
                if(line[i] == ' '){
                    labelStartIndex++;
                }
                if (!isalnum(line[i]) && line[i] != '_' && line[i] != ' ') {
                    isLabel = 0;
                    break;
                }
            }
            if (isLabel) {
                *colonPointer = '\0';
                labels[labelIndex].labelName = malloc(BUFFER_SIZE);
                char *label = &line[labelStartIndex];
                strcpy(labels[labelIndex].labelName, label);
                colonPointer++;
                while(isspace((unsigned char) *colonPointer)){
                    colonPointer++;
                }
                if(*colonPointer == '\n' || *colonPointer == '\0' || *colonPointer == '#'){ // if next instruction is not on the same line
                    labels[labelIndex].address = currentAddress;
                    currentAddress--;
                }
                else{
                    labels[labelIndex].address = currentAddress;
                }

                printf("Label: '%s' found on line %d\n", label, labels[labelIndex].address);
                labelIndex++;
            }
        }
        else{
            int index = 0;
            while(isspace(line[index])){
                index++;
            }
            if(line[index] == '#'){
                currentAddress--;
            }

        }

    }
    u_int16_t *instructions = malloc((currentAddress * 2 )* sizeof (u_int32_t));
    int numInstructions = 0;
    rewind(file);

    while (fscanf(file, "%99s", word) == 1) {
        INSTRUCTION_ID id = getInstructionId(word);
        u_int8_t code = id.opcode;
        u_int8_t func = id.func;
        currentLineNumber++;

        if (code == 127) {
            char x[BUFFER_SIZE];
            fgets(x, LINE_BUFFER_SIZE, file);
            continue;
        }
        current++;
        u_int8_t type = getInstructionType(code);

        if (type == 1) {
            instructions[current] = code << (INSTRUCTION_SIZE - OPCODE_SIZE);
            char dest[BUFFER_SIZE];
            char source1[BUFFER_SIZE];
            char source2[BUFFER_SIZE];
            if (code != 13 && code != 12) {
                if (fscanf(file, "%99s", dest) == 1) {
                    trimComma(dest);
                    u_int8_t rNum = getRegisterNumber(dest);
                    instructions[current] = instructions[current] + (rNum
                            << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE - REGISTER_SIZE - REGISTER_SIZE));
                }
            }
            if (code != 12 && code != 13) {
                if (fscanf(file, "%99s", source1) == 1) {
                    trimComma(source1);
                    u_int8_t sNum1 = getRegisterNumber(source1);
                    instructions[current] = instructions[current] +
                                            (sNum1 << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE));
                }
            }
            if (func != 1 && func != 2 && code != 12) {
                if (fscanf(file, "%99s", source2) == 1) {
                    trimComma(source2);
                    u_int8_t sNum2 = getRegisterNumber(source2);
                    instructions[current] = instructions[current] + (sNum2
                            << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE - REGISTER_SIZE));
                }
            }
            instructions[current] += func;
            numInstructions++;
        } else if (type == 0) {
            instructions[current] = code << (INSTRUCTION_SIZE - OPCODE_SIZE);
            char dest[BUFFER_SIZE];
            char source1[BUFFER_SIZE];


            if (fscanf(file, "%99s", dest) == 1) {
                trimComma(dest);
                u_int8_t rNum = getRegisterNumber(dest);
                if (code == 5 || code == 6 || code == 7) {//branches
                    instructions[current] =
                            instructions[current] + (rNum << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE));
                } else {
                    instructions[current] = instructions[current] +
                                            (rNum << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE -
                                                      REGISTER_SIZE));
                }
            }
            if (fscanf(file, "%99s", source1) == 1) {
                //this is either a register, a parenthesis, or an immediate number.
                if (source1[0] == '$') {
                    int hasImmediate = source1[strlen(source1) - 1] == ',';
                    trimComma(source1);
                    u_int8_t rNum = getRegisterNumber(source1);
                    if (code == 5 || code == 6 || code == 7) {//branches
                        instructions[current] =
                                instructions[current] + (rNum << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE - REGISTER_SIZE));
                    } else {
                        instructions[current] = instructions[current] +
                                                (rNum << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE));
                    }
                    if (hasImmediate) {
                        char im[BUFFER_SIZE];
                        if (fscanf(file, "%99s", im) == 1) {
                            int value = resolveImmediate(im);
                            if (errno == EINVAL) {
                                //this is a label
                                u_int32_t labelAddress = resolveLabel(im);
                                int offset = (int) labelAddress - (current + BASE);
                                value = offset;
                                errno = 0;
                            }
                            instructions[current] = instructions[current] + (value & IMM_MASK);
                        }
                    }
                } else {
                    if (source1[0] == '(') {
                        char *ptr = source1;
                        ptr++;
                        ptr[strlen(ptr) - 1] = '\0';
                        u_int8_t rNum = getRegisterNumber(ptr);
                        if (code == 5 || code == 6 || code == 7) {
                            instructions[current] = instructions[current] + (rNum
                                    << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE - REGISTER_SIZE));
                        } else {
                            instructions[current] =
                                    instructions[current] +
                                    (rNum << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE));
                        }
                    } else {
                        int i = 0;
                        int immediate = INFINITY;
                        while (i < strlen(source1)) {
                            if (source1[i] == '(') {
                                char offset[BUFFER_SIZE];
                                strcpy(offset, source1);
                                offset[i] = '\0';
                                immediate = resolveImmediate(offset);
                                break;
                            }
                            i++;
                        }
                        if (i == strlen(source1)) {
                            u_int32_t labelAddress = resolveLabel(source1);
                            immediate = (int) labelAddress - (current + BASE);
                            instructions[current] += ((immediate & IMM_MASK)
                                    << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE - REGISTER_SIZE -
                                        IMMEDIATE_SIZE));
                        } else {
                            char *ptr = source1;
                            ptr += ++i;
                            ptr[strlen(ptr) - 1] = '\0';
                            u_int8_t rNum = getRegisterNumber(ptr);
                            if (code == 5 || code == 6 || code == 7) { // branches and sw
                                instructions[current] = instructions[current] + (rNum
                                        << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE - REGISTER_SIZE)) +
                                                        ((immediate & IMM_MASK)
                                                                << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE -
                                                                    REGISTER_SIZE -
                                                                    IMMEDIATE_SIZE));
                            } else {
                                instructions[current] = instructions[current] + (rNum
                                        << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE)) +
                                                        ((immediate & IMM_MASK)
                                                                << (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE -
                                                                    REGISTER_SIZE -
                                                                    IMMEDIATE_SIZE));
                            }
                        }
                    }
                }
            }
            numInstructions++;
        } else if (type == 2) {
            char label[BUFFER_SIZE];
            int flag = 0;
            u_int32_t jumpAddress;
            if (fscanf(file, "%99s", label) == 1) {
                for (int i = 0; i < labelIndex; i++) {
                    if (strcmp(labels[i].labelName, label) == 0) {
                        jumpAddress = BASE + labels[i].address;
                        instructions[current] = (code << (INSTRUCTION_SIZE - OPCODE_SIZE)) +
                                                ((jumpAddress & 4095)
                                                        << (INSTRUCTION_SIZE - OPCODE_SIZE - JUMP_ADDRESS_SIZE));
                        flag = 1;
                        numInstructions++;
                    }
                }
                if (!flag) {
                    compileError("Invalid label in jump");
                }
            }
        }
    }


//
//    for(int i = 0; i < numInstructions; i++){
//        printBinary(instructions[i], INSTRUCTION_SIZE);
//        printf("\n");
//    }


    u_int16_t *resolvedInstructions = malloc(5 * numInstructions * sizeof (u_int16_t));
    int currentResolvedInstruction = 0;
    u_int8_t *activeRegisters = malloc(8 * sizeof(u_int8_t));
    u_int8_t *newLineNumbers = malloc(numInstructions * sizeof (u_int8_t));
    for(int i = 0; i < 8; i++){
        activeRegisters[i] = 0;
    }
    int mask = (int) pow(2, REGISTER_SIZE) - 1;
    int jumpAddressMask = (int) pow(2, JUMP_ADDRESS_SIZE) - 1;
    int immediateMask = (int) pow(2, IMMEDIATE_SIZE) - 1;
    //handle pseudo instructions here
    for(int i = 0; i < numInstructions; i++){
        for(int j = 0; j < 8; j++){
            activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
        }

        u_int16_t instruction = instructions[i];

        u_int16_t opcode = instruction >> (INSTRUCTION_SIZE - OPCODE_SIZE);
        u_int16_t type = getInstructionType(opcode);


        if(type == 1){
            u_int8_t source1 = (instruction >> (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE)) & mask;
            u_int8_t source2 = (instruction >> (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE - REGISTER_SIZE)) & mask;
            u_int8_t dest = (instruction >> (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE - REGISTER_SIZE - REGISTER_SIZE)) & mask;

            if((activeRegisters[dest] || activeRegisters[source1] || activeRegisters[source2]) && USE_OTHER_NOPS){ // not available
                int waitCycles = max(activeRegisters[dest], activeRegisters[source1], activeRegisters[source2]);
                for(int j = 0; j < waitCycles; j++){
                    resolvedInstructions[currentResolvedInstruction] = NO_OP;
                    currentResolvedInstruction++;
                    for(int j = 0; j < 8; j++){
                        activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
                    }
//                    for(int j = 0; j < 8; j++){
//                        activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
//                    }
//                    for(int j = 0; j < 8; j++){
//                        activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
//                    }
                } // add no ops
            }
            if(opcode == 12){
                if (USE_JUMP_NOPS) {
                    for (int j = 0; j < 8; j++) {
                        activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
                    }
                    for (int j = 0; j < 8; j++) {
                        activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
                    }
                    for (int j = 0; j < 8; j++) {
                        activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
                    }
                    for (int j = 0; j < 8; j++) {
                        activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
                    }
                    newLineNumbers[i] = currentResolvedInstruction;
                    resolvedInstructions[currentResolvedInstruction++] = instruction;
                    resolvedInstructions[currentResolvedInstruction++] = NO_OP;
                    resolvedInstructions[currentResolvedInstruction++] = NO_OP;
                    resolvedInstructions[currentResolvedInstruction++] = NO_OP;
                    continue;
                }
            }
            else if (opcode != 13){
                activeRegisters[source2] = 3;
            }
            else {
                activeRegisters[dest] = 3;
            }

        }
        else if(type == 0) {
            u_int8_t source1 = (instruction >> (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE)) & mask;
            u_int8_t dest = (instruction >> (INSTRUCTION_SIZE - OPCODE_SIZE - REGISTER_SIZE - REGISTER_SIZE)) & mask;

            if (((activeRegisters[dest] || activeRegisters[source1])) && USE_OTHER_NOPS) { // not available
                int waitCycles = activeRegisters[dest] > activeRegisters[source1] ? activeRegisters[dest]
                                                                                  : activeRegisters[source1];
                for (int j = 0; j < waitCycles; j++) {
                    resolvedInstructions[currentResolvedInstruction] = NO_OP;
                    currentResolvedInstruction++;
                    for(int j = 0; j < 8; j++) {
                        activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
                    }
                } // add no ops
            }
            if((opcode == 5 || opcode == 6 || opcode == 7)){
                if (USE_BRANCH_NOPS) {
                    for (int j = 0; j < 8; j++) {
                        activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
                    }
                    for (int j = 0; j < 8; j++) {
                        activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
                    }
                    for (int j = 0; j < 8; j++) {
                        activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
                    }
                    for (int j = 0; j < 8; j++) {
                        activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
                    }
                    newLineNumbers[i] = currentResolvedInstruction;
                    resolvedInstructions[currentResolvedInstruction++] = instruction;
                    resolvedInstructions[currentResolvedInstruction++] = NO_OP;
                    resolvedInstructions[currentResolvedInstruction++] = NO_OP;
                    resolvedInstructions[currentResolvedInstruction++] = NO_OP;
                    continue;
                }
            }
            else if (opcode == 10 && !USE_LW_NOPS){
                activeRegisters[dest] = 0;
            }
            else{
                activeRegisters[dest] = 3;
            }


        }
        else if(type == 2 && USE_JUMP_NOPS){ // jump
            for(int j = 0; j < 8; j++) {
                activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
            }
            for(int j = 0; j < 8; j++){
                activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
            }for(int j = 0; j < 8; j++){
                activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
            }for(int j = 0; j < 8; j++){
                activeRegisters[j] = activeRegisters[j] == 0 ? 0 : activeRegisters[j] - 1;
            }
            newLineNumbers[i] = currentResolvedInstruction;
            resolvedInstructions[currentResolvedInstruction++] = instruction;
            resolvedInstructions[currentResolvedInstruction++] = NO_OP;
            resolvedInstructions[currentResolvedInstruction++] = NO_OP;
            resolvedInstructions[currentResolvedInstruction++] = NO_OP;
            continue;
        }
        resolvedInstructions[currentResolvedInstruction] = instruction;
        newLineNumbers[i] = currentResolvedInstruction;
        currentResolvedInstruction++;
    }
    for(int i = 0; i < currentResolvedInstruction; i++){
        u_int16_t* instruction = &resolvedInstructions[i];
        u_int16_t opcode = *instruction >> (INSTRUCTION_SIZE - OPCODE_SIZE);
        u_int16_t type = getInstructionType(opcode);
        if(opcode == 5 || opcode == 6 || opcode == 7){ // branch
            int oldLineNumber = -1;
            for(int j = 0; j < numInstructions; j++){
                if(newLineNumbers[j] == i){
                    oldLineNumber = j;
                }
            }
            int branchAddress = *instruction & immediateMask;
            branchAddress = signExtend6Bit(branchAddress);
            if(branchAddress + oldLineNumber >= numInstructions){
                branchAddress = (currentResolvedInstruction + 1) - i;
            }
            if(newLineNumbers[oldLineNumber + branchAddress] - i > 31 || newLineNumbers[oldLineNumber + branchAddress] - i < -31){
                char str[BUFFER_SIZE];
                sprintf(str, "Out of bounds branching on line %d", oldLineNumber);
                compileError(str);
            }

            *instruction = *instruction & ~immediateMask;
            *instruction += (newLineNumbers[oldLineNumber + branchAddress ] - i) & IMM_MASK;
        }
        if(type == 2){
            int jumpAddress = *instruction & jumpAddressMask;
            int newJumpAddress = newLineNumbers[jumpAddress];
            if(jumpAddress >= numInstructions){
                newJumpAddress = currentResolvedInstruction + 1;
            }
            *instruction = *instruction & ~4095;
            *instruction += newJumpAddress & 4095;
        }
    }

    writeVHDLArrayInitializerToFile(resolvedInstructions, currentResolvedInstruction, outFile);
    FILE* instructionOutput = fopen(instructionsOutputFilename, "w");
    writeInstructionsToFile(resolvedInstructions, currentResolvedInstruction, instructionOutput);
    FILE* originalInstructionOutput = fopen(originalInstructionsFilename, "w");
    writeInstructionsToFile(instructions, numInstructions, originalInstructionOutput);
    fclose(file);
    free(labels);
    free(instructions);
    free(resolvedInstructions);
    free(activeRegisters);
    free(newLineNumbers);
    return 0;
}
