#include "fcws.hpp"
 
#define BLOB_MARGIN 3

// ------------------protected function---------
void CFCWS::BlobDump(string description, list<blob_t>& blobs)
{
    fcwsdbg("%s", description.c_str());

    for (auto& blob:blobs) {
        fcwsdbg("blob at (%d, %d) with (%d, %d)",
                blob.r,
                blob.c,
                blob.w,
                blob.h);
    }
}

BOOL CFCWS::BlobConvertToVehicleShape(uint32_t cols, list<blob_t>& blobs, list<candidate_t>& cands)
{
    candidate_t cand;
    int left_idx, right_idx, bottom_idx;
    int vehicle_startr, vehicle_startc;
    int vehicle_width, vehicle_height;

    if (!blobs.size()) {
        fcwsdbg();
        return FALSE;
    }

    for (auto& blob:blobs) {
        if (blob.valid == TRUE) {
            bottom_idx      = blob.r;
            
            //scale up searching region of blob with 1/4 time of width on the left & right respectively.
            left_idx        = (blob.c - (blob.w >> 2));
            right_idx       = (blob.c + blob.w + (blob.w >> 2));

            left_idx        = (left_idx < 0 ? 0 : left_idx);
            right_idx       = (right_idx >= cols ? cols - 1 : right_idx);

            vehicle_width   = (right_idx - left_idx + 1);

            if ((bottom_idx - (blob.w * 0.5)) < 0) {
                vehicle_startr = 0;
                vehicle_height = bottom_idx;
            } else {
                vehicle_height = blob.w * 0.5;
                vehicle_startr = bottom_idx - vehicle_height;
            }

            vehicle_startc  = left_idx;

            if (left_idx + vehicle_width >= cols) {
                right_idx       = cols - 1;
                vehicle_width   = right_idx - left_idx + 1;
            } 

            memset(&cand, 0x0, sizeof(candidate_t));

            cand.m_valid = blob.valid;
            cand.m_id= blob.number;
            cand.m_r = vehicle_startr;
            cand.m_c = vehicle_startc;
            cand.m_w = vehicle_width;
            cand.m_h = vehicle_height;

            cands.push_back(cand);
        }
    }

    return TRUE;
}

BOOL CFCWS::BlobRearrange(list<blob_t>& blobs)
{
    int32_t diff, number = 0;
    list<blob_t>::iterator cur, next, pre, temp;

    if (!blobs.size())
        return FALSE;

redo:

    for (cur = blobs.begin() ; cur != blobs.end() ; ) {
        for (next = std::next(cur, 1) ; next != blobs.end() ; ) {

            // next blob is above cur blob.
            if (next->r < cur->r) {
                /*
                 *  a:   next     ------
                 *       cur   ------------- (delete next)
                 *
                 *  b:   next  --------
                 *       cur      ---------- (delete next)
                 *
                 *  c:   next  -----------
                 *       cur     ------      (copy next to cur and delete next)
                 *
                 */
                if ((/*(a)*/next->c >= cur->c && next->c <= cur->c + cur->w) ||
                    (/*(b)*/next->c < cur->c && next->c + next->w > cur->c && next->c + next->w <= cur->c + cur->w)) {
                    next =  blobs.erase(next);
                } else if (/*(c)*/cur->c > next->c && cur->c + cur->w <= next->c + next->w) {
                    if ((cur->r - next->r) < (cur->w / 2)) {

                        cur->valid  = next->valid;
                        cur->number = next->number;
                        cur->r      = next->r;
                        cur->c      = next->c;
                        cur->w      = next->w;
                        cur->h      = next->h;
                        cur->density= next->density;

                        next = blobs.erase(next);
                        //fcwsdbg("redo");
                        goto redo;
                    }
                }
            }
            // next blob is below cur blob.
            else if (next->r > cur->r) {
                /*
                 *  d:   cur     ------             -------
                 *       next   ------------- or  -------        (copy next to cur and delete next)
                 *
                 *  e:   cur     --------
                 *       next      ----------                    (copy next to cur and delete next)
                 */
                if (/*(d)*/(cur->c >= next->c && cur->c <= next->c + next->w) ||
                    /*(e)*/(cur->c < next->c && cur->c + cur->w > next->c && cur->c + cur->w <= next->c + next->w)) {
                        cur->valid  = next->valid;
                        cur->number = next->number;
                        cur->r      = next->r;
                        cur->c      = next->c;
                        cur->w      = next->w;
                        cur->h      = next->h;
                        cur->density= next->density;

                        next = blobs.erase(next);
                        //fcwsdbg("redo");
                        goto redo;
                }
            }
            // next overlaps cur.
            else if (next->r == cur->r) {
                /*
                 *  f:        next       cur 
                 *       -----------==|-------        (merge cur & next)
                 *
                 *  g:        next    3 pixels   cur 
                 *       -----------|<------->|-------      (merge cur & next)
                 *
                 *  h:        cur       next 
                 *       -----------==|-------        (merge cur & next)
                 *
                 *  i:        cur    3 pixels   next 
                 *       -----------|<------->|-------      (merge cur & next)
                 */
                if (/*(f)*/(cur->c > next->c && cur->c <= next->c + next->w) || 
                    /*(g)*/(cur->c > next->c && cur->c > next->c + next->w && abs(cur->c - next->c - next->w) < 3) || 
                    /*(h)*/(cur->c < next->c && cur->c + cur->w >= next->c)  || 
                    /*(i)*/(cur->c < next->c && cur->c + cur->w < next->c && abs(cur->c + cur->w - next->c) < 3)) {
                    // Merge cur & next
                    if (cur->c > next->c) {
                        diff = next->c + next->w - cur->c;
                        cur->c = next->c;
                        cur->w = cur->w + next->w - diff;
                    } else {
                        diff = cur->c + cur->w - next->c;
                        cur->w = cur->w + next->w - diff;
                    }

                    next = blobs.erase(next);

                    goto redo;
                }
            }

            ++next;
        }

        ++cur;
    }

    for (auto& it:blobs) {
        it.number = number++;
    }

    return TRUE;
}

