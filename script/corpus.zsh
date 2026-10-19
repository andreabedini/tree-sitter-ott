#!/usr/bin/env zsh
# Parse every file of haskott's acceptance corpus that Ott 0.34 parses, and
# fail if any of them has an ERROR or MISSING node.
#
# The corpus is listed in haskott's test/ott-acceptance.tsv. Files Ott
# rejects with a parse error are skipped; files it parses and then rejects
# in checking are kept, since stage one, which this grammar covers, accepts
# them.
#
# Environment: HASKOTT (default ../haskott), HASKOTT_OTT (default ../../ott)
# and HASKOTT_REPORT_OTT (default ../haskell-2010-revised-report/formalisation),
# relative to this repository.

emulate -L zsh
setopt err_exit no_unset

root=${0:A:h:h}
cd $root
haskott=${HASKOTT:-../haskott}
ott=${HASKOTT_OTT:-../../ott}
report=${HASKOTT_REPORT_OTT:-../haskell-2010-revised-report/formalisation}

files=()
while IFS=$'\t' read -r class file; do
  [[ $class == '#'* || $class == parse-error || -z $file ]] && continue
  case $file in
    ott/*) files+=($ott/${file#ott/}) ;;
    haskell-report-ott/*) files+=($report/${file#haskell-report-ott/}) ;;
    *) print -u2 "unknown corpus path: $file"; exit 2 ;;
  esac
done < $haskott/test/ott-acceptance.tsv

for f in $files; do
  [[ -r $f ]] || { print -u2 "missing corpus file: $f"; exit 2 }
done

tree-sitter parse --quiet --stat $files
