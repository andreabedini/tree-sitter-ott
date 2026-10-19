// External scanner for Ott source files.
//
// Ott's lexer has modes (haskott GRAMMAR.md §2.3). The parser state tells
// us which mode we are in through the set of valid tokens:
//
//   hom name, hom text, quotation text valid -> hom / hom_star (§2.6)
//   element valid                            -> elements (§2.5)
//   line or rule break valid                 -> defnlang (§2.9)
//   otherwise                                -> metalang (§2.4), where only
//                                               block comments are ours
//
// The one piece of state is for a block comment in a definition that
// closes in the middle of a line: Ott lexes the rest of that line as a
// rule line (§2.9).

#include "tree_sitter/parser.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum TokenType {
  LINE,
  RULE_BREAK,
  BLOCK_COMMENT,
  ELEMENT,
  QUOTED_ELEMENT,
  ELEMENT_DOTS,
  COMP_LEFT,
  COMP_MIDDLE,
  COMP_RIGHT,
  COMP_IN,
  HOM_NAME,
  HOM_TEXT,
  HOM_STAR_TEXT,
  QUOTATION_TEXT,
  ERROR_SENTINEL,
};

typedef struct {
  // The last token was a block comment in a definition, which ended at
  // this column.
  bool after_comment;
  uint32_t comment_end_column;
} Scanner;

void *tree_sitter_ott_external_scanner_create(void) { return calloc(1, sizeof(Scanner)); }

void tree_sitter_ott_external_scanner_destroy(void *payload) { free(payload); }

unsigned tree_sitter_ott_external_scanner_serialize(void *payload, char *buffer) {
  Scanner *scanner = payload;
  if (!scanner->after_comment) return 0;
  memcpy(buffer, &scanner->comment_end_column, sizeof scanner->comment_end_column);
  return sizeof scanner->comment_end_column;
}

void tree_sitter_ott_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  Scanner *scanner = payload;
  scanner->after_comment = length == sizeof scanner->comment_end_column;
  scanner->comment_end_column = 0;
  if (scanner->after_comment) memcpy(&scanner->comment_end_column, buffer, length);
}

// §2.2 character classes

static bool is_nnws(int32_t c) { return c == ' ' || c == '\t' || c == '\f'; }
static bool is_newline(int32_t c) { return c == '\n' || c == '\r'; }
static bool is_ws(int32_t c) { return is_nnws(c) || is_newline(c); }

static void advance(TSLexer *lexer) { lexer->advance(lexer, false); }
static void skip(TSLexer *lexer) { lexer->advance(lexer, true); }

static void skip_ws(TSLexer *lexer) {
  while (!lexer->eof(lexer) && is_ws(lexer->lookahead)) skip(lexer);
}

static void advance_newline(TSLexer *lexer, bool as_skip) {
  if (lexer->lookahead == '\r') lexer->advance(lexer, as_skip);
  if (lexer->lookahead == '\n') lexer->advance(lexer, as_skip);
}

// A whitespace-delimited word, of which only the first few characters are
// kept: enough to compare with keywords and special tokens.
#define WORD_MAX 32

typedef struct {
  char text[WORD_MAX + 1];
  unsigned length;
  int32_t last;
} Word;

static void read_word(TSLexer *lexer, Word *word) {
  word->length = 0;
  word->last = 0;
  while (!lexer->eof(lexer) && !is_ws(lexer->lookahead)) {
    int32_t c = lexer->lookahead;
    if (word->length < WORD_MAX) word->text[word->length] = c < 128 ? (char)c : '?';
    word->length++;
    word->last = c;
    advance(lexer);
  }
  word->text[word->length < WORD_MAX ? word->length : WORD_MAX] = '\0';
}

static bool word_is(const Word *word, const char *s) {
  return word->length <= WORD_MAX && strcmp(word->text, s) == 0;
}

// §2.10: from just after an opening ">>" to its matching "<<". Block
// comments nest, and a '%' hides the rest of its line. An unclosed comment
// runs to the end of the file.
static bool finish_block_comment(TSLexer *lexer) {
  unsigned depth = 1;
  while (!lexer->eof(lexer)) {
    int32_t c = lexer->lookahead;
    advance(lexer);
    if (c == '%') {
      while (!lexer->eof(lexer) && !is_newline(lexer->lookahead)) advance(lexer);
    } else if (c == '>' && lexer->lookahead == '>') {
      advance(lexer);
      depth++;
    } else if (c == '<' && lexer->lookahead == '<') {
      advance(lexer);
      if (--depth == 0) break;
    }
  }
  lexer->mark_end(lexer);
  lexer->result_symbol = BLOCK_COMMENT;
  return true;
}

