/*
 * params.h -- protocol parameters for VOLEitH isogeny proof
 */
#ifndef PARAMS_H
#define PARAMS_H

#include <stdint.h>

/* ── security ─────────────────────────────────────────────── */
#define SECPAR        128
#define SECPAR_BYTES  16
#define POW_W         8     /* PoW grinding bits */

/* ── BAVC ──────────────────────────────────────────────────── */
#define K_VC          12    /* log2(leaves per vector) */
#define N_LEAVES      (1u << K_VC)  /* 4096 */

/* ── isogeny path (classical Phi_2, ell=2) ─────────────────── */
#define ELL           2
#define K_PATH        256   /* isogeny path length */
#define L_WIT         255   /* = k-1 intermediate j-invariants */
#define D_QS          4     /* max constraint degree */
#define T_CONSTRAINTS 256   /* = k */

/* ── RS code (from compute_proof_size.py: p252, w=8, k_vc=12, small) */
#define N_C           46    /* code length */
#define K_C           37    /* code dimension */
#define D_C           10    /* minimum distance (= n_C - k_C + 1) */

/* ── derived ───────────────────────────────────────────────── */
#define L_VOLE        7     /* ceil((L_WIT + D_QS - 1) / K_C) = ceil(258/37) */
#define L_HAT_VOLE    15    /* 2*L_VOLE + 1 */

/* compile-time assertions */
#if D_C * K_VC < SECPAR - POW_W
#error "d_C * k_vc < secpar - w: security constraint violated"
#endif

#endif /* PARAMS_H */
