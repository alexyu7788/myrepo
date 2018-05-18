#ifndef _FCWS_H_
#define _FCWS_H_

#include <pthread.h>
#include <vector>
#include <string>
#include <algorithm>
#include <math.h>
#include <time.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_statistics.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>

#include "models2.pb-c.h"

using namespace std;

#define MAX_ITERATIONS	1000
#define TOLERANCE		0.0001
#define ALPHA			1

class Para {
public:
		Para(FCWS__Para__Type para_type);

		~Para();

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


class LocalModel {
public:
		LocalModel(FCWS__VehicleModel__Type vm_type, FCWS__Position__Type pos_type);

		~LocalModel();

		int Set(FCWS__Position__Type pos, FCWS__Para__Type para_type, gsl_matrix *from);

		int Set(FCWS__Position__Type pos, FCWS__Para__Type para_type, int rows, int cols, double *from);

		int Set(FCWS__Para__Type para_type, gsl_matrix *from);

		int Set(FCWS__Para__Type para_type, int rows, int cols, double *from);

		void SetPCAAndICAComponents(int pca_first_k_components, int pca_compoments_offset, int ica_first_k_components, int ica_compoments_offset);

		void SetOutputPath(char *output_folder);

		int PickUpFiles(vector<string> & feedin, int rows, int cols);

		int Init();

//		Para* GetParaObj(FCWS__Para__Type para_type);

		FCWS__Position__Type GetPosType();

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

		void WriteBackMatrixToImages(gsl_matrix *x,  char *postfix);

protected:
		char*					m_output_folder;

		char*					m_vm_name;
		FCWS__VehicleModel__Type m_vm_type;

		char* 					m_local_name;
		FCWS__Position__Type	m_pos_type;
		vector<string>			m_filelist;

		int 					m_pca_first_k_components;
		int 					m_pca_compoments_offset;
		int 					m_ica_first_k_components;
		int 					m_ica_compoments_offset;

		int						m_para_count;
		Para*					m_para[FCWS__PARA__TYPE__TOTAL];

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
};

//Vehicle Model
class VehicleModel {
public:
		VehicleModel(FCWS__VehicleModel__Type vm_type);

		~VehicleModel();

		int	StartTrainingThreads();

		int Set(FCWS__VehicleModel__Type vm_type, FCWS__Position__Type pos, FCWS__Para__Type para_type, gsl_matrix *from);

		int Set(FCWS__VehicleModel__Type vm_type, FCWS__Position__Type pos, FCWS__Para__Type para_type, int rows, int cols, double *from);

		void SetPCAAndICAComponents(int pca_first_k_components, int pca_compoments_offset, int ica_first_k_components, int ica_compoments_offset);

		void SetOutputPath(char *output_folder);

		int PickUpFiles(vector<string> & feedin, int rows, int cols);

		int FinishTraining();

protected:
		static void* TrainingProcess(void* arg);

		static void* MonitorThread(void* arg);
protected:
		vector<string>			 m_filelist;
		FCWS__VehicleModel__Type m_vm_type;
		int						 m_local_model_count;
		LocalModel*				 m_local_model[FCWS__POSITION__TYPE__TOTAL];

		pthread_mutex_t	  		 m_Mutex;
		pthread_cond_t	  		 m_Cond;
		pthread_t				 m_thread[FCWS__POSITION__TYPE__TOTAL];
		pthread_t 				 m_monitor_thread;
		int						 m_finishtrain;
};

//Group of VehicleModel
class FCWS {
public:
		FCWS();

		~FCWS();

		int LoadFrom(char *model_name);

		int SaveTo(char *model_name);

		int FeedFiles(vector<string> & feedin, int rows, int cols);

		int DoTraining(int pca_first_k_components, 
					   int pca_compoments_offset, 
					   int ica_first_k_components, 
					   int ica_compoments_offset,
					   char *output_folder);

protected:
		int Init();

		static void* StartTrainingThreads(void *arg);

protected:

		int m_vm_count;
		VehicleModel*			m_vm[FCWS__VEHICLE__MODEL__TYPE__TOTAL];
		pthread_t 				m_threads[FCWS__VEHICLE__MODEL__TYPE__TOTAL];
		FCWS__Models*			m_models;




};
#endif
