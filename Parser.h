#ifndef PARSER_H
#define PARSER_H

/* Types de tokens reconnus par le scanner et attendus par le parser. */
typedef enum {
        BEGIN, END, READ, WRITE, SCANEOF, ID, INTLITERAL, REALLITERAL,
        LPAREN, RPAREN, SEMICOLON, COMMA, PLUSOP, MINUSOP, ASSIGNOP
} token;

/* Texte du dernier token, partage pour produire des messages d'erreur. */
extern char token_buffer[256];
extern unsigned int token_line; /* Ligne ou commence le token courant. */
extern unsigned int token_column; /* Colonne ou commence le token courant. */

/* Fonctions fournies par le scanner et le parser. */
token scanner(void);
void system_goal(void);

#endif