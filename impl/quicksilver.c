/*
 * quicksilver.c -- QuickSilver proof for isogeny constraints
 *
 * Prover evaluates Phi_2 on IT-MACs, batches, adds lift mask,
 * sends D_QS polynomial coefficients (excl. leading = z = 0).
 * Verifier recomputes via key ops and checks p_z(Delta') == k_z.
 */
#include "quicksilver.h"
#include <string.h>
#include <stdlib.h>

void qs_phi2_itmac(itmac_poly_t *res,
                       const itmac_poly_t *X,
                       const itmac_poly_t *Y)
{
    itmac_poly_t X2,Y2,X3,Y3,XY,X2Y2,X2Y,XY2,term,tmp;
    itmac_mul(&X2,X,X); itmac_mul(&Y2,Y,Y);
    itmac_mul(&X3,&X2,X); itmac_mul(&Y3,&Y2,Y);
    itmac_mul(&XY,X,Y);
    itmac_mul(&X2Y2,&XY,&XY);
    itmac_mul(&X2Y,&X2,Y); itmac_mul(&XY2,X,&Y2);

    itmac_add(res,&X3,&Y3); itmac_add(&tmp,res,&X2Y2); *res=tmp;

    fp2_t c; fp2_set_small(&c,1488,0); fp2_neg(&c,&c);
    for(int i=0;i<=X2Y.deg;i++) fp2_mul(&term.coeffs[i],&X2Y.coeffs[i],&c);
    for(int i=X2Y.deg+1;i<=ITMAC_MAX_DEG;i++) fp2_set_zero(&term.coeffs[i]);
    term.deg=X2Y.deg; itmac_add(&tmp,res,&term); *res=tmp;

    for(int i=0;i<=XY2.deg;i++) fp2_mul(&term.coeffs[i],&XY2.coeffs[i],&c);
    for(int i=XY2.deg+1;i<=ITMAC_MAX_DEG;i++) fp2_set_zero(&term.coeffs[i]);
    term.deg=XY2.deg; itmac_add(&tmp,res,&term); *res=tmp;

    fp2_set_small(&c,162000,0); fp2_neg(&c,&c);
    itmac_poly_t sqs; itmac_add(&sqs,&X2,&Y2);
    for(int i=0;i<=sqs.deg;i++) fp2_mul(&term.coeffs[i],&sqs.coeffs[i],&c);
    for(int i=sqs.deg+1;i<=ITMAC_MAX_DEG;i++) fp2_set_zero(&term.coeffs[i]);
    term.deg=sqs.deg; itmac_add(&tmp,res,&term); *res=tmp;

    fp2_set_small(&c,40773375,0);
    for(int i=0;i<=XY.deg;i++) fp2_mul(&term.coeffs[i],&XY.coeffs[i],&c);
    for(int i=XY.deg+1;i<=ITMAC_MAX_DEG;i++) fp2_set_zero(&term.coeffs[i]);
    term.deg=XY.deg; itmac_add(&tmp,res,&term); *res=tmp;

    fp2_set_small(&c,8748000000ULL,0);
    itmac_poly_t xpy; itmac_add(&xpy,X,Y);
    for(int i=0;i<=xpy.deg;i++) fp2_mul(&term.coeffs[i],&xpy.coeffs[i],&c);
    for(int i=xpy.deg+1;i<=ITMAC_MAX_DEG;i++) fp2_set_zero(&term.coeffs[i]);
    term.deg=xpy.deg; itmac_add(&tmp,res,&term); *res=tmp;

    fp2_set_small(&c,0x8F3C3F00,0); fp2_neg(&c,&c);
    itmac_poly_t cnst; itmac_const(&cnst,&c,0);
    itmac_add(&tmp,res,&cnst); *res=tmp;
}

