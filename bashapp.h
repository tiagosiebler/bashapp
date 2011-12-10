/*

Copyright (c) 2011 James Robson

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/


#ifndef BASHAPP_HEADER_INCLUDED
#define BASHAPP_HEADER_INCLUDED

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_KEY_LEN 32

#define USAGE "\n\
Usage: bashapp PATH_TO_BASH_SCRIPT APPNAME [KEY]\n\
\n\
E.g.\n\n\
  Create 'MyApp' with the default encryption key (recommended):\n\
  bashapp script.sh MyApp\n\
\n\
  Create 'MyApp' with your own key:\n\
  bashapp script.sh MyApp s#ZcrE33t\n\n"

#define C_TEMPLATE "#include <stdio.h>\n\
#include <stdlib.h>\n\
#include <unistd.h>\n\
#include <string.h>\n\
#define SCR_SIZE ___BASH_SCR_SIZE___\n\
#define KEY_LEN ___KEY_LEN___\n\
unsigned char key[KEY_LEN] = ___KEY___\n\
unsigned char script[SCR_SIZE] = ___BASH_SCR___\n\
char *xor_enc() {\n\
    int i, j = 0;\n\
    int k_len = KEY_LEN - 1;\n\
    char *ret = calloc(SCR_SIZE+1, sizeof(char));\n\
    for (i = 0; i < SCR_SIZE; i++) {\n\
        ret[i] = script[i] ^ key[j];\n\
        j++;\n\
        j = j > k_len ? 0 : j;\n\
    }\n\
    return ret;\n\
}\n\
int main() {\n\
    int i;\n\
    char *src;\n\
    int fd[2];\n\
    pid_t pid;\n\
    src = xor_enc();\n\
    if (pipe(fd) < 0)\n\
        return EXIT_FAILURE;\n\
    if ((pid = fork()) < 0)\n\
        return EXIT_FAILURE;\n\
    else if (pid != 0) {\n\
        close(fd[1]);\n\
        dup2(fd[0], STDIN_FILENO);\n\
        execlp(\"bash\", \"bash\", (char *)0);\n\
    } else {\n\
        close(fd[0]);\n\
        write(fd[1], src, SCR_SIZE);\n\
    }\n\
    free(src); src = NULL;\n\
    return EXIT_SUCCESS;\n\
}\n"

#define MAKE_APP "#!/bin/bash\n\
rm -fr ___APPNAME___.app\n\
mkdir -p ___APPNAME___.app/Contents/MacOS/\n\
gcc ___APPNAME___.c -o ___APPNAME___.app/Contents/MacOS/___APPNAME___\n\
chmod a+x ___APPNAME___.app/Contents/MacOS/___APPNAME___\n"


char *replace(const char *s, const char *olds, const char *news)
{
    char *ret;
    int i, count = 0;
    size_t newlen = strlen(news);
    size_t oldlen = strlen(olds);

    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], olds) == &s[i]) {
            count++;
            i += oldlen - 1;
        }
    }

    ret = malloc(i + 1 + count * (newlen - oldlen));

    if (ret == NULL) exit(EXIT_FAILURE);

    i = 0;
    while (*s) {
        if (strstr(s, olds) == s) {
            strcpy(&ret[i], news);
            i += newlen;
            s += oldlen;
        } else
            ret[i++] = *s++;
    }
    ret[i] = '\0';

    return ret;
}

/*
    Dynamically append strings 

    Usage:
        char *a = "zero";
        cats(&a, ",");
        cats(&a, "one");
        cats(&a, ",");
        cats(&a, "two");
        // ... use 'a' somewhere
        cats(&a, NULL); // free 'a'
*/
void cats(char **str, const char *str2) {
    char *tmp = NULL;

    // Reset *str
    if ( *str != NULL && str2 == NULL ) {
        free(*str);
        *str = NULL;
        return;
    }

    // Initial copy
    if (*str == NULL) {
        *str = calloc( strlen(str2)+1, sizeof(char) );
        memcpy( *str, str2, strlen(str2) );
    }
    else { // Append
        tmp = calloc( strlen(*str)+1, sizeof(char) );
        memcpy( tmp, *str, strlen(*str) );
        //free(*str); // why u no worky?
        *str = calloc( strlen(*str)+strlen(str2)+1, sizeof(char) );
        memcpy( *str, tmp, strlen(tmp) );
        memcpy( *str + strlen(*str), str2, strlen(str2) );
        free(tmp);
    }

} // cats()


/*
Convert char value to HEX
*/
char *atoh(unsigned char val) {

    char *ret = calloc(5, sizeof(char));
    snprintf(ret, 5, "0x%x", (unsigned int)val);
    return ret;

} // aoth()


/*
XOR encrypt an array of chars, returns encrypted/decrypted array
*/
char *xor_enc(char *src, int src_sz, char *key, int k_len) {

    int i, j = 0;
    char *ret = calloc(src_sz+1, sizeof(char));

    for (i = 0; i < src_sz; i++) {
        ret[i] = src[i] ^ key[j];
        j++;
        j = j > k_len ? 0 : j;
    }
    return ret;
    
} // xor_enc()


/*
This loads the script pointed to by path into a char array
*/
int load_script(const char *path, char **out) {
    
    FILE *fp = fopen(path, "r");
    int i, j, sz;
    struct stat fi;
    char *tmp = NULL;

    if ( fp == NULL ) {
        perror("Error");
        return EXIT_FAILURE;
    }

    // Get file info, i.e. size, and allocate memory
    stat(path, &fi);
    sz = (int)fi.st_size;
    tmp = calloc( sz+1, sizeof(char) );

    i = 0;
    while ( (j = getc(fp)) != EOF ) {
        tmp[i] = (char)j;
        i++;
    }
    fclose(fp);
    
    *out = tmp;
    return sz;

} // load_script()


int write_file(const char *path, const char *src) {

    FILE *fp;
    fp = fopen(path, "w");    

    if ( fp == NULL ) {
        perror("Error");
        return EXIT_FAILURE;
    }

    fprintf(fp, "%s", src);
    fclose(fp);

    return 1;

} // write_file

/*
Create a C99 hex-based array as a source string.
*/
char *src_hex_array(const char *array, int len) {
    char *ret = "{";
    char *tmp = NULL; 
    char *tmp2 = NULL;
    char cv;
    int i;
    
    tmp = calloc(5,sizeof(char)); // e.g. "0xff";
    for(i=0; i<len; i++) { 
        tmp2 = atoh(array[i]);
        snprintf(tmp, 5, "%s", tmp2);
        cats(&ret, tmp);
        if (i+1 == len) {
            cats(&ret, "};");
        }
        else {
            cats(&ret, ",");
        }
        free(tmp2);
    }

    // Free tmp
    cats(&tmp, NULL);
    return ret;

} // src_hex_array()


/*
Generate a random array of characters
*/
char *rand_array(int char_min, int char_max, int len) {

    char *ret;
    unsigned int i, c;

    srand( time(NULL) );
    ret = malloc( len + 1 );

    for (i = 0; i < len; i++) {
        c = (unsigned int)rand() % ( (char_max - char_min) + 1) + char_min; 
        ret[i] = c;
    }

    return ret;

}

#endif // BASHAPP_HEADER_INCLUDED
