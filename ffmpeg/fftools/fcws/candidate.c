#include "candidate.h"

void CandidateGetGeoInfo(Candidate* obj, uint32_t* r,  uint32_t* c, uint32_t* w, uint32_t* h)
{
    if (obj) {
        *r = obj->m_r;
        *c = obj->m_c;
        *w = obj->m_w;
        *h = obj->m_h;
    }
}

void CandidateSetGeoInfo(Candidate* obj, uint32_t r,  uint32_t c, uint32_t w, uint32_t h)
{
    if (obj) {
        obj->m_r = r;
        obj->m_c = c;
        obj->m_w = w;
        obj->m_h = h;
    }
}

//CCandidate::CCandidate()
//{
//    m_r = m_c = m_w = m_h = 0;
//}
//
//CCandidate::~CCandidate()
//{
//
//}
//
//void CCandidate::GetPos(uint32_t& r, uint32_t& c)
//{
//    r = m_r;
//    c = m_c;
//}
//
//void CCandidate::SetPos(uint32_t r, uint32_t c)
//{
//    m_r = r;
//    m_c = c;
//}
//
//void CCandidate::GetWH(uint32_t& w, uint32_t& h)
//{
//    w = m_w;
//    h = m_h;
//}
//
//void CCandidate::SetWH(uint32_t w, uint32_t h)
//{
//    m_w = w;
//    m_h = h;
//}
