/**
 * @file Tree-sitter grammar for Ott source files (.ott)
 * @license BSD-3-Clause
 *
 * Stage one of Ott's syntax: the metalanguage, as specified in haskott's
 * GRAMMAR.md. Symbolic terms (rule lines, function clauses and the insides
 * of [[ ]] quotations) are kept as text; they depend on the grammar the
 * file defines, and haskott's language server classifies them.
 *
 * Ott's lexer is context-dependent (GRAMMAR.md §2.3). Here the parse state
 * stands in for the lexer mode, and every token whose spelling depends on
 * the mode is lexed by src/scanner.c.
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

const sepBy1 = (sep, rule) => seq(rule, repeat(seq(sep, rule)));

module.exports = grammar({
  name: 'ott',

  externals: $ => [
    // defnlang (§2.9)
    $._line,
    $._rule_break,
    // comments (§2.10); block comments nest and depend on the mode
    $.block_comment,
    // elements (§2.5)
    $.element,
    $.quoted_element,
    $._element_dots,
    $._comp_left,
    $._comp_middle,
    $._comp_right,
    $._comp_in,
    // hom and hom_star (§2.6)
    $.hom_name,
    $._hom_text,
    $._hom_star_text,
    $._quotation_text,
    // never valid; tells the scanner the parser is recovering from an error
    $._error_sentinel,
  ],

  extras: $ => [/\s/, $.line_comment, $.block_comment],

  word: $ => $.identifier,

  conflicts: $ => [
    // Lines are premises if a dashed line follows, formulas otherwise (§4).
    [$.premise, $.formula],
  ],

  rules: {
    source_file: $ => repeat($._item),

    _item: $ => choice(
      $.metavar_declaration,
      $.grammar,
      $.definitions,
      $.functions,
      $.subrules,
      $.context_rules,
      $.substitutions,
      $.free_variables,
      $.embed,
      $.parsing,
      $.hom_section,
      $.coq_section_begin,
      $.coq_section_end,
      $.coq_variable,
    ),

    // §2.2: identchar+, which includes the quoted forms 't_' and ''.
    identifier: _ => /[A-Za-z0-9_']+/,

    line_comment: _ => token(seq('%', /[^\r\n]*/)),

    // §3.2

    metavar_declaration: $ => seq(
      field('kind', choice('metavar', 'indexvar')),
      $._names,
      '::=',
      repeat($.hom),
    ),

    _names: $ => sepBy1(',', $.root),

    root: $ => seq(field('name', $.identifier), repeat($.hom)),

    // §3.3

    grammar: $ => seq('grammar', repeat($.rule)),

    rule: $ => seq(
      $._names,
      '::',
      field('prefix', $.identifier),
      '::=',
      repeat($.hom),
      repeat($.production),
    ),

    production: $ => seq(
      '|',
      optional($.elements),
      '::',
      repeat($.category),
      '::',
      field('name', $.identifier),
      repeat($.bindspec_group),
      repeat($.hom),
    ),

    category: $ => $.identifier,

    // §3.4

    elements: $ => repeat1($._element),

    _element: $ => choice(
      $.element,
      $.quoted_element,
      $.dots,
      $.comprehension,
    ),

    dots: $ => $._element_dots,

    comprehension: $ => seq(
      alias($._comp_left, '</'),
      repeat(choice($.element, $.quoted_element, $.dots)),
      alias($._comp_middle, '//'),
      repeat(choice(
        $.element,
        $.dots,
        alias($._comp_middle, '//'),
        alias($._comp_in, 'IN'),
      )),
      alias($._comp_right, '/>'),
    ),

    // §3.5

    bindspec_group: $ => seq('(+', repeat($.bindspec), '+)'),

    bindspec: $ => choice(
      seq('bind', $.mse, 'in', $.identifier),
      seq($.identifier, '=', $.mse),
      seq('names', '(', $.mse, ')', choice('=', '#'), 'names', '(', $.mse, ')'),
      seq('distinctnames', '(', $.mse, ')'),
    ),

    mse: $ => choice(
      prec.right(seq($.mse, 'union', $.mse)),
      $._mse_atom,
    ),

    _mse_atom: $ => choice(
      seq($.identifier, optional(choice(
        seq($._bind_dots, $.identifier),
        seq('(', $._mse_argument, ')'),
        $.bind_comprehension,
      ))),
      $.bind_comprehension,
      '{}',
    ),

    _mse_argument: $ => choice(
      seq($.identifier, optional(seq($._bind_dots, $.identifier))),
      $.bind_comprehension,
    ),

    bind_comprehension: $ => seq(
      '</', $.identifier, '//', $.comp_bound, '/>',
    ),

    comp_bound: $ => seq(
      $.identifier,
      optional(seq('IN', $.identifier, optional(seq($._bind_dots, $.identifier)))),
    ),

    _bind_dots: $ => alias(choice('..', '...', '....'), $.dots),

    // §3.6, §3.7

    hom: $ => choice(
      seq(
        '{{',
        field('name', $.hom_name),
        optional(field('body', alias($._hom_body, $.hom_body))),
        '}}',
      ),
      seq(
        '{{*',
        field('name', $.hom_name),
        optional(field('body', alias($._hom_star_body, $.hom_body))),
        '*}}',
      ),
    ),

    _hom_body: $ => repeat1(choice($._hom_text, $.quotation)),

    _hom_star_body: $ => repeat1(choice($._hom_star_text, $.quotation)),

    quotation: $ => seq('[[', optional(alias($._quotation_text, $.term)), ']]'),

    embed: $ => seq('embed', repeat($.hom)),

    // §3.8

    subrules: $ => seq('subrules', repeat($.subrule)),

    subrule: $ => seq(
      field('lower', $.identifier),
      '<::',
      field('upper', $.identifier),
      repeat($.hom),
    ),

    context_rules: $ => seq('contextrules', repeat($.context_rule)),

    context_rule: $ => seq(
      field('context', $.identifier),
      '_::',
      field('target', $.identifier),
      '::',
      field('hole', $.identifier),
    ),

    substitutions: $ => seq('substitutions', repeat($.substitution)),

    substitution: $ => seq(
      field('kind', choice('single', 'multiple')),
      field('within', $.identifier),
      field('for', $.identifier),
      '::',
      field('name', $.identifier),
      repeat($.hom),
    ),

    free_variables: $ => seq('freevars', repeat($.free_variable)),

    free_variable: $ => seq(
      field('within', $.identifier),
      field('for', $.identifier),
      '::',
      field('name', $.identifier),
      repeat($.hom),
    ),

    // §3.9, §4

    definitions: $ => seq(
      'defns',
      field('name', $.identifier),
      '::',
      field('prefix', $.identifier),
      '::=',
      repeat($.hom),
      repeat($.definition),
    ),

    definition: $ => seq(
      'defn',
      optional($.elements),
      '::',
      repeat($.category),
      '::',
      field('name', $.identifier),
      '::',
      field('prefix', $.identifier),
      repeat($.hom),
      'by',
      repeat($._rule_block),
    ),

    // Rules are separated by blank lines (§4). The scanner marks the end of
    // a rule followed by one with a zero-width break, so that the blank
    // lines belong to no node.
    _rule_block: $ => seq(
      choice(
        $.inference_rule,
        $.formula_rule,
        alias($.hom, $.comment_block),
      ),
      optional($._rule_break),
    ),

    // The lines after the dashed line, up to a blank line, are conclusions.
    inference_rule: $ => prec.right(seq(
      repeat($.premise),
      $.rule_line,
      repeat($.conclusion),
    )),

    formula_rule: $ => prec.right(repeat1($.formula)),

    premise: $ => prec.dynamic(1, $._line),
    conclusion: $ => $._line,
    formula: $ => $._line,

    rule_line: $ => prec.right(seq(
      $.dashes,
      repeat($.category),
      '::',
      field('name', $.identifier),
      repeat($.hom),
    )),

    dashes: _ => /----+/,

    // §3.10

    functions: $ => seq(
      'funs',
      field('name', $.identifier),
      '::=',
      repeat($.hom),
      repeat($.function),
    ),

    function: $ => seq(
      'fun',
      optional($.elements),
      '::',
      field('result', $.identifier),
      '::',
      field('name', $.identifier),
      repeat($.hom),
      'by',
      repeat($.clause),
    ),

    clause: $ => $._line,

    // §3.11

    parsing: $ => seq('parsing', repeat($.priority)),

    priority: $ => seq(
      field('left', $.identifier),
      field('relation', choice('<=', 'left', 'right', 'non')),
      field('right', $.identifier),
    ),

    // §3.12

    hom_section: $ => seq(
      'homs',
      field('prefix', $.identifier),
      repeat($.hom_section_item),
    ),

    hom_section_item: $ => seq(
      '::',
      field('name', $.identifier),
      repeat($.bindspec_group),
      repeat($.hom),
    ),

    // §3.13

    coq_section_begin: $ => seq('begincoqsection', field('name', $.identifier)),
    coq_section_end: $ => seq('endcoqsection', field('name', $.identifier)),
    coq_variable: $ => seq('coqvariable', field('name', $.identifier)),
  },
});
