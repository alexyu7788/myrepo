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
#include "vehiclemodel.h"
#include "window.h"

using namespace std;

//Group of VehicleModel
class FCWS {
protected:
        bool                    m_terminate;

		vector<string>			m_filelist;
		int m_vm_count;
		CVehicleModel*			m_vm[FCWS__VEHICLE__MODEL__TYPE__TOTAL];
		pthread_t 				m_threads[FCWS__VEHICLE__MODEL__TYPE__TOTAL];
		FCWS__Models*			m_models;

        // debug window
        CMainWindow*            m_debugwindow;
        pthread_t               m_event_thread; 

public:
		FCWS();

		~FCWS();

		int LoadModel(string model_name);

		int SaveModel(string model_name);

		bool LoadFiles(string infolder);

		int FeedFiles(vector<string> & feedin, int rows, int cols);

		int DoTraining(int pca_first_k_components,
					   int pca_compoments_offset, 
					   int ica_first_k_components, 
					   int ica_compoments_offset,
					   string output_folder);

        int InitDetection();

		int DoDectection(uint8_t *image, uint32_t width, uint32_t height);

        // debug window
        bool InitDebugWindow(string title, int w, int h);

protected:
		int Init();

		static void* StartTrainingThreads(void *arg);

		static void* StartDetectionThreads(void *arg);

		void CreateOutputFolder(string out);

        // debug window
        static void* DWProcessEvent(void* arg);

};
#endif