BOOL CFCWS::BlobFindIdentical(list<blob_t>& blobs, 
                               blob_t& temp)
{
    list<blob_t>::iterator it;

    for (it = blobs.begin() ; it != blobs.end() ; ++it) {
        if (it->r == temp.r && it->c == temp.c &&
            it->w == temp.w && it->h == temp.h)
            return TRUE;
    }

    return FALSE;
}

BOOL CFCWS::BlobGenerate(const gsl_matrix* src,
                  uint32_t peak_idx,
                  list<blob_t>& blobs)
{
    uint32_t sm_r, sm_c, sm_w, sm_h;
    uint32_t blob_pixel_cnt, max_blob_pixel_cnt;

    int32_t r, c;
    int32_t blob_r_top, blob_r_bottom, blob_c, blob_w, blob_h;
    int32_t max_blob_r, max_blob_c, max_blob_w, max_blob_h;
    int32_t stop_wall_cnt;
    float blob_pixel_density;
    BOOL has_neighborhood = FALSE;
    blob_t tblob;
    gsl_matrix_view sm;

    if (!src) {
        fcwsdbg();
        return FALSE;
    }

    if (peak_idx > BLOB_MARGIN + 1 && peak_idx < (src->size1 - BLOB_MARGIN - 1)) {

        sm_r = peak_idx - BLOB_MARGIN;
        sm_c = 0;
        sm_w = src->size2;
        sm_h = BLOB_MARGIN * 2;
        sm   = gsl_matrix_submatrix((gsl_matrix*)src, sm_r, sm_c, sm_h, sm_w);

        blob_pixel_cnt = max_blob_pixel_cnt = 0;

        max_blob_r = max_blob_c = max_blob_w = max_blob_h = 
        blob_c = blob_w = blob_h = -1;
        blob_r_top = INT_MAX; blob_r_bottom = INT_MIN;
        blob_c = INT_MIN;

        for (c = 1 ; c < sm.matrix.size2 - 1 ; ++c) {
            for (r = 1 ; r < sm.matrix.size1 - 1 ; ++r) {
                if (r == 1) {
                    stop_wall_cnt = sm_h - 2;
                    has_neighborhood = FALSE;
                }

                if (gsl_matrix_get(&sm.matrix, r, c) != 255) {
                    
                    if (gsl_matrix_get(&sm.matrix, r-1, c+1) != 255 ||
                        gsl_matrix_get(&sm.matrix,   r, c+1) != 255 ||
                        gsl_matrix_get(&sm.matrix, r+1, c+1) != 255)
                        has_neighborhood = TRUE;

                    blob_pixel_cnt++;

                    if (r <= blob_r_top)
                        blob_r_top = r;

                    if (r >= blob_r_bottom)
                        blob_r_bottom = r;

                    if (blob_c == INT_MIN)
                        blob_c = c;

                    if (gsl_matrix_get(&sm.matrix, r-1, c+1) == 255 && 
                        gsl_matrix_get(&sm.matrix,   r, c+1) == 255 && 
                        gsl_matrix_get(&sm.matrix, r+1, c+1) == 255 &&
                        has_neighborhood == FALSE) {

examine:
                        if (blob_pixel_cnt > 1) {
                            blob_w = c - blob_c + 1;
                            blob_h = blob_r_bottom - blob_r_top + 1;
                            blob_pixel_density = blob_pixel_cnt / (float)(blob_w * blob_h);

                            if (blob_h > 1 &&
                                ((peak_idx <= src->size1 / 3.0 && blob_w >= 10 && blob_pixel_density >= 0.2) ||
                                 (peak_idx > src->size1 / 3.0 && blob_w >= 20 && blob_pixel_density >= 0.55))) {
                                tblob.valid = TRUE;
                                tblob.r     = peak_idx;
                                tblob.c     = sm_c + blob_c;
                                tblob.w     = blob_w;
                                tblob.h     = blob_h;

                                if (BlobFindIdentical(blobs, tblob) == FALSE) {
                                    blobs.push_back(tblob);
                                } else
                                    memset(&tblob, 0x0, sizeof(blob_t));
                            }
                        }

                        blob_pixel_cnt = 0;
                        blob_c = blob_w = blob_h = -1;
                        blob_r_top = INT_MAX; blob_r_bottom = INT_MIN;
                        blob_c = INT_MIN;
                        blob_pixel_density = 0;
                    }
                } else {
                    if (has_neighborhood == FALSE && --stop_wall_cnt <= 1) {
                        goto examine;
                    }
                }
            }
        }
    }

    return TRUE;
}

