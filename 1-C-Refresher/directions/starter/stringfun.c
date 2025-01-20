#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define BUFFER_SZ 50

//prototypes
void usage(char *);
void print_buff(char *, int);
int  setup_buff(char *, char *, int);
//prototypes for functions to handle required functionality
int  count_words(char *, int, int);
void reverse_string(char *, int);
void print_words(char *, int);
int replace_substring(int, char *[], char *, char *, int);
//add additional prototypes here


int setup_buff(char *buff, char *user_str, int len){
    if (strlen(user_str) > BUFFER_SZ) {
        return -1;
    }

    const char *source = user_str;
    char *destination = buff;
    int input_length = 0;
    int is_space_sequence = 0;

    while (*source != '\0' && (destination - buff) < len) {
        if (*source == ' ' || *source == '\t') {
            if (!is_space_sequence) {
                *destination++ = ' ';
                is_space_sequence = 1;
            }
        } else {
            *destination++ = *source;
            is_space_sequence = 0;
        }
        source++;
        input_length++;
    }

    while ((destination - buff) < len) {
        *destination++ = '.';
    }

    return input_length;
}

void print_buff(char *buff, int len){
    printf("Buffer:  ");
    for (int i=0; i<len; i++){
        putchar(*(buff+i));
    }
    putchar('\n');
}

void usage(char *exename){
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);

}

int count_words(char *buff, int len, int str_len) {
    int word_count = 0;
    int is_inside_word = 0;

    for (int i = 0; i < len && i < str_len; i++) {
        if (buff[i] != ' ') {
            if (!is_inside_word) {
                word_count++;
                is_inside_word = 1;
            }
        } else {
            is_inside_word = 0; 
        }
    }

    return word_count;
}

//ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS

void reverse_string(char *buff, int len) {
    char *start = buff;
    char *end = buff + len - 1;
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }

    printf("Reversed String: ");
    for (int i = 0; i < len; i++) {
        putchar(buff[i]);
    }
    putchar('\n');
}

void print_words(char *buff, int len) {
    printf("Word Print\n");
    printf("----------\n");
    int word_count = 1;
    char *start = buff;
    while (*start != '\0' && (start - buff) < len) {
        if (*start != ' ') {
            char *word_start = start;
            while (*start != ' ' && *start != '\0' && (start - buff) < len) {
                start++;
            }
            int word_len = start - word_start;
            printf("%d. ", word_count++);
            for (char *p = word_start; p < start; p++) {
                putchar(*p);
            }
            printf(" (%d)\n", word_len);
        } else {
            start++;
        }
    }
}

int replace_substring(int argc, char *argv[], char *input_string, char *output_buffer, int buffer_size) {
    if (argc != 5) {
        printf("Error: Invalid number of arguments for -x option.\n");
        return -1;
    }

    char *arg1 = argv[3]; 
    char *arg2 = argv[4]; 

    char *found = strstr(input_string, arg1);
    if (found == NULL) {
        printf("Substring '%s' not found in '%s'.\n", arg1, input_string);
        return -2;
    }

    int arg1_len = strlen(arg1);
    int arg2_len = strlen(arg2);
    int input_len = strlen(input_string);

    int replacement_len = input_len - arg1_len + arg2_len;
    if (replacement_len > buffer_size) {
        printf("Error: Replacement would exceed buffer size.\n");
        return -3;
    }

    strncpy(output_buffer, input_string, found - input_string);
    output_buffer[found - input_string] = '\0';

    strcat(output_buffer, arg2);

    strcat(output_buffer, found + arg1_len);

    printf("Modified String: %s\n", output_buffer);

    return 0;
}

int main(int argc, char *argv[]){

    char *buff;             //placehoder for the internal buffer
    char *input_string;     //holds the string provided by the user on cmd line
    char opt;               //used to capture user option from cmd line
    int  rc;                //used for return codes
    int  user_str_len;      //length of user supplied string

    //TODO:  #1. WHY IS THIS SAFE, aka what if arv[1] does not exist?
    //      PLACE A COMMENT BLOCK HERE EXPLAINING
    /*
      This is safe because the first argument of the or operator first checks if argc is less than 2,
      if it is, then the second argument is not checked, and the program will exit with a return code of 1.
    */
    if ((argc < 2) || (*argv[1] != '-')){
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1]+1);   //get the option flag

    //handle the help flag and then exit normally
    if (opt == 'h'){
        usage(argv[0]);
        exit(0);
    }

    //WE NOW WILL HANDLE THE REQUIRED OPERATIONS

    //TODO:  #2 Document the purpose of the if statement below
    //      PLACE A COMMENT BLOCK HERE EXPLAINING
    /*
       This if statement checks if the number of arguments is less than 3, if it is, then the usage function is called 
       to tell the user how to use the program, and the program exits with a return code of 1. If the arguments is 3 or more, 
       the program will continue to the next step.
    */
    if (argc < 3){
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; //capture the user input string

    //TODO:  #3 Allocate space for the buffer using malloc and
    //          handle error if malloc fails by exiting with a 
    //          return code of 99
    // CODE GOES HERE FOR #3
    buff = (char *)malloc(BUFFER_SZ);
    if (buff == NULL) {
        exit(99);
    }


    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);     //see todos
    if (user_str_len < 0){
        printf("Error setting up buffer, error = %d", user_str_len);
        exit(2);
    }

    switch (opt){
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);  //you need to implement
            if (rc < 0){
                printf("Error counting words, rc = %d", rc);
                exit(2);
            }
            printf("Word Count: %d\n", rc);
            break;

        case 'r':
            reverse_string(buff, user_str_len);
            break;
        case 'w':
            print_words(buff, user_str_len);
            break;
        case 'x':
            char modified_buffer[BUFFER_SZ];
            rc = replace_substring(argc, argv, input_string, modified_buffer, BUFFER_SZ);
            if (rc < 0) {
                printf("Error replacing substring, rc = %d", rc);
                exit(3);
            }
            break;
        default:
            usage(argv[0]);
            exit(1);
    }

    //TODO:  #6 Dont forget to free your buffer before exiting
    print_buff(buff,BUFFER_SZ);
    free(buff); 
    exit(0);
}

//TODO:  #7  Notice all of the helper functions provided in the 
//          starter take both the buffer as well as the length.  Why
//          do you think providing both the pointer and the length
//          is a good practice, after all we know from main() that 
//          the buff variable will have exactly 50 bytes?
//  
//          PLACE YOUR ANSWER HERE
/*
  Providing both the pointer and the length is a good practice it allows the functions to be easily resused in situations where the buffer size is different.
  This is because the pointer allows the function to access the buffer, and the length allows the function to know how much of the buffer to process.
  Another reason why it is a good practice is becasuse when the size of the buffer is provided, buffer overflow can be avoided.
*/