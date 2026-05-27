#!/usr/bin/env python3
"""
Compute VOLEitH proof size for proving isogeny path knowledge.

Formula (eq:proof-size from main.tex):
  |π|_bits = [ ℓ̂_vole·(n_C - k_C) + ℓ_wit + k_C + d_qs + ℓ_vole·k_C ] · log(p²)
           + n_C · k_vc
           + n_C · (k_vc + 2) · λ

where:
  ℓ_vole  = ⌈(ℓ_wit + d_qs - 1) / k_C⌉
  ℓ̂_vole  = 2·ℓ_vole + 1
  d_C     = n_C - k_C + 1   (Reed-Solomon)
  Security constraint: d_C · k_vc ≥ λ

  We jointly optimise (n_C, k_C) to minimise |π|.
  For fixed k_C, the optimal n_C is the minimum satisfying security:
      n_C = k_C - 1 + ⌈λ / k_vc⌉

  The optimisation then reduces to a 1D search over k_C.
"""

import math
from dataclasses import dataclass

SECPAR = 128
W      = 8    # proof-of-work grinding bits (2^w hashes)

# SIKE primes (p = 2^a · 3^b · f - 1)
PRIMES = {
    "p252": {"bits": 252, "p": 2**125 * 3**79  * 5  - 1},   # ~252-bit, ~SIKE-p252 class
    "p434": {"bits": 434, "p": 2**216 * 3**137       - 1},
    "p503": {"bits": 503, "p": 2**250 * 3**159       - 1},
    "p610": {"bits": 610, "p": 2**305 * 3**192       - 1},
    "p751": {"bits": 751, "p": 2**372 * 3**239       - 1},
}


@dataclass
class IsogenyConfig:
    label: str
    ell: int
    k: int
    ell_wit: int
    d_qs: int
    t: int
    encoding: str
    prime_name: str


# From tab:wit-deg-concrete at λ = 128 (p434)
CONFIGS_P434 = [
    IsogenyConfig(r"$\ell=2$, classical $\Phi_2$",     2,  256, 255,  4, 256, "classical", "p434"),
    IsogenyConfig(r"$\ell=2$, canonical $\Phi_2^c$",   2,  256, 511,  3, 512, "canonical", "p434"),
    IsogenyConfig(r"$\ell=3$, canonical $\Phi_3^c$",   3,  162, 323,  4, 324, "canonical", "p434"),
    IsogenyConfig(r"$\ell=5$, canonical $\Phi_5^c$",   5,  111, 221,  6, 222, "canonical", "p434"),
    IsogenyConfig(r"$\ell=7$, canonical $\Phi_7^c$",   7,   92, 183,  8, 184, "canonical", "p434"),
    IsogenyConfig(r"$\ell=13$, canonical $\Phi_{13}^c$", 13, 70, 139, 14, 140, "canonical", "p434"),
]

# p252 — teammate asks: classical ℓ=2 proof at 252-bit prime
CONFIGS_P252 = [
    IsogenyConfig(r"$\ell=2$, classical $\Phi_2$",     2,  256, 255,  4, 256, "classical", "p252"),
    IsogenyConfig(r"$\ell=2$, canonical $\Phi_2^c$",   2,  256, 511,  3, 512, "canonical", "p252"),
    IsogenyConfig(r"$\ell=3$, canonical $\Phi_3^c$",   3,  162, 323,  4, 324, "canonical", "p252"),
    IsogenyConfig(r"$\ell=5$, canonical $\Phi_5^c$",   5,  111, 221,  6, 222, "canonical", "p252"),
    IsogenyConfig(r"$\ell=7$, canonical $\Phi_7^c$",   7,   92, 183,  8, 184, "canonical", "p252"),
    IsogenyConfig(r"$\ell=13$, canonical $\Phi_{13}^c$", 13, 70, 139, 14, 140, "canonical", "p252"),
]

# ℓ=2,3,5 also benchmarked at p503 in prior work
CONFIGS_P503 = [
    IsogenyConfig(r"$\ell=2$, classical $\Phi_2$",     2,  256, 255,  4, 256, "classical", "p503"),
    IsogenyConfig(r"$\ell=2$, canonical $\Phi_2^c$",   2,  256, 511,  3, 512, "canonical", "p503"),
    IsogenyConfig(r"$\ell=3$, canonical $\Phi_3^c$",   3,  162, 323,  4, 324, "canonical", "p503"),
    IsogenyConfig(r"$\ell=5$, canonical $\Phi_5^c$",   5,  111, 221,  6, 222, "canonical", "p503"),
]


