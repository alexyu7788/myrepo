#ifndef _CANDIDATE_H_
#define _CANDIDATE_H_

#include <list>
#include "models2.pb-c.h"

using namespace std;

class CCandidate;
class CShiftWindow;

typedef list<CCandidate*>           Candidates;         // List of class CCandidate
typedef list<CCandidate*>::iterator CandidatesIt;       // Iterator of the above List 
typedef list<Candidates*>           CandidatesList;
typedef list<Candidates*>::iterator CandidatesListIt;

class CShiftWindow {
protected:
    int m_r;
    int m_c;
    int m_w;
    int m_h;

    double m_score;

public:
    CShiftWindow();

    ~CShiftWindow();

    void operator=(const CShiftWindow &shift_window);

    void GetPos(int &r, int &c);

    void GetWH(int &w, int &h);

    void SetPos(int r, int c);

    void SetWH(int w, int h);

    void GetInfo(int &r, int &c, int &w, int &h);  // used

    void SetInfo(int r, int c, int w, int h); //used

    double GetScore();

    void SetScore(double score);
};

class CLocalInfo {
protected:
    int m_r;
    int m_c;
    int m_w;
    int m_h;

    CShiftWindow  m_shiftwindow;

public:
    CLocalInfo();

    ~CLocalInfo();

    void operator=(const CLocalInfo &local_info);

    // local info itself
    void GetPos(int &r, int &c);

    void GetWH(int &w, int &h);

    void SetPos(int r, int c);

    void SetWH(int w, int h);

    // shift window
    void GetSWPos(int &r, int &c);

    void GetSWWH(int &w, int &h);

    void SetSWPos(int r, int c);

    void SetSWWH(int w, int h);

    void GetInfo(int &r, int &c, int &w, int &h);

    void SetInfo(int r, int c, int w, int h);

    void GetSWInfo(CShiftWindow &sw);

    void GetSWInfo(int &r, int &c, int &w, int &h);

    void SetSWInfo(const CShiftWindow &sw);

    void SetSWInfo(int r, int c, int w, int h);

};

class CCandidate
{
public:

protected:
    int m_r;
    int m_c;
    int m_w;
    int m_h;

    CLocalInfo  m_localinfo[FCWS__LOCAL__TYPE__TOTAL];

    FCWS__VehicleModel__Type m_vm_type;

    double m_score[FCWS__VEHICLE__MODEL__TYPE__TOTAL];


public:

    CCandidate();

    ~CCandidate();

    void operator=(CCandidate &candidate);

    void GetInfo(int &r, int &c, int &w, int &h);

    void SetInfo(int r, int c, int w, int h);

    void GetLocalInfo(FCWS__Local__Type local_type, int &r, int &c, int &w, int &h);

    void SetLocalInfo(FCWS__Local__Type local_type, const CLocalInfo &local_info);

    void SetLocalInfo(FCWS__Local__Type local_type, int r, int c, int w, int h);

    void GetSWInfo(FCWS__Local__Type local_type, CShiftWindow &sw);

    void GetSWInfo(FCWS__Local__Type local_type, int &r, int &c, int &w, int &h);

    void SetSWInfo(FCWS__Local__Type local_type, const CShiftWindow &sw);

    void SetSWInfo(FCWS__Local__Type local_type, int r, int c, int w, int h);
protected:


};
#endif
