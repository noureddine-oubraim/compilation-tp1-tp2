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
static unsigned int column_number = 0; /* Nombre de caracteres deja lus sur la ligne. */
static unsigned int previous_line_number = 1; /* Position restauree par unread_char. */
static unsigned int previous_column_number = 0; /* Colonne restauree par unread_char. */
static unsigned int last_char_line = 1; /* Ligne du dernier caractere lu. */
static unsigned int last_char_column = 0; /* Colonne du dernier caractere lu. */
unsigned int token_line = 1; /* Ligne de debut du token remis au parser. */
unsigned int token_column = 1; /* Colonne de debut du token remis au parser. */

static int read_char(void)
{
        int value; /* Stocke le caractere lu ou EOF. */

        previous_line_number = line_number; /* Memorise la position avant lecture. */
        previous_column_number = column_number; /* Memorise la colonne avant lecture. */
        value = getchar(); /* Lit un caractere depuis l'entree standard. */
        if (value != EOF) { /* Ignore EOF car ce n'est pas un caractere. */
                last_char_line = line_number; /* Associe le caractere a sa ligne. */
                last_char_column = column_number + 1; /* Associe le caractere a sa colonne. */
                if (value == '\n') { /* Passe a la ligne suivante apres un saut de ligne. */
                        line_number++; /* Incremente la ligne courante. */
                        column_number = 0; /* Recommence les colonnes au debut. */
                } else { /* Traite un caractere ordinaire. */
                        column_number++; /* Avance d'une colonne. */
                } /* Termine la mise a jour de position. */
        } /* Termine le traitement du caractere lu. */
        return value; /* Renvoie le caractere au scanner. */
} /* Termine la lecture avec suivi des positions. */

static void unread_char(int value)
{
        if (value != EOF && ungetc(value, stdin) != EOF) { /* Replace un caractere lu. */
                line_number = previous_line_number; /* Annule son avancee de ligne. */
                column_number = previous_column_number; /* Annule son avancee de colonne. */
        } /* Termine le retour arriere d'un caractere. */
} /* Termine le retour de caractere avec suivi des positions. */

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

static void lexical_error(int in_char, unsigned int error_line, unsigned int error_column)
{
        /* Affiche le caractere invalide de facon lisible quand c'est possible. */
        if (isprint((unsigned char)in_char)) {
                fprintf(stderr, "Lexical error on line %u, column %u: unexpected character '%c' (code %d)\n",
                        error_line, error_column, in_char, in_char);
        } else {
                fprintf(stderr, "Lexical error on line %u, column %u: unexpected character (code %d)\n",
                        error_line, error_column, in_char);
        }
}

static void lexical_error_token_too_long(void)
{
        /* Signale un lexeme qui depasse la capacite de token_buffer. */
        fprintf(stderr, "Stored lexeme prefix: %s\n", token_buffer);
        fprintf(stderr, "Lexical error on line %u, column %u: token exceeds maximum length of %zu characters\n",
                token_line, token_column, sizeof token_buffer - 1);
}

token scanner(void)
{
  int in_char, c ;
  unsigned int character_line, character_column; /* Position du caractere de depart. */
  clear_buffer();
        /* Ignore les espaces jusqu'au prochain caractere utile ou la fin du fichier. */
  while ((in_char = read_char()) != EOF) {
        if (isspace(in_char))
                continue ; /* do nothing */
        token_line = last_char_line; /* Memorise la ligne du debut du token. */
        token_column = last_char_column; /* Memorise la colonne du debut du token. */
        character_line = last_char_line; /* Garde la ligne pour les erreurs lexicales. */
        character_column = last_char_column; /* Garde la colonne pour les erreurs lexicales. */
        if (isalpha(in_char)) { /* Traite un identifiant ou un mot reserve. */
                /* Lit un mot complet, puis le classe en mot reserve ou identifiant. */
                /* ID ::= LETTER | ID LETTER
                 *               | ID DIGIT
                 *               | ID UNDERSCORE
                 */
                buffer_char(in_char);
                for ( c=read_char() ; isalnum(c) || c== '_';
                        c=read_char() )   buffer_char(c);
                unread_char(c); /* Garde le separateur pour le prochain appel. */
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
                for ( c=read_char() ; isdigit(c) ;
                        c=read_char() )
                    buffer_char(c);
                                if (c == '.') {
                                        unsigned int dot_line = last_char_line; /* Retient la ligne du point. */
                                        unsigned int dot_column = last_char_column; /* Retient la colonne du point. */
                                        c = read_char(); /* Lit le caractere apres le point. */
                                        if (isdigit(c)) {
                                                buffer_char('.');
                                                do {
                                                        buffer_char(c);
                                                        c = read_char();
                                                } while (isdigit(c));
                                                unread_char(c); /* Replace le separateur apres le reel. */
                                                if (token_buffer_overflow) {
                                                        lexical_error_token_too_long();
                                                        clear_buffer();
                                                        continue;
                                                }
                                                return REALLITERAL;
                                        }
                                        lexical_error('.', dot_line, dot_column); /* Signale le point mal forme. */
                                        unread_char(c); /* Replace le caractere qui suit le point. */
                                        if (token_buffer_overflow) {
                                                lexical_error_token_too_long();
                                                clear_buffer();
                                                continue;
                                        }
                                        return INTLITERAL;
                                }
                    unread_char(c); /* Replace le separateur apres l'entier. */
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
                c=read_char(); /* Lit le caractere qui pourrait former :=. */
                if (c == '=') {
                        buffer_char(in_char);
                        buffer_char(c);
                        return ASSIGNOP;
                } else {
                        unread_char(c); /* Replace le caractere qui ne complete pas :=. */
                        lexical_error(in_char, character_line, character_column); /* Signale le deux-points seul. */
                }
        } else if (in_char == '-') {
                /* Deux tirets ouvrent un commentaire; un seul est l'operateur moins. */
                                                                c=read_char(); /* Verifie si le second tiret ouvre un commentaire. */
                if (c == '-') {
                                                                        do  in_char = read_char();
                  while (in_char != '\n');
                } else {
                                                                        unread_char(c); /* Replace le caractere qui suit l'operateur moins. */
                  buffer_char(in_char);
                  return MINUSOP;
                }
        } else
                                                                lexical_error(in_char, character_line, character_column); /* Signale le caractere inconnu. */
        }/* while */
        /* Aucun caractere restant: le scanner a atteint la fin du fichier. */
                                token_line = line_number; /* Positionne EOF sur la ligne courante. */
                                token_column = column_number + 1; /* EOF se trouve apres le dernier caractere. */
        return SCANEOF;
}/* scanner() */