void CFCWS::VehicleDump(string description, list<candidate_t>& cands)
{
    fcwsdbg("%s", description.c_str());
    for (auto& cand:cands) {
        if (cand.m_valid == TRUE) {
            fcwsdbg("cand at (%d, %d) with (%d, %d)",
                    cand.m_r,
                    cand.m_c,
                    cand.m_w,
                    cand.m_h);
        }
    }
}

BOOL CFCWS::VehicleCheckByAR(list<candidate_t>& cands)
{
    double ar;

    for (auto& cand:cands) {
        if (cand.m_valid == TRUE) {
            ar = cand.m_w / (double)cand.m_h;

            if (AR_LB < ar && ar < AR_HB)
                cand.m_valid = TRUE;
            else
                cand.m_valid = FALSE;
        }
    }

    return TRUE;
}

BOOL CFCWS::VehicleCheckByVerticalEdge(const gsl_matrix* vedgeimg,
                                       list<candidate_t>& cands)
{
    uint32_t r, c;
    uint32_t pixel_cnt = 0, area; 
    float percentage;
    gsl_matrix_view submatrix;

    if (!vedgeimg) {
        fcwsdbg();
        return FALSE;
    }

    for (auto& cand:cands) {
        if (cand.m_valid == TRUE) {
            area = cand.m_w * cand.m_h;
            submatrix = gsl_matrix_submatrix((gsl_matrix*)vedgeimg, cand.m_r, cand.m_c, cand.m_h, cand.m_w);

            for (r=0 ; r<submatrix.matrix.size1 ; ++r) {
                for (c=0 ; c<submatrix.matrix.size2 ; ++c) {
                    if (gsl_matrix_get(&submatrix.matrix, r, c)) {
                        pixel_cnt++;
                    }
                }
            }

            percentage = pixel_cnt / (float)area;

            //fcwsdbg("VerticalEdge: %d / %d = %.04f", pixel_cnt, area, percentage);

            if (percentage < 0.2)
                cand.m_valid = FALSE;
        }
    }

    return TRUE;
}

BOOL CFCWS::VehicleCheck(const gsl_matrix* imgy,
                         const gsl_matrix* vedgeimg,
                         list<candidate_t>& cands)
{
    if (!imgy || !vedgeimg) {
        fcwsdbg();
        return FALSE;
    }

    VehicleCheckByVerticalEdge(vedgeimg, cands);

    VehicleCheckByAR(cands);

    return TRUE;
}

