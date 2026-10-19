; Types (t): metavariable declarations, grammar rules, and groups of
; definitions and functions. Inside is the productions, definitions or
; functions.

(metavar_declaration) @class.around

(rule
  (production)+ @class.inside) @class.around

(definitions
  (definition)+ @class.inside) @class.around

(functions
  (function)+ @class.inside) @class.around

; Functions (f): definitions of judgements and functions. Inside is the
; rules or clauses after "by".

(definition
  [
    (inference_rule)
    (formula_rule)
    (comment_block)
  ]+ @function.inside) @function.around

(function
  (clause)+ @function.inside) @function.around

; Arguments (a): the elements of a production, definition or function, and
; binding specifications.

(elements
  (_) @parameter.inside @parameter.around)

(bindspec_group
  (bindspec) @parameter.inside @parameter.around)

; Entries (e): the items of a rule, definition, function or section, and
; the premises and conclusions of an inference rule.

[
  (production)
  (inference_rule)
  (formula_rule)
  (clause)
  (subrule)
  (context_rule)
  (substitution)
  (free_variable)
  (priority)
  (hom_section_item)
] @entry.around

[
  (premise)
  (conclusion)
  (formula)
] @entry.inside @entry.around

; Comments (c)

(line_comment)+ @comment.around

(block_comment) @comment.around

(hom
  name: (hom_name) @_name
  body: (hom_body) @comment.inside
  (#eq? @_name "com")) @comment.around

(comment_block
  body: (hom_body) @comment.inside) @comment.around
