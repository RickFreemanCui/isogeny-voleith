# VOLEitH Isogeny Knowledge Proof

LaTeX paper using VOLEitH to prove knowledge of an isogeny φ: E₀ → Eₙ by committing to intermediate j-invariants and proving consecutive modular polynomial relations Φ_ℓ(j_{i-1}, j_i) = 0.

## Build

- `make` — full build (pdflatex → biber → pdflatex ×2)
- `make quick` — single pdflatex pass (no bibliography)
- `make watch` — continuous preview with latexmk
- `make view` — open PDF
- `make clean` — remove aux files; `make distclean` — also remove PDF

Compiler: `pdflatex`, bibliography: `biber`.
TeX Live 2026 at `/usr/local/texlive/2026`.

## Source files

| File | Purpose |
|------|---------|
| `main.tex` | Main document: preamble, content, bibliography |
| `references.bib` | BibTeX entries (biblatex, `style=alphabetic`, `backend=biber`) |
| `Makefile` | Build automation |

## Style conventions

Follows the style from `../SDitH-v2/MyNotes/`:
- `\documentclass[11pt,a4paper]{article}`
- Preamble sections labelled with `% ===== Section Name =====` separators
- Content sections separated by `% ===============================================================`
- Theorems: theorem, lemma, proposition, corollary (plain); definition, construction, assumption (definition style); remark, example (remark style). All numbered within `[section]` and sharing the `theorem` counter.
- Section labels use `\label{sec:...}`, equation labels `\label{eq:...}`, definition labels `\label{def:...}`.
- Reference with `\Cref{...}` (capitalized, from `cleveref`) — e.g., `\Cref{sec:voleith}`.

## Cryptocode protocols

Uses `cryptocode` with libraries: `n,operators,advantage,sets,adversary,landau,probability,notions,logic,ff,mm,primitives,events,complexity,oracles,asymptotics,keys,lambda`.

Protocols use `\procedureblock{Title}{...}` (NOT a `protocol` environment):
- `\<` = skip two columns (left-aligned column)
- `\sendmessageright*{msg}` — simple message arrow to the right
- `\sendmessageleft*{msg}` — simple message arrow to the left
- `\\[spacing][\hline]` — line break with optional spacing and horizontal rule
- `\\[-0.3\baselineskip]` — tighten spacing between protocol rows
- `\msgrightarrow{...}`, `\msgleftarrow{...}` — message arrows in protocols (wraps `\xrightarrow`/`\xleftarrow` with `\textstyle` for readable size)

## Key macros

| Command | Definition | Note |
|---------|-----------|------|
| `\F`, `\FF` | `\mathbb{F}` | `\F` via `\newcommand`; `\FF` via `\providecommand` (cryptocode `ff` library pre-defines `\FF`) |
| `\Z`, `\N`, `\R`, `\C`, `\G` | `\mathbb{Z}`, etc. | |
| `\calA`, `\calB`, `\calC`, `\calO` | `\mathcal{A}`, etc. | |
| `\calP`, `\calV`, `\calR` | Prover, Verifier, Relation | |
| `\calD` | `\mathcal{D}` | Challenge subset of $\F$ |
| `\eps` | `\varepsilon` | |
| `\Adv`, `\negl`, `\poly` | `\mathrm{Adv}`, etc. | `\providecommand` (already defined by cryptocode) |
| `\itmac{a}^{d}` | `\llbracket a \rrbracket` | IT-MAC notation (uses stmaryrd) |
| `\itmacP{#1}`, `\itmacV{#1}` | `\llbracket #1 \rrbracket_{\calP}`, etc. | Prover/verifier IT-MAC views; superscript added manually |
| `\polyrel` | `\Phi` | Modular polynomial |
| `\mat{#1}` | `\mathbf{#1}` | Matrix notation |
| `\msgrightarrow{#1}`, `\msgleftarrow{#1}` | `\xrightarrow{\textstyle#1}`, etc. | Protocol message arrows (full-size label) |
| `\sample` | `←$` | Sampling (provided by cryptocode) |
| `\secpar` | `n` | Security parameter (cryptocode `n` option) |

## Bibliography

Biblatex with `style=alphabetic`, `backend=biber`. Most entries use `@techreport` format. Key references:
- `faestv2` — FAEST v2 specification (BAVC definitions)
- `baumPubliclyVerifiableZeroKnowledge2023` — VOLEitH framework (CRYPTO 2023)
- `EC:ABBS25` — IT-MAC definition (EUROCRYPT 2025)
- `sdithv2` — SDitH v2 specification

## Drafting conventions

- Use `\calP` for prover, `\calV` for verifier
- Field is `\F_{p^2}` (supersingular elliptic curve base field)
- Security parameter is `\secpar` (= n)
- i-th component accessed via subscript: `j_i`, `\vec{u}_i`
- Protocol phases numbered with `(i)`, `(ii)`, `(iii)` using `enumitem` `[label=(\roman*)]`
- Matrix/vector notation: vectors use `\vec{...}` (redefined to bold `\mathbf`), matrices use `\mat{...}` (also bold), matrix slices use `[:,1..k]` syntax
- `cleveref` is loaded — always use `\Cref{...}` (not bare `\ref`) for cross-references

## Current state

Section 4 (`Proving Isogeny Path Knowledge`) contains a TODO block (~line 476): the VOLEitH 9-round protocol still needs to be instantiated for the isogeny path problem (witness = intermediate $j$-invariants, constraints = modular polynomial relations).