BOOL CFCWS::VehicleUpdateShapeByStrongVerticalEdge(const gsl_matrix* vedgeimg, list<candidate_t>& cands)
{
    uint32_t r, c;
    uint32_t rr, cc;
    uint32_t max_vedge_c, max_vedge_c_left;
    uint32_t pix_cnt, column_cnt;
    uint32_t max_edge_idx;
    int32_t  left_idx, right_idx, bottom_idx;
    double mean, sd, delta;
    double magnitude;
    double max_edge_value;
    double vedge_strength, max_vedge_strength;
    gsl_matrix_view sm, block;
    gsl_vector* strong_edge_value = NULL;

    if (!vedgeimg) {
        fcwsdbg();
        return FALSE;
    }

    for (auto& cand:cands) {
        if (cand.m_valid == TRUE) {

            sm = gsl_matrix_submatrix((gsl_matrix*)vedgeimg,
                                      cand.m_r,
                                      cand.m_c,
                                      cand.m_h,
                                      cand.m_w);

            CheckOrReallocVector(&strong_edge_value, sm.matrix.size2, TRUE);

            bottom_idx = cand.m_r + cand.m_h;

            column_cnt  =
            pix_cnt     =
            magnitude   =
            sd          = 
            mean        = 0;

            for (c = 0 ; c < sm.matrix.size2 - 2 ; ++c) {
                block = gsl_matrix_submatrix(&sm.matrix,    
                                             0,
                                             c,
                                             sm.matrix.size1,
                                             2);

                pix_cnt = magnitude = 0;

                for (rr = (block.matrix.size1 / 2) ; rr < block.matrix.size1 ; ++rr) {
                    for (cc = 0 ; cc < block.matrix.size2 ; ++cc) {
                        ++pix_cnt;
                        magnitude += gsl_matrix_get(&block.matrix, rr, cc);
                    }
                }

                vedge_strength = (magnitude / (double)(pix_cnt));
                gsl_vector_set(strong_edge_value, c, vedge_strength);
                //fcwsdbg("strong edge[%d] = %lf", c, vedge_strength);

                column_cnt++;
                mean += vedge_strength;
            }

            // mean & standart deviation
            mean /= column_cnt;

            for (c = 0 ; c < sm.matrix.size2 - 2 ; ++c) {
                delta = (mean - gsl_vector_get(strong_edge_value, c));
                sd   += (delta * delta);
            }

            sd = sqrt(sd / (double)(column_cnt -1));

            //fcwsdbg("%f, %f, %f", mean, sd, delta);
            // --------------Find left max-----------------------------------------
            max_edge_idx    =
            max_edge_value  = 0;

            for ( c = 0 ; c < strong_edge_value->size / 2 ; ++c) {
                if ((gsl_vector_get(strong_edge_value, c) > (mean + 2 * sd)) &&
                    (max_edge_value < gsl_vector_get(strong_edge_value, c))) {
                    max_edge_value = gsl_vector_get(strong_edge_value, c);
                    max_edge_idx   = c;
                }
            }

            if (max_edge_idx == 0) {
                cand.m_valid = FALSE;
                fcwsdbg();
                continue;
            }

            left_idx    = max_edge_idx;
            cand.m_c   += max_edge_idx;

            // --------------Find right max-----------------------------------------
            max_edge_idx    = 
            max_edge_value  = 0;

            for (c = strong_edge_value->size / 2 ; c < (strong_edge_value->size - 2); ++c) {
                if ((gsl_vector_get(strong_edge_value, c) > (mean + 2 * sd)) && 
                    (max_edge_value < gsl_vector_get(strong_edge_value, c))) {
                    max_edge_value = gsl_vector_get(strong_edge_value, c);
                    max_edge_idx   = c;
                }
            }

            if (max_edge_idx == 0) {
                cand.m_valid = FALSE;
                fcwsdbg();
                continue;
            }

            cand.m_w = max_edge_idx - left_idx + 1;
            cand.m_h = (cand.m_w * VHW_RATIO > bottom_idx) ? bottom_idx : cand.m_w * VHW_RATIO;
            cand.m_r = bottom_idx - cand.m_h;
            cand.m_valid = TRUE;
        }
    }

    FreeVector(&strong_edge_value);

    return TRUE;
}

