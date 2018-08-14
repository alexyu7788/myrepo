#ifndef _VEHICLEMODEL_H_
#define _VEHICLEMODEL_H_

#include <string>
#include <vector>
#include <pthread.h>

#include "models2.pb-c.h"
#include "localfeature.h"

using namespace std;

class CVehicleModel {
protected:
        bool                     m_terminate;
		bool					 m_finish;

		vector<string>			 m_filelist;
		FCWS__VehicleModel__Type m_vm_type;
		int						 m_local_feature_count;
		CLocalFeature*			 m_local_feature[FCWS__LOCAL__TYPE__TOTAL];

		pthread_mutex_t	  		 m_Mutex;
		pthread_cond_t	  		 m_Cond;
		pthread_t				 m_thread[FCWS__LOCAL__TYPE__TOTAL];
		pthread_t 				 m_monitor_thread;

public:
		CVehicleModel(FCWS__VehicleModel__Type vm_type);

		~CVehicleModel();

		int	StartTrainingThreads();
        
		int	StartDetectionThreads();

		int SetParam(FCWS__VehicleModel__Type vm_type, FCWS__Local__Type local_type, FCWS__Para__Type para_type, gsl_matrix *from);

		int LoadParam(FCWS__VehicleModel__Type vm_type, FCWS__Local__Type local_type, FCWS__Para__Type para_type, int rows, int cols, double *from);
        
		bool LoadParam(FCWS__VehicleModel__Type vm_type, FCWS__Local__Type local_type, FCWS__Para2* param);

        bool HasParam();

		bool SaveParam(FCWS__Local__Type local_type, FCWS__Para__Type para_type, FCWS__Para *param);

        bool SaveParam(FCWS__Local__Type local_type, FCWS__Para2* para);

		void SetPCAAndICAComponents(int pca_first_k_components, int pca_compoments_offset, int ica_first_k_components, int ica_compoments_offset);

		void SetOutputPath(string output_folder);

		int PickUpFiles(vector<string> & feedin, int rows = 0, int cols = 0);

		bool FinishTraining();

        void Stop();

        void SetDetectionSource(uint8_t *img, int o_width, int o_height, int start_r, int start_c, int width, int height);


protected:
		static void* TrainingProcess(void* arg);

        static void* DetectionProcess(void *arg);

		static void* TrainingMonitorThread(void* arg);

        static void* DetectionMonitorThread(void* arg);

        bool IsIdle();
};
#endif
