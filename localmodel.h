#ifndef _LOCALMODEL_H_
#define _LOCALMODEL_H_

#include <string>
#include <vector>
#include <algorithm>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

#include "models2.pb-c.h"
#include "param.h"

using namespace std;

class CLocalModel {
protected:
		string					m_output_folder;

		char*					m_vm_name;
		FCWS__VehicleModel__Type m_vm_type;

		char* 					m_local_name;
		FCWS__Local__Type		m_local_type;
		vector<string>			m_filelist;

		int 					m_pca_first_k_components;
		int 					m_pca_compoments_offset;
		int 					m_ica_first_k_components;
		int 					m_ica_compoments_offset;

		int						m_para_count;
		CParam*					m_para[FCWS__PARA__TYPE__TOTAL];

		int						image_w;
		int						image_h;
		gsl_matrix*				m_image_matrix;

		gsl_vector*				m_mean;
		gsl_matrix*				m_covar;
		gsl_vector*				m_eval;
		gsl_matrix*				m_evec;
		gsl_matrix_view 		m_FirstKEigVector;
		gsl_matrix*				m_projection;
		gsl_matrix*				m_reconstruction;
		gsl_matrix*				m_residual;

		gsl_vector*				m_residual_mean;
		gsl_matrix*				m_residual_covar;
		gsl_vector*				m_residual_eval;
		gsl_matrix*				m_residual_evec;
		gsl_matrix_view 		m_residual_FirstKEigVector;
		gsl_matrix*				m_residual_projection;
		gsl_matrix*				m_residual_reconstruction;
		gsl_matrix*				m_residual_residual;

public:
		CLocalModel(FCWS__VehicleModel__Type vm_type, FCWS__Local__Type local_type);

		~CLocalModel();

		int SetParam(FCWS__Local__Type local_type, FCWS__Para__Type para_type, gsl_matrix *from);

		int LoadParam(FCWS__Local__Type local_type, FCWS__Para__Type para_type, int rows, int cols, double *from);

		int SetParam(FCWS__Para__Type para_type, gsl_matrix *from);

		int LoadParam(FCWS__Para__Type para_type, int rows, int cols, double *from);

		bool SaveParam(FCWS__Para__Type para_type, FCWS__Para *param);

		void SetPCAAndICAComponents(int pca_first_k_components, int pca_compoments_offset, int ica_first_k_components, int ica_compoments_offset);

		void SetOutputPath(string output_folder);

		int PickUpFiles(vector<string> & feedin, int rows = 0, int cols = 0);

		int Init();

//		Para* GetParaObj(FCWS__Para__Type para_type);

		FCWS__Local__Type GetLocalType();

		void DeleteParaObj(FCWS__Para__Type para_type);

		int DoTraining();

protected:
		int	GenImageMat();

		gsl_vector*	CalMean(gsl_matrix *m);

		gsl_matrix* CalVariance(gsl_matrix *m, gsl_vector* mean);

		int CalEigenSpace(gsl_matrix* covar, gsl_vector** eval, gsl_matrix** evec);

		gsl_matrix_view GetFirstKComponents(gsl_matrix *evec, int firstk);

		gsl_matrix* GenerateProjectionMatrix(gsl_matrix *p, gsl_matrix *x);

		gsl_matrix* GenerateReconstructImageMatrixFromPCA(gsl_matrix *p, gsl_matrix *r, gsl_vector *mean);

		gsl_matrix* GenerateResidualImageMatrix(gsl_matrix *orignal_image_matrix, gsl_matrix *reconstruct_image_matrix);

		int PCA(gsl_matrix *im,
				gsl_vector **mean,
				gsl_matrix **covar,
				gsl_vector **eval,
				gsl_matrix **evec,
				int first_k_comp,
				int comp_offset,
				gsl_matrix_view *principle_evec,
				gsl_matrix **projection,
				gsl_matrix **reconstruction_images,
				gsl_matrix **residual_images);

		/* ICA related function */
		void MatrixCenter(gsl_matrix *m, gsl_vector **mean);

		void MatrixDeCenter(gsl_matrix *m, gsl_vector *mean);

		gsl_matrix* MatrixInvert(gsl_matrix *m);

		int FastICA(gsl_matrix *X,
					int compc,
					gsl_matrix *K,
					gsl_matrix *W,
					gsl_matrix *A,
					gsl_matrix *S);

		gsl_matrix* ICA_compute(gsl_matrix *X);

		void WriteBackMatrixToImages(gsl_matrix *x, string postfix);
};
#endif