BOOL CFCWS::VehicleUpdateHeatMap(gsl_matrix* map,
                                 gsl_matrix_char* id,
                                 list<candidate_t>& cands)
{
    BOOL pixel_hit = FALSE;
    uint32_t i, r, c;
    double val;

    if (!map) {
        fcwsdbg();
        return FALSE;
    }

    CheckOrReallocMatrixChar(&id, map->size1, map->size2, FALSE);

    // decrease/increase HeapMap
    for (r = 0 ; r < map->size1 ; ++r) {
        for (c = 0 ; c < map->size2 ; ++c) {
            pixel_hit = FALSE;

            // this pixel belongs to certain vehicle candidate.
            for (auto& cand:cands) {
                if (cand.m_valid == FALSE) 
                    continue;        

                if (r >= cand.m_r && r < (cand.m_r + cand.m_h) &&
                    c > cand.m_c && c < (cand.m_c + cand.m_w)) {
                    pixel_hit = TRUE;
                    break;
                }
            }

            val = gsl_matrix_get(map, r, c);

            if (pixel_hit) {
                val += HeatMapIncrease;
                if (val >= 255)
                    val = 255;
            } else {
                val -= HeatMapDecrease;
                if (val <= 0)
                    val = 0;
            }

            gsl_matrix_set(map, r, c, val);

            if (val < HeatMapAppearThreshold) {
                gsl_matrix_char_set(id, r, c, -1);
            }
        }
    }

    return TRUE;
}

BOOL CFCWS::HypothesisGenerate(const gsl_matrix* imgy,
                               const gsl_matrix* intimg,
                               const gsl_matrix* shadowimg,
                               const gsl_matrix* vedgeimg,
                               const gsl_vector* horizonproject,
                               gsl_vector* temp_horizonproject,
                               gsl_matrix* heatmap,
                               gsl_matrix_char* heatmapid,
                               list<blob_t>& blobs,
                               list<candidate_t>& cands,
                               list<candidate_t>& tracker
                               )
{
    uint32_t cur_peak, cur_peak_idx, max_peak; 

    if (!imgy || !intimg || !shadowimg || !vedgeimg || !horizonproject) {
        fcwsdbg();
        return FALSE;
    }

    CheckOrReallocVector(&temp_horizonproject, horizonproject->size, TRUE);
    gsl_vector_memcpy(temp_horizonproject, horizonproject);

    max_peak = gsl_vector_max(temp_horizonproject);

    if (max_peak == 0)
        return FALSE;

    blobs.clear();
    cands.clear();

    for(;;) {
        cur_peak = gsl_vector_max(temp_horizonproject);
        cur_peak_idx = gsl_vector_max_index(temp_horizonproject);

        if (cur_peak < (max_peak * 0.5))
            break;

        gsl_vector_set(temp_horizonproject, cur_peak_idx, 0);
        BlobGenerate(shadowimg, cur_peak_idx, blobs);
    }

    if (blobs.size()) {
        BlobRearrange(blobs);

        BlobConvertToVehicleShape(imgy->size2, blobs, cands);

        VehicleUpdateShapeByStrongVerticalEdge(vedgeimg, cands);

        VehicleCheck(imgy, vedgeimg, cands);

        for (list<candidate_t>::iterator it = cands.begin() ; it != cands.end(); ) {
            if (it->m_valid == TRUE) {

                it->m_r        = (it->m_r == 0 ? 1 : it->m_r);
                it->m_h        = (it->m_r == 0 ? it->m_h - 1 : it->m_h);
                it->m_st       = _Disappear;

                ++it;
            } else {
                it = cands.erase(it);
            }
        }
    }

    VehicleUpdateHeatMap(heatmap, heatmapid, cands);



    return TRUE;
}



















// ------------------public function---------
 
CFCWS::CFCWS()
{
    m_terminate = FALSE;

    m_ip = NULL;

    memset(&m_mutex, 0x0, sizeof(pthread_mutex_t));
    memset(&m_cond, 0x0, sizeof(pthread_cond_t));

    m_imgy      = 
    m_imgu      = 
    m_imgv      = 
    m_vedgeimg  =
    m_heatmap   =
    m_tempimg   =
    m_intimg    =   
    m_shadowimg = NULL;

    m_gradient  = NULL;

    m_direction = 
    m_heatmapid = NULL;

    m_horizonproject        =
    m_temp_horizonproject   = NULL;

    m_blobs.clear();
    m_candidates.clear();
    m_vc_tracker.clear();
}

