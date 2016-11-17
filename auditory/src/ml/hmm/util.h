/*
**      File:   util.h
**      Purpose: Memory allocation routines borrowed from the
**              book "Numerical Recipes" by Press, Flannery, Teukolsky,
**              and Vetterling.
**              state sequence and probablity of observing a sequence
**              given the model.
**      Organization: University of Maryland
**
**       ----------------------------------------------------------
**       Author : YAO Matrix( yaoweifeng0301@gmail.com )
**       Purpose: merge
**       Date   : 2012/06/05
**       ----------------------------------------------------------
**
**      $Id: util.h,v 1.2 1998/02/19 16:32:42 kanungo Exp kanungo $
*/

// #ifdef __cplusplus
// extern "C" {
//#endif

// vector & matrix
float* vector( int nl, int nh );
float** matrix( int nrl, int nrh, int ncl, int nch );
float** convert_matrix( float *a, int nrl, int nrh, int ncl, int nch );
double* dvector( int nl, int nh );
double** dmatrix( int nrl, int nrh, int ncl, int nch );
int* ivector( int nl, int nh );
int** imatrix( int nrl, int nrh, int ncl, int nch );
float** submatrix( float **a, int oldrl, int oldrh, int oldcl, int oldch, int newrl, int newcl );
void free_vector( float *v, int nl, int nh );
void free_dvector( double *v, int nl, int nh );
void free_ivector( int *v, int nl, int nh );
void free_matrix( float **m, int nrl, int nrh, int ncl, int nch );
void free_dmatrix( double **m, int nrl, int nrh, int ncl,int nch );
void free_imatrix( int **m, int nrl, int nrh, int ncl, int nch );
void free_submatrix( float **b, int nrl, int nrh, int ncl, int nch );
void free_convert_matrix( float **b, int nrl, int nrh, int ncl, int nch );
// #ifdef __cplusplus
// }
// #endif
