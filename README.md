# tree-sitter-ott

A [tree-sitter](https://tree-sitter.github.io) grammar for
[Ott](https://github.com/ott-lang/ott) source files (`.ott`), for editors
such as Neovim and Helix that highlight, fold and navigate with tree-sitter.

It follows the specification of Ott's source syntax in haskott's
[`GRAMMAR.md`](../haskott/GRAMMAR.md), which is Ott 0.34 as extended on the
`typst` work of [andreabedini/ott](https://github.com/andreabedini/ott).

## Scope

Ott parses a specification in two stages. This grammar covers stage one, the
metalanguage: metavariable declarations, grammar rules and productions,
binding specifications, homs, embeds, subrules, substitutions, free
variables, parsing priorities, hom sections and the structure of inductive
definitions and functions.

Stage two, the symbolic terms, depends on the grammar the file defines, so it
is left as text: each premise, conclusion, lineless formula and function
clause is one node, and so is the inside of each `[[ ]]` quotation (`term`).
haskott's language server (`haskott lsp`) knows that grammar and is the place
to classify those terms.

The grammar accepts every file Ott parses and gives it the structure Ott
does, but it does not try to reject every file Ott rejects: in an editor a
looser parse keeps more of a broken file highlighted, and haskott's language
server reports Ott's errors. A file Ott rejects may therefore parse here, for
example one with a `{{` nested inside a hom, a production comprehension with
an irregular bound, a keyword such as `left` used unquoted as a name, or a
final `%` comment without a newline.

## Lexer modes

Ott's lexer is context-dependent (`GRAMMAR.md` §2.3). Here the parse state
stands in for the lexer mode, and `src/scanner.c` lexes every token whose
spelling depends on it: production elements, hom names, hom text and
quotations, the lines of a definition body, and block comments, which nest
and are only recognised where Ott recognises them.

The quirks of §6 are reproduced where they decide how a file parses. For
example, a line in a definition that starts with a section keyword ends the
definition, `{{ com` at the start of a line opens a comment block, and a
block comment that closes in the middle of a line is followed by a rule line
made of the rest of it.

## Queries

- `queries/highlights.scm`: keywords, comments, declared names (roots,
  productions, definitions, rule names, categories), hom names, and `com`
  homs as documentation.
- `queries/injections.scm`: the bodies of `tex`, `typst`, `ocaml` and `coq`
  homs, without their `[[ ]]` quotations.
- `queries/folds.scm`.

Capture names follow nvim-treesitter's conventions. The grammar has not yet
been tried in an editor.

`queries/helix` holds Helix's versions (25.07 or later):

- `highlights.scm`: the same highlights with Helix's capture names.
- `textobjects.scm`: types (`t`) are metavariable declarations, grammar
  rules, and groups of definitions or functions. Functions (`f`) are
  `defn` and `fun` blocks. Arguments (`a`) are production elements and
  binding specifications. Entries (`e`) are productions, inference rules,
  function clauses and section items, and their premises and conclusions.
  Comments (`c`) include `com` homs.
- `indents.scm`: indents productions under their rule, rules and clauses
  under `defn` and `fun`, and section items under their keyword.
- `injections.scm`: a link to `queries/injections.scm`.

To use them, link the directory into Helix's runtime and declare the
language in `~/.config/helix/languages.toml`:

```console
$ ln -s "$PWD/queries/helix" ~/.config/helix/runtime/queries/ott
```

```toml
[[language]]
name = "ott"
scope = "source.ott"
file-types = ["ott"]
comment-token = "%"
block-comment-tokens = { start = ">>", end = "<<" }
indent = { tab-width = 2, unit = "  " }

[[grammar]]
name = "ott"
source = { git = "https://github.com/andreabedini/tree-sitter-ott", rev = "<commit>" }
```

and then `hx --grammar fetch` and `hx --grammar build`.

## Building and testing

```console
$ tree-sitter generate
$ tree-sitter test
$ script/corpus.zsh
```

`tree-sitter test` runs the cases in `test/corpus`. `script/corpus.zsh`
parses every file of haskott's acceptance corpus (Ott's `tests/` and
`examples/` and the report's `expressions.ott`) that Ott 0.34 parses, and
fails if any parse has an `ERROR` or `MISSING` node. It reads the list from
haskott's `test/ott-acceptance.tsv`, and finds the checkouts at `../haskott`,
`../../ott` and `../haskell-2010-revised-report/formalisation`; `HASKOTT`,
`HASKOTT_OTT` and `HASKOTT_REPORT_OTT` name them when they are elsewhere.

The generated parser in `src/` is committed, as is usual for tree-sitter
grammars, so that editors can build it without the tree-sitter CLI.
