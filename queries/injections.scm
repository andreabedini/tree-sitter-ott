; A hom's body is code for the back end its name says. The [[ ]] quotations
; inside it are child nodes, so they are left out of the injected text.

((hom
  name: (hom_name) @_name
  body: (hom_body) @injection.content)
  (#any-of? @_name "tex" "texvar" "tex-preamble")
  (#set! injection.language "latex"))

((hom
  name: (hom_name) @_name
  body: (hom_body) @injection.content)
  (#any-of? @_name "typst" "typstvar" "typst-preamble")
  (#set! injection.language "typst"))

((hom
  name: (hom_name) @_name
  body: (hom_body) @injection.content)
  (#any-of? @_name "ocaml" "ocamlvar")
  (#set! injection.language "ocaml"))

((hom
  name: (hom_name) @_name
  body: (hom_body) @injection.content)
  (#eq? @_name "coq")
  (#set! injection.language "coq"))
