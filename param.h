#ifndef _PARAM_H_
#define _PARAM_H_

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

#include "models2.pb-c.h"

class CParam {
public:
		CParam(FCWS__Para__Type para_type);

		~CParam();

		int 	Set(FCWS__Para__Type type, int rows, int cols, double *data);

		int 	Set(FCWS__Para__Type type, gsl_matrix *from);

		FCWS__Para__Type GetType();

		void	SetType(FCWS__Para__Type type);

		int		GetDataCount();

		void 	SetData(int row, int col, double d);

		double	GetData(int row, int col);

//		Para* 	GetObj();

protected:
		FCWS__Para__Type 	m_para_type;
		int					m_data_count;
		gsl_matrix*			m_data;

};

#endif
