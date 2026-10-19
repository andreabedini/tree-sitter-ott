; Productions are indented under their rule, rules and clauses under their
; defn or fun, and the items of a section under its keyword. Rules,
; definitions and functions themselves stay at the margin.

[
  (metavar_declaration)
  (rule)
  (definition)
  (function)
  (subrules)
  (context_rules)
  (substitutions)
  (free_variables)
  (parsing)
  (hom_section)
  (embed)
] @indent

; The rules of a definition are separated by blank lines, which belong to
; no node, so a line after them that is indented deeper than "defn" is
; still in the definition.

[
  (definition)
  (function)
] @extend

; A hom's text continues indented when it spans several lines.

(hom_body) @indent