// metalang and bindspec: a ">>" anywhere opens a block comment.
static bool scan_metalang(TSLexer *lexer) {
  skip_ws(lexer);
  if (lexer->lookahead != '>') return false;
  advance(lexer);
  if (lexer->lookahead != '>') return false;
  advance(lexer);
  return finish_block_comment(lexer);
}

// §2.5: every run of non-whitespace is one token. The special tokens win
// only when they are the whole run, and only where the parser can use
// them; elsewhere they are ordinary elements.
static bool scan_element(TSLexer *lexer, const bool *valid) {
  skip_ws(lexer);
  if (lexer->eof(lexer)) return false;
  Word word;
  read_word(lexer, &word);
  lexer->mark_end(lexer);

  enum TokenType special = ELEMENT;
  if (word_is(&word, "::")) return false;
  if (word_is(&word, "..") || word_is(&word, "...") || word_is(&word, "....")) special = ELEMENT_DOTS;
  else if (word_is(&word, "</")) special = COMP_LEFT;
  else if (word_is(&word, "//")) special = COMP_MIDDLE;
  else if (word_is(&word, "/>")) special = COMP_RIGHT;
  else if (word_is(&word, "IN")) special = COMP_IN;
  else if (word.length >= 2 && word.text[0] == '\'' && word.last == '\'') special = QUOTED_ELEMENT;

  if (special != ELEMENT && valid[special]) {
    lexer->result_symbol = special;
    return true;
  }
  if (!valid[ELEMENT]) return false;
  lexer->result_symbol = ELEMENT;
  return true;
}

// §3.6: the name of a hom is a hom-mode string, so it ends at whitespace,
// brackets, braces and dots.
static bool scan_hom_name(TSLexer *lexer) {
  skip_ws(lexer);
  unsigned length = 0;
  while (!lexer->eof(lexer) && !is_ws(lexer->lookahead) && !strchr("[]{}.", lexer->lookahead)) {
    advance(lexer);
    length++;
  }
  if (length == 0) return false;
  lexer->mark_end(lexer);
  lexer->result_symbol = HOM_NAME;
  return true;
}

// §2.6: hom text runs up to "[[" or the closing "}}" ("*}}" in a {{* *}}
// hom). Leading and trailing whitespace is left to the extras, so a text
// token starts and ends with a visible character.
static bool scan_hom_text(TSLexer *lexer, bool star) {
  skip_ws(lexer);
  bool content = false;
  while (!lexer->eof(lexer)) {
    int32_t c = lexer->lookahead;
    if (c == '[') {
      advance(lexer);
      if (lexer->lookahead == '[') break;
    } else if (!star && c == '}') {
      advance(lexer);
      if (lexer->lookahead == '}') break;
    } else if (star && c == '*') {
      advance(lexer);
      if (lexer->lookahead == '}') {
        advance(lexer);
        if (lexer->lookahead == '}') break;
      }
    } else {
      advance(lexer);
      if (is_ws(c)) continue;
    }
    content = true;
    lexer->mark_end(lexer);
  }
  if (!content) return false;
  lexer->result_symbol = star ? HOM_STAR_TEXT : HOM_TEXT;
  return true;
}

// The inside of a [[ ]] quotation, a symbolic term (stage two).
static bool scan_quotation(TSLexer *lexer) {
  skip_ws(lexer);
  bool content = false;
  while (!lexer->eof(lexer)) {
    int32_t c = lexer->lookahead;
    advance(lexer);
    if (c == ']' && lexer->lookahead == ']') break;
    if (is_ws(c)) continue;
    content = true;
    lexer->mark_end(lexer);
  }
  if (!content) return false;
  lexer->result_symbol = QUOTATION_TEXT;
  return true;
}

// Advance to the end of the line, leaving trailing whitespace and the
// newline out of the token.
static void finish_line(TSLexer *lexer) {
  lexer->mark_end(lexer);
  while (!lexer->eof(lexer) && !is_newline(lexer->lookahead)) {
    int32_t c = lexer->lookahead;
    advance(lexer);
    if (!is_nnws(c)) lexer->mark_end(lexer);
  }
}

