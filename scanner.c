#include <stdio.h>
/* character classification macros */
#include <ctype.h>
#include <string.h>
#include "Parser.h"

/* Le lexeme courant est partage avec le parser pour les erreurs syntaxiques. */
char token_buffer[256];
/* Etat interne du tampon et numero de ligne courant. */
static size_t token_buffer_length = 0;
static int token_buffer_overflow = 0;
static unsigned int line_number = 1;

void clear_buffer(){
        /* Reinitialise le texte avant de lire un nouveau token. */
        token_buffer_length = 0;
        token_buffer_overflow = 0;
        token_buffer[0] = '\0';
}

void buffer_char(int c){
        /* Garde une place pour le terminateur nul de la chaine C. */
        if (token_buffer_length < sizeof token_buffer - 1) {
                token_buffer[token_buffer_length++] = (char)c;
                token_buffer[token_buffer_length] = '\0';
        } else {
                token_buffer_overflow = 1;
        }
}


token check_reserved(void)
{
        /* Les mots reserves sont reconnus; les autres mots sont des identifiants. */
        if (strcmp(token_buffer, "begin") == 0) return BEGIN;
        if (strcmp(token_buffer, "end") == 0) return END;
        if (strcmp(token_buffer, "read") == 0) return READ;
        if (strcmp(token_buffer, "write") == 0) return WRITE;
        return ID;
}

void lexical_error(int in_char)
{
        /* Affiche le caractere invalide de facon lisible quand c'est possible. */
        if (isprint((unsigned char)in_char)) {
                fprintf(stderr, "Lexical error on line %u: unexpected character '%c' (code %d)\n",
                        line_number, in_char, in_char);
        } else {
                fprintf(stderr, "Lexical error on line %u: unexpected character (code %d)\n",
                        line_number, in_char);
        }
}

static void lexical_error_token_too_long(void)
{
        /* Signale un lexeme qui depasse la capacite de token_buffer. */
        fprintf(stderr, "Stored lexeme prefix: %s\n", token_buffer);
        fprintf(stderr, "Lexical error on line %u: token exceeds maximum length of %zu characters\n",
                line_number, sizeof token_buffer - 1);
}

token scanner(void)
{
  int in_char, c ;
  clear_buffer();
        /* Ignore les espaces jusqu'au prochain caractere utile ou la fin du fichier. */
  while ((in_char = getchar()) != EOF) {
        if (in_char == '\n')
                line_number++;
        if (isspace(in_char))
                continue ; /* do nothing */
        else if (isalpha(in_char)) {
                /* Lit un mot complet, puis le classe en mot reserve ou identifiant. */
                /* ID ::= LETTER | ID LETTER
                 *               | ID DIGIT
                 *               | ID UNDERSCORE
                 */
                buffer_char(in_char);
                for ( c=getchar() ; isalnum(c) || c== '_';
                        c=getchar() )   buffer_char(c);
                ungetc(c,stdin);
                if (token_buffer_overflow) {
                        lexical_error_token_too_long();
                        clear_buffer();
                        continue;
                }
                return check_reserved();
        } else if (isdigit(in_char)) {
                /* Lit un entier ou, si un point suivi de chiffres apparait, un reel. */
                /* INTLITERAL ::= DIGIT
                 *                    | INTLITERAL DIGIT
                 * REALLITERAL ::= DIGIT+ '.' DIGIT+
                 */
                buffer_char(in_char);
                for ( c=getchar() ; isdigit(c) ;
                        c=getchar() )
                    buffer_char(c);
                                if (c == '.') {
                                        c = getchar();
                                        if (isdigit(c)) {
                                                buffer_char('.');
                                                do {
                                                        buffer_char(c);
                                                        c = getchar();
                                                } while (isdigit(c));
                                                ungetc(c,stdin);
                                                if (token_buffer_overflow) {
                                                        lexical_error_token_too_long();
                                                        clear_buffer();
                                                        continue;
                                                }
                                                return REALLITERAL;
                                        }
                                        lexical_error('.');
                                        ungetc(c,stdin);
                                        if (token_buffer_overflow) {
                                                lexical_error_token_too_long();
                                                clear_buffer();
                                                continue;
                                        }
                                        return INTLITERAL;
                                }
                    ungetc(c,stdin);
                    if (token_buffer_overflow) {
                            lexical_error_token_too_long();
                            clear_buffer();
                            continue;
                    }
                    return INTLITERAL;
        } else if (in_char == '(') {
                /* Les separateurs et operateurs simples produisent chacun un token. */
                buffer_char(in_char);
                return LPAREN;
        } else if (in_char == ')') {
                buffer_char(in_char);
                return RPAREN;
        } else if (in_char == ';') {
                buffer_char(in_char);
                return SEMICOLON;
        } else if (in_char == ',') {
                buffer_char(in_char);
                return COMMA;
        } else if (in_char == '+') {
                buffer_char(in_char);
                return PLUSOP;
        } else if (in_char == ':') {
                /* Le deux-points est valide uniquement s'il est suivi de '='. */
                c=getchar();
                if (c == '=') {
                        buffer_char(in_char);
                        buffer_char(c);
                        return ASSIGNOP;
                } else {
                        ungetc(c,stdin);
                        lexical_error(in_char);
                }
        } else if (in_char == '-') {
                /* Deux tirets ouvrent un commentaire; un seul est l'operateur moins. */
                c=getchar();
                if (c == '-') {
                  do  in_char = getchar();
                  while (in_char != '\n');
                  line_number++;
                } else {
                  ungetc(c,stdin);
                  buffer_char(in_char);
                  return MINUSOP;
                }
        } else
                lexical_error(in_char);
        }/* while */
        /* Aucun caractere restant: le scanner a atteint la fin du fichier. */
        return SCANEOF;
}/* scanner() */