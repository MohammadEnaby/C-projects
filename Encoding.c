//project description: Purpose:
/*The program's primary purpose is to:

Encode a user-provided sentence using a user-provided key.
Store the encoded result and allow the user to extract specific words from the encoded string based on an index.
How It Works:

User Input:

The user is prompted to enter a sentence to be encoded.
The user is then prompted to enter a key for encoding.
Encoding Process:

The encode function is used to encode the sentence. This function takes the input sentence (str), the key (key), and produces the encoded string (e). It also stores the starting positions of each word in the encoded string in the terms array.
The encoding is done by shifting each letter in the sentence by the corresponding letter in the key. The positions are mapped from a-z or A-Z to 0-25, and the key is used cyclically.
Non-alphabetic characters are copied directly without any change.
The encoded characters are then converted back to letters and stored in the encoded string e.
Storing Word Positions:

The starting positions of each word in the encoded string are stored in the terms array. This allows the program to quickly access any word in the encoded string.
User Query:

The user is asked to enter an index.
The getIthElement function is then used to retrieve the word at the given index from the terms array and store it in the out array.
The extracted word is displayed to the user.
*/
    
#include <stdio.h>
#include <string.h>

#define _CRT_SECURE_NO_WARNINGS
#define MAX_STR 200
#define MAX_TERMS 20

// Function prototypes
int CharToInt(char ch);
char IntToChar(int c);
void encode(char str[], char key[], char e[], char* terms[]);
void getIthElement(char e[], char* terms[], int i, char out[]);

int main() {
    char str[MAX_STR];
    printf("Enter your sentence to encode it please!:\n");
    gets(str);

    char key[MAX_STR];
    printf("Enter your key please!:\n");
    gets(key);

    char e[MAX_STR];
    char* terms[MAX_TERMS];
    char out[MAX_STR];

    // Encode the string using the key
    encode(str, key, e, terms);

    // Print the encoded string
    puts(e);

    // Prompt user to enter an index
    int i;
    printf("Enter your number:\n");
    if (scanf("%d", &i) < 0) {
        printf("Invalid value\n");
        return 1;
    }

    // Get the ith element from the encoded string
    getIthElement(e, terms, i, out);
    printf("\nWhen i is %d, output is: %s.\n", i, out);

    return 0;
}

// Converts a character to an integer (position in alphabet)
int CharToInt(char ch) {
    if (ch >= 'a' && ch <= 'z') {
        return ch - 'a';
    } else if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A';
    }
    return -1; // Return -1 if character is not a letter
}

// Converts an integer to a character (position in alphabet)
char IntToChar(int c) {
    if (c >= 0 && c < 26) {
        return 'a' + c;
    }
    return '\0'; // Return null character if c is out of range
}

// Encodes the input string using the key and stores the result in e
void encode(char str[], char key[], char e[], char* terms[]) {
    int j = 0;
    int temp[MAX_STR];
    int str_len = strlen(str);
    int key_len = strlen(key);

    for (int i = 0; i < str_len; i++) {
        // If the character is not a letter, copy it directly
        if ((str[i] < 'a' || str[i] > 'z') && (str[i] < 'A' || str[i] > 'Z')) {
            e[i] = str[i];
        } else {
            // Calculate encoded character position
            int str_pos = CharToInt(str[i]);
            int key_pos = CharToInt(key[j]);
            if (str_pos == -1 || key_pos == -1) {
                e[i] = str[i];
                continue;
            }
            int encoded_pos = (str_pos + key_pos) % 26;
            e[i] = IntToChar(encoded_pos);

            j = (j + 1) % key_len; // Move to the next character in the key
        }
    }
    e[str_len] = '\0'; // Null-terminate the encoded string

    // Split the encoded string into terms
    int u = 0;
    if (e[0] != ' ') {
        terms[u++] = &e[0];
    }
    for (int k = 0; k < str_len; k++) {
        if (e[k] == ' ' && e[k + 1] >= 'a' && e[k + 1] <= 'z') {
            terms[u++] = &e[k + 1];
        }
    }
    for (int i = u; i < MAX_TERMS; i++) {
        terms[i] = NULL;
    }
}

// Retrieves the ith term from the encoded string
void getIthElement(char e[], char* terms[], int i, char out[]) {
    if (terms[i] == NULL) {
        out[0] = '\0';
        return;
    }

    char* ptr = terms[i];
    int n = 0;
    while (*ptr != ' ' && *ptr != '\0') {
        out[n++] = *ptr++;
    }
    out[n] = '\0'; // Null-terminate the output string
}
