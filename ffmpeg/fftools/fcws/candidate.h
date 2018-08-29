#ifndef _CANDIDATE_H_
#define _CANDIDATE_H_

#include "common.h"

#define MAX_CANDIDATES 12

typedef struct Candidate {
    uint32_t m_r;
    uint32_t m_c;
    uint32_t m_w;
    uint32_t m_h;
}Candidate;

void CandidateGetGeoInfo(Candidate* _candidate, uint32_t* r, uint32_t* c, uint32_t* w, uint32_t* h);

void CandidateSetGeoInfo(Candidate* _candidate, uint32_t r, uint32_t c, uint32_t w, uint32_t h);
//using namespace std;
//
//class CCandidate;
//typedef list<CCandidate*>           Candidates;
//typedef list<CCandidate*>::iterator CandidatesIT;
//
//class CCandidate {
//protected:
//    uint32_t m_r;
//    uint32_t m_c;
//    uint32_t m_w;
//    uint32_t m_h;
//
//public:
//    CCandidate();
//
//    ~CCandidate();
//
//    void GetPos(uint32_t& r, uint32_t& c);
//
//    void SetPos(uint32_t r, uint32_t c);
//
//    void GetWH(uint32_t& w, uint32_t& h); 
//
//    void SetWH(uint32_t w, uint32_t h); 
//};
#endif

