/*This C program checks whether the brackets in a given string are balanced or not. It uses a stack data structure to push opening brackets onto the stack and pop them off when encountering closing brackets to ensure that the brackets are properly matched. The main function initializes a string with brackets and calls the areBracketsBalanced function to determine if the brackets are balanced or not.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define the structure for the stack node
struct sNode {
    char data;
    struct sNode* next;
};

// Function to push an element onto the stack
void push(struct sNode** top_ref, char new_data) {
    struct sNode* new_node = (struct sNode*)malloc(sizeof(struct sNode));
    if (new_node == NULL) {
        printf("Stack overflow\n");
        exit(1);
    }
    new_node->data = new_data;
    new_node->next = *top_ref;
    *top_ref = new_node;
}

// Function to pop an element from the stack
char pop(struct sNode** top_ref) {
    if (*top_ref == NULL) {
        printf("Stack underflow\n");
        exit(1);
    }
    struct sNode* top = *top_ref;
    char res = top->data;
    *top_ref = top->next;
    free(top);
    return res;
}

// Function to check if a given character is a matching pair with the top element of the stack
int isMatchingPair(char character1, char character2) {
    if (character1 == '(' && character2 == ')') return 1;
    if (character1 == '{' && character2 == '}') return 1;
    if (character1 == '[' && character2 == ']') return 1;
    return 0;
}

// Function to check if the brackets are balanced
int areBracketsBalanced(char exp[]) {
    struct sNode* stack = NULL;
    for (int i = 0; exp[i] != '\0'; i++) {
        // If the current character is an opening bracket, push it to the stack
        if (exp[i] == '{' || exp[i] == '(' || exp[i] == '[') {
            push(&stack, exp[i]);
        }
        // If the current character is a closing bracket
        if (exp[i] == '}' || exp[i] == ')' || exp[i] == ']') {
            // If the stack is empty, return false
            if (stack == NULL) return 0;
            // If the top of the stack doesn't match the closing bracket, return false
            if (!isMatchingPair(pop(&stack), exp[i])) return 0;
        }
    }
    // If the stack is not empty at the end, return false
    return stack == NULL;
}

int main() {
    char in[50] = "{}{]}{}";
    if (areBracketsBalanced(in)) {
        printf("Balanced\n");
    }
    else {
        printf("Not Balanced\n");
    }
    return 0;
}
