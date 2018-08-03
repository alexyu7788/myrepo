#ifndef _VEHICLEMODEL_H_
#define _VEHICLEMODEL_H_

#include <string>
#include <vector>
#include <pthread.h>

#include "models2.pb-c.h"
#include "localmodel.h"

using namespace std;

class CVehicleModel {
public:
		CVehicleModel(FCWS__VehicleModel__Type vm_type);

		~CVehicleModel();

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
		CLocalModel*			 m_local_model[FCWS__POSITION__TYPE__TOTAL];

		pthread_mutex_t	  		 m_Mutex;
		pthread_cond_t	  		 m_Cond;
		pthread_t				 m_thread[FCWS__POSITION__TYPE__TOTAL];
		pthread_t 				 m_monitor_thread;
		int						 m_finishtrain;
};
#endif