CFCWS::~CFCWS()
{
    Deinit();
}

BOOL CFCWS::Init()
{
    pthread_mutexattr_t attr;

    m_ip = new CImgProc();

    if (m_ip)
        m_ip->Init();

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&m_mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_cond_init(&m_cond, NULL);

    return FALSE;
}

BOOL CFCWS::Deinit()
{
    m_terminate = TRUE;

    if (m_ip) {
        delete m_ip;
        m_ip = NULL;
    }

    pthread_mutex_lock(&m_mutex);
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_mutex);

    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);

    FreeMatrix(&m_imgy);
    FreeMatrix(&m_imgu);
    FreeMatrix(&m_imgv);
    FreeMatrix(&m_vedgeimg);
    FreeMatrix(&m_heatmap);
    FreeMatrix(&m_tempimg);
    FreeMatrix(&m_intimg);
    FreeMatrix(&m_shadowimg);

    FreeMatrix(&m_gradient);

    FreeMatrixChar(&m_direction);
    FreeMatrixChar(&m_heatmapid);

    FreeVector(&m_horizonproject);
    FreeVector(&m_temp_horizonproject);

    m_blobs.clear();
    m_candidates.clear();
    m_vc_tracker.clear();

    return TRUE;
}

BOOL CFCWS::DoDetection(uint8_t* imgy,
                        int w,
                        int h,
                        int linesize,
                        CLDWS* ldws_obj
                        )
{
    if (!m_ip || !imgy || !w || !h || !linesize) {
        fcwsdbg();
        return FALSE;
    }

    pthread_mutex_lock(&m_mutex);

    if (m_terminate == TRUE) {
        pthread_mutex_unlock(&m_mutex);
        return FALSE;
    }

    // Allocate/Clear matrixs.
    CheckOrReallocMatrix(&m_imgy, h, w, TRUE);
    CheckOrReallocMatrix(&m_imgu, h/2, w/2, TRUE);
    CheckOrReallocMatrix(&m_imgv, h/2, w/2, TRUE);
    CheckOrReallocMatrix(&m_vedgeimg, h, w, TRUE);
    CheckOrReallocMatrix(&m_tempimg, h, w, TRUE);
    CheckOrReallocMatrix(&m_intimg, h, w, TRUE);
    CheckOrReallocMatrix(&m_shadowimg, h, w, TRUE);
    CheckOrReallocMatrix(&m_gradient, h, w, TRUE);
    CheckOrReallocMatrixChar(&m_direction, h, w, TRUE);

    // Result will be used for next round.
    CheckOrReallocMatrix(&m_heatmap, h, w, FALSE);
    CheckOrReallocMatrixChar(&m_heatmapid, h, w, FALSE);

    CheckOrReallocVector(&m_horizonproject, h, TRUE);

    m_ip->CopyImage(imgy, m_imgy, w, h, linesize);
    m_ip->CopyImage(imgy, m_tempimg, w, h, linesize);

    // Integral Image & Thresholding
    m_ip->GenIntegralImage(m_imgy, m_intimg);
    m_ip->ThresholdingByIntegralImage(m_imgy, m_intimg, m_shadowimg, 50, 0.7);
 
    // ROI
    if (ldws_obj && ldws_obj->ApplyDynamicROI(m_shadowimg) == FALSE) {
        fcwsdbg();
    }
 
    m_ip->CalHorizonProject(m_shadowimg, m_horizonproject);

    m_ip->EdgeDetectForFCW(m_imgy, m_vedgeimg, m_gradient, m_direction, 1);
   
    // Hypothesis Generation
    HypothesisGenerate(m_imgy,
                       m_intimg,
                       m_shadowimg,
                       m_vedgeimg,
                       m_horizonproject,
                       m_temp_horizonproject,
                       m_heatmap,
                       m_heatmapid,
                       m_blobs,
                       m_candidates,
                       m_vc_tracker);


    // Hypothesis Verfication


    pthread_mutex_unlock(&m_mutex);

    return TRUE;
}
