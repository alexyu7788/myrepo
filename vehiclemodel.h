#ifndef _VEHICLEMODEL_H_
#define _VEHICLEMODEL_H_

#include <string>
#include <vector>
#include <pthread.h>

#include "models2.pb-c.h"
#include "localmodel.h"

using namespace std;

class CVehicleModel {
protected:
		vector<string>			 m_filelist;
		FCWS__VehicleModel__Type m_vm_type;
		int						 m_local_model_count;
		CLocalModel*			 m_local_model[FCWS__LOCAL__TYPE__TOTAL];

		pthread_mutex_t	  		 m_Mutex;
		pthread_cond_t	  		 m_Cond;
		pthread_t				 m_thread[FCWS__LOCAL__TYPE__TOTAL];
		pthread_t 				 m_monitor_thread;
		int						 m_finishtrain;

public:
		CVehicleModel(FCWS__VehicleModel__Type vm_type);

		~CVehicleModel();

		int	StartTrainingThreads();

		int SetParam(FCWS__VehicleModel__Type vm_type, FCWS__Local__Type local_type, FCWS__Para__Type para_type, gsl_matrix *from);

		int LoadParam(FCWS__VehicleModel__Type vm_type, FCWS__Local__Type local_type, FCWS__Para__Type para_type, int rows, int cols, double *from);

		bool SaveParam(FCWS__Local__Type local_type, FCWS__Para__Type para_type, FCWS__Para *param);

		void SetPCAAndICAComponents(int pca_first_k_components, int pca_compoments_offset, int ica_first_k_components, int ica_compoments_offset);

		void SetOutputPath(string output_folder);

		int PickUpFiles(vector<string> & feedin, int rows = 0, int cols = 0);

		int FinishTraining();

protected:
		static void* TrainingProcess(void* arg);

		static void* MonitorThread(void* arg);

};
#endif
