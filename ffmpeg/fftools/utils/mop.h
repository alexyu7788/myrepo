#ifndef __MOP_H__
#define __MOP_H__

#include <dlib/matrix.h>

typedef dlib::matrix<char>				    cmatrix;
typedef dlib::matrix<unsigned char>		    ucmatrix;
typedef dlib::matrix<short>				    smatrix;
typedef dlib::matrix<unsigned short>	    usmatrix;
typedef dlib::matrix<float>				    fmatrix;
typedef dlib::matrix<double>			    dmatrix;

typedef dlib::matrix<char, 0, 1>            cvector;
typedef dlib::matrix<unsigned char, 0, 1>   ucvector;
typedef dlib::matrix<short, 0, 1>           svector;
typedef dlib::matrix<unsigned short, 0, 1>  usvector;
typedef dlib::matrix<float, 0, 1>           fvector;
typedef dlib::matrix<double, 0, 1>          dvector;
#endif
