
#include "spx_matrix.hpp"

#if !defined(_WIN32)

#ifndef dswap
#define dswap dswap_
#endif

#ifndef dscal 
#define dscal dscal_
#endif

#ifndef dgemm
#define dgemm dgemm_
#endif

#endif

namespace spx {

/************************************************
 *  Matrix Implementation
 ************************************************/

Matrix::Matrix(mwSize rows, mwSize cols):
    m_pMatrix((double*)mxMalloc(rows * cols * sizeof(double))),
    m_rows(rows),
    m_cols(cols),
    m_bOwned(true)
{
}

Matrix::Matrix(const mxArray* pMat):
    m_pMatrix(mxGetPr(pMat)),
    m_rows(mxGetM(pMat)),
    m_cols(mxGetN(pMat)),
    m_bOwned(false){

}

Matrix::Matrix(double *pMatrix, mwSize rows, mwSize cols, bool bOwned):
    m_pMatrix(pMatrix),
    m_rows(rows),
    m_cols(cols),
    m_bOwned(bOwned)
{
}

Matrix::Matrix(Matrix& source, mwSize start_col, mwSize num_cols):
    m_pMatrix(source.m_pMatrix + source.rows() * start_col),
    m_rows(source.rows()),
    m_cols(num_cols),
    m_bOwned(false) {

}


Matrix::~Matrix() {
    if (m_bOwned) {
        // release the memory
        mxFree(m_pMatrix);
        m_pMatrix = 0;
    }
}

mwSize Matrix::rows() const {
    return m_rows;
}

mwSize Matrix::columns() const {
    return m_cols;
}


Matrix Matrix::columns_ref(mwIndex start, mwIndex end) const {
    if (start < 0){
        throw std::invalid_argument("start cannot be negative.");
    }
    if (end <= start){
        throw std::invalid_argument("end cannot be less than or equal to start.");
    }
    if (end > m_cols){
        throw std::invalid_argument("end cannot go beyond last column of matrix");
    }
    double* beg = m_pMatrix + start * m_rows;
    mwIndex ncols = end - start;
    return Matrix(beg, m_rows, ncols, false);
}


void Matrix::column(mwIndex index, double b[]) const {
    double* a = m_pMatrix + index * m_rows;
    mwSignedIndex  inc = 1;
    mwSignedIndex mm = m_rows;
    dcopy(&mm, a, &inc, b, &inc);
}

void Matrix::extract_columns( const mwIndex indices[], mwSize k,
                              double B[]) const {
    mat_col_extract(m_pMatrix, indices, B, m_rows, k);
}

void Matrix::extract_columns(const index_vector& indices, Matrix& output) const {
    double* B = output.m_pMatrix;
    const mwIndex* indices2 = &indices[0];
    mwSize k = indices.size();
    mat_col_extract(m_pMatrix, indices2, B, m_rows, k);
}

void Matrix::extract_rows( const mwIndex indices[], mwSize k, double B[]) const {
    mat_row_extract(m_pMatrix, indices, B, m_rows, m_cols, k);
}

void Matrix::mult_vec(const double x[], double y[]) const {
    mult_mat_vec(1, m_pMatrix, x, y, m_rows, m_cols);
}

// y = A * x
void Matrix::mult_vec(const Vec& x, Vec& y) const{
    if (m_cols != x.length()) {
        throw std::invalid_argument("x doesn't have appropriate size.");
    }
    if (m_rows != y.length()) {
        throw std::invalid_argument("y doesn't have appropriate size.");
    }
    mwSignedIndex  incx = x.inc();
    mwSignedIndex  incy = y.inc();
    char trans = 'N';
    double beta = 0;
    mwSignedIndex  mm = m_rows;
    mwSignedIndex  nn = m_cols;
    double alpha = 1;
    double* A = m_pMatrix;
    // y := alpha*A*x + beta*y
    dgemv(&trans, &mm, &nn, &alpha, A, &mm, x.head(), &incx, 
        &beta, y.head(), &incy);
}

// y = A' * x
void Matrix::mult_t_vec(const double x[], double y[]) const {
    mult_mat_t_vec(1, m_pMatrix, x, y, m_rows, m_cols);
}

void Matrix::mult_t_vec(const Vec& x, Vec& y) const {
    if (m_rows != x.length()) {
        throw std::invalid_argument("x doesn't have appropriate size.");
    }
    if (m_cols != y.length()) {
        throw std::invalid_argument("y doesn't have appropriate size.");
    }
    mwSignedIndex  incx = x.inc();
    mwSignedIndex  incy = y.inc();
    char trans = 'T';
    double beta = 0;
    mwSignedIndex  mm = m_rows;
    mwSignedIndex  nn = m_cols;
    double alpha = 1;
    double* A = m_pMatrix;
    dgemv(&trans, &mm, &nn, &alpha, A, &mm, x.head(), &incx, 
        &beta, y.head(), &incy);

}

void Matrix::mult_t_vec(const index_vector& indices, const Vec& x, Vec& y) const{
    if (m_rows != x.length()) {
        throw std::invalid_argument("x doesn't have appropriate size.");
    }
    if (indices.size() != y.length()) {
        throw std::invalid_argument("y doesn't have appropriate size.");
    }
    ::mult_submat_t_vec(1, m_pMatrix, &(indices[0]), 
        x.head(), y.head(), m_rows, indices.size());
}



void Matrix::mult_vec(const index_vector& indices, const Vec& x, Vec& y) const{
    if (x.length() != indices.size() ){
        throw std::length_error("Dimension of x is not same as number of columns.");
    }
    if (y.length() != m_rows ){
        throw std::length_error("Dimension of y is not same as number of rows");
    }
    const mwIndex *pindices = &(indices[0]);
    const double *px = x.head();
    double *py = y.head();
    int k = indices.size();
    ::mult_submat_vec(1, m_pMatrix, pindices, px, py, m_rows, k);
}

void Matrix::mult_vec( const mwIndex indices[], mwSize k, const double x[], double y[]) const {
    ::mult_submat_vec(1, m_pMatrix, indices, x, y, m_rows, k);
}

void Matrix::add_column_to_vec(double coeff, mwIndex index, double x[]) const {
    double* a = m_pMatrix + index * m_rows;
    sum_vec_vec(coeff, a, x, m_rows);
}

bool Matrix::copy_matrix_to(Matrix& dst) const {
    //! must be same size
    if (m_rows != dst.rows()) {
        return false;
    }
    if (m_cols != dst.columns()) {
        return false;
    }
    mwSize n = m_rows * m_cols;
    double* p_src = m_pMatrix;
    double* p_dst = dst.m_pMatrix;
    copy_vec_vec(p_src, p_dst, n);
    return true;
}

/************************************************
 *  Matrix Manipulation
 ************************************************/
std::tuple<double, mwIndex> Matrix::col_max(mwIndex col) const {
    // Pointer to the beginning of column
    const double* x = m_pMatrix + col * m_rows;
    mwIndex max_index = 0, k;
    double cur_value, max_value = *x;
    mwSize n = m_rows;
    for (k = 1; k < n; ++k) {
        cur_value = x[k];
        if (cur_value > max_value) {
            max_value = cur_value;
            max_index = k;
        }
    }
    return std::tuple<double, mwIndex>(max_value, max_index);
}
std::tuple<double, mwIndex> Matrix::col_min(mwIndex col) const {
    // Pointer to the beginning of column
    const double* x = m_pMatrix + col * m_rows;
    mwIndex min_index = 0, k;
    double cur_value, min_value = *x;
    mwSize n = m_rows;
    for (k = 1; k < n; ++k) {
        cur_value = x[k];
        if (cur_value < min_value) {
            min_value = cur_value;
            min_index = k;
        }
    }
    return std::tuple<double, mwIndex>(min_value, min_index);
}
std::tuple<double, mwIndex> Matrix::row_max(mwIndex row) const {
    // Pointer to the beginning of row
    const double* x = m_pMatrix + row;
    mwIndex max_index = 0, k;
    double cur_value, max_value = *x;
    mwSize n = m_cols;
    for (k = 1; k < n; ++k) {
        x += m_rows;
        cur_value = *x;
        if (cur_value > max_value) {
            max_value = cur_value;
            max_index = k;
        }
    }
    return std::tuple<double, mwIndex>(max_value, max_index);
}
std::tuple<double, mwIndex> Matrix::row_min(mwIndex row) const {
    // Pointer to the beginning of row
    const double* x = m_pMatrix + row;
    mwIndex min_index = 0, k;
    double cur_value, min_value = *x;
    mwSize n = m_cols;
    for (k = 1; k < n; ++k) {
        x += m_rows;
        cur_value = *x;
        if (cur_value < min_value) {
            min_value = cur_value;
            min_index = k;
        }
    }
    return std::tuple<double, mwIndex>(min_value, min_index);
}

void Matrix::add_to_col(mwIndex col, const double &value) {
    double* x = m_pMatrix + col * m_rows;
    mwSize n = m_rows;
    for (mwIndex k = 0; k < n; ++k) {
        x[k] += value;
    }
}


void Matrix::add_to_row(mwIndex row, const double &value) {
    double* x = m_pMatrix + row;
    mwSize n = m_cols;
    for (mwIndex k = 0; k < n; ++k) {
        *x += value;
        x += m_rows;
    }
}

void Matrix::set_column(mwIndex col, const Vec& input, double alpha) {
    if (col >= m_cols){
        throw std::length_error("Column number beyond range.");
    }
    if (m_rows != input.length()) {
        throw std::length_error("Number of rows mismatch");
    }
    double* x = m_pMatrix + col * m_rows;
    mwSize n = m_rows;
    const double* y = input.head();
    mwIndex inc = input.inc();
    if (alpha == 1) {
        for (mwIndex k = 0; k < n; ++k) {
            *x = *y;
            ++x;
            y += inc;
        }
    } else {
        for (mwIndex k = 0; k < n; ++k) {
            *x = alpha * (*y);
            ++x;
            y += inc;
        }
    }
}


void Matrix::set(double value) {
    int n = m_rows * m_cols;
    double* x = m_pMatrix;
    for (int i=0; i < n; ++i) {
        *x = value;
        ++x;
    }
}
void Matrix::set_diag(double value) {
    int n = std::min(m_rows, m_cols);
    for (int i=0; i < n; ++i) {
        (*this)(i, i) = value;
    }
}
void Matrix::set_diag(const Vec& input) {
    int n = std::min(m_rows, m_cols);
    if (input.length() < n) {
        throw std::length_error("Input vector has insufficient data.");
    }
    for (int i=0; i < n; ++i) {
        (*this)(i, i) = input[i];
    }
}


void Matrix::subtract_col_mins_from_cols() {
    mwSize n = m_cols;
    for (mwIndex c = 0; c < n; ++c) {
        auto min_val = col_min(c);
        add_to_col(c, -std::get<0>(min_val));
    }
}

void Matrix::subtract_row_mins_from_rows() {
    mwSize n = m_rows;
    for (mwIndex r = 0; r < n; ++r) {
        auto min_val = row_min(r);
        add_to_row(r, -std::get<0>(min_val));
    }
}

void Matrix::find_value(const double &value, Matrix& result) const {
    if (m_rows != result.rows()) {
        throw std::length_error("Number of rows mismatch");
    }
    if (m_cols != result.columns()) {
        throw std::length_error("Number of columns mismatch");
    }
    mwSize n  = m_rows * m_cols;
    double* p_src = m_pMatrix;
    double* p_dst = result.m_pMatrix;
    for (mwIndex i = 0; i < n; ++i) {
        *p_dst = *p_src == value;
        ++p_src;
        ++p_dst;
    }
}

void Matrix::gram(Matrix& output) const {
    if (output.rows() != output.columns()) {
        throw std::logic_error("Gram matrix must be symmetric");
    }
    if (output.columns() != columns()) {
        throw std::logic_error("Size of gram matrix must be equal to the number of columns in source matrix");
    }
    double* src = m_pMatrix;
    int M = rows();
    int N = columns();
    /* 
    It is funny to note that GEMM is much faster than the inner product
    approach.
    */
#if 0
    for (int i=0; i < N; ++i){
        double* vi = src + i*M;
        for (int j=i; j < N; ++j){
            double* vj = src + j*M;
            double result = inner_product(vi, vj, M);
            // double result = vi[0] * vj[0];
            // for (int k=1; k < M; ++k){
            //     result += vi[k] * vj[k];
            // }
            output(i, j) = result;
            if (j != i){
                output(j, i) = result;
            }
        }
    }
#else
    mult_mat_t_mat(1, src, src, output.m_pMatrix, N, N, M);
#endif
}

void Matrix::frame(Matrix& output) const{
    if (output.rows() != output.columns()) {
        throw std::logic_error("Frame matrix must be symmetric");
    }
    if (output.rows() != rows()) {
        throw std::logic_error("Size of frame matrix must be equal to the number of rows in source matrix");
    }
    double* src = m_pMatrix;
    int M = rows();
    int N = columns();
    char atrans = 'N';
    char btrans = 'T';
    double alpha = 1;
    double beta = 0;
    double* C = output.m_pMatrix;
    // Output is M x M,  Product is ( M x N )  x (N x M)
    // m = M, n = M, k = N
    mwSignedIndex  mm = rows();
    mwSignedIndex  nn = rows();
    mwSignedIndex  kk = columns();
    dgemm(&atrans, &btrans, 
        &mm, &nn, &kk, 
        &alpha, 
        src, &mm, 
        src, &mm, 
        &beta,
        C, &mm);
}


void Matrix::swap_columns(mwIndex i, mwIndex j){
    if (i >= m_cols || j >= m_cols){
        throw std::length_error("Column number beyond range.");
    }
    ptrdiff_t n = m_rows;
    mwSignedIndex inc = 1;
    double* x = m_pMatrix + i*m_rows;
    double* y = m_pMatrix + j*m_rows;
    dswap(&n, x, &inc, y, &inc);
}

void Matrix::swap_rows(mwIndex i, mwIndex j){
    if (i >= m_rows || j >= m_rows){
        throw std::length_error("Column number beyond range.");
    }
    ptrdiff_t n = m_cols;
    mwSignedIndex inc = m_rows;
    double* x = m_pMatrix + i;
    double* y = m_pMatrix + j;
    dswap(&n, x, &inc, y, &inc);
}

void Matrix::scale_column(mwIndex i, double value){
    ptrdiff_t n = m_rows;
    mwSignedIndex inc = 1;
    double* x = m_pMatrix + i*m_rows;
    dscal(&n, &value, x, &inc);    
}

void Matrix::scale_row(mwIndex i, double value){
    ptrdiff_t n = m_cols;
    mwSignedIndex inc = m_rows;
    double* x = m_pMatrix + i;
    dscal(&n, &value, x, &inc);    
}

void multiply(const Matrix& A, const Matrix& B, Matrix& C, 
    bool a_transpose, bool b_transpose){
    char atrans = a_transpose ? 'T' :  'N';
    char btrans = b_transpose ? 'T': 'N';
    mwSignedIndex a_rows = A.rows();
    mwSignedIndex a_cols = A.columns();
    mwSignedIndex b_rows = B.rows();
    mwSignedIndex b_cols = B.columns();
    mwSignedIndex c_rows = C.rows();
    double beta = 0;
    mwSignedIndex  mm = a_transpose ? a_cols : a_rows;
    mwSignedIndex  nn = b_transpose ? b_rows : b_cols;
    mwSignedIndex  kk = a_transpose ? a_rows : a_cols;
    double alpha = 1;
    dgemm(&atrans, &btrans, 
        &mm, &nn, &kk, 
        &alpha, 
        A.head(), &a_rows, 
        B.head(), &b_rows, 
        &beta, 
        C.head(), &c_rows);
    return;
}


/************************************************
 *  Matrix Printing
 ************************************************/

void Matrix::print_matrix(const char* name) const {
    ::print_matrix(m_pMatrix, m_rows, m_cols, name);
}

void Matrix::print_int_matrix(const char* name) const {
    int i, j;
    if (name[0]) {
        mexPrintf("\n%s = \n\n", name);
    }
    mwSize m = m_rows;
    mwSize n = m_cols;

    if (n * m == 0) {
        mexPrintf("   Empty matrix: %d-by-%d\n\n", n, m);
        return;
    }
    double* A = m_pMatrix;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < m; ++j)
            mexPrintf("   %d", (int)A[j * n + i]);
        mexPrintf("\n");
    }
    mexPrintf("\n");
}


}