def optimise_code(cfg: IsogenyConfig, secpar: int, k_vc: int, w: int = 0):
    """
    Find (n_C, k_C, d_C) that minimises proof size.

    With proof-of-work grinding (w bits), the subspace VOLE security
    requirement relaxes to  d_C · k_vc ≥ secpar - w.
    Hence d_C = ⌈(secpar - w) / k_vc⌉.  The prover pays 2^w hashes.
    """
    d_C_fixed = math.ceil((secpar - w) / k_vc)  # constant for all k_C

    best = None
    # Search up to the point where ℓ_vole reaches 1 (beyond which
    # increasing k_C only adds n_C overhead with no ℓ_vole benefit).
    max_k_C = cfg.ell_wit + cfg.d_qs + 50
    for k_C in range(1, max_k_C + 1):
        n_C = k_C - 1 + d_C_fixed
        d_C = n_C - k_C + 1  # = d_C_fixed

        ell_vole = math.ceil((cfg.ell_wit + cfg.d_qs - 1) / k_C)
        ell_hat_vole = 2 * ell_vole + 1

        # n_C - k_C = d_C - 1 = d_C_fixed - 1  (constant!)
        fe_terms = (ell_hat_vole * (n_C - k_C)
                    + cfg.ell_wit
                    + k_C
                    + cfg.d_qs
                    + ell_vole * k_C)
        field_bits = fe_terms * cfg.log_p2

        bitstring_bits = n_C * k_vc + n_C * (k_vc + 2) * secpar

        total_bits = field_bits + bitstring_bits

        if best is None or total_bits < best["total_bits"]:
            best = {
                "k_C": k_C,
                "n_C": n_C,
                "d_C": d_C,
                "ell_vole": ell_vole,
                "ell_hat_vole": ell_hat_vole,
                "fe_terms": fe_terms,
                "field_bits": field_bits,
                "field_kB": field_bits / (8 * 1024),
                "bitstring_bits": bitstring_bits,
                "bitstring_kB": bitstring_bits / (8 * 1024),
                "total_bits": total_bits,
                "total_kB": total_bits / (8 * 1024),
            }
    return best


def fmt_kB(bits):
    kB = bits / (8 * 1024)
    if kB >= 1000:
        return f"{kB / 1024:.1f} MB"
    return f"{kB:.1f} kB"


def setup_configs(configs, secpar):
    """Precompute log_p2 for each config."""
    for cfg in configs:
        prime = PRIMES[cfg.prime_name]
        cfg.log_p2 = math.ceil(2 * math.log2(prime["p"]))


def print_table(configs, k_vc, variant_name, w=0):
    print(f"\n{'='*110}")
    print(f"  {variant_name}:  k_vc = {k_vc},  w = {w},  λ_eff = {secpar - w}")
    print(f"{'='*110}")
    hdr = (f"{'Config':<36} {'ℓ_wit':>6} {'d_qs':>4} "
           f"{'n_C':>4} {'k_C':>4} {'d_C':>4} {'r=k_C/n_C':>10} "
           f"{'ℓ_vole':>7} {'|π| field':>14} {'|π| total':>14} {'FE/wit':>8}")
    print(hdr)
    print("-" * len(hdr))
    for cfg in configs:
        r = optimise_code(cfg, secpar, k_vc, w)
        rate = r["k_C"] / r["n_C"]
        per_wit = r["fe_terms"] / cfg.ell_wit
        print(f"{cfg.label:<36} {cfg.ell_wit:>6} {cfg.d_qs:>4} "
              f"{r['n_C']:>4} {r['k_C']:>4} {r['d_C']:>4} {rate:>9.3f} "
              f"{r['ell_vole']:>7} "
              f"{fmt_kB(r['field_bits']):>14} {fmt_kB(r['total_bits']):>14} "
              f"{per_wit:>7.3f}")


def print_detail(configs, k_vc, variant_name, w=0):
    print(f"\n{'='*100}")
    print(f"  DETAIL: {variant_name} (k_vc = {k_vc}, w = {w})")
    print(f"{'='*100}")
    for cfg in configs:
        r = optimise_code(cfg, secpar, k_vc, w)
        print(f"\n--- {cfg.label} ---")
        print(f"  Code:       [{r['n_C']}, {r['k_C']}, {r['d_C']}]  "
              f"rate = {r['k_C']}/{r['n_C']} = {r['k_C']/r['n_C']:.3f}")
        print(f"  ℓ_vole:     {r['ell_vole']}  (ℓ̂_vole = {r['ell_hat_vole']})")
        print(f"  FE terms:   {r['fe_terms']}  field elements")
        print(f"  Field:      {r['field_kB']:.1f} kB  ({r['field_bits']:.0f} bits)")
        print(f"  Bitstring:  {r['bitstring_kB']:.1f} kB  ({r['bitstring_bits']:.0f} bits)")
        print(f"  TOTAL:      {r['total_kB']:.1f} kB  ({r['total_bits']:.0f} bits)")


