#ifndef _PARAM_H_
#define _PARAM_H_

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

#include "models2.pb-c.h"

typedef struct _PCA
{
    int         mean_size;
    gsl_vector* mean_data;

    int         eval_size;
    gsl_vector* eval_data;

    int         evec_size1;
    int         evec_size2;
    gsl_matrix* evec_data;
}PCA;

class CParam2 {
protected:
        FCWS__Local__Type   m_local_type;
        PCA*                m_pca;
        PCA*                m_pca2;

public:
        CParam2(FCWS__Local__Type local_type);

        ~CParam2();
    
        bool SetPCAParam(gsl_vector* mean, gsl_vector* eval, gsl_matrix* evec);

        bool SetPCA2Param(gsl_vector* mean, gsl_vector* eval, gsl_matrix* evec);

        bool SaveParam(FCWS__Para2* param);
        
        bool LoadParam(FCWS__Para2* param);

        double CalProbability(const gsl_vector *img);

protected:
        bool SetPCAParam(PCA *pca, gsl_vector* mean, gsl_vector* eval, gsl_matrix* evec);

        bool InitAndSetPCAMember(FCWS__Para2__Pca *pca, PCA *mpca);

        bool InitAndSetICAMember(FCWS__Para2__Ica *ica, PCA *mpca);

        double CalProbabilityOfPCA(const gsl_vector *img, PCA *pca);
};

class CParam {
protected:
		FCWS__Para__Type 	m_para_type;
		int					m_data_count;
		gsl_matrix*			m_data;

public:
		CParam(FCWS__Para__Type para_type);

		~CParam();

		int 	LoadParam(FCWS__Para__Type type, int rows, int cols, double *data);

		int 	SetParam(FCWS__Para__Type type, gsl_matrix *from);

		bool	SaveParam(FCWS__Para *param);

		FCWS__Para__Type GetType();

		void	SetType(FCWS__Para__Type type);

		int		GetDataCount();

		void 	SetData(int row, int col, double d);

		double	GetData(int row, int col);


//		Para* 	GetObj();

protected:
};

#endif