static bool line(TSLexer *lexer, const bool *valid) {
  if (!valid[LINE]) return false;
  finish_line(lexer);
  lexer->result_symbol = LINE;
  return true;
}

// Words that end a definition when they begin a line (§2.9, §6).
static const char *const LINE_KEYWORDS[] = {
  "grammar", "embed", "homs", "metavar", "indexvar", "subrules",
  "contextrules", "substitutions", "freevars", "parsing", "defns",
  "begincoqsection", "endcoqsection", "coqvariable", "funs", "defn", "fun",
};

// §2.9: lexed one line at a time. Lines that start a keyword, a dashed
// line, a comment line or a {{ com }} block are left to the grammar's own
// tokens; any other non-blank line is one token. Blank lines are
// whitespace, but where a rule may end, the first one is announced by a
// zero-width break at the end of the line before it.
static bool scan_defnlang(Scanner *scanner, TSLexer *lexer, const bool *valid) {
  uint32_t column = lexer->get_column(lexer);
  bool at_line_start = column == 0;
  // A break ends here, however far the scan below looks ahead.
  lexer->mark_end(lexer);
  if (scanner->after_comment && column == scanner->comment_end_column) {
    while (!lexer->eof(lexer) && is_nnws(lexer->lookahead)) skip(lexer);
    if (!lexer->eof(lexer) && !is_newline(lexer->lookahead)) return line(lexer, valid);
  }
  for (;;) {
    while (!lexer->eof(lexer) && is_nnws(lexer->lookahead)) skip(lexer);
    if (lexer->eof(lexer)) return false;
    if (is_newline(lexer->lookahead)) {
      if (at_line_start && valid[RULE_BREAK]) {
        lexer->result_symbol = RULE_BREAK;
        return true;
      }
      advance_newline(lexer, true);
      at_line_start = true;
      continue;
    }
    // Something follows on the line of the previous token: an annotation
    // or hom after a dashed line, or an error. Not ours.
    if (!at_line_start) return false;
    break;
  }

  switch (lexer->lookahead) {
    case '%':
      return false;

    case '-': {
      unsigned dashes = 0;
      while (lexer->lookahead == '-' && dashes < 4) {
        advance(lexer);
        dashes++;
      }
      if (dashes == 4) return false;
      return line(lexer, valid);
    }

    case '>':
      advance(lexer);
      if (lexer->lookahead != '>') return line(lexer, valid);
      advance(lexer);
      while (!lexer->eof(lexer) && is_nnws(lexer->lookahead)) advance(lexer);
      if (lexer->eof(lexer) || is_newline(lexer->lookahead)) {
        finish_block_comment(lexer);
        scanner->after_comment = true;
        scanner->comment_end_column = lexer->get_column(lexer);
        return true;
      }
      return line(lexer, valid);

    case '{':
      advance(lexer);
      if (lexer->lookahead != '{') return line(lexer, valid);
      advance(lexer);
      while (!lexer->eof(lexer) && is_nnws(lexer->lookahead)) advance(lexer);
      {
        Word word;
        read_word(lexer, &word);
        if (word_is(&word, "com")) return false;
      }
      return line(lexer, valid);

    default: {
      Word word;
      read_word(lexer, &word);
      for (size_t i = 0; i < sizeof LINE_KEYWORDS / sizeof *LINE_KEYWORDS; i++) {
        if (word_is(&word, LINE_KEYWORDS[i])) return false;
      }
      return line(lexer, valid);
    }
  }
}

bool tree_sitter_ott_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid) {
  Scanner *scanner = payload;
  Scanner previous = *scanner;
  scanner->after_comment = false;

  // During error recovery every token is valid and the mode is unknown;
  // only block comments are recognised.
  if (valid[ERROR_SENTINEL]) return scan_metalang(lexer);

  if (valid[HOM_NAME]) return scan_hom_name(lexer);
  if (valid[HOM_TEXT]) return scan_hom_text(lexer, false);
  if (valid[HOM_STAR_TEXT]) return scan_hom_text(lexer, true);
  if (valid[QUOTATION_TEXT]) return scan_quotation(lexer);
  if (valid[ELEMENT]) return scan_element(lexer, valid);
  if (valid[LINE] || valid[RULE_BREAK]) {
    *scanner = previous;
    bool found = scan_defnlang(scanner, lexer, valid);
    if (found && lexer->result_symbol != BLOCK_COMMENT) scanner->after_comment = false;
    return found;
  }
  return scan_metalang(lexer);
}
