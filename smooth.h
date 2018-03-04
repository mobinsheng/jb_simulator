#ifndef SMOOTH_H
#define SMOOTH_H


/**
 * 线性拟合平滑处理：
 * 分别为三点线性平滑、五点线性平滑和七点线性平滑。
 *
 */
void linearSmooth3 ( double* in, double* out, int N );

void linearSmooth5 ( double* in, double* out, int N );

void linearSmooth7 ( double* in, double* out, int N );

/**
 * 二次函数拟合平滑
 *
 */
void quadraticSmooth5(double*in, double* out, int N);


void quadraticSmooth7(double*in, double* out, int N);

/**
 * 五点三次平滑
 *
 */
void cubicSmooth5 ( double* in, double* out, int N );
void cubicSmooth7(double* in, double* out, int N);

#endif // SMOOTH_H