def latex_table(configs, secpar, w=0):
    """Print a LaTeX-ready comparison table."""
    print(f"\n{'='*100}")
    print(f"  LATEX TABLE (w = {w}, λ_eff = {secpar - w})")
    print(f"{'='*100}")
    print(r"\begin{tabular}{c c c c c c c c c}")
    print(r"\toprule")
    print(r"$\ell$ & Enc. & $k$ & $\ell_{\sf wit}$ & $d_{\sf qs}$ & "
          r"Fast ($k_{\sf vc}=8$) & & Small ($k_{\sf vc}=12$) & & \\")
    print(r" & & & & & $[n_C,k_C,d_C]$ & $|\pi|$ & $[n_C,k_C,d_C]$ & $|\pi|$ \\")
    print(r"\midrule")
    for cfg in configs:
        r8 = optimise_code(cfg, secpar, 8, w)
        r12 = optimise_code(cfg, secpar, 12, w)
        print(rf"${cfg.ell}$ & {cfg.encoding} & ${cfg.k}$ & "
              rf"${cfg.ell_wit}$ & ${cfg.d_qs}$ & "
              rf"$[{r8['n_C']},{r8['k_C']},{r8['d_C']}]$ & "
              rf"${r8['total_kB']:.0f}$\,kB & "
              rf"$[{r12['n_C']},{r12['k_C']},{r12['d_C']}]$ & "
              rf"${r12['total_kB']:.0f}$\,kB \\")
    print(r"\bottomrule")
    print(r"\end{tabular}")


def sweep_pow(configs, secpar):
    """Show how proof sizes shrink as PoW grinding w increases."""
    best_config = configs[-1]  # ℓ=13 canonical (best absolute size)
    print(f"\n{'='*100}")
    print(f"  PoW SWEEP for {best_config.label}  (p434)")
    print(f"  Prover work = 2^w hashes")
    print(f"{'='*100}")
    hdr = (f"{'w':>3} {'λ_eff':>6} "
           f"{'Fast n_C':>9} {'Fast k_C':>9} {'Fast d_C':>9} {'Fast |π|':>12} "
           f"{'Small n_C':>9} {'Small k_C':>9} {'Small d_C':>9} {'Small |π|':>12}")
    print(hdr)
    print("-" * len(hdr))
    for w in [0, 8, 16, 24, 32, 40, 48, 56, 64]:
        r8 = optimise_code(best_config, secpar, 8, w)
        r12 = optimise_code(best_config, secpar, 12, w)
        print(f"{w:>3} {secpar - w:>6} "
              f"{r8['n_C']:>9} {r8['k_C']:>9} {r8['d_C']:>9} {fmt_kB(r8['total_bits']):>12} "
              f"{r12['n_C']:>9} {r12['k_C']:>9} {r12['d_C']:>9} {fmt_kB(r12['total_bits']):>12}")

    # Also show a summary for all configs at w = 32
    print(f"\n{'='*100}")
    print(f"  PoW with w = 32 for ALL configs")
    print(f"{'='*100}")
    for k_vc, name in [(8, "FAST"), (12, "SMALL")]:
        print(f"\n  --- {name} (k_vc = {k_vc}) ---")
        print(f"  {'Config':<36} {'w=0':>12} {'w=32':>12} {'saving':>10}")
        print(f"  {'-'*36} {'-'*12} {'-'*12} {'-'*10}")
        for cfg in configs:
            r0 = optimise_code(cfg, secpar, k_vc, 0)
            r32 = optimise_code(cfg, secpar, k_vc, 32)
            saving = (1 - r32['total_bits'] / r0['total_bits']) * 100
            print(f"  {cfg.label:<36} {fmt_kB(r0['total_bits']):>12} "
                  f"{fmt_kB(r32['total_bits']):>12} {saving:>9.1f}%")


if __name__ == "__main__":
    secpar = SECPAR
    w = W

    # ── p434 ──────────────────────────────────────────────────
    setup_configs(CONFIGS_P434, secpar)
    print_table(CONFIGS_P434, 8, "FAST (p434)", w)
    print_table(CONFIGS_P434, 12, "SMALL (p434)", w)
    latex_table(CONFIGS_P434, secpar, w)

    # ── p252 ──────────────────────────────────────────────────
    setup_configs(CONFIGS_P252, secpar)
    print_table(CONFIGS_P252, 8, "FAST (p252)", w)
    print_table(CONFIGS_P252, 12, "SMALL (p252)", w)
