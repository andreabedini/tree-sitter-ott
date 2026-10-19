; Rule lines, function clauses and [[ ]] quotations are symbolic terms of
; the grammar the file defines; haskott's language server highlights them
; with semantic tokens.

; Keywords

[
  "metavar"
  "indexvar"
  "grammar"
  "defns"
  "defn"
  "funs"
  "fun"
  "subrules"
  "contextrules"
  "substitutions"
  "freevars"
  "embed"
  "parsing"
  "homs"
  "begincoqsection"
  "endcoqsection"
  "coqvariable"
  "by"
] @keyword

[
  "single"
  "multiple"
  "left"
  "right"
  "non"
] @keyword.modifier

[
  "bind"
  "in"
  "names"
  "distinctnames"
  "union"
  "IN"
] @keyword.operator

; Comments

[
  (line_comment)
  (block_comment)
] @comment @spell

(hom
  name: (hom_name) @_name
  body: (hom_body) @comment.documentation @spell
  (#eq? @_name "com"))

(comment_block
  body: (hom_body) @comment.documentation @spell)

; Declarations

(metavar_declaration
  (root
    name: (identifier) @type.definition))

(rule
  (root
    name: (identifier) @type.definition))

[
  (rule
    prefix: (identifier))
  (definitions
    prefix: (identifier))
  (definition
    prefix: (identifier))
  (hom_section
    prefix: (identifier))
] @string.special

(production
  name: (identifier) @constructor)

(hom_section_item
  name: (identifier) @constructor)

(priority
  left: (identifier) @constructor
  right: (identifier) @constructor)

(category) @attribute

(definitions
  name: (identifier) @module)

(functions
  name: (identifier) @module)

[
  (definition
    name: (identifier))
  (function
    name: (identifier))
  (substitution
    name: (identifier))
  (free_variable
    name: (identifier))
] @function

(function
  result: (identifier) @type)

(rule_line
  name: (identifier) @label)

(subrule
  lower: (identifier) @type
  upper: (identifier) @type)

(context_rule
  context: (identifier) @type
  target: (identifier) @type
  hole: (identifier) @type)

(substitution
  within: (identifier) @type
  for: (identifier) @type)

(free_variable
  within: (identifier) @type
  for: (identifier) @type)

[
  (coq_section_begin
    name: (identifier))
  (coq_section_end
    name: (identifier))
  (coq_variable
    name: (identifier))
] @module

; Productions and forms

(quoted_element) @string

(dots) @punctuation.special

(comprehension
  [
    "</"
    "//"
    "/>"
  ] @punctuation.bracket)

; Binding specifications

(bind_comprehension
  [
    "</"
    "//"
    "/>"
  ] @punctuation.bracket)

[
  "(+"
  "+)"
  "("
  ")"
  "{}"
] @punctuation.bracket

[
  "="
  "#"
] @operator

; Homs

(hom_name) @attribute

[
  "{{"
  "}}"
  "{{*"
  "*}}"
  "[["
  "]]"
] @punctuation.special

; Punctuation

[
  "::="
  "<::"
  "_::"
  "<="
] @operator

[
  "::"
  "|"
  ","
] @punctuation.delimiter

(dashes) @punctuation.delimiter
