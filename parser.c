#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "Parser.h"

/* Un seul token est lu a l'avance puis conserve jusqu'a sa consommation. */
static token lookahead;
static bool has_lookahead = false;

/* Convertit un token en nom lisible pour les diagnostics. */
static const char *token_name(token value)
{
	static const char *names[] = {
		"BEGIN", "END", "READ", "WRITE", "SCANEOF", "ID",
		"INTLITERAL", "REALLITERAL", "LPAREN", "RPAREN", "SEMICOLON",
		"COMMA", "PLUSOP", "MINUSOP", "ASSIGNOP"
	};

	if (value < BEGIN || value > ASSIGNOP)
		return "UNKNOWN";
	return names[value];
}

static token next_token(void)
{
	/* Evite de demander plusieurs fois le meme token au scanner. */
	if (!has_lookahead) {
		lookahead = scanner();
		has_lookahead = true;
	}
	return lookahead;
}

static void syntax_error(token unexpected)
{
	/* Le scanner conserve le lexeme associe au token inattendu. */
	fprintf(stderr, "Syntax error: unexpected %s (lexeme '%s')\n",
		token_name(unexpected), token_buffer);
	exit(EXIT_FAILURE);
}

static void match(token expected)
{
	token actual = next_token();
	if (actual != expected)
		syntax_error(actual);
	/* Le token attendu est consomme; le prochain appel lira le suivant. */
	has_lookahead = false;
}

/* Prototypes des fonctions qui correspondent aux regles de grammaire. */
static void program(void);
static void inst_list(void);
static void inst(void);
static void id_list(void);
static void expression(void);
static void expr_list(void);
static void add_op(void);
static void prim(void);

void system_goal(void)
{
	/* Un programme valide doit commencer par BEGIN et finir au fichier. */
	program();
	match(SCANEOF);
}

static void program(void)
{
	/* Programme ::= BEGIN liste_instructions END. */
	match(BEGIN);
	inst_list();
	match(END);
}

static void inst_list(void)
{
	/* La grammaire impose au moins une instruction. */
	inst();
	while (true) {
		/* ID, READ et WRITE sont les debuts possibles d'une instruction. */
		switch (next_token()) {
		case ID:
		case READ:
		case WRITE:
			inst();
			break;
		default:
			return;
		}
	}
}

static void inst(void)
{
	token tok = next_token();
	/* La premiere unite determine laquelle des trois regles appliquer. */
	switch (tok) {
	case ID:
		/* Affectation ::= ID := expression ; */
		match(ID);
		match(ASSIGNOP);
		expression();
		match(SEMICOLON);
		break;
	case READ:
		/* Lecture ::= read ( liste_identifiants ) ; */
		match(READ);
		match(LPAREN);
		id_list();
		match(RPAREN);
		match(SEMICOLON);
		break;
	case WRITE:
		/* Ecriture ::= write ( liste_expressions ) ; */
		match(WRITE);
		match(LPAREN);
		expr_list();
		match(RPAREN);
		match(SEMICOLON);
		break;
	default:
		syntax_error(tok);
	}
}

static void id_list(void)
{
	/* La liste commence par un identifiant et peut en contenir d'autres. */
	match(ID);
	while (next_token() == COMMA) {
		match(COMMA);
		match(ID);
	}
}

static void expression(void)
{
	token tok;
	/* Expression ::= operande ( (+ | -) operande )*. */
	prim();
	while ((tok = next_token()) == PLUSOP || tok == MINUSOP) {
		add_op();
		prim();
	}
}

static void expr_list(void)
{
	/* Une expression est obligatoire; les suivantes sont separees par des virgules. */
	expression();
	while (next_token() == COMMA) {
		match(COMMA);
		expression();
	}
}

static void add_op(void)
{
	token tok = next_token();
	/* Cette regle n'accepte que les operateurs + et -. */
	if (tok == PLUSOP || tok == MINUSOP)
		match(tok);
	else
		syntax_error(tok);
}

static void prim(void)
{
	token tok = next_token();
	/* Un operande est une expression entre parentheses, un identifiant ou un entier. */
	switch (tok) {
	case LPAREN:
		match(LPAREN);
		expression();
		match(RPAREN);
		break;
	case ID:
		match(ID);
		break;
	case INTLITERAL:
		match(INTLITERAL);
		break;
	default:
		syntax_error(tok);
	}
}

int main(void)
{
	/* L'analyse syntaxique lit le programme depuis l'entree standard. */
	system_goal();
	puts("Syntaxe correcte.");
	return EXIT_SUCCESS;
}