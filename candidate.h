#ifndef _CANDIDATE_H_
#define _CANDIDATE_H_

#include "models2.pb-c.h"

class CCandidate
{
public:

protected:
    int m_r;
    int m_c;
    int m_w;
    int m_h;

    double m_score[FCWS__VEHICLE__MODEL__TYPE__TOTAL];
    FCWS__VehicleModel__Type m_vm_type;

public:

    CCandidate();

    ~CCandidate();

    void GetGeometricInfo(int &r, int &c, int &w, int &h);

    void SetGeometricInfo(int r, int c, int w, int h);

protected:


};









#endif
