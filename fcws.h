#ifndef _FCWS_H_
#define _FCWS_H_

#include <pthread.h>
#include <vector>
#include <list>
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
#include "candidate.h"

using namespace std;

//Group of VehicleModel
class FCWS {
protected:
        bool                    m_terminate;
        bool                    m_one_step_mode;

        uint8_t*                m_imgy;
        int                     m_width;
        int                     m_height;
		vector<string>			m_filelist;
		int m_vm_count;

		CVehicleModel*			m_vm[FCWS__VEHICLE__MODEL__TYPE__TOTAL];
		pthread_t 				m_threads[FCWS__VEHICLE__MODEL__TYPE__TOTAL];
		FCWS__Models*			m_models;

        // debug window
        CMainWindow*            m_debugwindow;
        pthread_t               m_event_thread; 

        // VH
        Candidates              m_vcs;

        //control flow
        pthread_t               m_cf_thread;
        pthread_mutex_t         m_cf_mutex;
        pthread_cond_t          m_cf_cond;
        bool                    m_cf_one_step_mode;
        bool                    m_cf_next_step;
        bool                    m_cf_next_file;
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

        bool InitDetection(string folder, bool debugwindow, bool onestep = false);

        int InitDetectionThreads(bool debugwindow);

		bool DoDetection(uint8_t *image, uint32_t width, uint32_t height);

        // debug window
        bool InitDebugWindow(string title, string folder, int w, int h);

protected:
		int Init();

		static void* StartTrainingThreads(void *arg);

		static void* StartDetectionThreads(void *arg);

		void CreateOutputFolder(string out);

        // debug window
        static void* DWProcessEvent(void* arg);

        // control flow
        static void* ControlFlow(void* arg);

};
#endif
