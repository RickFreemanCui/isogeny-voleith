/*
 * constraints.h -- isogeny path constraint polynomials
 */
#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

#include "fp2.h"
#include "params.h"

/* One isogeny path witness: intermediate j-invariants j_1..j_{k-1} */
typedef fp2_t witness_t[L_WIT];

/* Constraint coefficients for classical Phi_2 */
void constraints_classical_eval(fp2_t *result,           /* output fp2 */
                                const witness_t witness, /* j_1..j_{k-1} */
                                const fp2_t j0,          /* public start */
                                const fp2_t jk,          /* public end */
                                int constraint_idx);     /* 0..T_CONSTRAINTS-1 */

#endif