void qs_phi2_key(itmac_key_t *k_out,
                     const itmac_key_t *kX, const itmac_key_t *kY,
                     const fp2_t *D)
{
    itmac_key_t X2,Y2,X3,Y3,XY,X2Y2,X2Y,XY2;
    itmac_key_mul(&X2,kX,kX); itmac_key_mul(&Y2,kY,kY);
    itmac_key_mul(&X3,&X2,kX); itmac_key_mul(&Y3,&Y2,kY);
    itmac_key_mul(&XY,kX,kY);
    itmac_key_mul(&X2Y2,&XY,&XY);
    itmac_key_mul(&X2Y,&X2,kY); itmac_key_mul(&XY2,kX,&Y2);

    itmac_key_t a,t; fp2_t c;
    itmac_key_add(&a,&X3,3,&Y3,3,D);                   /* deg 3 */
    itmac_key_add(&t,&a,3,&X2Y2,4,D); fp2_copy(&a,&t); /* deg 4 */

    fp2_set_small(&c,1488,0); fp2_neg(&c,&c);
    itmac_key_mul(&t,&X2Y,&c); itmac_key_add(&a,&a,4,&t,3,D);  /* -1488*X2Y, deg 3 */
    itmac_key_mul(&t,&XY2,&c); itmac_key_add(&a,&a,4,&t,3,D);  /* -1488*XY2, deg 3 */
    itmac_key_t sqs; itmac_key_add(&sqs,&X2,2,&Y2,2,D);        /* X2+Y2, deg 2 */
    fp2_set_small(&c,162000,0); fp2_neg(&c,&c);
    itmac_key_mul(&t,&sqs,&c); itmac_key_add(&a,&a,4,&t,2,D);  /* -162000*(X2+Y2), deg 2 */
    fp2_set_small(&c,40773375,0);
    itmac_key_mul(&t,&XY,&c); itmac_key_add(&a,&a,4,&t,2,D);   /* +40773375*XY, deg 2 */
    fp2_set_small(&c,8748000000ULL,0);
    itmac_key_t xpy; itmac_key_add(&xpy,kX,1,kY,1,D);          /* X+Y, deg 1 */
    itmac_key_mul(&t,&xpy,&c); itmac_key_add(&a,&a,4,&t,1,D);  /* +8748000000*(X+Y), deg 1 */
    fp2_set_small(&c,0x8F3C3F00,0); fp2_neg(&c,&c);
    itmac_key_add(k_out,&a,4,&c,0,D);                          /* const, deg 0 */
}

void quicksilver_prove(fp2_t proof_out[D_QS + 1],
                       const fp2_t *Wmat, const fp2_t *Vmat,
                       const itmac_poly_t *rqs_macs,
                       const fp2_t *ch2,
                       const fp2_t *j0v, const fp2_t *jkv)
{
    itmac_poly_t *wmacs = malloc(L_WIT * sizeof(itmac_poly_t));
    for(int i=0;i<L_WIT;i++){
        int r=i/K_C, c=i%K_C;
        itmac_from_value_mask(&wmacs[i],&Wmat[r*K_C+c],&Vmat[r*K_C+c]);
    }
    fp2_t z; fp2_set_zero(&z);
    itmac_poly_t j0m,jkm;
    itmac_from_value_mask(&j0m,j0v,&z);
    itmac_from_value_mask(&jkm,jkv,&z);

    itmac_poly_t zacc; memset(&zacc,0,sizeof(zacc)); zacc.deg=0;
    fp2_t cp; fp2_set_one(&cp);

    for(int i=0;i<T_CONSTRAINTS;i++){
        const itmac_poly_t *X=(i==0)?&j0m:&wmacs[i-1];
        const itmac_poly_t *Y=(i==K_PATH-1)?&jkm:&wmacs[i];
        itmac_poly_t zi,zs,tm;
        qs_phi2_itmac(&zi,X,Y);
        for(int d=0;d<=zi.deg;d++) fp2_mul(&zs.coeffs[d],&zi.coeffs[d],&cp);
        for(int d=zi.deg+1;d<=ITMAC_MAX_DEG;d++) fp2_set_zero(&zs.coeffs[d]);
        zs.deg=zi.deg;
        itmac_add(&tm,&zacc,&zs); zacc=tm;
        fp2_mul(&cp,&cp,ch2);
    }

    itmac_poly_t lift;
    itmac_lift(&lift,rqs_macs,D_QS-1);
    itmac_poly_t pz;
    itmac_add(&pz,&zacc,&lift);

    for(int i=0;i<=D_QS;i++) fp2_copy(&proof_out[i],&pz.coeffs[i]);
    free(wmacs);
}

void quicksilver_verify(fp2_t *k_z_out,
                       const fp2_t *Qmat,
                       const fp2_t *krqs,
                       const fp2_t *ch2,
                       const fp2_t *Dp,
                       const fp2_t *j0v, const fp2_t *jkv)
{
    itmac_key_t *kw = malloc(L_WIT * sizeof(itmac_key_t));
    for(int i=0;i<L_WIT;i++){
        int r=i/K_C, c=i%K_C;
        fp2_copy(&kw[i],&Qmat[r*K_C+c]);
    }
    itmac_key_t kj0,kjk;
    itmac_key_const(&kj0,j0v,1,Dp);
    itmac_key_const(&kjk,jkv,1,Dp);

    fp2_t kz; fp2_set_zero(&kz);
    fp2_t cp; fp2_set_one(&cp);

    for(int i=0;i<T_CONSTRAINTS;i++){
        const itmac_key_t *kX=(i==0)?&kj0:&kw[i-1];
        const itmac_key_t *kY=(i==K_PATH-1)?&kjk:&kw[i];
        itmac_key_t zi,zs;
        qs_phi2_key(&zi,kX,kY,Dp);
        fp2_mul(&zs,&zi,&cp);
        fp2_add(&kz,&kz,&zs);
        fp2_mul(&cp,&cp,ch2);
    }

    itmac_key_t kl; itmac_key_lift(&kl,krqs,D_QS-1,Dp);
    fp2_add(k_z_out,&kz,&kl);

    free(kw);
}
